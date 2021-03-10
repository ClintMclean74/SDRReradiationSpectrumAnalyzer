#define _ITERATOR_DEBUG_LEVEL 0
#include <iostream>
#include <cstdarg>
#include "DebuggingUtilities.h"

namespace DebuggingUtilities
{
	void DebugPrint(const char * format, ...)
	{
		va_list list;
     
		va_start(list, format);

		printf(format, va_arg(list, int));
		fflush(stdout);
		
		va_end(list);		
	}
}