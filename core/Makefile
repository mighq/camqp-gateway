TARGET_BIN=manager
SOURCES=global.c		\
		options.c		\
		module.c		\
		config.c		\
		log.c			\
		queue.c			\
		messaging.c		\
		thread.c		\
		main.c

CORE_API_SYMBOLS=exported_api.sym

DIR_ROOT=..

DIR_API=${DIR_ROOT}/api
DIR_DIST=${DIR_ROOT}/dist
DIR_DIST_LIB=${DIR_DIST}/libs

DIR_GLIB=/usr

# dependencies
INCLUDES=									\
	-I${DIR_API}							\
	-I${DIR_GLIB}/include/glib-2.0			\
	-I${DIR_GLIB}/lib/glib-2.0/include

LIBS=					\
	-L${DIR_GLIB}/lib	\
		-lglib-2.0		\
		-lgthread-2.0	\
		-lgmodule-2.0

# environment
CC=gcc
LD=ld
STRIP=strip
CFLAGS=-Wall -O0 -ggdb
LFLAGS=-Wl,--export-dynamic

DIR_TMP=tmp
DIR_DEBUG=debug

TEST_OPTIONS=-c sqlite 1

# computed parts
OBJS=$(patsubst %.c,${DIR_TMP}/%.o,${SOURCES})
DEPS=$(patsubst %.c,${DIR_TMP}/%.d,${SOURCES})
TARGET=${DIR_DIST}/${TARGET_BIN}

all: ${TARGET}

${TARGET}: ${OBJS} | ${DIR_DIST}
	${CC} ${LFLAGS} -o $@ ${OBJS} ${LIBS}
#	${STRIP} -s @${CORE_API_SYMBOLS} ${TARGET}
	chmod +x $@

${DIR_TMP}/%.d: %.c | ${DIR_TMP}
	@${SHELL} -ec '${CC} -M ${INCLUDES} $< | sed -e "s#^$*.o#& $@#g" > $@'

${DIR_TMP}/%.o: %.c | ${DIR_TMP}
#	@${CC} -E ${CFLAGS} ${INCLUDES} -o $@.s -c $<
	${CC} ${CFLAGS} ${INCLUDES} -o $@ -c $<

# dependencies
-include ${DEPS}

# directories creation
${DIR_DIST}:
	mkdir ${DIR_DIST}

${DIR_TMP}:
	mkdir ${DIR_TMP}

# CLEANUP
clean:
	rm -rf ${TARGET}
	rm -rf ${DIR_TMP}

# testing
test-env:
	@echo "---=ENV=---"
	@stty -ctlecho
	@stty intr "^C"
	@stty quit "^\\"

test: ${TARGET} test-env
	@echo "---===---"
	@LD_LIBRARY_PATH="${DIR_DIST_LIB}:${LD_LIBRARY_PATH}" ${TARGET} ${TEST_OPTIONS}

check: ${TARGET} test-env
	@echo "---=TEST=---"
	@LANG=C.UTF-8 G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --tool=memcheck --leak-check=full --show-reachable=yes --leak-resolution=high --num-callers=20 ${TARGET} ${TEST_OPTIONS}

#check-supp: ${TARGET} test-env
#	@echo "---===---"
#	@valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=${DIR_DEBUG}/suppress_glib.dat ${TARGET} ${TEST_OPTIONS}
