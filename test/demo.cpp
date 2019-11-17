#include<iostream>

int main() {
	int nArr[5] = {1,2,3,4,5};
	for(auto &x : nArr)
	{
		x *=2;
	}
	return 0;
}
