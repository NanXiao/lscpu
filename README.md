# lscpu
lscpu for BSDs. The main usage of this program should be for x86 architecture since it leverages [CPUID](https://en.wikipedia.org/wiki/CPUID) instruction. For other architectures, it just shows very limited facts.  

lscpu has been verified to work on following BSDs:  

<table>
  <tr><td>macOS</td><td>OpenBSD</td><td>FreeBSD</td><td>NetBSD</td></tr>
  <tr><td>DragonFlyBSD</td><td>MidnightBSD</td><td>?</td><td></td></tr>
</table>

It should also works on other BSDs, though not tested. If you find lscpu can also runs on other BSD distros, welcome to tell me through [mail](mailto:nan@chinadtrace.org) or just open a new [issue](https://github.com/NanXiao/lscpu/issues/new), thanks very much in advance!  

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
Thanks for [yggdr](https://github.com/yggdr)'s testing on AMD processors.  
Thanks for [bit_of_hope](https://www.reddit.com/r/BSD/comments/72bi57/lscpu_for_openbsdfreebsd/dnhnifm/)'s testing on NetBSD.  
Thanks for Aleksej Lebedev's testing on DragonFlyBSD.  
Thanks for [Lucas Holt](https://github.com/laffer1)'s testing on MidnightBSD and add lscpu to [MidnightBSD mports tree](http://svn.midnightbsd.org/svn/mports/trunk/sysutils/lscpu/).  
Thanks for [zi0r](https://github.com/zi0r)'s adding lscpu to [FreeBSD ports tree](https://www.freshports.org/sysutils/lscpu).

