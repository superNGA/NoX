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


__attribute__((aligned(CLS))) unsigned char g_szLabelBuffer[256] = {0};


extern struct FilterDesc_t g_filter; // Bloom Filter.




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
int StrLen(const char* szInput)
{
    int iSize = 0;
    while (*szInput != '\0')
        ++iSize;

    return iSize;
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


    // Skip 8-byte UDP header + 12-byte DNS header directly to the QNAME field
    unsigned char *szRawQname = (unsigned char *)pUdph + sizeof(struct udphdr) + 12;
    strncpy(g_szLabelBuffer, szRawQname, sizeof(g_szLabelBuffer)); // Copy raw label into the buffer.
    g_szLabelBuffer[sizeof(g_szLabelBuffer) - 1] = '\0';           // Make sure raw-label is null-terminated.


    if (FixDomainInPlace(g_szLabelBuffer, sizeof(g_szLabelBuffer)) != EXIT_SUCCESS)
        return NF_ACCEPT;

    BF_FormatStrInPlace(g_szLabelBuffer);
    printk(KERN_INFO "%s [ bloom-filter : %d ]\n", g_szLabelBuffer, BF_CheckString(&g_filter, g_szLabelBuffer, StrLen(g_szLabelBuffer)));

    // unsigned int i = 0;
    // while (i < sizeof(g_szLabelBuffer))
    // {
    //     if (g_szLabelBuffer[i] == '\0')
    //         break;
    //
    //
    //     unsigned long long iLabelSize = (unsigned long long)g_szLabelBuffer[i];
    //     FixDomainInPlace(g_szLabelBuffer); BF_FormatStrInPlace(g_szLabelBuffer);
    //     int iMatchFound = BF_CheckString(&g_szLabelBuffer[i + 1], iLabelSize);
    //     if (iMatchFound == true)
    //     {
    //         printk(KERN_INFO "[ NoX ] Foul domain detected : %s\n", g_szLabelBuffer);
    //         // schedule_work(&NoxKernelRebootWork); // kaboom!
    //         break;
    //     }
    //
    //     i += iLabelSize + 1; // +1 so i moves past the label onto the next label size character.
    // }


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
    

    // No close NoX.
    if (strncmp(szModuleName, KBUILD_MODNAME, sizeof(szModuleName)) == 0)
    {
        printk(KERN_INFO "[ NoX ] No closing NoX");
        return 0; // Tell kernel this call has been handled.
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
static int InitModule(void)
{
    // Register the kprobe.
    g_kprobe.symbol_name  = "__x64_sys_delete_module";
    g_kprobe.pre_handler  = DeleteMod_PreHandler;
    g_kprobe.post_handler = DeleteMod_PostHandler;
    int iRet              = register_kprobe(&g_kprobe);
    if (iRet < 0)
    {
        printk(KERN_INFO "Failed to register kprobe for : %s\n", g_kprobe.symbol_name);
        return iRet;
    }


    // Register net-filter hook.
    g_nfHookOps.hook     = NfHook;
    g_nfHookOps.hooknum  = NF_INET_LOCAL_OUT; // Intercept outbound host traffic
    g_nfHookOps.pf       = PF_INET;           // IPv4
    g_nfHookOps.priority = NF_IP_PRI_FIRST;
    nf_register_net_hook(&init_net, &g_nfHookOps);


    printk(KERN_INFO "Bloom Filter Initialized.\n");


    printk(KERN_INFO "Registered the kprobe for : %s\n", g_kprobe.symbol_name);
    return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static void CleanupModule(void)
{
    unregister_kprobe(&g_kprobe);
    printk(KERN_INFO "Unregistered the kprobe : %s\n", g_kprobe.symbol_name);

    nf_unregister_net_hook(&init_net, &g_nfHookOps);
    printk(KERN_INFO "Unregistered Net-Filter hook.\n");
    return;
}


module_init(InitModule);
module_exit(CleanupModule);
