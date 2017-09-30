#include <stdio.h>

int main(int argc, char* argv[])
{
	int int_size = sizeof(int);
	int long_size = sizeof(long);
	int long_int_size = sizeof(long int);
	int long_long_size = sizeof(long long);
	int void_ptr = sizeof(void*);

	printf("int size = %d\n", int_size);
	printf("long size = %d\n", long_size);
	printf("long int size = %d\n", long_int_size);
	printf("long long size = %d\n", long_long_size);
	printf("void* size = %d\n", void_ptr);

	return 0;
}

