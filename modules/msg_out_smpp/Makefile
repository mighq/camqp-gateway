TARGET_BIN=msg_out_smpp.so
SOURCES=module.c

DIR_ROOT=../..

DIR_LIB_GLIB=/usr
DIR_LIB_SMPP=${DIR_ROOT}/libs/libsmpp34
DIR_LIB_CAMQP=../../libs/libcamqp

DIR_DIST=${DIR_ROOT}/dist
DIR_MODULES=${DIR_DIST}/modules
DIR_API=${DIR_ROOT}/api

# dependencies
INCLUDES=									\
	-I/usr/include/libxml2					\
	-I${DIR_LIB_GLIB}/include/glib-2.0		\
	-I${DIR_LIB_GLIB}/lib/glib-2.0/include	\
	-I${DIR_LIB_SMPP}/include				\
	-I${DIR_LIB_CAMQP}/include				\
	-I${DIR_API}

LIBS=						\
	-L${DIR_LIB_GLIB}/lib	\
	-L${DIR_LIB_SMPP}/lib	\
	-L${DIR_LIB_CAMQP}/lib	\
		-lglib-2.0			\
		-lgthread-2.0		\
		-lgmodule-2.0		\
		-lsmpp34			\
		-lcamqp

# environment
CC=gcc
LD=ld
STRIP=strip
CFLAGS=-Wall -O0 -ggdb -fPIC
LFLAGS=

DIR_OBJ=obj

# computed parts
OBJS=$(patsubst %.c,${DIR_OBJ}/%.o,${SOURCES})
TARGET=${DIR_MODULES}/${TARGET_BIN}

all: ${TARGET}

${TARGET}: ${OBJS} | ${DIR_MODULES}
	${LD} ${LFLAGS} -shared -soname ${TARGET_BIN} -o ${TARGET} ${OBJS} ${LIBS}
#	${STRIP} -s -K LoadModule -K UnloadModule -K OutputModule ${TARGET}


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
