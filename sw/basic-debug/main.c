int x = 5;
int y;

int f(int a, int b) {
	if (a > 0) {
		int ans = a + b;
		return ans;
	}
	a = -a;
	return b + a;
}

int main() {
	int a = 5;
	int b = 3;
	b--;
	a = a / b;
	a = a * 2;
	a = f(x, y);
	return a;
}
