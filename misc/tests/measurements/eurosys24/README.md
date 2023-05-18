## Machine info

- [Adelaide](https://github.com/TUM-DSE/doctor-cluster-config/blob/master/docs/hosts/adelaide.md)

### `nix-info -m`
- system: `"x86_64-linux"`
- host os: `Linux 6.2.12, NixOS, 22.11 (Raccoon), 22.11.20230508.5afd4c0`
- multi-user?: `yes`
- sandbox: `yes`
- version: `nix-env (Nix) 2.11.1`
- channels(yihe): `""`
- nixpkgs: `/nix/store/zn8sk5nc01mxm4dblqlngxc3pgjwgjp7-m1b29qzr7fk9v5mygg6zwsg504i9vff6-source`

### `lscpu`
```
Architecture:            x86_64
  CPU op-mode(s):        32-bit, 64-bit
  Address sizes:         46 bits physical, 57 bits virtual
  Byte Order:            Little Endian
CPU(s):                  12
  On-line CPU(s) list:   0-11
Vendor ID:               GenuineIntel
  Model name:            Intel(R) Xeon(R) Gold 5317 CPU @ 3.00GHz
    CPU family:          6
    Model:               106
    Thread(s) per core:  1
    Core(s) per socket:  12
    Socket(s):           1
    Stepping:            6
    CPU(s) scaling MHz:  92%
    CPU max MHz:         3000.0000
    CPU min MHz:         800.0000
    BogoMIPS:            6000.00
    Flags:               fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi m
                         mx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon p
                         ebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf pni pclmulqdq dtes64 monitor
                         ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe p
                         opcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault e
                         pb cat_l3 invpcid_single intel_ppin ssbd mba ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi
                          flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid cqm rdt
                         _a avx512f avx512dq rdseed adx smap avx512ifma clflushopt clwb intel_pt avx512cd sha_ni av
                         x512bw avx512vl xsaveopt xsavec xgetbv1 xsaves cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm
                         _local split_lock_detect wbnoinvd dtherm arat pln pts avx512vbmi umip pku ospke avx512_vbm
                         i2 gfni vaes vpclmulqdq avx512_vnni avx512_bitalg tme avx512_vpopcntdq la57 rdpid fsrm md_
                         clear pconfig flush_l1d arch_capabilities
Virtualization features:
  Virtualization:        VT-x
Caches (sum of all):
  L1d:                   576 KiB (12 instances)
  L1i:                   384 KiB (12 instances)
  L2:                    15 MiB (12 instances)
  L3:                    18 MiB (1 instance)
NUMA:
  NUMA node(s):          1
  NUMA node0 CPU(s):     0-11
Vulnerabilities:
  Itlb multihit:         Not affected
  L1tf:                  Not affected
  Mds:                   Not affected
  Meltdown:              Not affected
  Mmio stale data:       Mitigation; Clear CPU buffers; SMT disabled
  Retbleed:              Not affected
  Spec store bypass:     Mitigation; Speculative Store Bypass disabled via prctl
  Spectre v1:            Mitigation; usercopy/swapgs barriers and __user pointer sanitization
  Spectre v2:            Mitigation; Enhanced IBRS, IBPB conditional, RSB filling, PBRSB-eIBRS SW sequence
  Srbds:                 Not affected
  Tsx async abort:       Not affected
```

### `free -h`
```
              total        used        free      shared  buff/cache   available
Mem:          995Gi        87Gi       904Gi        26Mi       2.8Gi       902Gi
Swap:            0B          0B          0B
```

### `dmidecode --type memory`
```
# dmidecode 3.4
Getting SMBIOS data from sysfs.
SMBIOS 3.3.0 present.

Handle 0x002A, DMI type 16, 23 bytes
Physical Memory Array
        Location: System Board Or Motherboard
        Use: System Memory
        Error Correction Type: Single-bit ECC
        Maximum Capacity: 12 TB
        Error Information Handle: Not Provided
        Number Of Devices: 8

Handle 0x002B, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 64 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMMA1
        Bank Locator: P0_Node0_Channel0_Dimm0
        Type: DDR4
        Type Detail: Synchronous Cache DRAM Registered (Buffered)
        Speed: 3200 MT/s
        Manufacturer: Samsung
        Serial Number: H1KX00014148249FED
        Asset Tag: DIMMA1_AssetTag (date:21/41)
        Part Number: M393A8G40AB2-CWE
        Rank: 2
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: DRAM
        Memory Operating Mode Capability: Volatile memory
        Firmware Version: 0000
        Module Manufacturer ID: Bank 1, Hex 0xCE
        Module Product ID: Unknown
        Memory Subsystem Controller Manufacturer ID: Unknown
        Memory Subsystem Controller Product ID: Unknown
        Non-Volatile Size: None
        Volatile Size: None
        Cache Size: 64 GB
        Logical Size: None

Handle 0x002D, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 256 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMMB1
        Bank Locator: P0_Node0_Channel1_Dimm0
        Type: Logical non-volatile device
        Type Detail: Synchronous Non-Volatile LRDIMM
        Speed: 3200 MT/s
        Manufacturer: Intel
        Serial Number: 214400000D62
        Asset Tag: DIMMB1_AssetTag (date:21/44)
        Part Number: NMB1XXD256GPS
        Rank: 1
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: Intel Optane DC persistent memory
        Memory Operating Mode Capability: Volatile memory Byte-accessible persistent memory
        Firmware Version: 1553
        Module Manufacturer ID: Bank 1, Hex 0x89
        Module Product ID: 0x4AA7
        Memory Subsystem Controller Manufacturer ID: Bank 1, Hex 0x89
        Memory Subsystem Controller Product ID: 0x097B
        Non-Volatile Size: None
        Volatile Size: 253 GB
        Cache Size: None
        Logical Size: None

Handle 0x002F, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 64 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMMC1
        Bank Locator: P0_Node0_Channel2_Dimm0
        Type: DDR4
        Type Detail: Synchronous Cache DRAM Registered (Buffered)
        Speed: 3200 MT/s
        Manufacturer: Samsung
        Serial Number: H1KX00014148249FDD
        Asset Tag: DIMMC1_AssetTag (date:21/41)
        Part Number: M393A8G40AB2-CWE
        Rank: 2
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: DRAM
        Memory Operating Mode Capability: Volatile memory
        Firmware Version: 0000
        Module Manufacturer ID: Bank 1, Hex 0xCE
        Module Product ID: Unknown
        Memory Subsystem Controller Manufacturer ID: Unknown
        Memory Subsystem Controller Product ID: Unknown
        Non-Volatile Size: None
        Volatile Size: None
        Cache Size: 64 GB
        Logical Size: None

Handle 0x0031, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 256 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMMD1
        Bank Locator: P0_Node0_Channel3_Dimm0
        Type: Logical non-volatile device
        Type Detail: Synchronous Non-Volatile LRDIMM
        Speed: 3200 MT/s
        Manufacturer: Intel
        Serial Number: 214400000EE2
        Asset Tag: DIMMD1_AssetTag (date:21/44)
        Part Number: NMB1XXD256GPS
        Rank: 1
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: Intel Optane DC persistent memory
        Memory Operating Mode Capability: Volatile memory Byte-accessible persistent memory
        Firmware Version: 1553
        Module Manufacturer ID: Bank 1, Hex 0x89
        Module Product ID: 0x4AA7
        Memory Subsystem Controller Manufacturer ID: Bank 1, Hex 0x89
        Memory Subsystem Controller Product ID: 0x097B
        Non-Volatile Size: None
        Volatile Size: 253 GB
        Cache Size: None
        Logical Size: None

Handle 0x0033, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 64 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMME1
        Bank Locator: P0_Node1_Channel0_Dimm0
        Type: DDR4
        Type Detail: Synchronous Cache DRAM Registered (Buffered)
        Speed: 3200 MT/s
        Manufacturer: Samsung
        Serial Number: H1KX00014148249C59
        Asset Tag: DIMME1_AssetTag (date:21/41)
        Part Number: M393A8G40AB2-CWE
        Rank: 2
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: DRAM
        Memory Operating Mode Capability: Volatile memory
        Firmware Version: 0000
        Module Manufacturer ID: Bank 1, Hex 0xCE
        Module Product ID: Unknown
        Memory Subsystem Controller Manufacturer ID: Unknown
        Memory Subsystem Controller Product ID: Unknown
        Non-Volatile Size: None
        Volatile Size: None
        Cache Size: 64 GB
        Logical Size: None

Handle 0x0035, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 256 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMMF1
        Bank Locator: P0_Node1_Channel1_Dimm0
        Type: Logical non-volatile device
        Type Detail: Synchronous Non-Volatile LRDIMM
        Speed: 3200 MT/s
        Manufacturer: Intel
        Serial Number: 214400000E8D
        Asset Tag: DIMMF1_AssetTag (date:21/44)
        Part Number: NMB1XXD256GPS
        Rank: 1
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: Intel Optane DC persistent memory
        Memory Operating Mode Capability: Volatile memory Byte-accessible persistent memory
        Firmware Version: 1553
        Module Manufacturer ID: Bank 1, Hex 0x89
        Module Product ID: 0x4AA7
        Memory Subsystem Controller Manufacturer ID: Bank 1, Hex 0x89
        Memory Subsystem Controller Product ID: 0x097B
        Non-Volatile Size: None
        Volatile Size: 253 GB
        Cache Size: None
        Logical Size: None

Handle 0x0037, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 64 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMMG1
        Bank Locator: P0_Node1_Channel2_Dimm0
        Type: DDR4
        Type Detail: Synchronous Cache DRAM Registered (Buffered)
        Speed: 3200 MT/s
        Manufacturer: Samsung
        Serial Number: H1KX00014148249F0B
        Asset Tag: DIMMG1_AssetTag (date:21/41)
        Part Number: M393A8G40AB2-CWE
        Rank: 2
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: DRAM
        Memory Operating Mode Capability: Volatile memory
        Firmware Version: 0000
        Module Manufacturer ID: Bank 1, Hex 0xCE
        Module Product ID: Unknown
        Memory Subsystem Controller Manufacturer ID: Unknown
        Memory Subsystem Controller Product ID: Unknown
        Non-Volatile Size: None
        Volatile Size: None
        Cache Size: 64 GB
        Logical Size: None

Handle 0x0039, DMI type 17, 92 bytes
Memory Device
        Array Handle: 0x002A
        Error Information Handle: Not Provided
        Total Width: 72 bits
        Data Width: 64 bits
        Size: 256 GB
        Form Factor: DIMM
        Set: None
        Locator: DIMMH1
        Bank Locator: P0_Node1_Channel3_Dimm0
        Type: Logical non-volatile device
        Type Detail: Synchronous Non-Volatile LRDIMM
        Speed: 3200 MT/s
        Manufacturer: Intel
        Serial Number: 214400000D6C
        Asset Tag: DIMMH1_AssetTag (date:21/44)
        Part Number: NMB1XXD256GPS
        Rank: 1
        Configured Memory Speed: 2933 MT/s
        Minimum Voltage: 1.2 V
        Maximum Voltage: 1.2 V
        Configured Voltage: 1.2 V
        Memory Technology: Intel Optane DC persistent memory
        Memory Operating Mode Capability: Volatile memory Byte-accessible persistent memory
        Firmware Version: 1553
        Module Manufacturer ID: Bank 1, Hex 0x89
        Module Product ID: 0x4AA7
        Memory Subsystem Controller Manufacturer ID: Bank 1, Hex 0x89
        Memory Subsystem Controller Product ID: 0x097B
        Non-Volatile Size: None
        Volatile Size: 253 GB
        Cache Size: None
        Logical Size: None
```
