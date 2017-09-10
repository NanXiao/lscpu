#include <sys/param.h> 
#include <sys/sysctl.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))

typedef struct
{
    char arch[32];
    char vendor[32];
    int active_cpu_num;
    int total_cpu_num;
} gen_cpu_info;

typedef struct
{
    int mib_code;
    void *old;
    size_t old_len;
    char *err_msg;
} sysctl_get_cpu_info;

int main(int argc, char **argv) 
{
    int mib[2], i = 0;
    gen_cpu_info gen_info;
    sysctl_get_cpu_info sysctl_array[] = {
        {HW_MACHINE, gen_info.arch, sizeof(gen_info.arch) - 1, "HW_MACHINE"},
        {HW_VENDOR, gen_info.vendor, sizeof(gen_info.vendor) - 1, "HW_VENDOR"},
        {HW_NCPU, &(gen_info.active_cpu_num), sizeof(gen_info.active_cpu_num), "HW_NCPU"},
        {HW_NCPUFOUND, &(gen_info.total_cpu_num), sizeof(gen_info.total_cpu_num), "HW_NCPUFOUND"}};

    memset(&gen_info, 0, sizeof(gen_info));

    for (i = 0; i < ARRAY_LEN(sysctl_array); i++)
    {
        mib[0] = CTL_HW;
        mib[1] = sysctl_array[i].mib_code;
        if (sysctl(mib, ARRAY_LEN(mib), sysctl_array[i].old, &sysctl_array[i].old_len, NULL, 0) == -1)
        {
    		err(1, sysctl_array[i].err_msg);
        }
    }
   
    printf("%-16s: %s\n", "Architecture", gen_info.arch);
    printf("%-16s: %s\n", "Vendor", gen_info.vendor);
    printf("%-16s: %d\n", "Active CPU(s)", gen_info.active_cpu_num);
    printf("%-16s: %d\n", "Total CPU(s)", gen_info.total_cpu_num);
    return 0;    
}

