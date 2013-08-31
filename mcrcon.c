/*
 * Copyright (c) 2012-2013, Tiiffi <tiiffi -> gmail_dot_com>
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

/*
 * Name:     mcrcon (minecraft rcon)
 * Version:  0.0.5
 * Date:     31.08.2012
 *
 * License: zlib/libpng License
 *
 *
 * Contact:
 *  WWW:  http://sourceforge.net/projects/mcrcon/
 *  MAIL: tiiffi_at_gmail_dot_com
 *  IRC:  tiiffi @ quakenet
 *
 *
 * Description:
 *  Mcrcon is powerful IPv6 compliant minecraft rcon client with bukkit coloring support.
 *  It is well suited for remote administration and to be used as part of automated server maintenance scripts.
 *  Does not cause "IO: Broken pipe" or "IO: Connection reset" spam in server console.
 *
 *
 * Features:
 *  - Interacive terminal mode. Keeps the connection alive.
 *  - Send multiple commands in one command line.
 *  - Silent mode. Does not print rcon output.
 *  - Support for bukkit coloring on Windows and Linux (sh compatible shells).
 *  - Multiplatform code. Compiles on many platforms with minor changes.
 *  - IPv6 support.
 *
 *
 * Version history:
 *
 * 0.0.5
 *
 *  - IPv6 support!
 *     * Thanks to 'Tanja84dk' for addressing the real need of IPv6.
 *
 *  - Fixed bug causing crash / segmentation fault (invalid write) when receiving malformed rcon packet.
 *
 *  - Program makes use of C99 feature (variable-length arrays) so "-std=gnu99" flag on
 *    GCC-compiler must be used to avoid unecessary warnings.
 *
 *  - Rcon receive buffer is now bigger (2024 bytes -> 10240 bytes).
 *     * Thanks to 'gman_ftw' @ Bukkit forums.
 *
 *  - Fixed invalid error message when receiving empty rcon packet (10 bytes).
 *     * Thanks to 'pkmnfrk' @ bukkit forums.
 *
 *  - Terminal mode now closes automatically when rcon socket is closed by server
 *    or if packet size cannot be retrieved correctly.
 *
 *  - Client now tries to clean the incoming socket data if last package was out of spec.
 *
 *
 * 0.0.4
 *  - Reverted back to default getopts options error handler (opterr = 1).
 *    Custom error handler requires rewriting.
 *  - Some comestic fixes in program output strings.
 *  - Program usage(); function now waits for enter before exiting on Windows.
 *
 *
 * 0.0.3
 *  - Colors are now supported on Windows too!
 *  - Terminal mode is now triggered with "-t" flag. "-i" flag still works for
 *    backwards compatibility.
 *  - Bug fixes (Packet size check always evaluating false and color validity
 *    check always evaluating true).
 *
 *
 * 0.0.2
 *  - License changed from 'ISC License' to 'zlib/libpng License'.
 *  - Bug fixes & code cleanups
 *  - Interactive mode (-i flag). Client acts as interactive terminal.
 *  - Program return value is now the number of rcon commmands sent successfully.
 *    If connecting or authentication fails, the return value is -1.
 *  - Colors are now enabled by default. Now '-c' flag disables the color support.
 *
 *
 * 0.0.1
 *  - Added experimental support for bukkit colors.
 *    Should work with any sh compatible shell.
 *  - Packet string data limited to max 2048 (DATA_BUFFSIZE) bytes.
 *    No idea how Minecraft handles multiple rcon packets.
 *    If someone knows, please mail me so I can implement it.
 *
 *
 * TODO:
 *  - Make the receive buffer dynamic??
 *  - Change some of the packet size issues to fatal errors.
 *  - Code cleanups.
 *  - Check global variables (remove if possible).
 *  - Add some protocol checks (proper packet id check etc..).
 *  - Preprocessor (#ifdef / #ifndef) cleanups.
 *  - Follow valve rcon protocol standard strictly?
 *  - Multiple packet support if minecraft supports it?!
 *  - Investigate if player chat messages gets sent through rcon.
 *    If they are, the messaging system requires rewriting.
 *  - Name resolving should be integrated to connection creation function.
 *  - Dont try to cleanup the socket if not authenticated
 *  - Better sockets error reporting
 *  - Better error function (VA_ARGS support)
 *
 *
 * Bug reports and feature requests to tiiffi_at_gmail_dot_com.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef _WIN32
    /* for name resolving on windows */
    #define _WIN32_WINNT 0x0501

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

