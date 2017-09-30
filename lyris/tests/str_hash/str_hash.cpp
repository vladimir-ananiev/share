#include <stdio.h>
#include <string>

class  os_string_hasher2 
{
public:
	os_string_hasher2( long p = 1073741827 ) : prime_( p ) {}
	long operator()( const std::string& c ) const
	{
		int n = c.length();
		const char* d = c.data();
		int h = n; 
      
		for ( int i = 0; i < n; ++i, ++d )
			h = 613*h + *d;

		return ((h >= 0) ? (h % prime_) : (-h % prime_)); 
	}

protected:
	int prime_;
};


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

	std::string in_str = "test string";

	os_string_hasher2 StringHash;
	unsigned long hash = StringHash(in_str);

	printf("Hash of (%s) = %u\n", in_str.c_str(), hash);

	return 0;
}

