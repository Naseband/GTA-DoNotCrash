#pragma once

/* ------------------------------------------------------

Struct Parser

Parses an INI file into a struct.

Usage:

- Create a new object of the type of struct you want to parse data into. This is not the object the data goes into, it's just used to calculate offsets.
- Create a new instance of the StructParser class using the template for the desired struct.
- Set the base object to the instance you created earlier, either via the constructor or SetBase() member.
- Call Link for every value you want to parse into the target object later on.

To actually parse the data:

- Call Parse/ParseFile on the object you want to parse the data into. The source can be a file or data in memory.

IMPORTANT:
	The object used as base is not the target of the data being parsed. It is only used to calculate offsets for the values.
	Link() must be called with the members of the instance that you previously used as base.
	The base object can go out of scope or free'd after setting up all links.
	You can use SetBase(nullptr) to make sure no links are being created for an invalid struct once you are done setting it up (although there are sanity checks, the addresses can collide by coincidence).

*/// ----------------------------------------------------

#include <ctype.h>
#include <vector>

// ------------------------------------------------------

namespace LinkType
{
	enum
	{
		SIGNED = 0, // Signed value of any size
		UNSIGNED, // Unsigned value of any size
		BOOL = UNSIGNED, // Included for clarity
		FLOAT, // Floating point value, ie float or double
		STRING, // Zero terminated char array

		MAX
	};
}

// ------------------------------------------------------

template<typename T>
class StructParser
{
private:

	struct StructLink
	{
		int		iType = -1;
		size_t	iElementSize = 0;
		size_t	iIndexes = 0;

		size_t	iOffset = 0;

		char	*pSection = nullptr;
		char	*pKey = nullptr;
	};

	T*			m_pBase;

	std::vector<StructLink*>
				m_vLinks;

	bool _CmpStr(const char* szText1, const char* szText2, bool bIgnoreCase)
	{
		size_t
			iLen1 = strlen(szText1),
			iLen2 = strlen(szText2);

		if (iLen1 != iLen2)
			return false;

		if (bIgnoreCase)
		{
			for (size_t i = 0; i < iLen1; ++i)
				if (tolower((int)szText1[i]) != tolower((int)szText2[i]))
					return false;
		}
		else
		{
			for (size_t i = 0; i < iLen1; ++i)
				if (szText1[i] != szText2[i])
					return false;
		}

		return true;
	}

	size_t _RemovePadding(char* szText)
	{
		size_t
			iLen = strlen(szText),
			iLenNew,
			iFirstChar = iLen,
			iLastChar = iLen;

		for (size_t i = 0; i < iLen; ++i)
		{
			if (szText[i] != ' ')
			{
				if (iFirstChar == iLen)
					iFirstChar = i;

				iLastChar = i;
			}
		}

		if (iFirstChar == iLen)
		{
			szText[0] = 0;
			return 0;
		}
		else if (iFirstChar == iLastChar)
		{
			szText[0] = szText[iFirstChar];
			szText[1] = 0;
			return 1;
		}

		iLenNew = iLastChar - iFirstChar + 1;

		memmove(szText, szText + iFirstChar, iLenNew);
		szText[iLenNew] = 0;

		return iLenNew;
	}

public:

	StructParser(T* pBase = nullptr) :
		m_pBase(pBase)
	{

	}

	~StructParser()
	{
		ResetLinks();
	}

	void SetBase(T* pBase = nullptr)
	{
		m_pBase = pBase;
	}

	// iType: See LinkType::
	// iElementSize: Size of one element in bytes ie. sizeof(type). Supported sizes: 1, 2, 4, 8
	// iIndexes: The amount of indexes. If the array size is unknown you can use 0, however this is only safe for properly
	//   terminated char strings that are guaranteed to fit.
	// pTarget: Target variable within the base struct.
	// szSection: Name of the INI section.
	// szKey: Name of the INI key.
	bool Link(int iType, void* pTarget, size_t iElementSize, size_t iIndexes, const char* szSection, const char* szKey)
	{
		StructLink
			*pLink;

		size_t
			iLen,
			iOffset;

		if (iType < 0 || iType >= LinkType::MAX || !m_pBase || (size_t)(void*)pTarget < (size_t)(void*)m_pBase)
			return false;

		iOffset = (size_t)(void*)pTarget - (size_t)(void*)m_pBase;

		if (iOffset + iElementSize * (iIndexes == 0 ? 1 : iIndexes) >= sizeof(T))
			return false;

		pLink = new StructLink;

		pLink->iType = iType;
		pLink->iElementSize = iElementSize;
		pLink->iIndexes = iIndexes;

		pLink->iOffset = iOffset;

		iLen = strlen(szSection) + 1;
		pLink->pSection = new char[iLen];
		memcpy(pLink->pSection, szSection, iLen);

		iLen = strlen(szKey) + 1;
		pLink->pKey = new char[iLen];
		memcpy(pLink->pKey, szKey, iLen);

		m_vLinks.push_back(pLink);

		return true;
	}

	void ResetLinks()
	{
		for (auto &p : m_vLinks)
		{
			delete[] p->pSection;
			delete[] p->pKey;
			delete p;
		}

		m_vLinks.clear();
	}

