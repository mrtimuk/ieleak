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
	wchar_t buffer[32];
	CMainBrowserDlg::GetMemoryUsage(buffer);
	int before = ToInt(buffer);
	CPPUNIT_ASSERT(before > 0);

	char* dummyBuffer = new char[500];
	CMainBrowserDlg::GetMemoryUsage(buffer);
	int after = ToInt(buffer);
	delete [] dummyBuffer;
	
	CPPUNIT_ASSERT(before + 500 <= after);
}

int MemoryUsageTest::ToInt(wchar_t* stringBuffer)
{
	int result = 0;
	swscanf(stringBuffer, L"%d", &result);
	return result;
}

#endif

