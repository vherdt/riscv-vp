all: vps vp-display vp-breadboard

vps: vp/build/Makefile
	make install -C vp/build -j6

vp/build/Makefile:
	mkdir vp/build || true
	cd vp/build && cmake ..

env/basic/vp-display/build/Makefile:
	mkdir env/basic/vp-display/build || true
	cd env/basic/vp-display/build && cmake ..

vp-display: env/basic/vp-display/build/Makefile
	make -C  env/basic/vp-display/build -j4

env/hifive/vp-breadboard/build/Makefile:
	mkdir env/hifive/vp-breadboard/build || true
	cd env/hifive/vp-breadboard/build && cmake ..

vp-breadboard: env/hifive/vp-breadboard/build/Makefile
	make -C  env/hifive/vp-breadboard/build -j4

clean:
	rm -rf vp/build
	rm -rf env/basic/vp-display/build
	rm -rf env/hifive/vp-breadboard/build
