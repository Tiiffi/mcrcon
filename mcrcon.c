/*
 * Copyright (c) 2012-2016, Tiiffi <tiiffi -> gmail_dot_com>
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
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#ifdef _WIN32
    // for name resolving on windows
    // enable this if you get compiler whine about getaddrinfo on windows
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

#define VERSION "0.6.1"
#define IN_NAME "mcrcon"
#define VER_STR IN_NAME" "VERSION" (built: "__DATE__" "__TIME__")"

#define RCON_EXEC_COMMAND       2
#define RCON_AUTHENTICATE       3
#define RCON_RESPONSEVALUE      0
#define RCON_AUTH_RESPONSE      2
#define RCON_PID                0xBADC0DE

// a bit too big perhaps?
#define DATA_BUFFSIZE 10240

// rcon packet structure
typedef struct _rc_packet {
    int size;
    int id;
    int cmd;
    char data[DATA_BUFFSIZE];
    // ignoring string2 atm.
} rc_packet;

// ===================================
//  FUNCTION DEFINITIONS              
// ===================================

// endianness related functions
bool    is_bigendian(void);
int32_t reverse_int32(int32_t n);

// Network related functions
#ifdef _WIN32
void		net_init_WSA(void);
#endif
void		net_close(int sd);
int		net_connect(const char *host, const char *port);
int		net_send(int sd, const uint8_t *buffer, size_t size);
int		net_send_packet(int sd, rc_packet *packet);
rc_packet*	net_recv_packet(int sd);
int		net_clean_incoming(int sd, int size);

// Misc stuff
void		usage(void);
#ifndef _WIN32
void		print_color(int color);
#endif
int		get_line(char *buffer, int len);
int		run_terminal_mode(int rsock);
int		run_commands(int argc, char *argv[]);

// Rcon protocol related functions
rc_packet*	packet_build(int id, int cmd, char *s1);
uint8_t		*packet_build_malloc(size_t *size, int32_t id, int32_t cmd, char *string);
void		packet_print(rc_packet *packet);

int		rcon_auth(int rsock, char *passwd);
int		rcon_command(int rsock, char *command);


// =============================================
//  GLOBAL VARIABLES
// =============================================
static int raw_output = 0;
static int silent_mode = 0;
static int print_colors = 1;
static int connection_alive = 1;
static int rsock;

#ifdef _WIN32
  // console coloring on windows
  HANDLE console_handle;
#endif

// safety stuff (windows is still misbehaving)
void exit_proc(void)
{
	if (rsock != -1)
		net_close(rsock);
}

// Check windows & linux behaviour !!!
void sighandler(/*int sig*/)
{
	connection_alive = 0;
	#ifndef _WIN32
	    exit(EXIT_SUCCESS);
	#endif
}

