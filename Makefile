# if you want to cross compile 
# export PATH=$PATH:/path/to/compiler/bin
# export CROSS_COMPILE=arm-none-linux-gnueabi-
# make

ifeq ($(OS), Windows_NT)
	LINKER = -lws2_32
	EXENAME = mcrcon.exe
	RM = cmd /C del /F
else
	LINKER =
	EXENAME = mcrcon
	RM = rm -f
endif

CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Wpedantic -Os -s
EXTRAFLAGS = -fstack-protector-strong

all:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(EXTRAFLAGS) -o $(EXENAME) mcrcon.c $(LINKER)

ifneq ($(OS), Windows_NT)
install:
	cp $(EXENAME) /usr/local/bin/$(EXENAME)
	chmod 0755 /usr/local/bin/$(EXENAME)
	cp mcrcon.1 /usr/local/share/man/man1/mcrcon.1
	chmod 0644 /usr/local/share/man/man1/mcrcon.1
	@echo "\nmcrcon installed. Run 'make uninstall' if you want to uninstall.\n"
uninstall:
	rm -f /usr/local/bin/$(EXENAME)
	rm -f /usr/local/share/man/man1/mcrcon.1
	@echo "\nmcrcon uninstalled.\n"
endif

clean:
	$(RM) $(EXENAME)
