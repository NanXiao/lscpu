#include <sys/param.h> 
#include <sys/sysctl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <cpuid.h>

/* macro definitions */
#define ARRAY_LEN(array)    (sizeof(array) / sizeof(array[0]))

#define CPUID_STANDARD_0_MASK   (0x00)
#define CPUID_STANDARD_1_MASK   (0x01)
#define CPUID_STANDARD_2_MASK   (0x02)
#define CPUID_STANDARD_4_MASK   (0x04)
#define CPUID_STANDARD_7_MASK   (0x07)
#define CPUID_STANDARD_B_MASK   (0x0B)


#define CPUID_EXTENDED_1_MASK   (0x01)
#define CPUID_EXTENDED_5_MASK   (0x05)
#define CPUID_EXTENDED_6_MASK   (0x06)

#define CPUID_MAX_STANDARD_FUNCTION (0x17)
#define CPUID_MAX_EXTENDED_FUNCTION (0x08)

/* struct definitions */
typedef struct
{
    char arch[16];
    int byte_order;
    char model[128];
    char vendor[32];
    int active_cpu_num;
    int total_cpu_num;
    int speed;
} gen_cpu_info;

typedef struct
{
    int mib_code;
    void *old;
    size_t old_len;
    char *err_msg;
} sysctl_get_cpu_info;

typedef struct
{
    int standard_mask;
    int extended_mask;
    int intel_use_leaf_4_get_cache;
    char vendor[13];
    unsigned char stepping;
    unsigned char model;
    unsigned short family;
    int threads_per_core;
    int cores_per_socket;
    char *l1d_cache;
    char *l1i_cache;
    char *l2_cache;
    char *l3_cache;
    char flags[2048];
} x86_cpu_info;


/* function declarations */
static int is_x86_cpu(char *arch);
static int is_amd_cpu(char *vendor);
static int is_intel_cpu(char *vendor);
static int x86_cpu_support_standard_flag(int flag, int mask);
static int get_x86_cpu_standard_flags(int intel, uint32_t ecx, uint32_t edx, char *flags, size_t len);
static int get_x86_cpu_structured_extended_flags(int intel, uint32_t ebx, uint32_t ecx, char *flags, size_t len);
static int get_x86_cpu_extended_flags(int intel, uint32_t ecx, uint32_t edx, char *flags, size_t len);
static void get_x86_cpu_info(x86_cpu_info *x86_info);

static void usage(void);
static void print_cpu_info(gen_cpu_info *gen_info, x86_cpu_info *x86_info);


/* variables definitions */
gen_cpu_info gen_info;
x86_cpu_info x86_info;
char intel_l1d_cache[8];
char intel_l1i_cache[8];
char intel_l2_cache[8];
char intel_l3_cache[8];
char amd_l1d_cache[8];
char amd_l1i_cache[8];
char amd_l2_cache[8];
char amd_l3_cache[8];


/* function definitions */
static int is_x86_cpu(char *arch)
{
    return (!strcmp(arch, "i386") || !strcmp(arch, "amd64"));
}

static int is_amd_cpu(char *vendor)
{
    return (!strcmp(vendor, "AMDisbetter!") || !strcmp(vendor, "AuthenticAMD"));
}

static int is_intel_cpu(char *vendor)
{
    return !strcmp(vendor, "GenuineIntel");
}

static int x86_cpu_support_standard_flag(int flag, int mask)
{
    return (flag & (1 << mask));
}

