# ABC Parser Research and Implementation Options

**Document Version:** 1.0
**Date:** October 29, 2025
**Purpose:** Research findings on ABC file format specifications and open source implementation options

---

## Table of Contents

1. [Overview](#overview)
2. [Specification Verification](#specification-verification)
3. [Official Documentation Sources](#official-documentation-sources)
4. [ABC Format Specification Summary](#abc-format-specification-summary)
5. [Open Source Implementation Options](#open-source-implementation-options)
6. [Recommendations](#recommendations)
7. [References](#references)

---

## Overview

This document summarizes research conducted to verify the accuracy of the ABC_PARSER_GUIDE.md and to identify potential open source ABC parser implementations that could be used instead of implementing from scratch.

### Key Findings

1. **ABC_PARSER_GUIDE.md is highly accurate** - All details verified against official Adobe specifications
2. **Multiple open source implementations exist** - Ruffle (Rust), avmplus (C++), Lightspark (C++), RABCDAsm (D)
3. **License compatibility matters** - Only Ruffle and avmplus are compatible with SWFRecomp's MIT license
4. **Recommendation**: Use Ruffle as reference or extract from avmplus

---

## Specification Verification

### Verification Process

The ABC_PARSER_GUIDE.md was verified against the **official Adobe ABC file format specification** from the adobe-flash/avmplus repository:
- **Source:** `github.com/adobe-flash/avmplus/blob/master/doc/abcFormat-46-16.txt`
- **Version:** 46.16 (initial ABC format version)
- **Status:** Official Adobe documentation

### Verification Results

✅ **All core specifications verified as accurate:**

#### Version Numbers
- Major version: 46 ✅
- Minor version: 16 ✅
- Flash Player 9+ uses 46.16 ✅

#### Variable-Length Encoding (u30)
- 7 bits per byte with continuation bit ✅
- Up to 5 bytes maximum ✅
- High bit indicates more bytes follow ✅
- Implementation code is correct ✅

#### Constant Pool Index 0 Handling
- Index 0 represents special values (empty string, wildcard namespace, etc.) ✅
- Not stored in the file ✅
- Pool count includes implicit index 0 ✅

#### Namespace Kinds (All hex values match)
| Constant | Hex | Verified |
|----------|-----|----------|
| NAMESPACE | 0x08 | ✅ |
| PRIVATE_NS | 0x05 | ✅ |
| PACKAGE_NAMESPACE | 0x16 | ✅ |
| PACKAGE_INTERNAL_NS | 0x17 | ✅ |
| PROTECTED_NAMESPACE | 0x18 | ✅ |
| EXPLICIT_NAMESPACE | 0x19 | ✅ |
| STATIC_PROTECTED_NS | 0x1A | ✅ |

#### Multiname Kinds (All hex values match)
| Constant | Hex | Verified |
|----------|-----|----------|
| QNAME | 0x07 | ✅ |
| QNAME_A | 0x0D | ✅ |
| RTQNAME | 0x0F | ✅ |
| RTQNAME_A | 0x10 | ✅ |
| RTQNAME_L | 0x11 | ✅ |
| RTQNAME_LA | 0x12 | ✅ |
| MULTINAME | 0x09 | ✅ |
| MULTINAME_A | 0x0E | ✅ |
| MULTINAME_L | 0x1B | ✅ |

#### Trait Kinds
| Value | Type | Verified |
|-------|------|----------|
| 0 | Slot | ✅ |
| 1 | Method | ✅ |
| 2 | Getter | ✅ |
| 3 | Setter | ✅ |
| 4 | Class | ✅ |
| 5 | Function | ✅ |
| 6 | Const | ✅ |

#### Method Flags
| Flag | Hex | Verified |
|------|-----|----------|
| NEED_ARGUMENTS | 0x01 | ✅ |
| NEED_ACTIVATION | 0x02 | ✅ |
| NEED_REST | 0x04 | ✅ |
| HAS_OPTIONAL | 0x08 | ✅ |
| IGNORE_REST | 0x10 | ✅ |
| EXPLICIT | 0x20 | ✅ |
| SET_DXNS | 0x40 | ✅ |
| HAS_PARAM_NAMES | 0x80 | ✅ |

#### Class Flags
| Flag | Hex | Verified |
|------|-----|----------|
| CLASS_SEALED | 0x01 | ✅ |
| CLASS_FINAL | 0x02 | ✅ |
| CLASS_INTERFACE | 0x04 | ✅ |
| CLASS_PROTECTED_NS | 0x08 | ✅ |

#### ABC File Structure Order
The parsing order documented in ABC_PARSER_GUIDE.md matches the official specification exactly:

1. minor_version (U16) ✅
2. major_version (U16) ✅
3. constant_int_pool ✅
4. constant_uint_pool ✅
5. constant_double_pool ✅
6. constant_string_pool ✅
7. constant_namespace_pool ✅
8. constant_namespace_set_pool ✅
9. constant_multiname_pool ✅
10. methods (MethodInfo array) ✅
11. metadata (MetadataInfo array) ✅
12. instances (InstanceInfo array) ✅
13. classes (ClassInfo array) ✅
14. scripts (ScriptInfo array) ✅
15. method_bodies (MethodBodyInfo array) ✅

### Minor Observations

⚠️ **TYPENAME (0x1D)** - Documented in the guide but not in the official 46.16 spec. This multiname kind was added in later versions (Flash Player 10.3+, version 46.17 or 47.x) for generic type support like `Vector.<T>`. This is acceptable as the guide mentions support for newer Flash Player versions.

---

## Official Documentation Sources

### Primary Specification

**Adobe avmplus ABC Format Specification**
- **Location:** `github.com/adobe-flash/avmplus/blob/master/doc/abcFormat-46-16.txt`
- **Format:** Plain text
- **Status:** Official Adobe documentation
- **Version:** 46.16 (initial release)
- **Accessibility:** ✅ Available on GitHub

### Secondary Sources

**Adobe AVM2 Overview PDF**
- **Original location:** `www.adobe.com/go/avm2overview` (now broken)
- **Archived locations:**
  - Internet Archive
  - `github.com/fallending/swfplayer-x/blob/master/doc/avm2overview.pdf`
- **Status:** Official Adobe specification (archived)
- **Format:** PDF (binary, requires extraction for analysis)

**SWF File Format Specification Version 19**
- **Mirror:** `open-flash.github.io/mirrors/swf-spec-19.pdf`
- **Maintained by:** Open Flash project
- **Status:** Official Adobe specification (archived)
- **Content:** Includes DoABC/DoABC2 tag formats

**Mozilla Tamarin Project**
- **History:** Adobe open-sourced the ActionScript VM as Mozilla Tamarin
- **License:** MPL/GPL/LGPL tri-license
- **Status:** Historical, no longer actively maintained
- **Value:** Contains implementation documentation and source code

### Community Resources

**Open Flash Project**
- **Website:** `open-flash.github.io`
- **Purpose:** Maintains mirrors of Adobe Flash specifications
- **Status:** Active community preservation effort

**Stack Overflow Discussions**
- Various technical discussions about ABC format parsing
- Licensing questions regarding AVM2/ABC specifications
- Implementation guidance from community developers

---

## ABC Format Specification Summary

### File Structure

```
ABC File Structure:
├── Header (U16 minor_version, U16 major_version)
├── Constant Pools
│   ├── Integer pool (S32 variable-length encoded)
│   ├── Unsigned integer pool (U32 variable-length encoded)
│   ├── Double pool (U64 little-endian)
│   ├── String pool (UTF-8 with length prefix)
│   ├── Namespace pool
│   ├── Namespace set pool
│   └── Multiname pool
├── Method Info Array (method signatures)
├── Metadata Info Array (optional metadata)
├── Class Definitions
│   ├── Instance Info Array (instance members, inheritance)
│   └── Class Info Array (static members)
├── Script Info Array (initialization code)
└── Method Body Array (bytecode, exception handlers)
```

### Variable-Length Encoding

**U30 Encoding Format:**
- Each byte contributes 7 bits to the value
- High bit (bit 7) is continuation flag
- If high bit is set, read next byte
- Maximum 5 bytes
- Little-endian bit order
- Must have no nonzero bits above bit 30

**Example:**
```
0x00-0x7F:    1 byte  (0xxxxxxx)
0x80-0x3FFF:  2 bytes (1xxxxxxx 0xxxxxxx)
0x4000+:      3-5 bytes
```

### Critical Implementation Details

#### Index 0 Special Handling
- **All constant pools:** Index 0 is implicit and not stored in the file
- **String pool:** Index 0 = empty string `""`
- **Namespace pool:** Index 0 = any namespace (wildcard)
- **Namespace set pool:** Index 0 = empty set
- **Multiname pool:** Index 0 = any name (wildcard)
- **Pool count:** If count is `n`, file contains `n-1` entries (indices 1 through n-1)

#### String Format
```
String entry:
├── Length (U30) - number of UTF-8 bytes
└── Data (UTF-8 bytes, not null-terminated)
```

#### Namespace Format
```
Namespace entry:
├── Kind (U8) - namespace type constant
└── Name index (U30) - index into string pool
```

#### Multiname Complexity
Multinames represent qualified names with namespace information. Different kinds have different structures:

- **QName:** namespace index + name index
- **Multiname:** name index + namespace set index
- **RTQName:** Runtime qualified name (name index only)
- **RTQNameL:** Late-bound runtime qualified name (no data)
- **TypeName:** Generic type with type parameters (e.g., Vector.<T>)

#### Method Signature
```
MethodInfo:
├── param_count (U30)
├── return_type (U30) - multiname index
├── param_types (U30[param_count]) - multiname indices
├── name_index (U30) - debug info
├── flags (U8)
├── [optional] options (OptionDetail[])
└── [optional] param_names (U30[])
```

#### Method Body
```
MethodBodyInfo:
├── method_index (U30) - which method this is for
├── max_stack (U30) - maximum stack depth
├── max_regs (U30) - number of local registers
├── scope_depth (U30) - initial scope depth
├── max_scope_depth (U30) - maximum scope depth
├── code_length (U30)
├── code (U8[code_length]) - bytecode instructions
├── exception_count (U30)
├── exceptions (ExceptionInfo[])
├── trait_count (U30)
└── traits (Trait[])
```

#### Trait Structure
```
Trait:
├── name_index (U30) - multiname index
├── kind (U8) - low 4 bits = kind, high 4 bits = attributes
└── [kind-specific data]
    ├── Slot/Const: slot_id, type_name, vindex, vkind
    ├── Method/Getter/Setter: disp_id, method_index
    ├── Class: slot_id, class_index
    └── Function: slot_id, method_index
```

---

## Open Source Implementation Options

### 1. Ruffle (Rust)

**Project:** Flash Player emulator written in Rust
**Repository:** `github.com/ruffle-rs/ruffle`
**ABC Parser Location:** `swf/src/avm2/read.rs`

#### License
- **Type:** Dual-licensed Apache 2.0 / MIT
- **Compatibility:** ✅ **Fully compatible with MIT** (SWFRecomp's license)
- **Restrictions:** None for derivative works
- **Can use in SWFRecomp:** Yes, with attribution

#### Implementation Details
- **Language:** Rust
- **Status:** Actively maintained (2025)
- **Maturity:** Production-ready, handles real-world SWF files
- **Architecture:** Clean, well-structured reader implementation
- **Coupling:** Moderately coupled - designed as part of SWF crate but relatively standalone
- **Memory safety:** Guaranteed by Rust language

#### Code Quality
- Modern, idiomatic Rust
- Clear separation of concerns
- Comprehensive structure parsing
- Well-documented

#### Structures Parsed
- ✅ All constant pools (int, uint, double, string, namespace, namespace set, multiname)
- ✅ Method definitions with full signature support
- ✅ Class and instance information
- ✅ Traits (all types: slot, method, getter, setter, class, function, const)
- ✅ Scripts
- ✅ Method bodies with bytecode and exception handlers
- ✅ Metadata

#### ActionScript Support
- AS1: 99% language, 79% API
- AS2: 99% language, 79% API
- AS3: 90% language, 77% API implemented, 9% partially implemented

#### Integration Options

**Option A: Use Rust directly**
```cmake
# Add Rust to CMake build
include(FetchContent)
FetchContent_Declare(
    ruffle_swf
    GIT_REPOSITORY https://github.com/ruffle-rs/ruffle.git
)
```

**Option B: Translate to C++**
- Rust parsing code is straightforward
- No complex Rust-specific features in ABC reader
- Direct translation to C++ would be manageable
- Can verify correctness against Ruffle tests

**Option C: Use as reference**
- Implement ABC parser from scratch in C++
- Use Ruffle's implementation to verify correctness
- Learn from their design decisions

#### Pros
- ✅ Best license compatibility (MIT/Apache)
- ✅ Modern, actively maintained
- ✅ Clean, understandable code
- ✅ Production-tested
- ✅ Memory-safe by design

#### Cons
- ⚠️ Written in Rust (requires translation or Rust integration)
- ⚠️ Part of larger SWF parsing crate (some extraction needed)

---

### 2. Adobe avmplus (C++)

**Project:** Official ActionScript Virtual Machine
**Repository:** `github.com/adobe/avmplus` and `github.com/adobe-flash/avmplus`
**ABC Parser Location:** `core/` directory, multiple files

#### License
- **Type:** Mozilla Public License 2.0 (MPL 2.0)
- **Tri-license option:** MPL / GPL / LGPL
- **Compatibility:** ⚠️ **Compatible with MIT but with requirements**
- **Restrictions:** Must document modifications, provide source code for MPL portions
- **Can use in SWFRecomp:** Yes, but MPL code must remain under MPL

#### MPL 2.0 Key Points
- Permits derivative works ✅
- Allows mixing with MIT code ✅
- File-level copyleft (not project-level like GPL)
- Modified MPL files must stay under MPL
- Can combine with proprietary code
- Patent grant included

#### Implementation Details
- **Language:** C++
- **Status:** No longer actively maintained (legacy)
- **Maturity:** Production-ready, used in Flash Player and Adobe AIR
- **Architecture:** Complex, integrated with full VM runtime
- **Coupling:** Very tightly coupled to execution engine
- **Size:** Large, heavyweight codebase

#### Code Characteristics
- Production C++ from Adobe
- Authoritative reference implementation
- Complex memory management
- Designed for runtime execution, not build-time parsing
- Includes JIT compilation, garbage collection, etc.

#### Relevant Files
- `core/AvmCore.cpp` - Core VM functionality
- `core/Interpreter.cpp` - Bytecode interpreter
- Various ABC parsing functions throughout codebase
- `aot-compiler/compileabc.cpp` - AOT compilation tools

#### Integration Options

**Option A: Extract parsing code**
- Identify minimal ABC parsing functions
- Extract to standalone module
- Keep under MPL 2.0
- Link with MIT-licensed SWFRecomp

**Option B: Use as reference**
- Study implementation approaches
- Learn from Adobe's decisions
- Implement from scratch in MIT-licensed code
- No licensing concerns

#### Pros
- ✅ Official Adobe implementation (authoritative)
- ✅ Production-tested at massive scale
- ✅ Native C++ (same as SWFRecomp)
- ✅ Complete documentation
- ✅ Well-understood by community

#### Cons
- ⚠️ MPL 2.0 requires tracking modified files
- ⚠️ Very complex, hard to extract parsing code
- ⚠️ Designed for runtime VM, not build-time parsing
- ⚠️ No longer maintained
- ⚠️ Large dependency footprint

---

### 3. Lightspark (C++)

**Project:** Open source Flash player for Linux
**Repository:** `github.com/lightspark/lightspark`
**ABC Parser Location:** `src/scripting/abc.cpp`, `src/scripting/abc_opcodes.cpp`

#### License
- **Type:** LGPL 3.0 / GPL 3.0 dual-license
- **Compatibility:** ❌ **NOT compatible with MIT for static linking**
- **Restrictions:** LGPL requires dynamic linking or full source release
- **Can use in SWFRecomp:** Only if kept as separate shared library

#### Implementation Details
- **Language:** C++ (46.1%), ActionScript (40.1%), C (8.3%)
- **Status:** Actively maintained (alpha state)
- **Maturity:** 88% of Flash APIs implemented
- **Architecture:** Integrated Flash player with rendering pipeline
- **Coupling:** Very tightly coupled to Lightspark runtime

#### Code Characteristics
- Modern C++ implementation
- Complete ABC parser and VM
- Handles AVM1 (AS1/AS2) and AVM2 (AS3)
- Complex memory management and threading
- Deep integration with display system

#### ABC Parser Features
- Complete constant pool parsing
- Full multiname and namespace support
- Class and trait parsing
- Method body and bytecode handling
- Event system integration
- LLVM compilation support

#### Integration Challenges
- GPL/LGPL licensing conflicts with MIT
- Extremely coupled to runtime environment
- Custom memory allocators
- Threading and synchronization dependencies
- Display system dependencies

#### Pros
- ✅ Native C++ implementation
- ✅ Actively maintained
- ✅ Complete ABC parser
- ✅ Production-quality code

#### Cons
- ❌ GPL/LGPL license incompatible with MIT
- ❌ Cannot static link without GPL contamination
- ❌ Very difficult to extract parser code
- ❌ Heavy runtime dependencies

#### License Implications
- **Static linking:** Would require entire SWFRecomp to become GPL
- **Dynamic linking:** Would require distributing Lightspark as separate .so/.dll
- **Code copying:** Would require GPL for copied portions
- **Reference only:** Acceptable if reimplemented cleanly

---

### 4. RABCDAsm (D Language)

**Project:** ActionScript 3 assembler/disassembler
**Repository:** `github.com/CyberShadow/RABCDAsm`

#### License
- **Type:** GPL v3 or later
- **Compatibility:** ❌ **NOT compatible with MIT**
- **Restrictions:** Viral - requires entire project to become GPL
- **Can use in SWFRecomp:** No, unless kept completely separate

#### Implementation Details
- **Language:** D programming language (D v2)
- **Status:** Mature, stable
- **Purpose:** ABC manipulation tools (disassembler, assembler)
- **Architecture:** Designed for ABC file manipulation, not execution

#### Tools Included
- `rabcdasm` - ABC disassembler
- `rabcasm` - ABC assembler
- `abcexport` - Extract ABC from SWF
- `abcreplace` - Replace ABC in SWF
- `swfdecompress` - Decompress SWF files

#### Code Characteristics
- Clean separation between parsing and execution
- Uses pointers instead of indices for easy manipulation
- Automatic constant pool management
- Well-designed for programmatic ABC manipulation

#### Integration Options

**Option A: External tool**
- Use as command-line tool in build process
- No code integration, no license issues
- Call from CMake or shell scripts

**Option B: GPL the project**
- Not feasible - SWFRecomp is MIT licensed

**Option C: Reference only**
- Study implementation
- Reimplement in C++ under MIT

#### Pros
- ✅ Specifically designed for ABC manipulation
- ✅ Clean, focused implementation
- ✅ Good for understanding ABC structure
- ✅ Mature and stable

#### Cons
- ❌ GPL license incompatible with MIT
- ❌ D language, not C++
- ❌ Would require D compiler in build chain
- ❌ Cannot integrate code directly

---

### 5. Other Implementations

#### python-avm2
- **Language:** Python
- **Repository:** `github.com/eigenein/python-avm2`
- **License:** Not verified
- **Use case:** Reference implementation, not suitable for integration

#### swiffas
- **Language:** Python
- **Repository:** `github.com/ahixon/swiffas`
- **Description:** SWF parser and AVM2 bytecode parser
- **Use case:** Reference only

#### as3-commons-bytecode
- **Language:** ActionScript 3
- **Description:** ABC manipulation library written in AS3
- **Use case:** Historical reference

---

## Recommendations

### Primary Recommendation: Use Ruffle as Reference

**Approach:** Translate Ruffle's ABC parser from Rust to C++

**Rationale:**
1. ✅ **License:** MIT/Apache is fully compatible with SWFRecomp's MIT license
2. ✅ **Code quality:** Modern, clean, well-structured implementation
3. ✅ **Proven:** Production-tested on real-world SWF files
4. ✅ **Maintainability:** Active development, bug fixes, improvements
5. ✅ **Simplicity:** Parser code is relatively straightforward
6. ✅ **Safety:** Rust's memory safety translates to safer C++ design

**Implementation Strategy:**
1. Study Ruffle's `swf/src/avm2/read.rs` implementation
2. Create C++ equivalents of Rust structures
3. Translate parsing logic method-by-method
4. Add unit tests based on Ruffle's test cases
5. Verify against ABC_PARSER_GUIDE.md specification
6. Test with real SWF files (Seedling.swf)

**Effort estimate:** 1-2 weeks for experienced C++ developer

**Code structure:**
```cpp
// Ruffle-inspired C++ structure
src/abc/
├── abc_reader.h         // Low-level binary reading
├── abc_reader.cpp
├── abc_types.h          // ABC data structures
├── abc_parser.h         // High-level parser interface
├── abc_parser.cpp
└── abc_test.cpp         // Unit tests
```

---

### Alternative Recommendation: Extract from avmplus

**Approach:** Use Adobe's official implementation as reference

**Rationale:**
1. ✅ **Authoritative:** Official Adobe implementation
2. ✅ **Native C++:** No translation needed
3. ✅ **License:** MPL 2.0 allows mixing with MIT
4. ⚠️ **Complexity:** Harder to extract clean parsing code
5. ⚠️ **Tracking:** Must keep MPL code separate

**Implementation Strategy:**
1. Identify core parsing functions in avmplus
2. Extract minimal necessary code
3. Keep extracted code in separate files under MPL 2.0
4. Document which files are MPL vs MIT
5. Create clean interface between MPL and MIT code

**Effort estimate:** 2-4 weeks (extraction complexity)

**License management:**
```
src/abc/
├── LICENSE.MIT          # SWFRecomp MIT license
├── LICENSE.MPL          # avmplus MPL 2.0 license
├── abc_parser.h         # MIT - your interface
├── abc_parser.cpp       # MIT - your implementation
└── avmplus/             # MPL 2.0 - extracted code
    ├── README.md        # Notes on avmplus extraction
    └── *.cpp/h          # MPL-licensed files
```

---

### Hybrid Recommendation: Reference Multiple Implementations

**Approach:** Use both Ruffle and avmplus as references, implement from scratch

**Rationale:**
1. ✅ **Clean MIT license:** No external code, no complications
2. ✅ **Learn from best:** Study both modern (Ruffle) and authoritative (avmplus)
3. ✅ **Verification:** Cross-check against official spec and two implementations
4. ✅ **Optimization:** Design specifically for build-time parsing
5. ⚠️ **Time:** More initial development time

**Implementation Strategy:**
1. Follow ABC_PARSER_GUIDE.md as primary guide
2. Reference Ruffle for modern design patterns
3. Reference avmplus for edge cases and Adobe decisions
4. Implement test-first based on official spec
5. Verify against both implementations

**Effort estimate:** 2-3 weeks (fresh implementation)

---

### NOT Recommended

❌ **Do not use Lightspark:** GPL/LGPL licensing incompatible with MIT
❌ **Do not use RABCDAsm:** GPL licensing incompatible, different language
❌ **Do not integrate as libraries:** Complexity and coupling too high

---

## Summary Comparison Table

| Implementation | Language | License | MIT Compatible | Complexity | Maintenance | Recommendation |
|----------------|----------|---------|----------------|------------|-------------|----------------|
| **Ruffle** | Rust | Apache/MIT | ✅ Yes | Medium | Active | ⭐ **Best choice** |
| **avmplus** | C++ | MPL 2.0 | ⚠️ Yes* | Very High | Inactive | Reference |
| **Lightspark** | C++ | GPL/LGPL | ❌ No | Very High | Active | Avoid |
| **RABCDAsm** | D | GPL v3+ | ❌ No | Medium | Stable | Avoid |
| **Custom** | C++ | MIT | ✅ Yes | Medium | You | Alternative |

*MPL 2.0 compatible but requires file-level license tracking

---

## Implementation Roadmap

### Phase 1: Research and Planning (Completed ✅)
- [x] Verify ABC_PARSER_GUIDE.md against official specifications
- [x] Survey open source implementations
- [x] Evaluate license compatibility
- [x] Select implementation approach
- [x] Document findings

### Phase 2: Setup (1-2 days)
- [ ] Create ABC parser project structure
- [ ] Set up unit test framework
- [ ] Define data structures (ABCFile, MethodInfo, etc.)
- [ ] Implement low-level binary reader utilities

### Phase 3: Core Parsing (1 week)
- [ ] Implement u30 variable-length encoding
- [ ] Parse ABC header
- [ ] Parse constant pools (int, uint, double, string)
- [ ] Parse namespace pool
- [ ] Parse namespace set pool
- [ ] Parse multiname pool
- [ ] Add unit tests for each pool type

### Phase 4: Method and Class Parsing (3-4 days)
- [ ] Parse method info array
- [ ] Parse metadata array
- [ ] Parse instance info
- [ ] Parse class info
- [ ] Parse traits
- [ ] Add validation

### Phase 5: Script and Body Parsing (3-4 days)
- [ ] Parse script info
- [ ] Parse method bodies
- [ ] Parse exception handlers
- [ ] Parse bytecode blocks
- [ ] Add comprehensive tests

### Phase 6: Integration and Testing (3-4 days)
- [ ] Integrate with SWF tag parsing
- [ ] Extract ABC from DoABC tags
- [ ] Test with simple SWF files
- [ ] Test with Seedling.swf
- [ ] Add error handling and validation
- [ ] Create debug output utilities

### Phase 7: Documentation (1-2 days)
- [ ] Document API usage
- [ ] Add code examples
- [ ] Create troubleshooting guide
- [ ] Update project documentation

**Total Estimated Time:** 2-3 weeks for complete implementation

---

## References

### Official Specifications
1. **Adobe ABC Format 46.16 Specification**
   `github.com/adobe-flash/avmplus/blob/master/doc/abcFormat-46-16.txt`

2. **Adobe AVM2 Overview** (archived)
   `github.com/fallending/swfplayer-x/blob/master/doc/avm2overview.pdf`

3. **SWF File Format Specification Version 19**
   `open-flash.github.io/mirrors/swf-spec-19.pdf`

### Open Source Implementations
1. **Ruffle - Flash Player Emulator (Rust)**
   `github.com/ruffle-rs/ruffle`
   License: Apache 2.0 / MIT

2. **Adobe avmplus - ActionScript VM (C++)**
   `github.com/adobe/avmplus`
   License: MPL 2.0

3. **Lightspark - Flash Player (C++)**
   `github.com/lightspark/lightspark`
   License: LGPL 3.0 / GPL 3.0

4. **RABCDAsm - ABC Assembler/Disassembler (D)**
   `github.com/CyberShadow/RABCDAsm`
   License: GPL v3+

### Community Resources
1. **Open Flash Project**
   `open-flash.github.io`

2. **Mozilla Tamarin Wiki**
   `wiki.mozilla.org/Tamarin`

3. **Stack Overflow - AVM2/ABC Questions**
   Multiple discussions on ABC format and implementation

### Internal Documentation
1. **ABC_PARSER_GUIDE.md** - Detailed implementation guide (verified accurate)
2. **AS3_IMPLEMENTATION_PLAN.md** - Overall AS3 support plan
3. **ABC_PARSER_RESEARCH.md** - This document

---

## Conclusion

The ABC file format is well-documented and has multiple high-quality open source implementations available. The ABC_PARSER_GUIDE.md is highly accurate and can be trusted as a reference for implementation.

**Recommended approach:** Use Ruffle's MIT/Apache-licensed Rust implementation as a reference to create a C++ ABC parser for SWFRecomp. This provides the best balance of license compatibility, code quality, and implementation simplicity.

The parser can be implemented in 2-3 weeks and will serve as the foundation for Phase 2 (code generation) of the AS3 support project.

---

**Document prepared:** October 29, 2025
**Verification sources:** Adobe official specifications, multiple open source implementations
**Status:** Ready for implementation
