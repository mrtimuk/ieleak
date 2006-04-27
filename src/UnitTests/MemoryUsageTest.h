#pragma once
#include "d:\work\drip\nanocppunit\test.h"

#ifdef TEST

class MemoryUsageTest :
	public TestCase
{
public:
	MemoryUsageTest(void);
public:
	virtual ~MemoryUsageTest(void);
public:
	static int ToInt(wchar_t* stringBuffer);
};

#endif