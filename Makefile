# Cross comple, eg. 
# export PATH=$PATH:/path/to/compiler/bin
# export CROSS_COMPILE=arm-none-linux-gnueabi-
# make

all:
	$(CROSS_COMPILE)gcc -std=gnu99 -pedantic -Wall -Wextra -O2 -s -o mcrcon mcrcon.c

