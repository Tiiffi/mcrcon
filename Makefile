# if you want to cross compile 
# export PATH=$PATH:/path/to/compiler/bin
# export CROSS_COMPILE=arm-none-linux-gnueabi-
# make

CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -Wpedantic -O2

all:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o mcrcon mcrcon.c

clean:
	rm -f mcrcon


