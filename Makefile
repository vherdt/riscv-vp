NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)

vps: vp/build/Makefile vp/dependencies/systemc-dist vp/dependencies/softfloat-dist
	make install -C vp/build -j$(NPROCS)

vp/dependencies/systemc-dist:
	cd vp/dependencies/ && ./build_systemc_233.sh

vp/dependencies/softfloat-dist:
	cd vp/dependencies/ && ./build_softfloat.sh

all: vps vp-display vp-breadboard

vp/build/Makefile:
	mkdir vp/build || true
	cd vp/build && cmake ..

env/basic/vp-display/build/Makefile:
	mkdir env/basic/vp-display/build || true
	cd env/basic/vp-display/build && cmake ..

vp-display: env/basic/vp-display/build/Makefile
	make -C  env/basic/vp-display/build -j$(NPROCS)

env/hifive/vp-breadboard/build/Makefile:
	mkdir env/hifive/vp-breadboard/build || true
	cd env/hifive/vp-breadboard/build && cmake ..

vp-breadboard: env/hifive/vp-breadboard/build/Makefile
	make -C  env/hifive/vp-breadboard/build -j$(NPROCS)

vp-clean:
	rm -rf vp/build

qt-clean:
	rm -rf env/basic/vp-display/build
	rm -rf env/hifive/vp-breadboard/build

sysc-clean:
	rm -rf vp/dependencies/systemc*

softfloat-clean:
	rm -rf vp/dependencies/softfloat-dist

clean-all: vp-clean qt-clean sysc-clean softfloat-clean

clean: vp-clean

codestyle:
	find . -not -path '*/\.*' -name "*.h*" -o -name "*.cpp" | xargs clang-format -i -style=file      #file is .clang-format 
