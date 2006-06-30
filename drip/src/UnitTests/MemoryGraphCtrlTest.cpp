#include "stdafx.h"
#include "MemoryGraphCtrlTest.h"

#include "../MemoryGraphCtrl.h"

#ifdef TEST

MemoryGraphCtrlTest::MemoryGraphCtrlTest(void)
{
}

MemoryGraphCtrlTest::~MemoryGraphCtrlTest(void)
{
}

TEST_(MemoryGraphCtrlTest, TestMenu)
{
	// getPopupMenu will assert if the menu doesn't exist or if the radio buttons cannot be set
	CMenu* menu = CMemoryGraphCtrl::getPopupMenu(CMemoryGraphCtrl::kMemory, CMemoryGraphCtrl::kNormal);
	ASSERT(menu);
}

#endif