/* absolute value macro
#define absolute(x) (x < 0) ? (0 - x) : x
*/

#define RCON_EXEC_COMMAND       2
#define RCON_AUTHENTICATE       3
#define RCON_RESPONSEVALUE      0
#define RCON_AUTH_RESPONSE      2
#define RCON_PID                42

/* Safe value I think. This should me made dynamic for more stable performance! */
#define DATA_BUFFSIZE 10240

#define VERSION "0.0.5"
#define IN_NAME "mcrcon"
#define VER_STR IN_NAME" "VERSION

/* rcon packet structure */
typedef struct _rc_packet {
    int size;
    int id;
    int cmd;
    char data[DATA_BUFFSIZE];
    /* ignoring string2 atm.. */
} rc_packet;

/* functions */
void            usage(void);
void            error(char *errstring);
#ifndef _WIN32
void            print_color(int color);
#endif

struct addrinfo *net_resolve(char *host, char *port);
void            net_close_socket(int sd);
int             net_open_socket(char *host, char *port);
int             net_send_packet(int sd, rc_packet *packet);
rc_packet*      net_recv_packet(int sd);
#ifdef _WIN32
void            net_init_WSA(void);
#endif
int             net_clean_incoming(int sd, int size);

rc_packet*      packet_build(int id, int cmd, char *s1);
void            packet_print(rc_packet *packet);

int             rcon_auth(int rsock, char *passwd);
int             rcon_command(int rsock, char *command);

int             get_line(char *buffer, int len);
int             run_terminal_mode(int rsock);
int             run_commands(int argc, char *argv[]);


/* some globals */
int silent_mode = 0;
int print_colors = 1;
int connection_alive = 1;
int rsock; /* rcon socket */

#ifdef _WIN32
  /* console coloring on windows */
  HANDLE console_handle;
#endif

/* safety stuff (windows is still misbehaving) */
void exit_proc(void) {
    if(rsock != -1) net_close_socket(rsock);
}

/* Check windows & linux behaviour !!! */
void sighandler(/*int sig*/) {
    connection_alive = 0;
    #ifndef _WIN32
      exit(-1);
    #endif
}

int main(int argc, char *argv[])
{
    int opt, ret = 0;
    int terminal_mode = 0;

    char *host = NULL;
    char *pass = "";
    char *port = "25575";

    if(argc < 2) usage();

    opterr = 1; /* default error handler enabled */
    while((opt = getopt(argc, argv, "tcshH:p:P:i")) != -1)
    {
        switch(opt)
        {
            case 'H': host = optarg;        break;
            case 'P': port = optarg;        break;
            case 'p': pass = optarg;        break;
            case 'C':
            case 'c': print_colors = 0;     break;
            case 'S':
            case 's': silent_mode = 1;      break;
            case 'T':
            case 't':
            case 'I':
            case 'i': terminal_mode = 1;    break;
            case 'h':
            case '?':
                /*
                if(optopt == 'P' || optopt == 'H' || optopt == 'p')
                    fprintf (stderr, "Option -%c requires an argument.\n\n", optopt);
                */

                /* else fprintf (stderr, "Unknown option -%c\n\n", optopt); */

                usage();
            break;

            default: abort();
        }
    }

    if(host == NULL) {
        fputs("Host not defined. Check -H flag.\n\n", stdout);
        usage();
    }

    if(optind == argc && terminal_mode == 0) {
        fputs("No commands specified.\n\n", stdout);
        usage();
    }

    /* safety features to prevent "IO: Connection reset" bug on the server side */
    atexit(&exit_proc);
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    #ifdef _WIN32
      net_init_WSA();
      console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
      if(console_handle == INVALID_HANDLE_VALUE) console_handle = NULL;
    #endif

    /* open socket */
    rsock = net_open_socket(host, port);

    /* auth & commands */
    if(rcon_auth(rsock, pass))
    {
        if(terminal_mode)
            ret = run_terminal_mode(rsock);
        else
            ret = run_commands(argc, argv);
    }
    else /* auth failed */
    {
        ret = -1;
        fprintf(stdout, "Authentication failed!\n");
    }

    /* cleanup */
    net_close_socket(rsock);
    rsock = -1;

    return ret;
}

