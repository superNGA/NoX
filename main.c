//=========================================================================
//                      NoX
//=========================================================================
// by      : INSANE
// created : 13/09/2026
//
// purpose : NoX Kernel Module. Hooked delete_module function.
//-------------------------------------------------------------------------
#include <linux/kernel.h>
#include <linux/module.h>  // /usr/lib/modules/7.1.9-arch1-2/build/include/linux/module.h
#include <linux/kprobes.h> // /usr/lib/modules/7.1.9-arch1-2/build/include/linux/kprobes.h
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include <linux/workqueue.h>
#include <linux/reboot.h>
#include <linux/kmod.h>

#include <asm/msr.h>

#include "BloomFilter/BloomFilter.h"
#include "Defs.h"



MODULE_LICENSE    ("GPL");
MODULE_DESCRIPTION("NoX Linux Kernel Module");
MODULE_AUTHOR     ("insane");


#define DNS_LOOKUP_PORT (53)
#define EXIT_FAILURE    (1)
#define EXIT_SUCCESS    (0)


static struct nf_hook_ops g_nfHookOps;

extern struct FilterDesc_t g_filter; // Bloom Filter.



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void DeferredReboot(struct work_struct *pWork)
{
    orderly_reboot();
}
static DECLARE_WORK(NoxOrderlyRebootWork, DeferredReboot);


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int FixDomainInPlace(char* szDomain, unsigned long long iSize)
{
    if (iSize <= 1)
        return EXIT_FAILURE;


    for(int i = 0; i < iSize; i++)
    {
        char c = szDomain[i];

        if (c == '\0')
            break;

        int bUpperCase = c >= 'A' && c <= 'Z';
        int bLowerCase = c >= 'a' && c <= 'z';
        int bNum       = c >= '0' && c <= '9';
        int bMinusSign = c == '-';

        if (bUpperCase == 0 && bLowerCase == 0 && bNum == 0 && bMinusSign == 0)
            szDomain[i] = '.';
    }


    // Shift all chars to left by one.
    for (int i = 0; i < iSize - 1; i++)
    {
        if (szDomain[i] == '\0')
            break;

        szDomain[i] = szDomain[i + 1];
    }

    return EXIT_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int StrLen(const char* szInput, int iMaxLen)
{
    int i = 0;

    while ( i <= iMaxLen && szInput[i] != '\0')
        ++i;

    return i;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static unsigned int NfHook(void *pPriv, struct sk_buff *pSkb, const struct nf_hook_state *pState)
{
	struct iphdr  *pIph  = NULL;
	struct udphdr *pUdph = NULL;


	if (pSkb == NULL)
		return NF_ACCEPT;

	pIph = ip_hdr(pSkb);
	if (!pIph || pIph->protocol != IPPROTO_UDP)
		return NF_ACCEPT;

	pUdph = udp_hdr(pSkb);
	if (!pUdph || ntohs(pUdph->dest) != DNS_LOOKUP_PORT)
		return NF_ACCEPT;


	int required_len = skb_transport_offset(pSkb) + sizeof(struct udphdr) + 12 + 1;
	if (skb_is_nonlinear(pSkb) || skb_headlen(pSkb) < required_len)
		return NF_ACCEPT; 


	// Skip 8-byte UDP header + 12-byte DNS header directly to the QNAME field
	__attribute__((aligned(CLS))) unsigned char buffer[256] = {0};

	unsigned char *szRawQname = (unsigned char*)pUdph + sizeof(struct udphdr) + 12;
	strncpy(buffer, szRawQname, sizeof(buffer)); // Copy raw label into the buffer.
	buffer[sizeof(buffer) - 1] = '\0';           // Make sure raw-label is null-terminated.


	if (FixDomainInPlace(buffer, sizeof(buffer)) != EXIT_SUCCESS)
		return NF_ACCEPT;


	BF_FormatStrInPlace(buffer); buffer[sizeof(buffer) - 1] = '\0';
	// Check all sub-domain strings where atleast 1 '.' char remains.
	int domain_len = StrLen(buffer, sizeof(buffer));
	int checkpoint = 0;
	for (int i = 0; i < sizeof(buffer); ++i) { 
		char c = buffer[i];

		if (c == '\0')
			break;

		if (c != '.')
			continue;

		if (BF_CheckString(&g_filter, buffer + checkpoint, domain_len - checkpoint) != 0) {
			schedule_work(&NoxOrderlyRebootWork);
			return NF_ACCEPT;
		}
		checkpoint = i + 1;
	}

	return NF_ACCEPT;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int RegisterNFHook(struct nf_hook_ops* pNfHook)
{
    int iRet = nf_register_net_hook(&init_net, &g_nfHookOps);
    if (iRet < 0)
        printk(KERN_INFO "[ NoX ] Failed to register Net-Filter hook");
    else
        printk(KERN_INFO "[ NoX ] Registered Net-Filter hook");

    return iRet;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int InitModule(void) 
{
	// Register net-filter hook.
	g_nfHookOps.hook     = NfHook;
	g_nfHookOps.hooknum  = NF_INET_LOCAL_OUT;
	g_nfHookOps.pf       = PF_INET;
	g_nfHookOps.priority = NF_IP_PRI_FIRST;
	int iRet             = RegisterNFHook(&g_nfHookOps);
	if (iRet < 0)
		return iRet;

	printk(KERN_INFO "[ NoX ] Initialized");
	return 0;
}


module_init(InitModule);
