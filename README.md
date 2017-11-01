# !CR

The *chromium-debug* is a set of debugger extensions to help debugging Chromium on Windows.

Happy debugging! :sunglasses:

### Command examples

```
0:000> !cr.dom @rcx
Target Engine = 61.3163
    0 0000030a`f73625c0 blink::HTMLDocument 00001405
    1  0000030a`f7363210 blink::DocumentType 000c1400
    2  0000030a`f7363270 blink::HTMLHtmlElement 0000161c
    3   0000030a`f73632d8 blink::HTMLHeadElement 0000141c
    4    0000030a`f7363340 blink::Text 00001402
    5    0000030a`f7363390 blink::HTMLMetaElement 0000141c
    6    0000030a`f73633f8 blink::Text 00001402
    7    0000030a`f7363448 blink::HTMLStyleElement 0000141c
    8     0000030a`f73634d8 blink::Text 00001402
    9    0000030a`f7363528 blink::Text 00001402
   10    0000030a`f7363578 blink::HTMLScriptElement 0000141c
   11     0000030a`f7363600 blink::Text 00001402
   12    0000030a`f7363650 blink::Text 00001402
```