void usage(void)
{
    fputs(
        "Usage: "IN_NAME" [OPTIONS]... [COMMANDS]...\n"
        "Sends rcon commands to minecraft server.\n\n"
        "Option:\n"
        "  -h\t\tPrints usage.\n"
        "  -s\t\tSilent mode. Do not print data received from rcon.\n"
        "  -t\t\tTerminal mode. Acts as interactive terminal.\n"
        "  -p\t\tRcon password. Default: \"\".\n"
        "  -H\t\tHost address or ip.\n"
        "  -P\t\tPort. Default: 25575.\n"
        "  -c\t\tDo not print colors. Disables bukkit color printing.\n"
    ,stdout);

    puts("\nInvidual commands must be separated with spaces.\n");
    puts("Example:\n  "IN_NAME" -c -H 192.168.1.42 -P 9999 -p password cmd1 \"cmd2 with spaces\"\n");
    puts("minecraft rcon ("IN_NAME") "VERSION".\nReport bugs to tiiffi_at_gmail_dot_com.\n");

    #ifdef _WIN32
      puts("Press enter to exit.");
      getchar();
    #endif
    exit(0);
}

void error(char *errstring)
{
    fputs(errstring, stderr);
    exit(-1);
}

#ifdef _WIN32
void net_init_WSA(void)
{
    WSADATA wsadata;
    int err;

    err = WSAStartup(MAKEWORD(1, 1), &wsadata);
    if(err != 0)
    {
        fprintf(stderr, "WSAStartup failed. Errno: %d.\n", err);
        exit(-1);
    }
}
#endif

struct addrinfo *net_resolve(char *host, char *port)
{
    /* !!! This function should be integrated to open_socket function for cleaner code !!! */

    //struct in_addr6 serveraddr;
    struct addrinfo hints, *result;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    /* hints.ai_flags    = AI_NUMERICSERV; // Windows retardism */

    ret = getaddrinfo(host, port, &hints, &result);
    if(ret != 0)
    {
        if(ret == EAI_SERVICE) fprintf(stderr, "Invalid port (%s).\n", port);
        fprintf(stderr, "Error: Unable to resolve hostname (%s).\n", host);
        exit(-1);
    }

    return result;
}

/* socket close and cleanup */
void net_close_socket(int sd)
{
    #ifdef _WIN32
        closesocket(sd);
        WSACleanup();
    #else
        close(sd);
    #endif
}

/* Opens and connects socket */
int net_open_socket(char *host, char *port)
{
    int sd;

    struct addrinfo *serverinfo;

    serverinfo = net_resolve(host, port);

    sd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
    if(sd < 0)
	{
        #ifdef _WIN32
            WSACleanup();
        #endif
        freeaddrinfo(serverinfo);
        error("Error: cannot create socket.\n");
	}

    if(connect(sd, serverinfo->ai_addr, serverinfo->ai_addrlen) != 0)
    {
        net_close_socket(sd);
        fprintf(stderr, "Error: connection failed (%s).\n", host);
        freeaddrinfo(serverinfo);
        exit(-1);
    }

    freeaddrinfo(serverinfo);
    return sd;
}

