<p align="center">
  <img src="Images/NoX.png" width="180" height="180" alt="NoX logo">
</p>


NoX is a offensive anti-porn linux kernel module. If your requested domain is present in an exhaustive list of porn domains containing 4.6 million domains, we force a system reboot.

By adding NoX as an auto-start ( at-boot ) kernel module, NoX should provide enough resistance between dishonorable domains.

Some work is pending to make NoX hard to terminate. By hooking syscalls we can make it prevent the user from terminating NoX kernel module.