static void parse_intel_cache_value(x86_cpu_info *x86_info, unsigned char value)
{
    switch (value)
    {
        case 0x06:
        {
            x86_info->l1i_cache = "8K";
            break;
        }
        case 0x08:
        {
            x86_info->l1i_cache = "16K";
            break;
        }
        case 0x09:
        case 0x30:
        {
            x86_info->l1i_cache = "32K";
            break;
        }
        case 0x0A:
        case 0x66:
        {
            x86_info->l1d_cache = "8K";
            break;
        }
        case 0x0C:
        case 0x0D:
        case 0x60:
        case 0x67:
        {
            x86_info->l1d_cache = "16K";
            break;
        }
        case 0x68:
        case 0x2C:
        {
            x86_info->l1d_cache = "32K";
            break;
        }
        case 0x39:
        case 0x3B:
        case 0x41:
        case 0x79:
        {
            x86_info->l2_cache = "128K";
            break;
        }
        case 0x3A:
        {
            x86_info->l2_cache = "192K";
            break;
        }
        case 0x3C:
        case 0x42:
        case 0x7A:
        case 0x82:
        {
            x86_info->l2_cache = "256K";
            break;
        }
        case 0x3D:
        {
            x86_info->l2_cache = "384K";
            break;
        }
        case 0x3E:
        case 0x43:
        case 0x7B:
        case 0x7F:
        case 0x83:
        case 0x86:
        {
            x86_info->l2_cache = "512K";
            break;
        }
        case 0x44:
        case 0x7C:
        case 0x84:
        case 0x87:
        {
            x86_info->l2_cache = "1M";
            break;
        }
        case 0x45:
        case 0x7D:
        case 0x85:
        {
            x86_info->l2_cache = "2M";
            break;
        }
        case 0x48:
        {
            x86_info->l2_cache = "3M";
            break;
        }
        case 0x4E:
        {
            x86_info->l2_cache = "6M";
            break;
        }
        case 0x49:
        {
            x86_info->l3_cache = x86_info->l2_cache = "4M";
            break;
        }
        case 0xD0:
        {
            x86_info->l3_cache = "512K";
            break;
        }
        case 0x23:
        case 0xD1:
        case 0xD6:
        {
            x86_info->l3_cache = "1M";
            break;
        }
        case 0xDC:
        {
            x86_info->l3_cache = "1.5M";
            break;
        }
        case 0xDD:
        {
            x86_info->l3_cache = "3M";
            break;
        }
        case 0x25:
        case 0xD2:
        case 0xD7:
        case 0xE2:
        {
            x86_info->l3_cache = "2M";
            break;
        }
        case 0x29:
        case 0x46:
        case 0xD8:
        case 0xE3:
        {
            x86_info->l3_cache = "4M";
            break;
        }
        case 0x4A:
        case 0xDE:
        {
            x86_info->l3_cache = "6M";
            break;
        }
        case 0x47:
        case 0x4B:
        case 0xE4:
        {
            x86_info->l3_cache = "8M";
            break;
        }
        case 0x4C:
        case 0xEA:
        {
            x86_info->l3_cache = "12M";
            break;
        }
        case 0x4D:
        {
            x86_info->l3_cache = "16M";
            break;
        }
        case 0xEB:
        {
            x86_info->l3_cache = "18M";
            break;
        }
        case 0xEC:
        {
            x86_info->l3_cache = "24M";
            break;
        }
        case 0xFF:
        {
            x86_info->intel_use_leaf_4_get_cache = 1;
            break;
        }
        default:
        {
            break;
        }
    }
    return;
}

