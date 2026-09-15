#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../BloomFilter.h"
#include "../FilterDesc.h"
#include "../../Defs.h"



const char* g_szFilterFile = NULL;
const char* g_szHelpString = R"(
    Bloom-Filter Gen output verification program.
    Usage : ./BloomFilterGenCli -f filter_file
)";



static int ExtractFilterDesc(FILE* pFilterFile, struct FilterDesc_t* pFilterDescOut);
static int ExtractCmdLineArgs(int nArgs, char** szArgs);



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int main(int nArgs, char** szArgs)
{
    int   iOk         = EXIT_SUCCESS;
    FILE* pFilterFile = NULL;

    // Command line args...
    if (ExtractCmdLineArgs(nArgs, szArgs) != 0)
    {
        printf("%s\n", g_szHelpString); 
        return iOk;
    }


    // Open filter file.
    pFilterFile = fopen(g_szFilterFile, "r");
    if (pFilterFile == NULL)
    { 
        printf("Failed to open filter file : %s\n", g_szFilterFile);
        iOk = EXIT_FAILURE; goto EXIT;
    }



    // Extract the filter description header for filter file.
    struct FilterDesc_t filterDesc = {0}; ExtractFilterDesc(pFilterFile, &filterDesc);
    printf("Filter size : %zu bits, Data size : %zu entries, K : %zu, BlockSize : %zu\n",
            filterDesc.m_iFilterSize, filterDesc.m_iDataSize, filterDesc.m_iK, filterDesc.m_iBlockSizeBytes);

    // Is filter description even valid?
    if (filterDesc.m_iFilterSize <= 8 || filterDesc.m_iDataSize <= 0 || filterDesc.m_iK <= 0 || filterDesc.m_iBlockSizeBytes <= 0)
    {
        printf("Invalid filter file.\n");
        iOk = EXIT_FAILURE; goto EXIT;
    }

    // Read bloom filter from file to memory.
    unsigned long long iFitlerSizeBytes = filterDesc.m_iFilterSize / 8;
    unsigned char*     pBloomFilter     = calloc(iFitlerSizeBytes, 1);

    char c = ' ';
    rewind(pFilterFile); while((c = fgetc(pFilterFile)) != '\n'); // This sets the file-cursor to the start of next line.
    for (unsigned long long i = 0; i < iFitlerSizeBytes; ++i)
    {
        pBloomFilter[i] = fgetc(pFilterFile);
    }
    printf("Bloom-Filter loaded\n");



    char szBuffer[256] = {0};
    while (true)
    {
        printf("[ ~ to exit ] Search : ");
        scanf("%250s", szBuffer);

        if (szBuffer[0] == '~')
            break;

        BF_FormatStrInPlace(szBuffer);
        int iFound = BF_CheckString(pBloomFilter, &filterDesc, szBuffer, strlen(szBuffer));
        printf("[ %s ]\n", iFound == 0 ? "Not-Found" : "Found");
    }

    

EXIT:
    if (pFilterFile != NULL) fclose(pFilterFile);
    return iOk;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int ExtractFilterDesc(FILE* pFilterFile, struct FilterDesc_t* pFilterDescOut)
{
    rewind(pFilterFile); char szBuffer[256] = {0};


    int     iOutputIt    = 0;
    size_t* vecOutputs[] = {
        &pFilterDescOut->m_iFilterSize, 
        &pFilterDescOut->m_iK, 
        &pFilterDescOut->m_iDataSize, 
        &pFilterDescOut->m_iBlockSizeBytes};


    char cTemp = ' '; int iBufferIt = 0;
    while(cTemp != '\n')
    {
        if (cTemp == EOF) return 1; // Invalid header in filter file.

        // Buffer too small? ( Likely invalid header for filter file. )
        if (iBufferIt >= sizeof(szBuffer) - 2) // Must have capacity for 2 more characters. 
            return 1;


        // Store this character & null terminate it.
        szBuffer[iBufferIt] = cTemp; ++iBufferIt;
        szBuffer[iBufferIt] = '\0';

        // If this number ends, store it in appropriate variable.
        char cNext = fgetc(pFilterFile);
        if (cTemp == ',' || cNext == '\n')
        {
            *vecOutputs[iOutputIt] = atoi(szBuffer);
            iOutputIt++;
            iBufferIt = 0;
        }

        cTemp = cNext;
    }


    // Not all variables in the output list got initialized.
    if (iOutputIt != sizeof(vecOutputs))
        return 1;

    return 0;
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

        if (strncmp(szThisArg, "-f", sizeof("-f")) == 0)
        {
            if (bLastArg == true)
            {
                printf("No output file found.\n");
                return 1;
            }

            g_szFilterFile = szArgs[iArgIndex + 1];
            iArgIndex++; // Consumed next argument.
        }
        else
            return 1;
    }


    return (g_szFilterFile != NULL) ? 0 : 1;
}

