# lscpu
lscpu for OpenBSD/FreeBSD.  

## Usage

	$ git clone https://github.com/NanXiao/lscpu.git
	$ cd lscpu
	$ make
	$ ./lscpu

## Output

	$ ./lscpu
	Architecture:    amd64
	Byte Order:      Little Endian
	Active CPU(s):   2
	Total CPU(s):    2
	Vendor:          GenuineIntel
	CPU family:      6
	Model:           23
	Model name:      Intel(R) Core(TM)2 Duo CPU P8700 @ 2.53GHz
	Stepping:        10
	CPU MHz:         2534
	L1d cache:       32K
	L1i cache:       32K
	L2 cache:        3M
	Flags:           fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 cflsh ds acpi mmx fxsr sse sse2 ss htt tm pbe sse3 dtes64 monitor ds_cpl vmx smx est tm2 ssse3 cx16 xtpr pdcm sse4_1 xsave osxsave syscall pdpe1gb rdtscp lm lahf_lm lzcnt