	int ParseFile(const char* szFileName, T* pTarget, bool bIgnoreCase = true)
	{
		FILE
			*pFile;

		char
			*pData;

		size_t
			iLen;

		int
			iParsedValues;

		if (fopen_s(&pFile, szFileName, "r"))
			return -1;

#if _WIN64
		_fseeki64(pFile, 0, SEEK_END);
		iLen = (size_t)_ftelli64(pFile);
		_fseeki64(pFile, 0, SEEK_SET);
#else
		fseek(pFile, 0, SEEK_END);
		iLen = (size_t)ftell(pFile);
		fseek(pFile, 0, SEEK_SET);
#endif

		pData = new char[iLen + 1U];
		pData[iLen] = 0;

		fread(pData, sizeof(char), iLen, pFile);
		fclose(pFile);

		iParsedValues = Parse(pData, pTarget, bIgnoreCase, iLen);

		delete[] pData;

		return iParsedValues;
	}

	int Parse(char* pSource, T* pTarget, bool bIgnoreCase = true, size_t iSourceLen = 0)
	{
		char
			*pLine = nullptr,
			*pSection = nullptr;

		int
			iParsedValues = 0;

		size_t
			iLen = (iSourceLen == 0 ? strlen(pSource) : iSourceLen),
			iLineStart = 0,
			iLineLen = 0,
			iTmpLen;

		if (!iLen)
			return -1;

		for (size_t i = 0; i <= iLen; ++i)
		{
			if (i == iLen || pSource[i] == '\r' || pSource[i] == '\n' || pSource[i] == 0)
			{
				if (iLineLen < 2)
				{
					iLineStart = i + 1;
					iLineLen = 0;
					continue;
				}

				// Free previous line

				if (pLine)
				{
					delete[] pLine;
					pLine = nullptr;
				}

				// Allocate a new line and fill it

				pLine = new char[iLineLen + 1];
				memcpy(pLine, pSource + iLineStart, iLineLen);
				pLine[iLineLen] = 0;

				// Reset line counter

				iLineStart = i + 1;
				iLineLen = 0;

				// Remove padding and check remaining length

				iTmpLen = _RemovePadding(pLine);

				if (iTmpLen < 2)
					continue;

				if (pLine[0] == '[' && pLine[iTmpLen - 1] == ']')
				{
					// If this is a section copy the pointer to the section pointer and delete the old one (if any)

					pLine[0] = ' ';
					pLine[iTmpLen - 1] = ' ';

					if (pSection)
						delete[] pSection;

					// After removing [ and ] make sure there is text left, otherwise reset the section

					if (_RemovePadding(pLine))
					{
						pSection = pLine;
						pLine = nullptr;
					}
					else
					{
						pSection = nullptr;
					}
				}
				else
				{
					// If this is a value, parse it

					if (ParseValue(pSection, pLine, pTarget, bIgnoreCase))
						++iParsedValues;
				}
			}
			else
			{
				++iLineLen;
			}
		}

		// Clean up

		if (pSection)
			delete[] pSection;

		if(pLine)
			delete[] pLine;

		return iParsedValues;
	}

	bool ParseValue(char* pSection, char* pLine, T* pTarget, bool bIgnoreCase)
	{
		char
			*pKey,
			*pValue,
			*pContext;

		size_t
			iSize;

		size_t
			iPointer;

		pKey = strtok_s(pLine, "=", &pContext);

		if (!pKey || !_RemovePadding(pKey))
			return false;

		pValue = strtok_s(nullptr, "=", &pContext);

		if (!pValue || !_RemovePadding(pValue))
			return false;

		if (_CmpStr(pValue, "true", true))
		{
			pValue[0] = 0;
			strcat_s(pValue, 2, "1");
		}
		else if (_CmpStr(pValue, "false", true))
		{
			pValue[0] = 0;
			strcat_s(pValue, 2, "0");
		}

		for (auto &pLink : m_vLinks)
		{
			// Check if the current value and link have either no section, or the same section
			// and if the keys match.

			if (((!pSection && !pLink->pSection) || (pSection && _CmpStr(pSection, pLink->pSection, bIgnoreCase))) &&
				_CmpStr(pKey, pLink->pKey, bIgnoreCase)
				)
			{
				iPointer = (size_t)pTarget + pLink->iOffset;

				switch (pLink->iType)
				{
				case LinkType::SIGNED:

					if (pLink->iElementSize == 1)
						sscanf_s(pValue, "%hhi", (__int8*)iPointer);
					else if (pLink->iElementSize == 2)
						sscanf_s(pValue, "%hi", (__int16*)iPointer);
					else if (pLink->iElementSize == 4)
						sscanf_s(pValue, "%i", (__int32*)iPointer);
					else if (pLink->iElementSize == 8)
						sscanf_s(pValue, "%lli", (__int64*)iPointer);

					break;

				case LinkType::UNSIGNED:

					if (pLink->iElementSize == 1)
						sscanf_s(pValue, "%hhu", (unsigned __int8*)iPointer);
					else if (pLink->iElementSize == 2)
						sscanf_s(pValue, "%hu", (unsigned __int16*)iPointer);
					else if (pLink->iElementSize == 4)
						sscanf_s(pValue, "%u", (unsigned __int32*)iPointer);
					else if (pLink->iElementSize == 8)
						sscanf_s(pValue, "%llu", (unsigned __int64*)iPointer);

					break;

				case LinkType::FLOAT:

					if (pLink->iElementSize == 4)
						sscanf_s(pValue, "%f", (float*)iPointer);
					else if (pLink->iElementSize == 8)
						sscanf_s(pValue, "%lf", (double*)iPointer);

					break;

				case LinkType::STRING:

					if (pLink->iIndexes == 0)
						iSize = strlen(pValue);
					else
						iSize = pLink->iIndexes;

					if (pLink->iElementSize == 1)
						sscanf_s(pValue, "%[^\t\n]", (char*)iPointer, (unsigned int)iSize);

					break;
				}
			}
		}

		return true;
	}
};

// ------------------------------------------------------