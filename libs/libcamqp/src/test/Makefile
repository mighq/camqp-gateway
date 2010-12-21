# environment
CC=gcc
STRIP=strip
CFLAGS=-std=c99 -Wall -O0 -ggdb -fPIC -pedantic
LFLAGS=

TEST=run

all: ${TEST}

${TEST}:
	${CC} ${CFLAGS} -I../../include -I/usr/include/libxml2 -L../../lib -lcamqp -L/usr/lib -lxml2 -luuid -o $@ test.c

# test instance
test: ${TEST}
	@LD_LIBRARY_PATH="../../lib:${LD_LIBRARY_PATH}" ./${TEST}

check: ${TEST}
	@LD_LIBRARY_PATH="../../lib:${LD_LIBRARY_PATH}" valgrind --tool=memcheck  --leak-check=full --show-reachable=yes --leak-resolution=high --suppressions=./suppress.txt ./${TEST}

clean:
	rm -rf ${TEST}
