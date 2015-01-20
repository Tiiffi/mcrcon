####Compiling:

Raw command:
```gcc -std=gnu11 -pedantic -Wall -Wextra -O2 -s -o mcrcon mcrcon.c```

or just run **make**.

On windows, remember to link with winsockets.
Add ```-lws2_32``` to compiler command line on Mingw GCC.

---

####Usage:
Usage: mcrcon [OPTIONS]... [COMMANDS]...
Sends rcon commands to minecraft server.

```
Option:
  -h		Prints usage.
  -s		Silent mode. Do not print data received from rcon.
  -t		Terminal mode. Acts as interactive terminal.
  -p		Rcon password. Default: "".
  -H		Host address or ip.
  -P		Port. Default: 25575.
  -c		Do not print colors. Disables bukkit color printing.
  -r		Print everything in raw mode.
		    Good for debugging and custom handling of the output.
```
Invidual commands must be separated with spaces.

Example:
  ```mcrcon -c -H 192.168.1.42 -P 9999 -p password cmd1 "cmd2 with spaces"```

#####Enable rcon
Remember to enable rcon by changing/adding these lines to ```server.properties``` file.
```
enable-rcon=true
rcon.password=your_rcon_pasword
rcon.port=9999
```

---

####Contact:
```
WWW:            [http://sourceforge.net/projects/mcrcon/] (http://sourceforge.net/projects/mcrcon/)
MAIL:           tiiffi_at_gmail_dot_com
IRC:            tiiffi @ quakenet
BUG REPORTS:    [https://github.com/Tiiffi/mcrcon/issues] (https://github.com/Tiiffi/mcrcon/issues)
```

---

####Version history:
######0.0.5
  - IPv6 support!
     * Thanks to 'Tanja84dk' for addressing the real need of IPv6.

  - Fixed bug causing crash / segmentation fault (invalid write) when receiving malformed rcon packet.

  - Program makes use of C99 feature (variable-length arrays) so "-std=gnu99" flag on
    GCC-compiler must be used to avoid unecessary warnings.

  - Rcon receive buffer is now bigger (2024 bytes -> 10240 bytes).
     * Thanks to 'gman_ftw' @ Bukkit forums.

  - Fixed invalid error message when receiving empty rcon packet (10 bytes).
     * Thanks to 'pkmnfrk' @ bukkit forums.

  - Terminal mode now closes automatically when rcon socket is closed by server
    or if packet size cannot be retrieved correctly.

  - Client now tries to clean the incoming socket data if last package was out of spec.

######0.0.4
  - Reverted back to default getopts options error handler (opterr = 1).
    Custom error handler requires rewriting.
  - Some comestic fixes in program output strings.
  - Program usage(); function now waits for enter before exiting on Windows.

######0.0.3
  - Colors are now supported on Windows too!
  - Terminal mode is now triggered with "-t" flag. "-i" flag still works for
    backwards compatibility.
  - Bug fixes (Packet size check always evaluating false and color validity
    check always evaluating true).

######0.0.2
  - License changed from 'ISC License' to 'zlib/libpng License'.
  - Bug fixes & code cleanups
  - Interactive mode (-i flag). Client acts as interactive terminal.
  - Program return value is now the number of rcon commmands sent successfully.
    If connecting or authentication fails, the return value is -1.
  - Colors are now enabled by default. Now '-c' flag disables the color support.

######0.0.1
  - Added experimental support for bukkit colors.
    Should work with any sh compatible shell.
  - Packet string data limited to max 2048 (DATA_BUFFSIZE) bytes.
    No idea how Minecraft handles multiple rcon packets.
    If someone knows, please mail me so I can implement it.

####TODO:
  - Make the receive buffer dynamic??
  - Change some of the packet size issues to fatal errors.
  - Code cleanups.
  - Check global variables (remove if possible).
  - Add some protocol checks (proper packet id check etc..).
  - Preprocessor (#ifdef / #ifndef) cleanups.
  - Follow valve rcon protocol standard strictly?
  - Multiple packet support if minecraft supports it?!
  - Investigate if player chat messages gets sent through rcon.
    If they are, the messaging system requires rewriting.
  - Name resolving should be integrated to connection creation function.
  - Dont try to cleanup the socket if not authenticated
  - Better sockets error reporting
  - Better error function (VA_ARGS support)
