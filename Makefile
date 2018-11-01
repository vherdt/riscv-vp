all:
	mkdir vp/build || true
	cd vp/build && cmake ..
	make -C vp/build -j 4
