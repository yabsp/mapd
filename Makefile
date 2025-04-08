CC = gcc # set compiler, default is 'cc'

# -Wall: Enable common warnings
# -fPIC: (Position Independent Code) Required for compilation into shared library
# -g: Include debugging info in compile binary
CFLAGS = -Wall -fPIC -g

.PHONY: all valgrind clean

all: libmemwrap.so test_alloc

libmemwrap.so: src/memwrapper.c
	$(CC) $(CFLAGS) -shared -o $@ $^ -ldl

test_alloc: tests/test_alloc.c
	$(CC) $(CFLAGS) -o $@ $^

test: all
	LD_PRELOAD=./libmemwrap.so ./test_alloc

valgrind: all
	LD_PRELOAD=./libmemwrap.so valgrind --leak-check=full --error-exitcode=1 ./test_alloc

clean:
	rm -f libmemwrap.so test_alloc