static int get_x86_cpu_standard_flags(int intel, uint32_t ecx, uint32_t edx, char *flags, size_t len)
{
    return snprintf(flags, len,
                /* edx*/
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s"
                "%s%s%s%s"
                "%s%s%s"
                
                /* ecx */
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s",

                edx & 0x00000001 ? "fpu " : "",
                edx & 0x00000002 ? "vme " : "",
                edx & 0x00000004 ? "de " : "",
                edx & 0x00000008 ? "pse " : "",
                
                edx & 0x00000010 ? "tsc " : "",
                edx & 0x00000020 ? "msr " : "",
                edx & 0x00000040 ? "pae " : "",
                edx & 0x00000080 ? "mce " : "",
                
                edx & 0x00000100 ? "cx8 " : "",
                edx & 0x00000200 ? "apic " : "",
                edx & 0x00000800 ? "sep " : "",
                
                edx & 0x00001000 ? "mtrr " : "",
                edx & 0x00002000 ? "pge " : "",
                edx & 0x00004000 ? "mca " : "",
                edx & 0x00008000 ? "cmov " : "",
                
                edx & 0x00010000 ? "pat " : "",
                edx & 0x00020000 ? "pse36 " : "",
                intel ? (edx & 0x00040000 ? "psn " : "") : "",
                edx & 0x00080000 ? "cflsh " : "",
                
                intel ? (edx & 0x00200000 ? "ds " : "") : "",
                intel ? (edx & 0x00400000 ? "acpi " : "") : "",
                edx & 0x00800000 ? "mmx " : "",
                
                edx & 0x01000000 ? "fxsr " : "",
                edx & 0x02000000 ? "sse " : "",
                edx & 0x04000000 ? "sse2 " : "",
                intel ? (edx & 0x08000000 ? "ss " : "") : "",
                
                edx & 0x10000000 ? "htt " : "",
                intel ? (edx & 0x20000000 ? "tm " : "") : "",
                intel ? (edx & 0x80000000 ? "pbe " : "") : "",

                ecx & 0x00000001 ? "sse3 " : "",
                ecx & 0x00000002 ? "pclmulqdq " : "",
                intel ? (ecx & 0x00000004 ? "dtes64 " : "") : "",
                ecx & 0x00000008 ? "monitor " : "",
                
                intel ? (ecx & 0x00000010 ? "ds_cpl " : "") : "",
                intel ? (ecx & 0x00000020 ? "vmx " : "") : "",
                intel ? (ecx & 0x00000040 ? "smx " : "") : "",
                intel ? (ecx & 0x00000080 ? "est " : "") : "",
                
                intel ? (ecx & 0x00000100 ? "tm2 " : "") : "",
                ecx & 0x00000200 ? "ssse3 " : "",
                intel ? (ecx & 0x00000400 ? "cnxt-id " : "") : "",
                intel ? (ecx & 0x00000800 ? "sdbg " : "") : "",

                ecx & 0x00001000 ? "fma " : "",
                ecx & 0x00002000 ? "cx16 " : "",
                intel ? (ecx & 0x00004000 ? "xtpr " : "") : "",
                intel ? (ecx & 0x00008000 ? "pdcm " : "") : "",

                intel ? (ecx & 0x00020000 ? "pcid " : "") : "", 
                intel ? (ecx & 0x00040000 ? "dca " : "") : "",
                ecx & 0x00080000 ? "sse4_1 " : "",

                ecx & 0x00100000 ? "sse4_2 " : "",
                intel ? (ecx & 0x00200000 ? "x2apic " : "") : "", 
                intel ? (ecx & 0x00400000 ? "movbe " : "") : "",
                ecx & 0x00800000 ? "popcnt " : "",

                intel ? (ecx & 0x01000000 ? "tsc_deadline " : "") : "",
                ecx & 0x02000000 ? "aes " : "",
                ecx & 0x04000000 ? "xsave " : "",
                ecx & 0x08000000 ? "osxsave " : "",

                ecx & 0x10000000 ? "avx " : "",
                ecx & 0x20000000 ? "f16c " : "",
                ecx & 0x40000000 ? "rdrnd " : "",
                ecx & 0x80000000 ? "hypervisor " : "");
}

