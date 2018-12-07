vp/build/Makefile:
	mkdir vp/build || true
	cd vp/build && cmake ..

all: vp/build/Makefile
	make -C vp/build -j 4
