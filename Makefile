CC=gcc
CFLAGS=-Wall -g -O0 -std=c11 -pipe -march=native
MAKEFLAGS=-j$(shell grep -c processor /proc/cpuinfo)

programs=vichess

all: $(programs)

src=$(wildcard src/*.c)
obj=$(src:.c=.o)
vichess: $(obj) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -o vichess \
	-g \
	$(obj) \
	-lpthread -lncursesw -lm -lrt -o vichess \

clean:
	-rm -rfv $(obj) $(programs)

run: vichess
	valgrind --log-file=valgrind --leak-check=full --track-origins=yes\
	  --show-reachable=yes --suppressions=valgrind.supp \
	  ./vichess 2>log

log: vichess
	 tail -f -n 100 log