static int get_x86_cpu_structured_extended_flags(int intel, uint32_t ebx, uint32_t ecx, char *flags, size_t len)
{
    return snprintf(flags, len,
                /* ebx */
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s"
                "%s%s"
                "%s"
                /* ecx */
                "%s%s%s"
                "%s"
                "%s"
                "%s",

                ebx & 0x00000001 ? "fsgsbase " : "",
                intel ? (ebx & 0x00000002 ? "tsc_adjust " : "") : "",
                intel ? (ebx & 0x00000004 ? "sgx " : "") : "",
                ebx & 0x00000008 ? "bmi1 " : "",

                intel ? (ebx & 0x00000010 ? "hle " : "") : "",
                ebx & 0x00000020 ? "avx2 " : "",
                intel ? (ebx & 0x00000040 ? "fp_dp " : "") : "",
                ebx & 0x00000080 ? "smep " : "",

                ebx & 0x00000100 ? "bmi2 " : "",
                intel ? (ebx & 0x00000200 ? "erms " : "") : "",
                intel ? (ebx & 0x00000400 ? "invpcid " : "") : "",
                intel ? (ebx & 0x00000800 ? "rtm " : "") : "",
                
                intel ? (ebx & 0x00001000 ? "pqm " : "") : "",
                intel ? (ebx & 0x00002000 ? "fpcsds " : "") : "",
                intel ? (ebx & 0x00004000 ? "mpx " : "") : "",
                intel ? (ebx & 0x00008000 ? "pqe " : "") : "",

                intel ? (ebx & 0x00010000 ? "pat " : "") : "",
                intel ? (ebx & 0x00020000 ? "pse36 " : "") : "",
                intel ? (ebx & 0x00040000 ? "rdseed " : "") : "",
                intel ? (ebx & 0x00080000 ? "adx " : "") : "",

                intel ? (ebx & 0x00100000 ? "smap " : "") : "",
                intel ? (ebx & 0x00800000 ? "clflushopt " : "") : "",

                intel ? (ebx & 0x01000000 ? "clwb " : "") : "",
                intel ? (ebx & 0x02000000 ? "intel_pt " : "") : "",

                intel ? (ebx & 0x10000000 ? "sha " : "") : "",

                intel ? (ecx & 0x00000001 ? "prefetchwt1 " : "") : "",
                intel ? (ecx & 0x00000004 ? "umip " : "") : "",
                intel ? (ecx & 0x00000008 ? "pku " : "") : "",

                intel ? (ecx & 0x00000010 ? "ospke " : "") : "",

                intel ? (ecx & 0x00040000 ? "rdpid " : "") : "",

                intel ? (ecx & 0x40000000 ? "sgx_lc " : "") : "");
}

