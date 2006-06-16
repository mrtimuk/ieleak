#pragma once
#include "..\nanocppunit\test.h"

#ifdef TEST

class MemoryUsageTest :
	public TestCase
{
public:
	MemoryUsageTest(void);
public:
	virtual ~MemoryUsageTest(void);
public:
	static int ToInt(CStringW stringBuffer);
};

#endif