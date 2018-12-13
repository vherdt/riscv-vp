all: vp/build/Makefile
	make install -C vp/build -j 4

vp/build/Makefile:
	mkdir vp/build || true
	cd vp/build && cmake ..
