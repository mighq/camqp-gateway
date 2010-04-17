TARGET_BIN=cfg_sqlite.so
SOURCES=module.c

DIR_ROOT=../..

DIR_GLIB=/usr
DIR_DIST=${DIR_ROOT}/dist
DIR_MODULES=${DIR_DIST}/modules
DIR_API=${DIR_ROOT}/api

# dependencies
INCLUDES=									\
	-I${DIR_GLIB}/include/glib-2.0			\
	-I${DIR_GLIB}/lib/glib-2.0/include		\
	-I${DIR_API}

LIBS=					\
	-L${DIR_GLIB}/lib	\
		-lglib-2.0		\
		-lgthread-2.0	\
		-lgmodule-2.0

# environment
CC=gcc
CFLAGS=-Wall -O0 -ggdb -fPIC

DIR_OBJ=obj

# computed parts
OBJS=$(patsubst %.c,${DIR_OBJ}/%.o,${SOURCES})
TARGET=${DIR_MODULES}/${TARGET_BIN}

all: ${TARGET}

${TARGET}: ${OBJS} | ${DIR_MODULES}
	${CC} -shared -o ${TARGET} ${OBJS} ${LIBS}

${DIR_OBJ}/%.o: %.c | ${DIR_OBJ}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ -c $<

# directories creation
${DIR_MODULES}:
	mkdir ${DIR_MODULES}

${DIR_OBJ}:
	mkdir ${DIR_OBJ}

# CLEANUP
clean:
	rm -rf ${TARGET}
	rm -rf ${DIR_OBJ}
