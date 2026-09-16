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


extern struct FilterDesc_t g_filter;



const char* data_file   = NULL;
const char* help_string = R"(
    Bloom-Filter Gen output verification program.
    Usage : ./BloomFilterGen -d dataset_file
)";



static int extract_cmdline_args(int nArgs, char** szArgs);



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    if (extract_cmdline_args(argc, argv) != EXIT_SUCCESS) {
        printf("%s\n", help_string); 
        return EXIT_SUCCESS;
    }


    if (g_filter.m_iFilterSize <= 8 || g_filter.m_iDataSize <= 0 || g_filter.m_iK <= 0 || g_filter.m_iBlockSizeBytes <= 0) {
        printf("Invalid filter.\n");
        return EXIT_FAILURE;
    }


    FILE* h_datafile = fopen(data_file, "r");
    if (h_datafile == NULL) {
        printf("Failed to open data file : %s\n", data_file);
        return EXIT_FAILURE;
    }


    int fails = 0, tests = 0;
    __attribute__((aligned(CLS))) char szBuffer[256] = {0};
    while(fgets(szBuffer, sizeof(szBuffer), h_datafile) != NULL) {
        BF_FormatStrInPlace(szBuffer);

        int bFound = BF_CheckString(g_filter.m_pBloomFilter, &g_filter, szBuffer, strlen(szBuffer));
        if (bFound == false) {
            printf("Failed to find : %s\n", szBuffer);
            ++fails;
        }
        ++tests;
    }
    printf("%d strings tested. %d tests failed.\n", tests, fails);
    

    if (h_datafile != NULL)
        fclose(h_datafile);

    return EXIT_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int extract_cmdline_args(int argc, char** argv)
{
    if (argc <= 1)
        return 1;


    for (size_t i = 1; i < argc; i++) {
        const char* arg       = argv[i];
        bool        last_iter = i + 1 >= argc;

        if(strncmp(arg, "-d", sizeof("-d")) == 0) {
            if (last_iter == true) {
                printf("No input file found.\n");
                return 1;
            }

            data_file = argv[i + 1];
            i++; // Consumed next argument.
        }
        else
            return 1;
    }


    return (data_file == NULL) ? EXIT_FAILURE : EXIT_SUCCESS;
}

