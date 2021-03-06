#include <stdio.h>
#include <string>

class  os_string_hasher2_my
{
public:
	os_string_hasher2_my( long p = 1073741827 ) : prime_( p ) {}
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

typedef std::string os_string;

class  os_string_hasher2 
  {
  public:
    os_string_hasher2( long p = 1073741827 ) : prime_( p ) {}
    long operator()( const os_string& c ) const;

  protected:
    long prime_;
  };
long os_string_hasher2::operator()( const os_string& c ) const
  {
  int n = c.length();
  const char* d = c.data();
  int h = n; 
      
  for ( int i = 0; i < n; ++i, ++d )
    h = 613*h + *d;

  return ((h >= 0) ? (h % prime_) : (-h % prime_)); 
  }



int main(int argc, char* argv[])
{
	int int_size = sizeof(int);
	int long_size = sizeof(long);
	int long_int_size = sizeof(long int);
	int long_long_size = sizeof(long long);
	int void_ptr_size = sizeof(void*);

	printf("int size = %d\n", int_size);
	printf("long size = %d\n", long_size);
	printf("long int size = %d\n", long_int_size);
	printf("long long size = %d\n", long_long_size);
	printf("void* size = %d\n", void_ptr_size);

	std::string in_str = "Lyris ListManagerThis is an evaluation version of Lyris ListManager.";
	in_str += "\n";
	in_str += "\n";
	in_str += "To purchase Lyris ListManager, see: http://go.aurea.com/contact-us";
	in_str += "\n";
	in_str += "\n";

	os_string_hasher2 StringHash;
	unsigned long hash = StringHash(in_str);

	printf("Hash of (%s) = %u, len=%d\n", in_str.c_str(), hash, in_str.length());

	return 0;
}

