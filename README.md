# !CR

The *chromium-debug* is a set of debugger extensions to help debugging Chromium on Windows.

Happy debugging! :sunglasses:

Please note that this extension highly depends on the data structure of Chromium core (chrome_child.dll in Google Chrome) and does not keep backward compatibility.  Some commands may not work with an older Chrome/Chromium version.

## Command examples

### DOM & Layout tree

```
0:000> .load cr
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
0:017> !page 00000bad`97401e80
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
