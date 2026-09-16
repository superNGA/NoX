//=========================================================================
//                      Filter Description
//=========================================================================
// by      : INSANE
// created : 14/09/2026
//
// purpose : Bloom filter description. To be used as header for bloom filter file.
//-------------------------------------------------------------------------
#ifndef FILTER_DESC_H
#define FILTER_DESC_H




///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct FilterDesc_t
{
    unsigned long long m_iFilterSize;
    unsigned long long m_iK;
    unsigned long long m_iDataSize;
    unsigned long long m_iBlockSizeBytes;

    unsigned char*     m_pBloomFilter;
};



#endif
