# !CR

The *chromium-debug* is a set of debugger extensions to help debugging Chromium on Windows.

Please note that this extension highly depends on the data structure of Chromium core (chrome_child.dll in Google Chrome) and does not keep backward compatibility.  Some commands may not work with an older Chrome/Chromium version.  Moreover, most of the commands do not work for 32bit Chromium process.  Sorry for the inconvenience.

## Command examples

### DOM & Layout tree

```
0:000> !lay @rcx
Target Engine = 61.3163
    0 0000020e`86c04000 blink::LayoutView 0000003b`7c3425c0 blink::HTMLDocument
    1  0000020e`86c20000 blink::LayoutBlockFlow 0000003b`7c343270 blink::HTMLHtmlElement
    2   0000020e`86c200f8 blink::LayoutBlockFlow 0000003b`7c3436f0 blink::HTMLBodyElement
    3    0000020e`86c201f0 blink::LayoutBlockFlow 0000003b`7c3437a8 blink::HTMLHeadingElement
    4     0000020e`86c24000 blink::LayoutText 0000003b`7c343810 blink::Text
    5    0000020e`86c202e8 blink::LayoutBlockFlow 0000003b`7c3438b0 blink::HTMLUListElement
    6     0000020e`86c34000 blink::LayoutListItem 0000003b`7c343968 blink::HTMLLIElement
    7      0000020e`86c38000 blink::LayoutListMarker 0000003b`7c3425c0 blink::HTMLDocument
    8      0000020e`86c48000 blink::LayoutInline 0000003b`7c3439d0 blink::HTMLAnchorElement
    9     0000020e`86c34110 blink::LayoutListItem 0000003b`7c343b00 blink::HTMLLIElement
   10      0000020e`86c380e8 blink::LayoutListMarker 0000003b`7c3425c0 blink::HTMLDocument
   11      0000020e`86c48090 blink::LayoutInline 0000003b`7c343b68 blink::HTMLAnchorElement
   12     0000020e`86c34220 blink::LayoutListItem 0000003b`7c343c98 blink::HTMLLIElement
   13      0000020e`86c381d0 blink::LayoutListMarker 0000003b`7c3425c0 blink::HTMLDocument
   14      0000020e`86c48120 blink::LayoutInline 0000003b`7c343d00 blink::HTMLAnchorElement
0:000> !dom 0000003b`7c343810
    0 0000003b`7c3425c0 blink::HTMLDocument 00001405
    1  0000003b`7c343210 blink::DocumentType 000c1400
    2  0000003b`7c343270 blink::HTMLHtmlElement 0000161c
    3   0000003b`7c3432d8 blink::HTMLHeadElement 0000141c
    4    0000003b`7c343340 blink::Text 00001402
    5    0000003b`7c343390 blink::HTMLMetaElement 0000141c
...snip...
   31       0000003b`7c343d90 blink::Text 00001402
   32     0000003b`7c343de0 blink::Text 00001402
   33    0000003b`7c343e30 blink::Text 00001402
```

### PartitionAlloc

```
0:017> !layout_allocator
Target Engine = 65.3325

WTF::Partitions::layout_allocator_ 00007fff`aa8f3818 -> base::SizeSpecificPartitionAllocator<1024> 00007fff`aa8fab48
extent(s): 00002773`88a00000-00002773`88c00000 (1)

0:017> !buffer_allocator -buckets
WTF::Partitions::buffer_allocator_ 00007fff`aa8f3810 -> base::PartitionAllocatorGeneric 00007fff`aa8f84f0
extent(s): 00000bad`96800000-00000bad`97e00000 (11)
   0 00007fff`aa8f9a40     8   4 00000bad`96e01fa0 550!
   8 00007fff`aa8f9b40    10   4 00000bad`97001f60 923
                                 00000bad`97601300 303
  12 00007fff`aa8f9bc0    18  12 00000bad`97401e80 2029
                                 00000bad`96e01520 2024
                                 00000bad`976016e0 1352
  16 00007fff`aa8f9c40    20   4 00000bad`97201ae0 510
                                 00000bad`96c011e0 510
                                 00000bad`974014c0 472
...snip...
0:017> !bucket 00007fff`aa8f9bc0
slot_size (bytes):              0x18
num_system_pages_per_slot_span: 0n12
total slots per span:           0n2048
active pages / num_allocated_slots:
00000bad`97400000 #116  00000bad`97401e80 2029
00000bad`96e00000 #41   00000bad`96e01520 2024
00000bad`97600000 #55   00000bad`976016e0 1352
0:017> !ppage 00000bad`97401e80
bucket:                  00007fff`aa8f9bc0 (total slots=2048)
num_allocated_slots:     2029
num_unprovisioned_slots: 0
free slots (19):
00000bad`975d8058 00000bad`975d81a8 00000bad`975d83b8 00000bad`975d82b0
00000bad`975d7e48 00000bad`975d7de8 00000bad`975d81d8 00000bad`975d8208
00000bad`975d7e30 00000bad`975d7fb0 00000bad`975d8028 00000bad`975d8010
00000bad`975d7ec0 00000bad`975d8178 00000bad`975d8118 00000bad`975d8130
00000bad`975d80b8 00000bad`975d7ed8 00000bad`975d5610
0:017> !slot 00000bad`975d5610
SuperPage 00000bad`97400000 #117-1
base::PartitionPage 00000bad`97401e80
```

### Oilpan

```
0:025> !ts
Target Engine = 67.3396

