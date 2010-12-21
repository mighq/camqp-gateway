PARTS=\
	libs/libcamqp/src	\
	core				\
	modules

all:
	@for d in ${PARTS}; do cd "$$d"; make || exit 1; cd -; done

clean:
	@for d in ${PARTS}; do cd "$$d"; make clean || exit 1; cd -; done
