#include "stdafx.h"
#include "MemoryUsageTest.h"

#include "../MainBrowserDlg.hpp"

#ifdef TEST

MemoryUsageTest::MemoryUsageTest(void)
{
}

MemoryUsageTest::~MemoryUsageTest(void)
{
}

TEST_(MemoryUsageTest, TestMemoryIncreases)
{
	// Use a large enough buffer to ensure an increase in memory usage.
	const int allocSize = 4*1024;

	int before = ToInt(CMainBrowserDlg::GetMemoryUsage());
	CPPUNIT_ASSERT(before > 0);

	char* dummyBuffer = new char[allocSize];
	int after = ToInt(CMainBrowserDlg::GetMemoryUsage());
	delete [] dummyBuffer;
	
	CPPUNIT_ASSERT(before + allocSize <= after);
}

int MemoryUsageTest::ToInt(CStringW stringBuffer)
{
	int result = 0;
	swscanf(stringBuffer, L"%d", &result);
	return result;
}

#endif

