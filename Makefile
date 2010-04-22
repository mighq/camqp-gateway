PARTS=\
	core	\
	modules

all:
	@for d in ${PARTS}; do cd "$$d"; make; cd ..; done

clean:
	@for d in ${PARTS}; do cd "$$d"; make clean; cd ..; done
