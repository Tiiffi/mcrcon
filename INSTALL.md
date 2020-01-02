Building and installing
------------------------

Only dependency is C library and POSIX getopt support. 

Compiling with GCC or CLANG:

    cc -std=gnu99 -Wpedantic -Wall -Wextra -Os -s -o mcrcon mcrcon.c
    
Note: on Windows remember to link with winsock by adding `-lws2_32` to your compiler command line.

Or you can just run "**make**":

    make           - compiles mcrcon
    make install   - installs compiled binaries and manpage to the system
    make uninstall - removes binaries and manpage from the system
    
    file install locations:
        /usr/local/bin/mcrcon
        /usr/local/share/man/man1/mcrcon.1

Makefile "**install**" and "**uninstall**" rules are disabled on windows.
