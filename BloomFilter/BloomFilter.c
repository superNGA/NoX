//=========================================================================
//                      Bloom Filter
//=========================================================================
// by      : INSANE
// created : 14/09/2026
//
// purpose : Quick bloom filter implemenation
//-------------------------------------------------------------------------
#include "BloomFilter.h"

#include "../Hash/murmur3.h"
#include "FilterDesc.h"

// #include <stdbool.h>
#define false (0)
#define true  (1)




///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void BF_AddString(struct FilterDesc_t* pFilterDesc, const char* szInput, unsigned long iLen)
{
    unsigned long long iBlockSizeBits   = pFilterDesc->m_iBlockSizeBytes * 8;
    unsigned long long iFilterSizeBytes = pFilterDesc->m_iFilterSize / 8;
    unsigned long long nTotalBlocks     = ((iFilterSizeBytes - 1) / pFilterDesc->m_iBlockSizeBytes) + 1; // Filter size and filter size in bytes are one-based.


    unsigned int       iBlockHash       = BF_GetHash(szInput, iLen, BF_GetBlockSeed());
    unsigned int       iBitOffset       = (iBlockHash % nTotalBlocks) * (pFilterDesc->m_iBlockSizeBytes * 8);

    for (int iK = 0; iK < pFilterDesc->m_iK; iK++)
    {
        unsigned int iHash             = BF_GetHash(szInput, iLen, BF_GetSeed(iK));
        unsigned int iBitIndexRelative = iHash % iBlockSizeBits; // Block size in bits.
        unsigned int iBitIndexAbs      = iBitOffset + iBitIndexRelative;

        // assert(iBitIndexAbs < pFilterDesc->m_iFilterSize && "Invalid absolute bit index.");
        BF_ToggleBit(pFilterDesc, iBitIndexAbs, 1);
    }
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int BF_CheckString(struct FilterDesc_t* pFilterDesc, const char* szInput, unsigned long iLen)
{
    unsigned long long iBlockSizeBits   = pFilterDesc->m_iBlockSizeBytes * 8;
    unsigned long long iFilterSizeBytes = pFilterDesc->m_iFilterSize / 8;
    unsigned long long nTotalBlocks     = ((iFilterSizeBytes - 1) / pFilterDesc->m_iBlockSizeBytes) + 1; // Filter size and filter size in bytes are one-based.


    unsigned int       iBlockHash       = BF_GetHash(szInput, iLen, BF_GetBlockSeed());
    unsigned int       iBitOffset       = (iBlockHash % nTotalBlocks) * (pFilterDesc->m_iBlockSizeBytes * 8);

    for (int iK = 0; iK < pFilterDesc->m_iK; iK++)
    {
        unsigned int iHash             = BF_GetHash(szInput, iLen, BF_GetSeed(iK));
        unsigned int iBitIndexRelative = iHash % iBlockSizeBits; // Block size in bits.
        unsigned int iBitIndexAbs      = iBitOffset + iBitIndexRelative;

        // assert(iBitIndexAbs < pFilterDesc->m_iFilterSize && "Invalid absolute bit index.");
        if (BF_CheckBit(pFilterDesc, iBitIndexAbs) == false)
            return 0;
    }

    return 1;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
unsigned int BF_GetSeed(int iK)
{
    // assert(iK >= 0 && "Invalid K value");
    return iK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
unsigned int BF_GetBlockSeed()
{
    return 67; // :)
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
unsigned int BF_GetHash(const char* szInput, unsigned long iLen, unsigned int iSeed)
{
    unsigned int iHash = 0; MurmurHash3_x86_32(szInput, iLen, iSeed, &iHash);
    return iHash;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void BF_ToggleBit(struct FilterDesc_t* pFilterDesc, unsigned long iBitIndex, int bEnable)
{
    unsigned long long iByteIndex = iBitIndex / 8;
    unsigned char      iMask      = (1 << (7 - (iBitIndex % 8))) & 0xFF; // Subtracting from '7' because 0th bit is right-most, but we need it to be left-most.

    if (bEnable == false)
        pFilterDesc->m_pBloomFilter[iByteIndex] &= ~iMask;
    else
        pFilterDesc->m_pBloomFilter[iByteIndex] |= iMask;

    return;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int BF_CheckBit(struct FilterDesc_t* pFilterDesc, unsigned long iBitIndex)
{
    unsigned long long iByteIndex = iBitIndex / 8;
    unsigned char      iMask      = (1 << (7 - (iBitIndex % 8))) & 0xFF; // Subtracting from '7' because 0th bit is right-most, but we need it to be left-most.

    return pFilterDesc->m_pBloomFilter[iByteIndex] & iMask;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void BF_FormatStrInPlace(char* szInput)
{
    while (*szInput != '\0')
    {
        // No new-line char!
        if (*szInput == '\n')
        {
            *szInput = '\0';
            break;
        }

        // Case-insensitive.
        if (*szInput >= 'A' && *szInput <= 'Z')
            *szInput = *szInput - 'A' + 'a';

        szInput++;
    }

    return;
}