int net_send_packet(int sd, rc_packet *packet)
{
    int len;
    int total = 0;        /* how many bytes we've sent */
    int bytesleft;        /* how many we have left to send */
    int ret = -1;

    bytesleft = len = packet->size + sizeof(int);

    while(total < len)
    {
        ret = send(sd, (char *) packet + total, bytesleft, 0);
        if(ret == -1) { break; }
        total += ret;
        bytesleft -= ret;
    }

    /* return -1 on failure, 0 on success */
    return ret == -1 ? -1 : 1;
}

rc_packet *net_recv_packet(int sd)
{
    int psize;
    static rc_packet packet = {0, 0, 0, { 0x00 }};

    /* packet.size = packet.id = packet.cmd = 0; */

    int ret = recv(sd, (char *) &psize, sizeof(int), 0);

    if(ret == 0) {
        fprintf(stderr, "Connection lost.\n");
        connection_alive = 0;
        return NULL;
    }

    if(ret != sizeof(int)) {
        fprintf(stderr, "Error: recv() failed. Invalid packet size (%d).\n", ret);
        connection_alive = 0;
        return NULL;
    }

    if(psize < 10 || psize > DATA_BUFFSIZE) {
        fprintf(stderr, "Warning: invalid packet size (%d). Must over 10 and less than %d.\n", psize, DATA_BUFFSIZE);
        if(psize > DATA_BUFFSIZE  || psize < 0) psize = DATA_BUFFSIZE;
        net_clean_incoming(sd, psize);
        return NULL;
    }

    packet.size = psize;

    ret = recv(sd, (char *) &packet + sizeof(int), psize, 0);
    if(ret == 0) {
        fprintf(stderr, "Connection lost.\n");
        connection_alive = 0;
        return NULL;
    }
    if(ret != psize) {
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

    if(ret == 0) {
        fprintf(stderr, "Connection lost.\n");
        connection_alive = 0;
    }

    return ret;
}

void print_color(int color)
{
    /* sh compatible color array */
    #ifndef _WIN32
    char *colors[] = {
        "\033[0;30m", /* 00 BLACK    0x30 */
        "\033[0;34m", /* 01 BLUE     0x31 */
        "\033[0;32m", /* 02 GREEN    0x32 */
        "\033[0;36m", /* 03 CYAN     0x33 */
        "\033[0;31m", /* 04 RED      0x34 */
        "\033[0;35m", /* 05 PURPLE   0x35 */
        "\033[0;33m", /* 06 GOLD     0x36 */
        "\033[0;37m", /* 07 GREY     0x37 */
        "\033[1;30m", /* 08 DGREY    0x38 */
        "\033[1;34m", /* 09 LBLUE    0x39 */
        "\033[1;32m", /* 10 LGREEN   0x61 */
        "\033[1;36m", /* 11 LCYAN    0x62 */
        "\033[1;31m", /* 12 LRED     0x63 */
        "\033[1;35m", /* 13 LPURPLE  0x64 */
        "\033[1;33m", /* 14 YELLOW   0x65 */
        "\033[1;37m", /* 15 WHITE    0x66 */
    };

    if(color == 0) {
        fputs("\033[0m", stdout); /* CANCEL COLOR */
    }
    else
    #endif
    {
        if(color >= 0x61 && color <= 0x66) color -= 0x57;
        else if(color >= 0x30 && color <= 0x39) color -= 0x30;
        else return;

        #ifndef _WIN32
          fputs(colors[color], stdout);
        #else
          SetConsoleTextAttribute(console_handle, color);
        #endif
    }
}

/* this hacky mess might use some optmizing */
void packet_print(rc_packet *packet)
{
    int i;
    int def_color = 0;

    #ifdef _WIN32
      CONSOLE_SCREEN_BUFFER_INFO console_info;
      if(GetConsoleScreenBufferInfo(console_handle, &console_info) != 0)
          def_color = console_info.wAttributes + 0x30;
      else def_color = 0x37;
    #endif

    /* colors enabled so try to handle the bukkit colors for terminal */
    if(print_colors == 1) {

        for(i = 0; (unsigned char) packet->data[i] != 0; ++i) {
            if((unsigned char) packet->data[i] == 0xa7) {
                ++i;
                print_color(packet->data[i]);
                continue;
            }
            if(packet->data[i] == 0x0A) print_color(def_color);

            putchar(packet->data[i]);
        }
        print_color(def_color); /* cancel coloring */

    }
    /* strip colors */
    else
    {
        for(i = 0; (unsigned char) packet->data[i] != 0; ++i) {
            if((unsigned char) packet->data[i] == 0xa7) {
                ++i;
                continue;
            }
            putchar(packet->data[i]);
        }
    }

    /* print newline if string has no newline */
    if(packet->data[i-1] != 10 && packet->data[i-1] != 13)
        putchar('\n');
}

rc_packet *packet_build(int id, int cmd, char *s1)
{   /* hacky function */
    static rc_packet packet = {0, 0, 0, { 0x00 }};

    /* size + id + cmd + s1 + s2 NULL terminator */
    int s1_len = strlen(s1);
    if(s1_len > DATA_BUFFSIZE) {
        fprintf(stderr, "Warning: Command string too long (%d). Maximum allowed: %d.\n", s1_len, DATA_BUFFSIZE);
        return NULL;
    }

    packet.size = sizeof(int) * 2 + s1_len + 2;
    packet.id = id;
    packet.cmd = cmd;
    strncpy(packet.data, s1, DATA_BUFFSIZE);

    return &packet;
}

int rcon_auth(int rsock, char *passwd)
{
    int ret;

    rc_packet *packet = packet_build(RCON_PID, RCON_AUTHENTICATE, passwd);
    if(packet == NULL) return 0;

    ret = net_send_packet(rsock, packet);
    if(!ret) return 0; /* send failed */

    packet = net_recv_packet(rsock);
    if(packet == NULL) return 0;

    /* return 1 if authentication OK */
    return packet->id == -1 ? 0 : 1;
}

int rcon_command(int rsock, char *command)
{
    int ret;

    rc_packet *packet = packet_build(RCON_PID, RCON_EXEC_COMMAND, command);
    if(packet == NULL) {
        connection_alive = 0;
        return 0;
    }

    ret = net_send_packet(rsock, packet);
    if(!ret) return 0; /* send failed */

    packet = net_recv_packet(rsock);
    if(packet == NULL) return 0;

    if(packet->id != RCON_PID) return 0; /* wrong packet id */

    if(!silent_mode) {
        /*
        if(packet->size == 10) {
            printf("Unknown command \"%s\". Type \"help\" or \"?\" for help.\n", command);
        }
        else
        */
        if(packet->size > 10)
            packet_print(packet);
    }

    /* return 1 if world was saved */
    return 1;
}

int run_commands(int argc, char *argv[])
{
    int i, ok = 1, ret = 0;

    for(i = optind; i < argc && ok; i++) {
        ok = rcon_command(rsock, argv[i]);
        ret += ok;
    }

    return ret;
}

/* interactive terminal mode */
int run_terminal_mode(int rsock)
{
    int ret = 0;
    char command[DATA_BUFFSIZE] = {0x00};

    puts("Logged in. Type \"Q\" to quit!");

    while(connection_alive) {

        int len = get_line(command, DATA_BUFFSIZE);
        if(command[0] == 'Q' && command[1] == 0) break;

        if(len > 0 && connection_alive) ret += rcon_command(rsock, command);

        command[0] = len = 0;
    }

    return ret;
}

/* gets line from stdin and deals with rubbish left in input buffer */
int get_line(char *buffer, int bsize)
{
    int ch, len;

    fputs("> ", stdout);
    fgets(buffer, bsize, stdin);

    if(buffer[0] == 0) connection_alive = 0;

    /* remove unwanted characters from the buffer */
    buffer[strcspn(buffer, "\r\n")] = '\0';

    len = strlen(buffer);

    /* clean input buffer if needed */
    if(len == bsize - 1)
        while ((ch = getchar()) != '\n' && ch != EOF);

    return len;
}
