# SWFRecomp

This is a stupid idea.

# Let's do it anyway.

SWFRecomp is largely inspired by Wiseguy's [N64Recomp](https://github.com/N64Recomp/N64Recomp), a static recompiler (not emulator) for N64 games that translates N64 MIPS instructions into C code that can be compiled into a native port. You should probably check that out, because it's epic. So is he. :D

This project aims to do the same thing, except with SWF files, i.e. Adobe Flash applications.

Fortunately, Adobe released most of their control over Flash as part of the [Open Screen Project](https://web.archive.org/web/20080506095459/http://www.adobe.com/aboutadobe/pressroom/pressreleases/200804/050108AdobeOSP.html), Adobe removed many restrictions from their SWF format, including restrictions on creating software that _plays and renders SWF files_:

> To support this mission, and as part of Adobe’s ongoing commitment to enable Web innovation, Adobe will continue to open access to Adobe Flash technology, accelerating the deployment of content and rich Internet applications (RIAs). This work will include:
> 
> - Removing restrictions on use of the SWF and FLV/F4V specifications
> - Publishing the device porting layer APIs for Adobe Flash Player
> - Publishing the Adobe Flash® Cast™ protocol and the AMF protocol for robust data services
> - Removing licensing fees - making next major releases of Adobe Flash Player and Adobe AIR for devices free

They even went as far as to [donate the Flex 3 SDK to Apache](https://www.pcworld.com/article/478324/adobe_donates_flex_to_apache-2.html) back in 2011, in favor of the wild uprising of HTML5. Apache Flex is now licensed under the Apache License (a permissive open-source license).

## So what can this do right now?

Currently it successfully decompresses all forms of SWF compression, reads the data from the header, and recompiles most graphics data defined in `DefineShape` tags. It also handles many ActionScript actions from SWF 4, and recompiles them into the equivalent C code.

# Special Thanks

All the people that wildly inspire me. 😋

My very dear friend BlazinWarior.

From N64Recomp:

- Wiseguy
- Darío
- dcvz
- Reonu
- thecozies
- danielryb
- kentonm

From Archipelago:

- TheRealPixelShake
- Muervo_
- seto10987
- Rogue
- ArsonAssassin
- CelestialKitsune
- Vincent'sSin
- LegendaryLinux
- zakwiz
- jjjj12212

From RotMG:

- HuskyJew
- Nequ
- snowden
- MoonMan
- Auru
