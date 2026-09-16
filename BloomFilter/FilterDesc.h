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



typedef unsigned long size_t;



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
struct FilterDesc_t
{
    size_t m_iFilterSize;
    size_t m_iK;
    size_t m_iDataSize;
    size_t m_iBlockSizeBytes;
};



#endif