static int get_x86_cpu_extended_flags(int intel, uint32_t ecx, uint32_t edx, char *flags, size_t len)
{
    return snprintf(flags, len,
                /* edx*/
                ""
                ""
                "%s"
                ""
                "%s"
                "%s%s"
                "%s%s%s"
                "%s%s%s"
                
                /* ecx */
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s%s"
                "%s%s%s"
                "%s%s%s"
                "%s%s%s"
                "%s%s%s"
                "%s",

                edx & 0x00000800 ? "syscall " : "",

                intel ? "" : (edx & 0x00080000 ? "mp " : ""),

                edx & 0x00100000 ? "nx " : "",
                intel ? "" : (edx & 0x00400000 ? "mmxext " : ""),

                intel ? "" : (edx & 0x02000000 ? "fxsr_opt " : ""),
                edx & 0x04000000 ? "pdpe1gb " : "",
                edx & 0x08000000 ? "rdtscp " : "",

                edx & 0x20000000 ? "lm " : "",
                intel ? "" : (edx & 0x40000000 ? "3dnowext " : ""),
                intel ? "" : (edx & 0x80000000 ? "3dnow " : ""),

                ecx & 0x00000001 ? "lahf_lm " : "",
                intel ? "" : (ecx & 0x00000002 ? "cmp_legacy " : ""),
                intel ? "" : (ecx & 0x00000004 ? "svm " : ""),
                intel ? "" : (ecx & 0x00000008 ? "extapic " : ""),

                intel ? "" : (ecx & 0x00000010 ? "cr8_legacy " : ""),
                ecx & 0x00000020 ? "lzcnt " : "",
                intel ? "" : (ecx & 0x00000040 ? "sse4a " : ""),
                intel ? "" : (ecx & 0x00000080 ? "misalignsse " : ""),

                intel ? "" : (ecx & 0x00000100 ? "3dnowprefetch " : ""),
                intel ? "" : (ecx & 0x00000200 ? "osvw " : ""),
                intel ? "" : (ecx & 0x00000400 ? "ibs " : ""),
                intel ? "" : (ecx & 0x00000800 ? "xop " : ""),

                intel ? "" : (ecx & 0x00001000 ? "skinit " : ""),
                intel ? "" : (ecx & 0x00002000 ? "wdt " : ""),
                intel ? "" : (ecx & 0x00008000 ? "lwp " : ""),

                intel ? "" : (ecx & 0x00010000 ? "fma4 " : ""),
                intel ? "" : (ecx & 0x00020000 ? "tce " : ""),
                intel ? "" : (ecx & 0x00080000 ? "nodeid_msr " : ""),

                intel ? "" : (ecx & 0x00200000 ? "tbm " : ""),
                intel ? "" : (ecx & 0x00400000 ? "topoext " : ""),
                intel ? "" : (ecx & 0x00800000 ? "perfctr_core " : ""),

                intel ? "" : (ecx & 0x01000000 ? "perfctr_nb " : ""),
                intel ? "" : (ecx & 0x02000000 ? "dbx " : ""),
                intel ? "" : (ecx & 0x08000000 ? "perftsc " : ""),

                intel ? "" : (ecx & 0x10000000 ? "pcx_l2i " : ""));
}


