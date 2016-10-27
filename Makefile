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
CFLAGS = -std=gnu11 -Wall -Wextra -Wpedantic -O2

all:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(EXENAME) mcrcon.c $(LINKER)

clean:
	$(RM) $(EXENAME)

