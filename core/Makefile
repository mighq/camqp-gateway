TARGET_BIN=manager
SOURCES=main.c

DIR_GLIB=/usr
DIR_SQLITE=/usr

# dependencies
INCLUDES=									\
	-I${DIR_GLIB}/include/glib-2.0			\
	-I${DIR_GLIB}/lib/glib-2.0/include		\
	-I${DIR_SQLITE}/include
LIBS=					\
	-L${DIR_GLIB}/lib	\
		-lglib-2.0		\
		-lgthread-2.0	\
		-lgmodule-2.0	\
	-L${DIR_SQLITE}/lib	\
		-lsqlite3

# environment
CC=gcc
CFLAGS=-Wall -O0 -ggdb

DIR_BIN=bin
DIR_OBJ=obj
DIR_DEBUG=debug

# computed parts
OBJS=$(patsubst %.c,${DIR_OBJ}/%.o,${SOURCES})
TARGET=${DIR_BIN}/${TARGET_BIN}

all: ${TARGET}

${TARGET}: ${OBJS} | ${DIR_BIN}
	${CC} ${LFLAGS} -o $@ ${OBJS} ${LIBS}
	chmod +x $@

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

# testing
test-env:
	@echo "---===---"
	@stty -ctlecho
	@stty intr "^C"
	@stty quit "^\\"
	export LANG="C.UTF-8"

test: ${TARGET} test-env
	@echo "---===---"
	@${TARGET}

check: ${TARGET} test-env
	@echo "---===---"
	@valgrind --tool=memcheck --leak-check=full --show-reachable=yes ${TARGET}
	echo "poss :  744 in   3"
	echo "reach: 9098 in 134"

check-supp: ${TARGET} test-env
	@echo "---===---"
	@valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=${DIR_DEBUG}/suppress_glib.dat ${TARGET}
	echo "suppressed: 9842 in 137"