static void get_x86_cpu_info(x86_cpu_info *x86_info)
{
    int i = 0, flag_len = 0;
    uint32_t eax, ebx, ecx, edx;
    
    __cpuid(0, eax, ebx, ecx, edx);
    memcpy(x86_info->vendor, &ebx, sizeof(ebx));
    memcpy(&(x86_info->vendor[4]), &edx, sizeof(edx));
    memcpy(&(x86_info->vendor[8]), &ecx, sizeof(ecx));
    for (i = 0; (i <= eax) && (i <= CPUID_MAX_STANDARD_FUNCTION); i++)
    {
        x86_info->standard_mask |= (1 << i);
    }

    __cpuid(0x80000000, eax, ebx, ecx, edx);
    for (i = 0; (i <= eax) && (i <= CPUID_MAX_EXTENDED_FUNCTION); i++)
    {
        x86_info->extended_mask |= (1 << i);
    }

    eax = CPUID_STANDARD_1_MASK;
    if (x86_info->standard_mask & (1 << eax))
    {
        __cpuid(eax, eax, ebx, ecx, edx);
        x86_info->stepping = eax & 0xF;
        x86_info->family = (eax >> 8) & 0xF;
        x86_info->model = (eax >> 4) & 0xF;
        if ((x86_info->family == 6) || (x86_info->family == 15))
        {
            x86_info->model |= (eax >> 12) & 0xF0;
            if (x86_info->family == 15)
            {
                x86_info->family |= (eax >> 16) & 0xFF0;
            }
        }
        if (is_intel_cpu(x86_info->vendor))
        {
            flag_len += get_x86_cpu_standard_flags(1, ecx, edx, x86_info->flags + flag_len, sizeof(x86_info->flags) - flag_len);
        }
        else if (is_amd_cpu(x86_info->vendor))
        {
            flag_len += get_x86_cpu_standard_flags(0, ecx, edx, x86_info->flags + flag_len, sizeof(x86_info->flags) - flag_len);
        }
    }

    eax = CPUID_STANDARD_2_MASK;
    if ((is_intel_cpu(x86_info->vendor)) && (x86_info->standard_mask & (1 << eax)))
    {
        int i = 0, count = 0;
        uint32_t cache[4]; /* eax, ebx, ecx, edx */
        
        __cpuid(eax, cache[0], cache[1], cache[2], cache[3]);
        count = cache[0] & 0xFF;
        while (count--)
        {
            for (i = 0; i < 4; i++)
            {
                if (!(cache[i] & 0x80000000))
                {
                    if (i)
                    {
                        parse_intel_cache_value(x86_info, cache[i] & 0xFF);
                    }
                    parse_intel_cache_value(x86_info, (cache[i] >> 8) & (0xFF));
                    parse_intel_cache_value(x86_info, (cache[i] >> 16) & (0xFF));
                    parse_intel_cache_value(x86_info, (cache[i] >> 24) & (0xFF));
                }
            }
        }
    }

    if ((is_intel_cpu(x86_info->vendor)) && (x86_info->standard_mask & (1 << eax)) && (x86_info->intel_use_leaf_4_get_cache))
    {
        int subleaf = 0;
        for (subleaf = 0; ; subleaf++)
        {
            unsigned char cache_type = 0, cache_level = 0;
            int cache_size;

            __cpuid_count(CPUID_STANDARD_4_MASK, subleaf, eax, ebx, ecx, edx);
            
            cache_type = eax & 0x1F;
            if (!cache_type)
            {
                break;
            }

            cache_size = (int)((((ebx >> 22) & 0x3FF) + 1) * (((ebx >> 10) & 0x3FF) + 1) * 
                            ((ebx & 0xFFF) + 1) * (ecx + 1) / 1024);
            cache_level = (eax >> 5) & 0x7;
            switch (cache_level)
            {
                case 1:
                {
                    if (cache_type == 1)
                    {
                        snprintf(intel_l1d_cache, sizeof(intel_l1d_cache), "%dK", cache_size);
                        x86_info->l1d_cache = intel_l1d_cache;
                    }
                    else if (cache_type == 2)
                    {
                        snprintf(intel_l1i_cache, sizeof(intel_l1i_cache), "%dK", cache_size);
                        x86_info->l1i_cache = intel_l1i_cache;
                    }
                    break;
                }
                case 2:
                {
                    if (cache_type == 3)
                    {
                        snprintf(intel_l2_cache, sizeof(intel_l2_cache), "%dK", cache_size);
                        x86_info->l2_cache = intel_l2_cache;
                    }
                    break;
                }
                case 3:
                {
                    if (cache_type == 3)
                    {
                        snprintf(intel_l3_cache, sizeof(intel_l3_cache), "%dM", cache_size / 1024);
                        x86_info->l3_cache = intel_l3_cache;
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

    eax = CPUID_STANDARD_7_MASK;
    ecx = 0;
    if (x86_info->standard_mask & (1 << eax))
    {
        __cpuid(eax, eax, ebx, ecx, edx);
        if (is_intel_cpu(x86_info->vendor))
        {
            flag_len += get_x86_cpu_structured_extended_flags(1, ebx, ecx, x86_info->flags + flag_len, sizeof(x86_info->flags) - flag_len);
        }
        else if (is_amd_cpu(x86_info->vendor))
        {
            flag_len += get_x86_cpu_structured_extended_flags(0, ebx, ecx, x86_info->flags + flag_len, sizeof(x86_info->flags) - flag_len);
        }
    }

    if (x86_info->standard_mask & (1 << CPUID_STANDARD_B_MASK))
    {
        int subleaf = 0;
        for (subleaf = 0; ; subleaf++)
        {
            int level_type = 0;
            __cpuid_count(CPUID_STANDARD_B_MASK, subleaf, eax, ebx, ecx, edx);
            
            if (!eax && !ebx)
            {
                break;
            }

            level_type = (ecx >> 8) & 0xFF;
            if (level_type == 1)
            {
                x86_info->threads_per_core = ebx;
            }
            else if (level_type == 2)
            {
                x86_info->cores_per_socket = ebx;
            }
        }

        if (x86_info->threads_per_core)
        {
            x86_info->cores_per_socket = x86_info->cores_per_socket / x86_info->threads_per_core;
        }
    }

    if (x86_info->extended_mask & (1 << CPUID_EXTENDED_1_MASK))
    {
        __cpuid(0x80000000 | CPUID_EXTENDED_1_MASK, eax, ebx, ecx, edx);
        if (is_intel_cpu(x86_info->vendor))
        {
            flag_len += get_x86_cpu_extended_flags(1, ecx, edx, x86_info->flags + flag_len, sizeof(x86_info->flags) - flag_len);
        }
        else if (is_amd_cpu(x86_info->vendor))
        {
            flag_len += get_x86_cpu_extended_flags(0, ecx, edx, x86_info->flags + flag_len, sizeof(x86_info->flags) - flag_len);
        }
    }

    if ((is_amd_cpu(x86_info->vendor)) && (x86_info->extended_mask & (1 << CPUID_EXTENDED_5_MASK)))
    {
        __cpuid(0x80000000 | CPUID_EXTENDED_5_MASK, eax, ebx, ecx, edx);

        snprintf(amd_l1d_cache, sizeof(amd_l1d_cache), "%dK", ((ecx >> 24) & 0xFF));
        x86_info->l1d_cache = amd_l1d_cache;

        snprintf(amd_l1i_cache, sizeof(amd_l1i_cache), "%dK", ((edx >> 24) & 0xFF));
        x86_info->l1i_cache = amd_l1i_cache;
    }

    if ((is_amd_cpu(x86_info->vendor)) && (x86_info->extended_mask & (1 << CPUID_EXTENDED_5_MASK)))
    {
        __cpuid(0x80000000 | CPUID_EXTENDED_6_MASK, eax, ebx, ecx, edx);

        snprintf(amd_l2_cache, sizeof(amd_l2_cache), "%dK", ((ecx >> 16) & 0xFFFF));
        x86_info->l2_cache = amd_l2_cache;

        snprintf(amd_l3_cache, sizeof(amd_l3_cache), "%dM", (int)(((edx >> 18) & 0x3FFF) * 0.512));
        x86_info->l3_cache = amd_l3_cache;
    }

    /* Remove last space */
    if (flag_len && (x86_info->flags[flag_len - 1] == ' '))
    {
        x86_info->flags[flag_len - 1] = '\0';
    }
    return;
}

static void usage(void)
{
    fprintf(stderr, "usage: lscpu [-h]\n");
    exit(1);
}

static void print_cpu_info(gen_cpu_info *gen_info, x86_cpu_info *x86_info)
{
    printf("%-24s %s\n", "Architecture:", gen_info->arch);
    printf("%-24s %s\n", "Byte Order:", gen_info->byte_order == 1234 ? "Little Endian" : "Big Endian");
#ifdef __OpenBSD__
    printf("%-24s %d\n", "Active CPU(s):", gen_info->active_cpu_num);
    printf("%-24s %d\n", "Total CPU(s):", gen_info->total_cpu_num);
#else /* __FreeBSD__ */
    printf("%-24s %d\n", "Total CPU(s):", gen_info->active_cpu_num);
#endif

    if (x86_info->threads_per_core)
    {
        printf("%-24s %d\n", "Thread(s) per core:", x86_info->threads_per_core);
    }

    if (x86_info->cores_per_socket)
    {
        printf("%-24s %d\n", "Core(s) per socket:", x86_info->cores_per_socket);
    }

    if ((x86_info->threads_per_core) && (x86_info->cores_per_socket))
    {
#ifdef __OpenBSD__
        int total_cpu_num = gen_info->total_cpu_num;
#else /* __FreeBSD__ */
        int total_cpu_num = gen_info->total_cpu_num;
#endif
    printf("%-24s %d\n", "Socket(s):", total_cpu_num / ((x86_info->threads_per_core) * (x86_info->cores_per_socket)));
    }

    if (x86_cpu_support_standard_flag(x86_info->standard_mask, CPUID_STANDARD_0_MASK))
    {
        printf("%-24s %s\n", "Vendor:", x86_info->vendor);
    }
    else 
    {        
#ifdef __OpenBSD__
        printf("%-24s %s\n", "Vendor:", gen_info->vendor);
#endif
    }

    if (x86_cpu_support_standard_flag(x86_info->standard_mask, CPUID_STANDARD_1_MASK))
    {
        printf("%-24s %d\n", "CPU family:", x86_info->family);
        printf("%-24s %d\n", "Model:", x86_info->model);        
    }
    printf("%-24s %s\n", "Model name:", gen_info->model);
    if (x86_cpu_support_standard_flag(x86_info->standard_mask, CPUID_STANDARD_1_MASK))
    {   
        printf("%-24s %d\n", "Stepping:", x86_info->stepping);
    }

#ifdef __OpenBSD__
    printf("%-24s %d\n", "CPU MHz:", gen_info->speed);
#endif

    if (x86_info->l1d_cache)
    {
        printf("%-24s %s\n", "L1d cache:", x86_info->l1d_cache);
    }
    if (x86_info->l1i_cache)
    {
        printf("%-24s %s\n", "L1i cache:", x86_info->l1i_cache);
    }
    if (x86_info->l2_cache)
    {
        printf("%-24s %s\n", "L2 cache:", x86_info->l2_cache);
    }
    if (x86_info->l3_cache)
    {
        printf("%-24s %s\n", "L3 cache:", x86_info->l3_cache);
    }

    if (x86_info->flags[0])
    {
        printf("%-24s %s\n", "Flags:", x86_info->flags);
    }

    return;
}

int main(int argc, char **argv) 
{
    int mib[2], ch = 0, i = 0;

    sysctl_get_cpu_info sysctl_array[] = {
        {HW_MACHINE, gen_info.arch, sizeof(gen_info.arch), "HW_MACHINE"},
        {HW_BYTEORDER, &(gen_info.byte_order), sizeof(gen_info.byte_order), "HW_BYTEORDER"},
        {HW_MODEL, gen_info.model, sizeof(gen_info.model), "HW_MODEL"},
        {HW_NCPU, &(gen_info.active_cpu_num), sizeof(gen_info.active_cpu_num), "HW_NCPU"},
#ifdef __OpenBSD__
        {HW_VENDOR, gen_info.vendor, sizeof(gen_info.vendor), "HW_VENDOR"},
        {HW_NCPUFOUND, &(gen_info.total_cpu_num), sizeof(gen_info.total_cpu_num), "HW_NCPUFOUND"},
        {HW_CPUSPEED, &(gen_info.speed), sizeof(gen_info.speed), "HW_CPUSPEED"},
#endif
    };

    while ((ch = getopt(argc, argv, "h")) != -1) 
    {
        switch (ch)
        {
            case 'h':
            case '?':
            default:
            {
                usage();
            }
        }
    }

    argc -= optind;
    argv += optind;

    if (argc)
    {
        usage();
    }

    for (i = 0; i < ARRAY_LEN(sysctl_array); i++)
    {
        mib[0] = CTL_HW;
        mib[1] = sysctl_array[i].mib_code;
        if (sysctl(mib, ARRAY_LEN(mib), sysctl_array[i].old, &sysctl_array[i].old_len, NULL, 0) == -1)
        {
            if (errno == EOPNOTSUPP)
            {
                continue;
            }
            err(1, "%s", sysctl_array[i].err_msg);
        }
    }

    if (is_x86_cpu(gen_info.arch))
    {
        get_x86_cpu_info(&x86_info);
    }

    print_cpu_info(&gen_info, &x86_info);

    return 0;
}

