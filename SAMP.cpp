#include "SAMP.h"
#include <Windows.h>

// ------------------------------------------------ 

unsigned long g_iBaseAddress = 0;

bool IsSAMP()
{
	static bool
		bChecked = false,
		bIsSAMP = false;

	if (bChecked)
		return bIsSAMP;

	HMODULE
		hModule = GetModuleHandleA("SAMP.dll");

	if (hModule != NULL)
	{
		g_iBaseAddress = (unsigned long)hModule;
		bIsSAMP = true;
	}
	
	bChecked = true;

	return bIsSAMP;
}

bool SAMP_GetLocalPlayerName(char* pName, unsigned int iSize)
{
	if (!IsSAMP())
		return false;

	unsigned long
		iAddress = g_iBaseAddress + SAMP_OFFSET_PLAYER_NAME;

	VirtualProtect((void*)iAddress, SAMP_MAX_PLAYER_NAME + 1, PAGE_EXECUTE_READ, nullptr);

	strcpy_s(pName, iSize, (char*)iAddress);

	return true;
}

bool SAMP_GetCurrentChatMessage(char* pText, unsigned int iSize)
{
	if (!IsSAMP())
		return false;

	unsigned long
		iAddress = g_iBaseAddress + SAMP_OFFSET_CUR_CHAT_MSG;

	VirtualProtect((void*)iAddress, SAMP_MAX_CHAT_MSG + 1, PAGE_EXECUTE_READ, nullptr);

	strcpy_s(pText, iSize, (char*)iAddress);

	return true;
}

// ------------------------------------------------ 