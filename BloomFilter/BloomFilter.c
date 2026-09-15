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

#include <assert.h>
#include <stdbool.h>
#include <string.h>




///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void BF_AddString(unsigned char* pBloomFilter, struct FilterDesc_t* pFilterDesc, const char* szInput, unsigned long iLen)
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

        assert(iBitIndexAbs < pFilterDesc->m_iFilterSize && "Invalid absolute bit index.");
        BF_ToggleBit(pBloomFilter, iBitIndexAbs, 1);
    }
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int BF_CheckString(unsigned char* pBloomFilter, struct FilterDesc_t* pFilterDesc, const char* szInput, unsigned long iLen)
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

        assert(iBitIndexAbs < pFilterDesc->m_iFilterSize && "Invalid absolute bit index.");
        if (BF_CheckBit(pBloomFilter, iBitIndexAbs) == false)
            return 0;
    }

    return 1;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
unsigned int BF_GetSeed(int iK)
{
    assert(iK >= 0 && "Invalid K value");
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
void BF_ToggleBit(unsigned char* pBloomFilter, unsigned long iBitIndex, int bEnable)
{
    size_t        iByteIndex = iBitIndex / 8;
    unsigned char iMask      = (1 << (7 - (iBitIndex % 8))) & 0xFF; // Subtracting from '7' because 0th bit is right-most, but we need it to be left-most.

    if (bEnable == false)
        pBloomFilter[iByteIndex] &= ~iMask;
    else
        pBloomFilter[iByteIndex] |= iMask;

    return;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int BF_CheckBit(unsigned char* pBloomFilter, unsigned long iBitIndex)
{
    size_t        iByteIndex = iBitIndex / 8;
    unsigned char iMask      = (1 << (7 - (iBitIndex % 8))) & 0xFF; // Subtracting from '7' because 0th bit is right-most, but we need it to be left-most.

    return pBloomFilter[iByteIndex] & iMask;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void BF_FormatStrInPlace(char* szInput)
{
    unsigned long long i = 0;
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
