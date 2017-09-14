#include <sys/param.h> 
#include <sys/sysctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <machine/cpu.h>
#include <cpuid.h>

/* macro definitions */
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define CPUID_MAX_STANDARD_FUNCTION (0x17)
#define CPUID_MAX_EXTENDED_FUNCTION (0x08)

/* struct definitions */
typedef struct
{
    char arch[16];
    int byte_order;
    char model[64];
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
    char vendor[13];
    unsigned char stepping;
    unsigned char model;
    unsigned short family;
    char standard_flags[256];
} x86_cpu_info;

/* variables definitions */
gen_cpu_info gen_info;
x86_cpu_info x86_info;

/* function definitions*/
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

static void get_cpu_standard_flags(int intel, uint32_t ecx, uint32_t edx, char *flags, size_t len)
{
    snprintf(flags, len,
            /* edx*/
            "%s%s%s%s%s%s%s%s"
            "%s%s%s%s%s%s%s"
            "%s%s%s%s%s%s%s"
            "%s%s%s%s%s%s%s"
            /* ecx */
            "%s%s"
            "%s%s%s"
            "%s%s"
            "%s",
            edx & 0x00000001 ? "fpu " : "",
            edx & 0x00000002 ? "vme " : "",
            edx & 0x00000004 ? "de " : "",
            edx & 0x00000008 ? "pse " : "",
            edx & 0x00000010 ? "msr " : "",
            edx & 0x00000020 ? "tsc " : "",
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
            intel ? (ecx & 0x00000008 ? "mwait " : "") : "",
            intel ? (ecx & 0x00000010 ? "cpl " : "") : "",
            intel ? (ecx & 0x00000020 ? "vmx " : "") : "",
            intel ? (ecx & 0x00000080 ? "est " : "") : "",
            intel ? (ecx & 0x00000100 ? "tm2 " : "") : "",
            intel ? (ecx & 0x00000400 ? "l1 " : "") : "",
            ecx & 0x00002000 ? "cmpxchg16b " : "");
}

static void get_x86_cpu_info(x86_cpu_info *x86_info)
{
    int i = 0;
    uint32_t eax, ebx, ecx, edx;
    
    __cpuid (0, eax, ebx, ecx, edx);
    memcpy(x86_info->vendor, &ebx, sizeof(ebx));
    memcpy(&(x86_info->vendor[4]), &edx, sizeof(edx));
    memcpy(&(x86_info->vendor[8]), &ecx, sizeof(ecx));
    for (i = 0; (i <= eax) && (i <= CPUID_MAX_STANDARD_FUNCTION); i++)
    {
        x86_info->standard_mask |= (1 << i);
    }

    eax = 1;
    if (x86_info->standard_mask & (1 << eax))
    {
        __cpuid (eax, eax, ebx, ecx, edx);
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
            get_cpu_standard_flags(1, ecx, edx, x86_info->standard_flags, sizeof(x86_info->standard_flags));
        }
        else if (is_amd_cpu(x86_info->vendor))
        {
            get_cpu_standard_flags(0, ecx, edx, x86_info->standard_flags, sizeof(x86_info->standard_flags));
        }
    }
}

static void usage(void)
{
    fprintf(stderr, "usage: lscpu [-h]\n");
    exit(1);
}

int main(int argc, char **argv) 
{
    int mib[2], ch = 0, i = 0;

    sysctl_get_cpu_info sysctl_array[] = {
        {HW_MACHINE, gen_info.arch, sizeof(gen_info.arch), "HW_MACHINE"},
        {HW_BYTEORDER, &(gen_info.byte_order), sizeof(gen_info.byte_order), "HW_BYTEORDER"},
        {HW_MODEL, gen_info.model, sizeof(gen_info.model), "HW_MODEL"},
        {HW_VENDOR, gen_info.vendor, sizeof(gen_info.vendor), "HW_VENDOR"},
        {HW_NCPU, &(gen_info.active_cpu_num), sizeof(gen_info.active_cpu_num), "HW_NCPU"},
        {HW_NCPUFOUND, &(gen_info.total_cpu_num), sizeof(gen_info.total_cpu_num), "HW_NCPUFOUND"},
        {HW_CPUSPEED, &(gen_info.speed), sizeof(gen_info.speed), "HW_CPUSPEED"},};

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
            err(1, sysctl_array[i].err_msg);
        }
    }

    if (is_x86_cpu(gen_info.arch))
    {
        get_x86_cpu_info(&x86_info);
    }

    printf("%-16s %s\n", "Architecture:", gen_info.arch);
    printf("%-16s %s\n", "Byte Order:", gen_info.byte_order == 1234 ? "Little Endian" : "Big Endian");    
    printf("%-16s %d\n", "Active CPU(s):", gen_info.active_cpu_num);
    printf("%-16s %d\n", "Total CPU(s):", gen_info.total_cpu_num);
    if (is_x86_cpu(gen_info.arch))
    {
        printf("%-16s %s\n", "Vendor:", x86_info.vendor);
        printf("%-16s %d\n", "CPU family:", x86_info.family);
        printf("%-16s %d\n", "Model:", x86_info.model);        
    }
    else 
    {        
        printf("%-16s %s\n", "Vendor:", gen_info.vendor);
    }
    printf("%-16s %s\n", "Model name:", gen_info.model);
    if (is_x86_cpu(gen_info.arch))
    {   
        printf("%-16s %d\n", "Stepping:", x86_info.stepping);
    }
    printf("%-16s %d\n", "CPU MHz:", gen_info.speed);
    if (is_intel_cpu(x86_info.vendor) || is_amd_cpu(x86_info.vendor))
    {
        printf("%-16s %s\n", "Flags:", x86_info.standard_flags);
    }
    return 0;    
}

