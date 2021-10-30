/*
 * Copyright (c) 2012-2021, Tiiffi <tiiffi at gmail>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software
 *   in a product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not be
 *   misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *   distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#ifdef _WIN32
    // for name resolving on windows
    // enable this if you get compiler whine about getaddrinfo() on windows
    //#define _WIN32_WINNT 0x0501

    #include <ws2tcpip.h>
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

#define VERSION "0.7.2"
#define IN_NAME "mcrcon"
#define VER_STR IN_NAME" "VERSION" (built: "__DATE__" "__TIME__")"

#define RCON_EXEC_COMMAND       2
#define RCON_AUTHENTICATE       3
#define RCON_RESPONSEVALUE      0
#define RCON_AUTH_RESPONSE      2
#define RCON_PID                0xBADC0DE

#define DATA_BUFFSIZE 4096

// rcon packet structure
typedef struct _rc_packet {
    int size;
    int id;
    int cmd;
    char data[DATA_BUFFSIZE];
    // ignoring string2 for now
} rc_packet;


// ===================================
//  FUNCTION DEFINITIONS              
// ===================================

// Network related functions
#ifdef _WIN32
void        net_init_WSA(void);
#endif
void        net_close(int sd);
int         net_connect(const char *host, const char *port);
int         net_send(int sd, const uint8_t *buffer, size_t size);
int         net_send_packet(int sd, rc_packet *packet);
rc_packet*  net_recv_packet(int sd);
int         net_clean_incoming(int sd, int size);

// Misc stuff
void        usage(void);
#ifndef _WIN32
void        print_color(int color);
#endif
int         get_line(char *buffer, int len);
int         run_terminal_mode(int sock);
int         run_commands(int argc, char *argv[]);

// Rcon protocol related functions
rc_packet*  packet_build(int id, int cmd, char *s1);
void        packet_print(rc_packet *packet);
int         rcon_auth(int sock, char *passwd);
int         rcon_command(int sock, char *command);


// =============================================
//  GLOBAL VARIABLES
// =============================================
static int global_raw_output = 0;
static int global_silent_mode = 0;
static int global_disable_colors = 0;
static int global_connection_alive = 1;
static int global_rsock;
static int global_wait_seconds = 0;

#ifdef _WIN32
  // console coloring on windows
  HANDLE console_handle;
#endif

// safety stuff (windows is still misbehaving)
void exit_proc(void)
{
	if (global_rsock != -1)
		net_close(global_rsock);
}

// Check windows & linux behaviour !!!
void sighandler(int sig)
{
	if (sig == SIGINT)
		putchar('\n');

	global_connection_alive = 0;
	#ifndef _WIN32
	    exit(EXIT_SUCCESS);
	#endif
}

#define MAX_WAIT_TIME 600

unsigned int mcrcon_parse_seconds(char *str)
{
	char *end;
	long result = strtol(str, &end, 10);

	if (errno != 0) {
		fprintf(stderr, "-w invalid value.\nerror %d: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (end == str) {
		fprintf(stderr, "-w invalid value (not a number?)\n");
		exit(EXIT_FAILURE);
	}

	if (result <= 0 || result > MAX_WAIT_TIME) {
		fprintf(stderr, "-w value out of range.\nAcceptable value is 1 - %d (seconds).\n", MAX_WAIT_TIME);
		exit(EXIT_FAILURE);
	}

	return (unsigned int) result;
}

int main(int argc, char *argv[])
{
	int terminal_mode = 0;

	char *host = getenv("MCRCON_HOST");
	char *pass = getenv("MCRCON_PASS");
	char *port = getenv("MCRCON_PORT");
	
	if (!port) port = "25575";
	if (!host) host = "localhost";

	// disable output buffering (https://github.com/Tiiffi/mcrcon/pull/39)
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	if(argc < 1 && pass == NULL) usage();

	// default getopt error handler enabled
	opterr = 1;
	int opt;
	while ((opt = getopt(argc, argv, "vrtcshw:H:p:P:")) != -1)
	{
		switch (opt) {
			case 'H': host = optarg;                break;
			case 'P': port = optarg;                break;
			case 'p': pass = optarg;                break;
			case 'c': global_disable_colors = 1;    break;
			case 's': global_silent_mode = 1;       break;
			case 'i': /* reserved for interp mode */break;
			case 't': terminal_mode = 1;            break;
			case 'r': global_raw_output = 1;        break;
			case 'w':
				global_wait_seconds = mcrcon_parse_seconds(optarg);
			break;

			case 'v':
				puts(VER_STR" - https://github.com/Tiiffi/mcrcon");
				puts("Bug reports:\n\ttiiffi+mcrcon at gmail\n\thttps://github.com/Tiiffi/mcrcon/issues/");
				exit(EXIT_SUCCESS);

			case 'h': usage(); break;
			case '?':
			default:
				puts("Try 'mcrcon -h' or 'man mcrcon' for help.");
				exit(EXIT_FAILURE);
		}
	}

	if (pass == NULL) {
		puts("You must give password (-p password).\nTry 'mcrcon -h' or 'man mcrcon' for help.");
		return 0;
	}

	if(optind == argc && terminal_mode == 0)
		terminal_mode = 1;

	// safety features to prevent "IO: Connection reset" bug on the server side
	atexit(&exit_proc);
	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	#ifdef _WIN32
		net_init_WSA();
		console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (console_handle == INVALID_HANDLE_VALUE)
			console_handle = NULL;
	#endif

	// open socket
	global_rsock = net_connect(host, port);

	int exit_code = EXIT_SUCCESS;

	// auth & commands
	if (rcon_auth(global_rsock, pass)) {
		if (terminal_mode)
			run_terminal_mode(global_rsock);
		else
			exit_code = run_commands(argc, argv);
	}
	else { // auth failed
		fprintf(stdout, "Authentication failed!\n");
		exit_code = EXIT_FAILURE;
	}

	exit(exit_code);
}

