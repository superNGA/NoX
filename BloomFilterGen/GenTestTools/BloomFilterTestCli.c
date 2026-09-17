//=========================================================================
//                      Bloom Filter Test
//=========================================================================
// by      : INSANE
// created : 15/09/2026
//
// purpose : Tests Bloom-Filter against input data set.
//-------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../../BloomFilter/BloomFilter.h"
#include "../../BloomFilter/FilterDesc.h"
#include "../../Defs.h"


// Bloom filter.
extern struct FilterDesc_t g_filter;



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    if (g_filter.m_iFilterSize <= 8 || g_filter.m_iDataSize <= 0 || g_filter.m_iK <= 0 || g_filter.m_iBlockSizeBytes <= 0) {
        printf("Invalid filter.\n");
        return EXIT_FAILURE;
    }


    __attribute__((aligned(CLS))) char szBuffer[256] = {0};
    while (true)
    {
        printf("[ ~ to exit ] Search : "); scanf("%200s", szBuffer);

        if (szBuffer[0] == '~')
            break;

        BF_FormatStrInPlace(szBuffer); 
        int bFound = BF_CheckString(&g_filter, szBuffer, strlen(szBuffer));

        printf("%s\n", bFound == 0 ? "Not-Found" : "Found");
    }

    return EXIT_SUCCESS;
}

