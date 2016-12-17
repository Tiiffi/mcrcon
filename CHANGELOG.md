####Version history:
######0.6.0
 - Version numbering changed to more sane system (0.0.5 -> 0.6.0)
 - Fixed munged output
 - Support for using environment variables to set some basic options
 - Cleaned networking code
 - Various code cleanups
 - Changes and updates in usage text and error reporting
 - Version option flag (-v) added
 - Man page added 
 - Proper makefile added

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
  - Some cosmetic changes in program output strings.
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
