# lscpu
lscpu for OpenBSD/FreeBSD.  

## Usage

	$ git clone https://github.com/NanXiao/lscpu.git
	$ cd lscpu
	$ make
	$ ./lscpu

## Output

	$ ./lscpu
	Architecture:            i386
	Byte Order:              Little Endian
	Active CPU(s):           2
	Total CPU(s):            2
	Thread(s) per core:      1
	Core(s) per socket:      2
	Socket(s):               1
	Vendor:                  GenuineIntel
	CPU family:              6
	Model:                   63
	Model name:              Intel(R) Xeon(R) CPU E5-2620 v3 @ 2.40GHz ("GenuineIntel" 686-class)
	Stepping:                2
	CPU MHz:                 2401
	L1d cache:               32K
	L1i cache:               32K
	L2 cache:                256K
	L3 cache:                15M
	Flags:                   fpu vme de pse tsc msr mce cx8 apic sep mtrr pge mca cmov pat pse36 cflsh mmx fxsr sse sse2 htt sse3 pclmulqdq ssse3 cx16 sse4_1 sse4_2 movbe popcnt aes xsave avx rdrnd fpcsds syscall pdpe1gb lahf_lm
## Acknowledgement
Thanks for [yggdr](https://github.com/yggdr)'s testing on `AMD` processors.