int main(int argc, char *argv[])
{
	int opt;
	int terminal_mode = 0;

	char *host = getenv("MCRCON_HOST");
	char *pass = getenv("MCRCON_PASS");
	char *port = getenv("MCRCON_PORT");
	
	if (!port)
		port = "25575";

	if (!host)
		host = "localhost";

	if(argc < 1 && pass == NULL)
		usage();

	// default getopt error handler enabled
	opterr = 1;

	while ((opt = getopt(argc, argv, "vrtcshH:p:P:i")) != -1)
	{
		switch (opt)
		{
			case 'H': host = optarg;	break;
			case 'P': port = optarg;	break;
			case 'p': pass = optarg;	break;
			case 'C':
			case 'c': print_colors = 0;	break;
			case 'S':
			case 's': silent_mode = 1;	break;
			case 'T':
			case 't':
			case 'I':
			case 'i': terminal_mode = 1;	break;
			case 'r': raw_output = 1;	break;
			case 'v':
				puts(VER_STR"\nhttps://github.com/Tiiffi/mcrcon");
				exit(EXIT_SUCCESS);
			break;
			case 'h':
			case '?': usage();		break;
			/*
			  if(optopt == 'P' || optopt == 'H' || optopt == 'p')
		    	  fprintf (stderr, "Option -%c requires an argument.\n\n", optopt);
			  else fprintf (stderr, "Unknown option -%c\n\n", optopt);
			*/

			default: exit(EXIT_FAILURE);
		}
	}

	if (pass == NULL)
	{
		fputs("Password not defined (-p flag). Try 'mcrcon -h' 'man mcrcon' for more information.\n\n", stdout);
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
	    if (console_handle == INVALID_HANDLE_VALUE) console_handle = NULL;
	#endif

	// open socket
	rsock = net_connect(host, port);

	// auth & commands
	if (rcon_auth(rsock, pass))
	{
		if (terminal_mode)
			run_terminal_mode(rsock);
		else
			run_commands(argc, argv);
	}
	else // auth failed
		fprintf(stdout, "Authentication failed!\n");

	net_close(rsock);
	rsock = -1;

	return EXIT_SUCCESS;
}

void usage(void)
{
	fputs(
		"Usage: "IN_NAME" [OPTIONS]... [COMMANDS]...\n\n"
		"Sends rcon commands to Minecraft server.\n\n"
		"Option:\n"
		"  -h\t\tPrint usage\n"
		"  -H\t\tServer address (default is localhost)\n"
		"  -P\t\tPort (default is 25575)\n"
		"  -p\t\tRcon password\n"
		"  -t\t\tInteractive terminal mode\n"
		"  -s\t\tSilent mode (do not print received packets)\n"
		"  -c\t\tDisable colors\n"
		"  -r\t\tOutput raw packets (debugging and custom handling)\n"
		"  -v\t\tOutput version information\n\n"
		"Server address, port and password can be set using following environment variables:\n"
		"  MCRCON_HOST\n"
		"  MCRCON_PORT\n"
		"  MCRCON_PASS\n\n"
		,stdout
	);

	puts("Command-line options will override environment variables.");
	puts("Rcon commands with arguments must be enclosed in quotes.\n");
	puts("Example:\n\t"IN_NAME" -H my.minecraft.server -p password \"say Server is restarting!\" save-all stop\n");
	puts(VER_STR"\nReport bugs to tiiffi_at_gmail_dot_com or https://github.com/Tiiffi/mcrcon/issues/\n");

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
	if (err != 0)
	{
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
	if (ret != 0)
	{
		fprintf(stderr, "Name resolution failed.\n");
		#ifdef _WIN32
			fprintf(stderr, "Error %d: %s", ret, gai_strerror(ret));
		#else
			fprintf(stderr, "Error %d: %s\n", ret, gai_strerror(ret));
		#endif
		exit(EXIT_FAILURE);
	}

	// Go through the hosts and try to connect
	for (p = server_info; p != NULL; p = p->ai_next)
	{
		sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sd == -1)
			continue;

		ret = connect(sd, p->ai_addr, p->ai_addrlen);
		if (ret == -1)
		{
			net_close(sd);
			continue;
		}
		// Get out of the loop when connect is successful
		break;
	}

	if (p == NULL)
	{
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

	while (sent < size)
	{
		int result = send(sd, (const char *) buff + sent, left, 0);

		if (result == -1) return -1;

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

	while(total < len)
	{
		ret = send(sd, (char *) packet + total, bytesleft, 0);
		if(ret == -1) { break; }
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

	if (ret == 0)
	{
		fprintf(stderr, "Connection lost.\n");
		connection_alive = 0;
		return NULL;
	}

	if (ret != sizeof(int))
	{
		fprintf(stderr, "Error: recv() failed. Invalid packet size (%d).\n", ret);
		connection_alive = 0;
		return NULL;
	}

	if (psize < 10 || psize > DATA_BUFFSIZE)
	{
		fprintf(stderr, "Warning: invalid packet size (%d). Must over 10 and less than %d.\n", psize, DATA_BUFFSIZE);

		if(psize > DATA_BUFFSIZE  || psize < 0) psize = DATA_BUFFSIZE;
		net_clean_incoming(sd, psize);

		return NULL;
	}

	packet.size = psize;

	ret = recv(sd, (char *) &packet + sizeof(int), psize, 0);
	if (ret == 0)
	{
		fprintf(stderr, "Connection lost.\n");
		connection_alive = 0;
		return NULL;
	}

	if(ret != psize)
	{
		fprintf(stderr, "Warning: recv() return value (%d) does not match expected packet size (%d).\n", ret, psize);
		net_clean_incoming(sd, DATA_BUFFSIZE); /* Should be enough. Needs some checking */
		return NULL;
	}

	return &packet;
}

int net_clean_incoming(int sd, int size)
{
	char tmp[size];

	int ret = recv(sd, tmp, size, 0);

	if(ret == 0)
	{
		fprintf(stderr, "Connection lost.\n");
		connection_alive = 0;
	}

	return ret;
}

void print_color(int color)
{
	// sh compatible color array
	#ifndef _WIN32
	char *colors[] =
	{
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
		"\033[0:1;36m", /* 11 LCYAN     0x62 */
		"\033[0;1;31m", /* 12 LRED      0x63 */
		"\033[0;1;35m", /* 13 LPURPLE   0x64 */
		"\033[0;1;33m", /* 14 YELLOW    0x65 */
		"\033[0;1;37m", /* 15 WHITE     0x66 */
		"\033[4m"       /* 16 UNDERLINE 0x6e */
	};

	if(color == 0 || color == 0x72) /* 0x72: 'r' */
	{
		fputs("\033[0m", stdout); /* CANCEL COLOR */
	}
	else
	#endif
	{
		if(color >= 0x61 && color <= 0x66) color -= 0x57;
		else if(color >= 0x30 && color <= 0x39) color -= 0x30;
		else if(color == 0x6e) color=16; /* 0x6e: 'n' */
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
	if (raw_output == 1)
	{
		for (int i = 0; packet->data[i] != 0; ++i) putchar(packet->data[i]);
		return;
	}

	int i;
	int def_color = 0;

	#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO console_info;

	if (GetConsoleScreenBufferInfo(console_handle, &console_info) != 0)
		def_color = console_info.wAttributes + 0x30;
	else
		def_color = 0x37;
	#endif

	// colors enabled so try to handle the bukkit colors for terminal
	if (print_colors == 1)
	{
		for (i = 0; (unsigned char) packet->data[i] != 0; ++i)
		{
			if (packet->data[i] == 0x0A) print_color(def_color);
			else if((unsigned char) packet->data[i] == 0xc2 && (unsigned char) packet->data[i+1] == 0xa7){
				print_color(packet->data[i+=2]);
				continue;
			}
			putchar(packet->data[i]);
		}
		print_color(def_color); // cancel coloring
	}
	// strip colors
	else
	{
		for (i = 0; (unsigned char) packet->data[i] != 0; ++i)
		{
			if ((unsigned char) packet->data[i] == 0xc2 && (unsigned char) packet->data[i+1] == 0xa7){
				i+=2;
				continue;
			}	
			putchar(packet->data[i]);
		}
	}

	// print newline if string has no newline
	if (packet->data[i-1] != 10 && packet->data[i-1] != 13)
		putchar('\n');
}

rc_packet *packet_build(int id, int cmd, char *s1)
{
	static rc_packet packet = {0, 0, 0, { 0x00 }};

	// size + id + cmd + s1 + s2 NULL terminator
	int s1_len = strlen(s1);
	if (s1_len > DATA_BUFFSIZE)
	{
		fprintf(stderr, "Warning: Command string too long (%d). Maximum allowed: %d.\n", s1_len, DATA_BUFFSIZE);
		return NULL;
	}

	packet.size = sizeof(int) * 2 + s1_len + 2;
	packet.id = id;
	packet.cmd = cmd;
	strncpy(packet.data, s1, DATA_BUFFSIZE);

	return &packet;
}

// TODO(Tiiffi): String length limit?
uint8_t *packet_build_malloc(size_t *size, int32_t id, int32_t cmd, char *string)
{
	size_t string_length = strlen(string);

	*size = 3 * sizeof(int32_t) + string_length + 2;
	uint8_t *packet = malloc(*size);
	if (packet == NULL) return NULL;

	int32_t *p = (int32_t *) packet;
	p[0] = (int32_t) *size - sizeof(int32_t);
	p[1] = id;
	p[2] = cmd;

	memcpy(&p[3], string, string_length);

	packet[12 + string_length] = 0;
	packet[13 + string_length] = 0;

	return packet;
}

// rcon packet structure
#define MAX_PACKET_SIZE  (size_t) 1460 // including size member
#define MIN_PACKET_SIZE  (size_t) 10
#define MAX_STRING_SIZE  (size_t) (MAX_PACKET_SIZE - 2 - 3 * sizeof(int32_t))
#define SIZEOF_PACKET(x) (size_t) (x.size + sizeof(int32_t))

struct rcon_packet
{
	int32_t size;
	int32_t id;
	int32_t cmd;
	uint8_t data[MAX_STRING_SIZE];
};

struct rcon_packet packet_build_new(int32_t id, int32_t cmd, char *string)
{
	struct rcon_packet packet;
	size_t string_length = strlen(string);

	if (string_length > MAX_STRING_SIZE)
	{
		string_length = MAX_STRING_SIZE;
		fprintf(stderr,
			"Warning: command string is too long. Truncating to "
			"%u characters.\n", (unsigned) MAX_STRING_SIZE
		);
	}

	packet.size = 2 * sizeof(int32_t) + string_length + 2;
	packet.id = id;
	packet.cmd = cmd;
	memcpy(packet.data, string, string_length);
	packet.data[string_length] = 0;
	packet.data[string_length + 1] = 0;

	return packet;
}

int rcon_auth(int rsock, char *passwd)
{
	int ret;

	rc_packet *packet = packet_build(RCON_PID, RCON_AUTHENTICATE, passwd);
	if (packet == NULL)
		return 0;

	ret = net_send_packet(rsock, packet);
	if (!ret)
		return 0; // send failed

	packet = net_recv_packet(rsock);
	if (packet == NULL)
		return 0;

	// return 1 if authentication OK
	return packet->id == -1 ? 0 : 1;
}

int rcon_command(int rsock, char *command)
{
	int ret; (void) ret;

	size_t size;
	uint8_t *p = packet_build_malloc(&size, RCON_PID, RCON_EXEC_COMMAND, command);
	if (p == NULL)
	{
		connection_alive = 0;
		return 0;
	}

	net_send(rsock, p, size);

	free(p);

	//ret = net_send_packet(rsock, packet);
	//if(!ret) return 0; /* send failed */

	rc_packet *packet;
	packet = net_recv_packet(rsock);
	if (packet == NULL)
		return 0;

	if (packet->id != RCON_PID)
		return 0;

	if (!silent_mode)
	{
	/*
	if(packet->size == 10) {
	    printf("Unknown command \"%s\". Type \"help\" or \"?\" for help.\n", command);
	}
	else
	*/
		if (packet->size > 10)
			packet_print(packet);
	}

	return 1;
}

int run_commands(int argc, char *argv[])
{
	int i, ok = 1, ret = 0;

	for (i = optind; i < argc && ok; i++)
	{
		ok = rcon_command(rsock, argv[i]);
		ret += ok;
	}

	return ret;
}

// interactive terminal mode
int run_terminal_mode(int rsock)
{
	int ret = 0;
	char command[DATA_BUFFSIZE] = {0x00};

	puts("Logged in. Type \"Q\" to quit!");

	while (connection_alive)
	{
		int len = get_line(command, DATA_BUFFSIZE);
		if(command[0] == 'Q' && command[1] == 0)
			break;

		if(len > 0 && connection_alive)
			ret += rcon_command(rsock, command);

		command[0] = len = 0;
	}

	return ret;
}

// gets line from stdin and deals with rubbish left in the input buffer
int get_line(char *buffer, int bsize)
{
	int ch, len;

	fputs(">", stdout);
	(void) fgets(buffer, bsize, stdin);

	if (buffer[0] == 0)
		connection_alive = 0;

	// remove unwanted characters from the buffer
	buffer[strcspn(buffer, "\r\n")] = '\0';

	len = strlen(buffer);

	// clean input buffer if needed 
	if (len == bsize - 1)
		while ((ch = getchar()) != '\n' && ch != EOF);

	return len;
}

// http://www.ibm.com/developerworks/aix/library/au-endianc/
bool is_bigendian(void)
{
	const int32_t n = 1;
	if (*(uint8_t *) &n == 0 ) return true;
	return false;
}

int32_t reverse_int32(int32_t n)
{
	int32_t tmp;
	uint8_t *t = (uint8_t *) &tmp;
	uint8_t *p = (uint8_t *) &n;

	t[0] = p[3];
	t[1] = p[2];
	t[2] = p[1];
	t[3] = p[0];

	return tmp;
}
