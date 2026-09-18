<p align="center">
  <img src="Images/NoX.png" width="180" height="180" alt="NoX logo">
</p>


**NoX** is an aggressive, kernel-level anti-porn enforcement module for Linux. By inspecting DNS queries directly in the network stack, NoX enforces immediate system reboots upon detecting domain requests matching a local dataset of over 4.6 million adult domains.

---

## Technical Architecture

* **Kernel Space Interception:** Hooks `NF_INET_LOCAL_OUT` via Netfilter to inspect outgoing UDP port 53 DNS queries prior to payload processing.
* **High-Performance Filter:** Utilizes an ( 8 MiB ) 64-byte cache-line aligned Blocked Bloom Filter (O(1) bitwise lookup) to store ~4.6M domain hashes with a ~0.1% false-positive tolerance.
* **Immediate Enforcement:** Triggers a deferred `orderly_reboot()` immediately upon a Bloom filter hit, halting user sessions before TCP connections establish.

---

A syscall hook can prevent user from unloading NoX kernel module ( gotta add that soon ).

---

> **Warning:** NoX executes `orderly_reboot()` immediately upon domain detection. Any unsaved work in user-space applications will be lost when a blocked domain is requested.
