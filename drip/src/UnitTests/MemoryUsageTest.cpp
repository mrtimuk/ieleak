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
	const size_t allocSize = 40*1024;

	size_t before = CMainBrowserDlg::GetMemoryUsage();
	CPPUNIT_ASSERT(before > 0);

	char* dummyBuffer = new char[allocSize];
	size_t after = CMainBrowserDlg::GetMemoryUsage();
	delete [] dummyBuffer;

	CPPUNIT_ASSERT(before + allocSize <= after);
}

int MemoryUsageTest::ToInt(CStringW stringBuffer)
{
	int result = 0;
	swscanf_s(stringBuffer, L"%d", &result);
	return result;
}

#endif

