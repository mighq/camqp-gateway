TARGET_BIN=in_sample.so
SOURCES=in_sample.c

DIR_CORE=../../core

DIR_GLIB=/usr
DIR_MANAGER_API=${DIR_CORE}/api

# dependencies
INCLUDES=									\
	-I${DIR_GLIB}/include/glib-2.0			\
	-I${DIR_GLIB}/lib/glib-2.0/include		\
	-I${DIR_MANAGER_API}

LIBS=					\
	-L${DIR_GLIB}/lib	\
		-lglib-2.0		\
		-lgthread-2.0	\
		-lgmodule-2.0

# environment
CC=gcc
CFLAGS=-Wall -O0 -ggdb -fPIC

DIR_BIN=bin
DIR_OBJ=obj

# computed parts
OBJS=$(patsubst %.c,${DIR_OBJ}/%.o,${SOURCES})
TARGET=${DIR_BIN}/${TARGET_BIN}

all: ${TARGET}

${TARGET}: ${OBJS} | ${DIR_BIN}
	${CC} -shared -o ${TARGET} ${OBJS} ${LIBS}

${DIR_OBJ}/%.o: %.c ${DIR_OBJ}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ -c $<

# directories creation
${DIR_BIN}:
	mkdir ${DIR_BIN}

${DIR_OBJ}:
	mkdir ${DIR_OBJ}

# CLEANUP
clean:
	rm -rf ${TARGET}
	rm -rf ${DIR_OBJ}

install: ${TARGET}
	mkdir -p ${DIR_CORE}/bin/plugins
	cp ${TARGET} ${DIR_CORE}/bin/plugins
