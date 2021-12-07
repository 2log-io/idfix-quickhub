#ifndef STRINGCOMPARE_H
#define STRINGCOMPARE_H

#include <functional>

extern "C"
{
	#include <string.h>
}

class StringComparison: public std::binary_function<const char*, const char*, bool>
{
	public:

		bool operator() (const char* str1, const char* str2) const
		{
			return strcmp(str1, str2) < 0;
		}
};

#endif
