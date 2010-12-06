TARGET_LIB=libcamqp.so
TEST_BIN=test
SOURCES=libcamqp.c

# dependencies
INCLUDES=	\
	-I.

LIBS=	\
	-lxml2

# environment
CC=gcc
LD=ld
STRIP=strip
CFLAGS=-Wall -O0 -ggdb -fPIC
LFLAGS=

DIR_OBJ=obj
DIR_DONE=done

# computed parts
OBJS=$(patsubst %.c,${DIR_OBJ}/%.o,${SOURCES})
TARGET=${DIR_DONE}/${TARGET_LIB}
TEST=${DIR_DONE}/${TEST_BIN}

all: ${TARGET} ${TEST}

${TARGET}: ${OBJS} | ${DIR_DONE}
	${LD} ${LFLAGS} -shared -soname ${TARGET_LIB} -o ${TARGET} ${OBJS} ${LIBS}

${TEST}: ${TARGET} | ${DIR_DONE}
	${CC} ${CFLAGS} -I. -L${DIR_DONE} -lcamqp -o $@ test.c

${DIR_OBJ}/%.o: %.c | ${DIR_OBJ}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ -c $<

# directories creation
${DIR_DONE}:
	mkdir -p ${DIR_DONE}

${DIR_OBJ}:
	mkdir -p ${DIR_OBJ}

# test instance
test: ${TEST}
	@cd ${DIR_DONE} && LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}" ./${TEST_BIN}

# CLEANUP
clean:
	rm -rf ${TARGET}
	rm -rf ${TEST}
	rm -rf ${DIR_DONE}
	rm -rf ${DIR_OBJ}
