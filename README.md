# !CR

The *chromium-debug* is a set of debugger extensions to help debugging Chromium on Windows.

Happy debugging! :sunglasses:

### Command examples

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
