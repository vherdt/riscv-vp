#include "stdint.h"

int x = 1;
int y;

int64_t main() {
	int64_t rt = 0;
	int a = 10;
	int b = 100;
	a = a + b + x + y;
	a = a + 1000;
	y = 10010 - 10;
	a = a + y;
	y = 10000 * 10;
	x = 10000000 / 10;
	a = a + x + y;
	b = a/0;
	a = a * (-2);
	a = a + b;
	if(2>a){
	a = a + 1100;
	}else{
	a = a + 1000;
	}
	rt = a;
	for(int i = 0;i<8;i++){
	rt = rt * 10;
	}
	return rt;
}