void usage(void)
{
	puts(
		"Usage: "IN_NAME" [OPTIONS] [COMMANDS]\n\n"
		"Send rcon commands to Minecraft server.\n\n"
		"Options:\n"
		"  -H\t\tServer address (default: localhost)\n"
		"  -P\t\tPort (default: 25575)\n"
		"  -p\t\tRcon password\n"
		"  -t\t\tTerminal mode\n"
		"  -s\t\tSilent mode\n"
		"  -c\t\tDisable colors\n"
		"  -r\t\tOutput raw packets\n"
		"  -w\t\tWait for specified duration (seconds) between each command (1 - 600s)\n"
		"  -h\t\tPrint usage\n"
		"  -v\t\tVersion information\n\n"
		"Server address, port and password can be set with following environment variables:\n"
		"  MCRCON_HOST\n"
		"  MCRCON_PORT\n"
		"  MCRCON_PASS\n"
	);

	puts (
		"- mcrcon will start in terminal mode if no commands are given\n"
		"- Command-line options will override environment variables\n"
		"- Rcon commands with spaces must be enclosed in quotes\n"
	);
	puts("Example:\n\t"IN_NAME" -H my.minecraft.server -p password -w 5 \"say Server is restarting!\" save-all stop\n");

	#ifdef _WIN32
	    puts("Press enter to exit.");
	    getchar();
	#endif

	exit(EXIT_SUCCESS);
}

#ifdef _WIN32
void net_init_WSA(void)
{
	WSADATA wsadata;

	// Request winsock 2.2 for now.
	// Should be compatible down to Win XP.
	WORD version = MAKEWORD(2, 2);

	int err = WSAStartup(version, &wsadata);
	if (err != 0) {
		fprintf(stderr, "WSAStartup failed. Error: %d.\n", err);
		exit(EXIT_FAILURE);
	}
}
#endif

// socket close and cleanup
void net_close(int sd)
{
	#ifdef _WIN32
		closesocket(sd);
		WSACleanup();
	#else
		close(sd);
	#endif
}

