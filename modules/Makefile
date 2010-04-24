MODULES=$(shell find . -depth -maxdepth 1 -type d -and -not -wholename '.')

all:
	@for d in ${MODULES}; do cd "$$d"; make || exit 1; cd ..; done

clean:
	@for d in ${MODULES}; do cd "$$d"; make clean || exit 1; cd ..; done
