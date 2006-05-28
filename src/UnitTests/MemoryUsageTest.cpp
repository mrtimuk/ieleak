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
	int before = ToInt(CMainBrowserDlg::GetMemoryUsage());
	CPPUNIT_ASSERT(before > 0);

	char* dummyBuffer = new char[500];
	int after = ToInt(CMainBrowserDlg::GetMemoryUsage());
	delete [] dummyBuffer;
	
	CPPUNIT_ASSERT(before + 500 <= after);
}

int MemoryUsageTest::ToInt(CStringW stringBuffer)
{
	int result = 0;
	swscanf(stringBuffer, L"%d", &result);
	return result;
}

#endif

