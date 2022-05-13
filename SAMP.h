#pragma once

constexpr unsigned int SAMP_MAX_PLAYERS = 1000;
constexpr unsigned int SAMP_MAX_PLAYER_NAME = 24;
constexpr unsigned int SAMP_MAX_CHAT_MSG = 128;

// char
constexpr unsigned long SAMP_OFFSET_PLAYER_NAME = 0x26E16F; // samp.dll+26E16F
constexpr unsigned long SAMP_OFFSET_CUR_CHAT_MSG = 0x141BA0; // samp.dll+141BA0

// ------------------------------------------------ 

bool IsSAMP();
bool SAMP_GetLocalPlayerName(char* pName, unsigned int iSize);
bool SAMP_GetCurrentChatMessage(char* pText, unsigned int iSize);

// ------------------------------------------------ 