MainThreadState() = chrome_child!blink::ThreadState::main_thread_state_storage_
chrome_child!blink::ThreadState 00007ff9`dfe2a050
chrome_child!blink::ThreadHeap 00000215`81b7ab60
chrome_child!blink::PersistentRegion 00003362`a0004020

0:025> !heap 00000215`81b7ab60
arenas:
  0 blink::NormalPageArena 00003362`a0044000
  1 blink::NormalPageArena 00003362`a00440e0
  2 blink::NormalPageArena 00003362`a00441c0
  3 blink::NormalPageArena 00003362`a00442a0
  4 blink::NormalPageArena 00003362`a0044380
  5 blink::NormalPageArena 00003362`a0044460
  6 blink::NormalPageArena 00003362`a0044540
  7 blink::NormalPageArena 00003362`a0044620
  8 blink::NormalPageArena 00003362`a0044700
  9 blink::NormalPageArena 00003362`a00447e0
 10 blink::NormalPageArena 00003362`a00448c0
 11 blink::NormalPageArena 00003362`a00449a0
 12 blink::NormalPageArena 00003362`a0044a80
 13 blink::LargeObjectArena 00003362`a004c000
free page pool entries:
  0 blink::PagePool::PoolEntry 00003362`a0548ac0 6
  1 blink::PagePool::PoolEntry 00003362`a0004260 4
  2 blink::PagePool::PoolEntry 00003362`a0549740 24
  3 blink::PagePool::PoolEntry 00003362`a0004090 2
  4 blink::PagePool::PoolEntry 00003362`a054a9b0 12
  5 blink::PagePool::PoolEntry 00003362`a0548550 8
  6 blink::PagePool::PoolEntry 00003362`a00044e0 8
  7 blink::PagePool::PoolEntry 00003362`a0549a40 9
  8 blink::PagePool::PoolEntry 00003362`a0549b00 9
  9 blink::PagePool::PoolEntry 00003362`a00046e0 9
 10 blink::PagePool::PoolEntry 00003362`a0548580 14
 11 blink::PagePool::PoolEntry 00003362`a00045a0 2
 12 blink::PagePool::PoolEntry 00003362`a0004840 6
region tree:
 0000306a`08a20000
  00002b5f`53140000
   00000da1`a87e0000
    00000ae2`89480000
    00001ad7`c6f40000
     00001a87`1a7c0000
      0000163f`48760000
     0000207d`7c6c0000
      00001bed`bc640000
      00002a07`54480000
   00002e40`ccf80000
  00003ecb`91560000
   000074cf`8e960000
    000074b4`7dbc0000
     000055ab`ded80000
      000049a6`bfb80000
       00004a5d`6d7a0000
      00006e26`41a80000
       00005989`e16c0000
       00006e97`2b760000
    000077b0`7c200000

0:025> !arena 00003362`a00449a0
blink::NormalPageArena 00003362`a00449a0
blink::ThreadHeap 00000215`81b7ab60 Arena#11
current_allocation_point       0000207d`7c79ce98
remaining_allocation_size      1080
last_remaining_allocation_size 5632
active pages:
blink::NormalPage 0000207d`7c781000 [0000207d`7c781838-0000207d`7c79f000] #
blink::NormalPage 0000207d`7c7a1000 [0000207d`7c7a1838-0000207d`7c7bf000]
blink::NormalPage 0000207d`7c7e1000 [0000207d`7c7e1838-0000207d`7c7ff000]
blink::NormalPage 0000207d`7c6c1000 [0000207d`7c6c1838-0000207d`7c6df000]
blink::NormalPage 0000207d`7c7c1000 [0000207d`7c7c1838-0000207d`7c7df000]
blink::NormalPage 0000207d`7c761000 [0000207d`7c761838-0000207d`7c77f000]
blink::NormalPage 0000207d`7c741000 [0000207d`7c741838-0000207d`7c75f000]
blink::NormalPage 0000207d`7c721000 [0000207d`7c721838-0000207d`7c73f000]
unswept pages:
free chunks:
  4 0000207d`7c77efe8 24
  5 0000207d`7c784498 40 0000207d`7c7b9970 48 0000207d`7c7b9778 40 0000207d`7c7b9568 40
    0000207d`7c7d6ae8 40
...
 12 0000207d`7c78f398 6320 0000207d`7c78cb48 6728 0000207d`7c7e6de8 5168 0000207d`7c7d3f18 7304
    0000207d`7c76fce0 7176 0000207d`7c7338f8 7112
0:025> !hpage 0000207d`7c781000
blink::NormalPage 0000207d`7c781000 [0000207d`7c781838-0000207d`7c79f000]
Arena#11 00003362`a00449a0
ThreadState 00007ff9`dfe2a050
0:025> !scan 0000207d`7c781838
   0 0000207d`7c781838          34    112
   1 0000207d`7c7818a8          34    112
   2 0000207d`7c781918          34    112
   3 0000207d`7c781988          34    112
   4 0000207d`7c7819f8          34    112
...
 644 0000207d`7c79dc10          34    104
 645 0000207d`7c79dc78          34    104
 646 0000207d`7c79dce0 F         0   1344
 647 0000207d`7c79e220          34    312
 648 0000207d`7c79e358 F         0   3240
```