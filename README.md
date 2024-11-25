Pagewalk
========

Just a dummy little tool because I needed this way too many times but faced that there's no `info tlb` in qemu-aarch64, `info mtree`
is not what you think, qemu never had a `page` command in the first place, and bochs does not support aarch64 at all... So this
tool comes to the rescue, works with both **x86_64** and **AArch64** memory dumps and displays a tree of the page translation
tables or a detailed path for a specific virtual address for debugging purposes. Totally dependency-free.

Compilation
-----------

```
gcc pagewalk.c -o pagewalk
```

Usage
-----

```
pagewalk <arch> <memdump> <root> [virtual address]

  <arch>      x86_64 / aarch64
  <memdump>   memory dump file (from address 0, as big as necessary)
  <root>      paging table root in hex (value of CR3 / TTBR0 / TTBR1)
  [address]   address to look up in hex (in lack draws a tree)

Bochs (no need, use built-in "info tab" / "page" commands, but if you want):
  1. press break button                 - get to debugger
  2. writemem "memdump" 0 (size)        - create dump
  3. creg                               - display value of CR3
qemu:
  1. Ctrl+Alt+2                         - get to debugger
  2. pmemsave 0 (size) memdump          - create dump
  3. info registers                     - display CR3 / TTBR
```

Examples:

```
$ ./pagewalk aarch64 aaa.bin 2000 FFFFFFFFC0000000
TTBR1        0000000000002000
L1   idx 511 0000000001047703 ..0...GA4fC.SWX 01047000
L2   idx 511 0000000001048703 ..0...GA4fC.SWX 01048000
L3   idx 511 0000000000000000 ..0...G.4eCGSWX (not present)
data (?)
```

```
$ ./pagewalk x86_64 aaa.bin 2000 FFFFFFFFC0000000
CR3          0000000000002000
PML4 idx 511 0000000001047703 .G...C.SWX 01047000
PDPE idx 511 0000000001048703 .G...C.SWX 01048000
PDE  idx 511 0000000000000000 .....C.SRX (not present)
PTE  (?)
data (?)
```

Cheers,
bzt