// Opens and connects socket
// http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
// https://bugs.chromium.org/p/chromium/issues/detail?id=44489
int net_connect(const char *host, const char *port)
{
	int sd;

	struct addrinfo hints;
	struct addrinfo *server_info, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	#ifdef _WIN32
	  net_init_WSA();
	#endif

	int ret = getaddrinfo(host, port, &hints, &server_info);
	if (ret != 0) {
		fprintf(stderr, "Name resolution failed.\n");
		#ifdef _WIN32
			fprintf(stderr, "Error %d: %s", ret, gai_strerror(ret));
		#else
			fprintf(stderr, "Error %d: %s\n", ret, gai_strerror(ret));
		#endif

		exit(EXIT_FAILURE);
	}

	// Go through the hosts and try to connect
	for (p = server_info; p != NULL; p = p->ai_next) {
		sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (sd == -1)
			continue;

		ret = connect(sd, p->ai_addr, p->ai_addrlen);
		if (ret == -1) {
			net_close(sd);
			continue;
		}
		// Get out of the loop when connect is successful
		break;
	}

	if (p == NULL) {
		/* TODO (Tiiffi): Check why windows does not report errors */
		fprintf(stderr, "Connection failed.\n");
		#ifndef _WIN32
			fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
		#endif

		freeaddrinfo(server_info);
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(server_info);
	return sd;
}

int net_send(int sd, const uint8_t *buff, size_t size)
{
	size_t sent = 0;
	size_t left = size;

	while (sent < size) {
		int result = send(sd, (const char *) buff + sent, left, 0);

		if (result == -1)
			return -1;

		sent += result;
		left -= sent;
	}

	return 0;
}

int net_send_packet(int sd, rc_packet *packet)
{
	int len;
	int total = 0;	// bytes we've sent
	int bytesleft;	// bytes left to send 
	int ret = -1;

	bytesleft = len = packet->size + sizeof(int);

	while (total < len) {
		ret = send(sd, (char *) packet + total, bytesleft, 0);
		if(ret == -1) break;
		total += ret;
		bytesleft -= ret;
	}

	return ret == -1 ? -1 : 1;
}

rc_packet *net_recv_packet(int sd)
{
	int psize;
	static rc_packet packet = {0, 0, 0, { 0x00 }};

	// packet.size = packet.id = packet.cmd = 0;

	int ret = recv(sd, (char *) &psize, sizeof(int), 0);

	if (ret == 0) {
		fprintf(stderr, "Connection lost.\n");
		global_connection_alive = 0;
		return NULL;
	}

	if (ret != sizeof(int)) {
		fprintf(stderr, "Error: recv() failed. Invalid packet size (%d).\n", ret);
		global_connection_alive = 0;
		return NULL;
	}

	// NOTE(Tiiffi): This should fail if size is out of spec!
	if (psize < 10 || psize > DATA_BUFFSIZE) {
		fprintf(stderr, "Warning: invalid packet size (%d). Must over 10 and less than %d.\n", psize, DATA_BUFFSIZE);

		if(psize > DATA_BUFFSIZE  || psize < 0) psize = DATA_BUFFSIZE;
		net_clean_incoming(sd, psize);

		return NULL;
	}

	packet.size = psize;

	int received = 0;
	while (received < psize) {
		ret = recv(sd, (char *) &packet + sizeof(int) + received, psize - received, 0);
		if (ret == 0) { /* connection closed before completing receving */
			fprintf(stderr, "Connection lost.\n");
			global_connection_alive = 0;
			return NULL;
		}

		received += ret;
	}

	return &packet;
}

int net_clean_incoming(int sd, int size)
{
	char tmp[size];
	int ret = recv(sd, tmp, size, 0);

	if(ret == 0) {
		fprintf(stderr, "Connection lost.\n");
		global_connection_alive = 0;
	}

	return ret;
}

void print_color(int color)
{
	// sh compatible color array
	#ifndef _WIN32
	char *colors[] = {
		"\033[0;30m",   /* 00 BLACK     0x30 */
		"\033[0;34m",   /* 01 BLUE      0x31 */
		"\033[0;32m",   /* 02 GREEN     0x32 */
		"\033[0;36m",   /* 03 CYAN      0x33 */
		"\033[0;31m",   /* 04 RED       0x34 */
		"\033[0;35m",   /* 05 PURPLE    0x35 */
		"\033[0;33m",   /* 06 GOLD      0x36 */
		"\033[0;37m",   /* 07 GREY      0x37 */
		"\033[0;1;30m", /* 08 DGREY     0x38 */
		"\033[0;1;34m", /* 09 LBLUE     0x39 */
		"\033[0;1;32m", /* 10 LGREEN    0x61 */
		"\033[0;1;36m", /* 11 LCYAN     0x62 */
		"\033[0;1;31m", /* 12 LRED      0x63 */
		"\033[0;1;35m", /* 13 LPURPLE   0x64 */
		"\033[0;1;33m", /* 14 YELLOW    0x65 */
		"\033[0;1;37m", /* 15 WHITE     0x66 */
		"\033[4m"       /* 16 UNDERLINE 0x6e */
	};

	/* 0x72: 'r' */
	if (color == 0 || color == 0x72) fputs("\033[0m", stdout); /* CANCEL COLOR */
	else
	#endif
	{
		if (color >= 0x61 && color <= 0x66) color -= 0x57;
		else if (color >= 0x30 && color <= 0x39)
			color -= 0x30;
		else if (color == 0x6e)
			color = 16; /* 0x6e: 'n' */
		else return;

		#ifndef _WIN32
			fputs(colors[color], stdout);
		#else
			SetConsoleTextAttribute(console_handle, color);
		#endif
	}
}

// this hacky mess might use some optimizing
void packet_print(rc_packet *packet)
{
	if (global_raw_output == 1) {
		for (int i = 0; packet->data[i] != 0; ++i)
			putchar(packet->data[i]);

		return;
	}

	int i;
	int def_color = 0;

	#ifdef _WIN32
		CONSOLE_SCREEN_BUFFER_INFO console_info;
		if (GetConsoleScreenBufferInfo(console_handle, &console_info) != 0) {
			def_color = console_info.wAttributes + 0x30;
		} else def_color = 0x37;
	#endif

	// colors enabled so try to handle the bukkit colors for terminal
	if (global_disable_colors == 0) {
		for (i = 0; (unsigned char) packet->data[i] != 0; ++i) {
			if (packet->data[i] == 0x0A) print_color(def_color);
			else if((unsigned char) packet->data[i] == 0xc2 && (unsigned char) packet->data[i+1] == 0xa7) {
				print_color(packet->data[i+=2]);
				continue;
			}
			putchar(packet->data[i]);
		}
		print_color(def_color); // cancel coloring
	}
	// strip colors
	else {
		for (i = 0; (unsigned char) packet->data[i] != 0; ++i) {
			if ((unsigned char) packet->data[i] == 0xc2 && (unsigned char) packet->data[i+1] == 0xa7) {
				i+=2;
				continue;
			}	
			putchar(packet->data[i]);
		}
	}

	// print newline if string has no newline
	if (packet->data[i-1] != 10 && packet->data[i-1] != 13) putchar('\n');
}

rc_packet *packet_build(int id, int cmd, char *s1)
{
	static rc_packet packet = {0, 0, 0, { 0x00 }};

	// size + id + cmd + s1 + s2 NULL terminator
	int len = strlen(s1);
	if (len >= DATA_BUFFSIZE) {
		fprintf(stderr, "Warning: Command string too long (%d). Maximum allowed: %d.\n", len, DATA_BUFFSIZE - 1);
		return NULL;
	}

	packet.size = sizeof(int) * 2 + len + 2;
	packet.id = id;
	packet.cmd = cmd;
	strncpy(packet.data, s1, DATA_BUFFSIZE - 1);

	return &packet;
}

int rcon_auth(int sock, char *passwd)
{
	int ret;

	rc_packet *packet = packet_build(RCON_PID, RCON_AUTHENTICATE, passwd);
	if (packet == NULL)
		return 0;

	ret = net_send_packet(sock, packet);
	if (!ret)
		return 0; // send failed

	packet = net_recv_packet(sock);
	if (packet == NULL)
		return 0;

	// return 1 if authentication OK
	return packet->id == -1 ? 0 : 1;
}

int rcon_command(int sock, char *command)
{
	rc_packet *packet = packet_build(RCON_PID, RCON_EXEC_COMMAND, command);
	if (packet == NULL) {
		global_connection_alive = 0;
		return 0;
	}

	net_send_packet(sock, packet);

	packet = net_recv_packet(sock);
	if (packet == NULL)
		return 0;

	if (packet->id != RCON_PID)
		return 0;

	if (!global_silent_mode) {
		if (packet->size > 10)
		packet_print(packet);
	}

	return 1;
}

int run_commands(int argc, char *argv[])
{
	int i = optind;

	for (;;) {
		if (!rcon_command(global_rsock, argv[i]))
			return EXIT_FAILURE;

		if (++i >= argc)
			return EXIT_SUCCESS;

		if (global_wait_seconds > 0) {
			#ifdef _WIN32
				Sleep(global_wait_seconds * 1000);
			#else
				sleep(global_wait_seconds);
			#endif
		}
	}
}

// interactive terminal mode
int run_terminal_mode(int sock)
{
	int ret = 0;
	char command[DATA_BUFFSIZE] = {0x00};

	puts("Logged in.\nType 'Q' or press Ctrl-D / Ctrl-C to disconnect.");

	while (global_connection_alive) {
		putchar('>');

		int len = get_line(command, DATA_BUFFSIZE);
		if (len < 1) continue; 
	
		if (strcasecmp(command, "Q") == 0)
			break;

		if (len > 0 && global_connection_alive)
			ret += rcon_command(sock, command);

		/* Special case for "stop" command to prevent server-side bug.
		 * https://bugs.mojang.com/browse/MC-154617
		 * 
		 * NOTE: This is hacky workaround which should be handled better to
		 *       ensure compatibility with other servers using source RCON.
		 * NOTE: strcasecmp() is POSIX function.
		 */
		if (strcasecmp(command, "stop") == 0) {
			break;
		}

		//command[0] = len = 0;
	}

	return ret;
}

// gets line from stdin and deals with rubbish left in the input buffer
int get_line(char *buffer, int bsize)
{
	char *ret = fgets(buffer, bsize, stdin);
	if (ret == NULL) {
		if (ferror(stdin)) {
			fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
		putchar('\n');
		exit(EXIT_SUCCESS);
	}

	// remove unwanted characters from the buffer
	buffer[strcspn(buffer, "\r\n")] = '\0';

	int len = strlen(buffer);

	// clean input buffer if needed 
	if (len == bsize - 1) {
		int ch;
		while ((ch = getchar()) != '\n' && ch != EOF);
	}

	return len;
}
