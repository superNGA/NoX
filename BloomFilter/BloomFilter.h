//=========================================================================
//                      Bloom Filter
//=========================================================================
// by      : INSANE
// created : 14/09/2026
//
// purpose : Quick bloom filter implemenation
//-------------------------------------------------------------------------
#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H


struct FilterDesc_t;



void         BF_AddString   (unsigned char* pBloomFilter, struct FilterDesc_t* pFilterDesc, const char* szInput, unsigned long iLen);
// 0 -> String doesn't exist in teh bloom filter. 1 -> String might exist in the bloom filter.
int          BF_CheckString (unsigned char* pBloomFilter, struct FilterDesc_t* pFilterDesc, const char* szInput, unsigned long iLen);
unsigned int BF_GetSeed     (int iK);
unsigned int BF_GetBlockSeed();
unsigned int BF_GetHash     (const char* szInput, unsigned long iLen, unsigned int iSeed);
void         BF_ToggleBit   (unsigned char* pBloomFilter, unsigned long iBitIndex, int bEnable);
int          BF_CheckBit    (unsigned char* pBloomFilter, unsigned long iBitIndex);

// A null terminated of a new line terminated string is expected.
void         BF_FormatStrInPlace(char* szInput);



#endif
