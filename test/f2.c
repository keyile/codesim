
int f(int x, int y) {
	if (y == 0) {
		return x;
	}
	else
		return f(y, x % y);
}
