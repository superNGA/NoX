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


static struct kprobe      g_kprobe;
static struct nf_hook_ops g_nfHookOps;

extern struct FilterDesc_t g_filter; // Bloom Filter.

static int g_iFilterHits = 0;



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void DeferredReboot(struct work_struct *pWork)
{
    orderly_reboot();
}
static DECLARE_WORK(NoxKernelRebootWork, DeferredReboot);


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
    if (skb_is_nonlinear(pSkb) || skb_headlen(pSkb) < required_len) {
        return NF_ACCEPT; 
    }


    // Skip 8-byte UDP header + 12-byte DNS header directly to the QNAME field
    __attribute__((aligned(CLS))) unsigned char szLabelBuffer[256] = {0};
    unsigned char *szRawQname = (unsigned char*)pUdph + sizeof(struct udphdr) + 12;
    strncpy(szLabelBuffer, szRawQname, sizeof(szLabelBuffer)); // Copy raw label into the buffer.
    szLabelBuffer[sizeof(szLabelBuffer) - 1] = '\0';           // Make sure raw-label is null-terminated.


    if (FixDomainInPlace(szLabelBuffer, sizeof(szLabelBuffer)) != EXIT_SUCCESS)
        return NF_ACCEPT;


    BF_FormatStrInPlace(szLabelBuffer); szLabelBuffer[sizeof(szLabelBuffer) - 1] = '\0';
    int iDomainLen = StrLen(szLabelBuffer, sizeof(szLabelBuffer));
    int iFilterHit = BF_CheckString(&g_filter, szLabelBuffer, iDomainLen);
    if (iFilterHit != 0)
    {
        ++g_iFilterHits;
        printk(KERN_INFO "Filter hit on : %s. Total hits : %d\n", szLabelBuffer, g_iFilterHits);
    }


    return NF_ACCEPT;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int DeleteMod_PreHandler(struct kprobe *pKp, struct pt_regs *pReg)
{
    char szModuleName[CLS]              = {0};
    const char __user* szUserModuleName = (const char __user*)((struct pt_regs*)pReg->di)->di; // RDI -> first argument.


    // Read module name from user space.
    long iStrnCpy = strncpy_from_user(szModuleName, szUserModuleName, sizeof(szModuleName));
    if (iStrnCpy <= 0 || iStrnCpy == -EFAULT)
    {
        printk(KERN_INFO "Failed to read module name from user space.\n");
        return 0;
    }
    szModuleName[sizeof(szModuleName) - 1] = 0; // Assert null terminated string here.
    
    printk(KERN_INFO "Trying to close : %s", szUserModuleName);

    // No close NoX.
    if (strncmp(szModuleName, KBUILD_MODNAME, sizeof(szModuleName)) == 0)
    {
        printk(KERN_INFO "[ NoX ] No closing NoX");
        return 0; // return 1; // Tell kernel this call has been handled.
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void DeleteMod_PostHandler(struct kprobe *pKp, struct pt_regs *pReg, unsigned long flags)
{
    // Post-Handler for DeleteModule function hook.
    return;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int RegisterKProbe(struct kprobe* pKprobe) 
{
    int iRet = register_kprobe(pKprobe);
    if (iRet < 0)
        printk(KERN_INFO "[ NoX ] Failed to register kprobe for symbol %s", pKprobe->symbol_name);
    else
        printk(KERN_INFO "[ NoX ] Registered kprobe for symbol %s", pKprobe->symbol_name);

    return iRet;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void UnregisterKProbe(struct kprobe* pKprobe)
{
    unregister_kprobe(pKprobe);
    printk(KERN_INFO "[ NoX ] Unregistered kprobe for symbol %s", pKprobe->symbol_name);
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
static void UnregisterNFHook(struct nf_hook_ops* pNfHook)
{
    nf_unregister_net_hook(&init_net, pNfHook);
    printk(KERN_INFO "[ NoX ] Unregistered Net-Filter Hook");
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int InitModule(void)
{
    // Register the kprobe.
    g_kprobe.symbol_name  = "__x64_sys_delete_module";
    g_kprobe.pre_handler  = DeleteMod_PreHandler;
    g_kprobe.post_handler = DeleteMod_PostHandler;
    int iKprobeRet        = RegisterKProbe(&g_kprobe);
    if (iKprobeRet < 0)
        return iKprobeRet;


    // Register net-filter hook.
    g_nfHookOps.hook     = NfHook;
    g_nfHookOps.hooknum  = NF_INET_LOCAL_OUT;
    g_nfHookOps.pf       = PF_INET;
    g_nfHookOps.priority = NF_IP_PRI_FIRST;
    int iNfHookRet       = RegisterNFHook(&g_nfHookOps);
    if (iNfHookRet < 0)
    {
        UnregisterKProbe(&g_kprobe);
        return iNfHookRet;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void CleanupModule(void)
{
    UnregisterKProbe(&g_kprobe);
    UnregisterNFHook(&g_nfHookOps);
}


module_init(InitModule);
module_exit(CleanupModule);
