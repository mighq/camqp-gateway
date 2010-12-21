TARGET_LIB=libcamqp.so
SOURCES=\
	utils.c		\
	data.c		\
	context.c	\
	elements.c	\
	scalar.c	\
	vector.c	\
	composite.c	\
	encoding.c	\
	decoding.c	\
	query.c

# dependencies
INCLUDES=\
	-I.						\
	-I/usr/include/libxml2

LIBS=\
	-L/usr/lib	\
	-lxml2		\
	-luuid

# dist
DIST_INCLUDE=../include
DIST_LIB=../lib

DIST_FILES=\
	${DIST_INCLUDE}/camqp.h	\
	${DIST_LIB}/${TARGET}

# environment
CC=gcc
LD=ld
STRIP=strip
CFLAGS=-std=c99 -Wall -O0 -ggdb -fPIC -pedantic
LFLAGS=

DIR_TMP=tmp

# computed parts
OBJS=$(patsubst %.c,${DIR_TMP}/%.o,${SOURCES})
TARGET=${DIR_TMP}/${TARGET_LIB}

all: dist

dist: ${TARGET} ${DIST_FILES}

${DIST_FILES}:
	[ -d ${DIST_INCLUDE} ] || mkdir ${DIST_INCLUDE}
	[ -d ${DIST_LIB} ] || mkdir ${DIST_LIB}
	cp -r ${TARGET} ${DIST_LIB}
	cp -r camqp.h ${DIST_INCLUDE}

${TARGET}: ${OBJS} | ${DIR_TMP}
	${LD} ${LFLAGS} -shared -soname ${TARGET_LIB} -o ${TARGET} ${OBJS} ${LIBS}

${DIR_TMP}/%.o: %.c | ${DIR_TMP}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ -c $<

# directories creation
${DIR_TMP}:
	mkdir -p ${DIR_TMP}

# CLEANUP
clean:
	rm -rf ${TARGET}
	rm -rf ${DIR_TMP}
	rm -rf ../include
	rm -rf ../lib
	cd test && make clean
