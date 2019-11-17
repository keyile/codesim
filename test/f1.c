#include <stdio.h>

int f(int a, int b) {
	if (b == 0) return a;
	else {
		return f(b, a % b);
	}
}
