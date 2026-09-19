<p align="center">
  <img src="Images/NoX.png" width="180" height="180" alt="NoX logo">
</p>


**NoX** is an aggressive, kernel-level anti-porn enforcement module for Linux. By inspecting DNS queries directly in the network stack, NoX enforces immediate system reboots upon detecting domain requests matching a local dataset of over 4.6 million adult domains.

---

## Details

* **Kernel Space Interception:** Hooks `NF_INET_LOCAL_OUT` via Netfilter to inspect outgoing UDP port 53 DNS queries prior to payload processing.
* **High-Performance Filter:** Utilizes an ( 8 MiB ) 64-byte cache-line aligned Blocked Bloom Filter (O(1) bitwise lookup) to store ~4.6M domain hashes with a ~0.1% false-positive tolerance.
* **Immediate Enforcement:** Triggers a deferred `orderly_reboot()` immediately upon a Bloom filter hit, halting user sessions before TCP connections establish.
* **Permanent Module:** Cannot be terminated once loaded ( unless CONFIG_MODULE_FORCE_UNLOAD ) because no module_exit function is defined.

---

## Bloom-Filter
* Filter is generated as a .c file, and needs to build into the NoX Kernel-Module binary. ( not optimal? send PR. )
* To generate a filter, use the BloomFilteGen. ( build : `gcc BloomFilterGen/BloomFilterGen.c BloomFilter/BloomFilter.c Hash/murmur3.c -lm -o BloomFilterGen/BloomFilterGen.out` )
* Data feed into the BloomFilterGen is expected to be in `domain + \n` format.
* Once BloomFilter is generated it can be tested using the BloomFilter-Test-Tools ( BloomFilterGen/GenTestTools ). 
* Build the test tools using `gcc BloomFilterGen/GenTestTools/BloomFilterTest.c BloomFilter/BloomFilter.c Hash/murmur3.c Filter.c -o BloomFilterGen/GenTestTools/BloomFilterTest.out` and `gcc BloomFilterGen/GenTestTools/BloomFilterTestCli.c BloomFilter/BloomFilter.c Hash/murmur3.c Filter.c -o BloomFilterGen/GenTestTools/BloomFilterTestCli.out`

---

## How To Build
* Make sure `Filter.c` file contains our filter and run `./build.sh`

---

> **Warning:** NoX executes `orderly_reboot()` immediately upon domain detection. Any unsaved work in user-space applications will be lost when a blocked domain is requested.
