# if you want to cross compile
# export PATH=$PATH:/path/to/compiler/bin
# export CROSS_COMPILE=arm-none-linux-gnueabi-
# make

EXENAME = mcrcon
PREFIX ?= /usr/local

EXTRAFLAGS ?= -fstack-protector-strong

INSTALL = install
LINKER =
RM = rm -f

ifeq ($(OS), Windows_NT)
    CC = gcc
	LINKER = -lws2_32
	EXENAME = mcrcon.exe
	RM = cmd /C del /F
endif

ifeq ($(shell uname), Darwin)
	INSTALL = ginstall
	CFLAGS ?= -std=gnu99 -Wall -Wextra -Wpedantic -Os
else
	CFLAGS ?= -std=gnu99 -Wall -Wextra -Wpedantic -Os -s
endif

.PHONY: all
all: $(EXENAME)

$(EXENAME): mcrcon.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(EXTRAFLAGS) -o $@ $< $(LINKER)

ifneq ($(OS), Windows_NT)
.PHONY: install
install:
	$(INSTALL) -vD $(EXENAME) $(PREFIX)/bin/$(EXENAME)
	$(INSTALL) -vD -m 0644 mcrcon.1 $(PREFIX)/share/man/man1/mcrcon.1
	@echo "\nmcrcon installed. Run 'make uninstall' if you want to uninstall.\n"

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(EXENAME) $(PREFIX)/share/man/man1/mcrcon.1
	@echo "\nmcrcon uninstalled.\n"
endif

.PHONY: clean
clean:
	$(RM) $(EXENAME)
