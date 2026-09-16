//=========================================================================
//                      Bloom Filter Gen
//=========================================================================
// by      : INSANE
// created : 15/09/2026
//
// purpose : Preprocess data and generate bloom filter to file.
//-------------------------------------------------------------------------
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "../BloomFilter/BloomFilter.h"
#include "../BloomFilter/FilterDesc.h"
#include "../Defs.h"


#define BLOCK_SIZE (CLS)

#define FILTER_DESC_HEADER "BloomFilter/FilterDesc.h"



// Command line parameters...
const char* g_szInputFile  = NULL;
const char* g_szOutputFile = NULL;
const char* g_szHelpString = R"(
    Bloom Filter Gen.
    Usage : ./BloomFilterGen -i input_file -o output_file
)";

const char* g_szOutputFileHdr = R"(This file is generated using BloomFilerGen tool.)";




static int    ExtractCmdLineArgs(int    nArgs, char** szArgs);
static size_t CountLinesInFile  (FILE*  pFile); 
static size_t CalcFilterSize    (size_t iDataSize, double flErrRate, size_t iBlockSizeBits);
static size_t CalcK             (size_t iDataSize, size_t iFilterSize);

static void   WriteFilterToFile(FILE* pOutputFile, struct FilterDesc_t* pFilterDesc);



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int main(int nArgs, char** szArgs)
{
    int iOk = 0;
    FILE* pOutputFile = NULL; FILE* pInputFile = NULL;


    // Command line arguments...
    if (ExtractCmdLineArgs(nArgs, szArgs) != 0)
    {
        printf("%s\n", g_szHelpString);
        return iOk;
    }
    assert(g_szInputFile != NULL && g_szOutputFile != NULL && "Input or output file not initialized.");



    // Open input file.
    pInputFile = fopen(g_szInputFile, "r");
    if (pInputFile == NULL)
    {
        perror("Error opening file.");
        iOk = 1; return iOk;
    }



    // Calculate filter specs.
    struct FilterDesc_t filterDesc;
    filterDesc.m_iBlockSizeBytes  = BLOCK_SIZE;
    filterDesc.m_iDataSize        = CountLinesInFile(pInputFile);
    filterDesc.m_iFilterSize      = CalcFilterSize  (filterDesc.m_iDataSize, 0.01, BLOCK_SIZE * 8);
    if (filterDesc.m_iFilterSize == 0 || filterDesc.m_iDataSize == 0 || filterDesc.m_iFilterSize % filterDesc.m_iBlockSizeBytes != 0)
    {
        printf("Invalid filter size or data size\n.");
        iOk = 1; goto EXIT;
    }
    filterDesc.m_iK = CalcK(filterDesc.m_iDataSize, filterDesc.m_iFilterSize);



    // Cross-check filter specs with user before proceeding.
    printf("Filter size : %llu bytes, Data size : %llu entries, K : %llu\n", filterDesc.m_iFilterSize / 8, filterDesc.m_iDataSize, filterDesc.m_iK);
    printf("[y] to proceed : "); char cUsrInput = 'n'; scanf("%c", &cUsrInput);
    if (cUsrInput != 'y')
    {
        printf("Aborting bloom-filter gen.\n");
        iOk = 1; goto EXIT;
    }



    // Write header to file.
    pOutputFile = fopen(g_szOutputFile, "wb");
    if (pOutputFile == NULL)
    {
        printf("Failed to create output file.\n");
        iOk = 1; goto EXIT;
    }

    // fprintf(pOutputFile, "%llu, %llu, %llu, %llu\n", filterDesc.m_iFilterSize, filterDesc.m_iK, filterDesc.m_iDataSize, filterDesc.m_iBlockSizeBytes);



    // Constructing Bloom-Filter.
    unsigned long long iFilterSizeBytes = filterDesc.m_iFilterSize / 8;
    filterDesc.m_pBloomFilter           = calloc(iFilterSizeBytes, 1);
    if (filterDesc.m_pBloomFilter == NULL)
    {
        printf("Failed to allocate bloom filter : %llu bits or %llu bytes\n", filterDesc.m_iFilterSize, iFilterSizeBytes);
        goto EXIT;
    }

    rewind(pInputFile); // So we read from beginning.
    __attribute__((aligned(CLS))) char szBuffer[256] = {0};
    while (fgets(szBuffer, sizeof(szBuffer), pInputFile) != NULL)
    {
        BF_FormatStrInPlace(szBuffer);
        BF_AddString(filterDesc.m_pBloomFilter, &filterDesc, szBuffer, strlen(szBuffer));
    }



    // Write bloom filter to file.
    // fwrite(filterDesc.m_pBloomFilter, sizeof(char), iFilterSizeBytes, pOutputFile);
    WriteFilterToFile(pOutputFile, &filterDesc);

    printf("Bloom filter written to file [ %s ]\n", g_szOutputFile);


EXIT:
    if (pInputFile  != NULL) fclose(pInputFile);
    if (pOutputFile != NULL) fclose(pOutputFile);
    return iOk;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ExtractCmdLineArgs(int nArgs, char** szArgs)
{
    if (nArgs <= 1)
        return 1;


    for (size_t iArgIndex = 1; iArgIndex < nArgs; iArgIndex++)
    {
        const char* szThisArg = szArgs[iArgIndex];
        bool        bLastArg  = iArgIndex + 1 >= nArgs;

        if (strncmp(szThisArg, "-o", sizeof("-o")) == 0)
        {
            if (bLastArg == true)
            {
                printf("No output file found.\n");
                return 1;
            }

            g_szOutputFile = szArgs[iArgIndex + 1];
            iArgIndex++; // Consumed next argument.
        }
        else if(strncmp(szThisArg, "-i", sizeof("-i")) == 0)
        {
            if (bLastArg == true)
            {
                printf("No input file found.\n");
                return 1;
            }

            g_szInputFile = szArgs[iArgIndex + 1];
            iArgIndex++; // Consumed next argument.
        }
        else
            return 1;
    }


    return (g_szInputFile != NULL && g_szOutputFile != NULL) ? 0 : 1;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static size_t CountLinesInFile(FILE* pFile)
{
    size_t nLines = 0;
    char   c      = ' '; 
    while((c = fgetc(pFile)) != EOF) if (c == '\n') ++nLines;

    return nLines;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static size_t CalcFilterSize(size_t iDataSize, double flErrRate, size_t iBlockSizeBits)
{
    assert(flErrRate >= 0.0 && flErrRate <= 1.0 && "Invalid error rate. [0.0, 1.0] is the valid range.");

    size_t nBits = (size_t)(((double)iDataSize * -log(flErrRate)) / (log(2.0) * log(2.0)));
    if (nBits == 0)
        return 0;

    return ((nBits - 1) / iBlockSizeBits) * iBlockSizeBits + iBlockSizeBits;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static size_t CalcK(size_t iDataSize, size_t iFilterSize)
{
    size_t iK = (size_t)(log(2.0) * (double)iFilterSize / (double)iDataSize);
    if (iK <= 0)
        iK = 1;

    return iK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void WriteFilterToFile(FILE* pOutputFile, struct FilterDesc_t* pFilterDesc)
{
    fprintf(pOutputFile, "/*\n");
    fprintf(pOutputFile, "%s\n", g_szOutputFileHdr);

    // Get date string.
    time_t raw_time         = time(NULL); struct tm *now = localtime(&raw_time);
    char   szDateBuffer[11] = {0};        strftime(szDateBuffer, sizeof(szDateBuffer), "%Y-%m-%d", now);

    fprintf(pOutputFile, "Generated on : %s\n", szDateBuffer);
    fprintf(pOutputFile, "*/\n\n");


    fprintf(pOutputFile, "#include \"%s\"\n\n\n", FILTER_DESC_HEADER);


    size_t iFilterSizeBytes = pFilterDesc->m_iFilterSize / 8; // Size is in bits right here.
    fprintf(pOutputFile, "static unsigned char g_pBloomFilterRaw[%zu] = {", iFilterSizeBytes);
    for (size_t iByteIndex = 0; iByteIndex < iFilterSizeBytes; iByteIndex++)
    {
        if (iByteIndex % 16 == 0)
            fprintf(pOutputFile, "\n    ");

        fprintf(pOutputFile, "0x%02X, ", pFilterDesc->m_pBloomFilter[iByteIndex]);
    }
    fprintf(pOutputFile, "\n};\n\n\n");


    fprintf(pOutputFile, "struct FilterDesc_t g_filter = {\n");
    fprintf(pOutputFile, "    .m_iFilterSize     = %llu,\n", pFilterDesc->m_iFilterSize);
    fprintf(pOutputFile, "    .m_iK              = %llu,\n", pFilterDesc->m_iK);
    fprintf(pOutputFile, "    .m_iDataSize       = %llu,\n", pFilterDesc->m_iDataSize);
    fprintf(pOutputFile, "    .m_iBlockSizeBytes = %llu,\n", pFilterDesc->m_iBlockSizeBytes);
    fprintf(pOutputFile, "    .m_pBloomFilter    = g_pBloomFilterRaw\n");

    fprintf(pOutputFile, "\n};");
}
