Compiling and installing
------------------------

Only dependency is C library with POSIX getopt support. 

Compiling with GCC or CLANG:

    cc -std=gnu99 -Wpedantic -Wall -Wextra -Os -s -o mcrcon mcrcon.c
    
Note: on Window remember to link with winsockets by adding "-lws2_32" to your compiler command line.

Or you can just run "make":

    make           - compiles mcrcon
    make install   - installs compiled binaries and manpage to the system
    make uninstall - removes binaries and manpage from the system
    
    file install locations:
        /usr/share/bin/mcrcon
        /usr/share/man/man1/mcrcon.1

Makefile "install" and "uninstall" rules are disabled on windows.
