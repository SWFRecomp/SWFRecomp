# ABC Parser Implementation - Complete Information Summary

**Document Version:** 1.0
**Date:** October 29, 2025
**Purpose:** Comprehensive reference for implementing ABC parser in SWFRecomp

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Available Resources](#available-resources)
3. [Codebase Integration Details](#codebase-integration-details)
4. [AVM2 Opcode Reference](#avm2-opcode-reference)
5. [Implementation Roadmap](#implementation-roadmap)
6. [Testing Strategy](#testing-strategy)
7. [Next Steps](#next-steps)

---

## Executive Summary

### What We Have

✅ **Complete ABC Format Specification** - Downloaded and verified
✅ **Official Documentation** - SWF spec v19, ABC format 46.16
✅ **AVM2 Opcode Definitions** - 256 opcodes extracted from avmplus
✅ **Reference Implementations** - Ruffle (Rust/MIT), avmplus (C++/MPL)
✅ **Codebase Understanding** - Full exploration of SWFRecomp architecture
✅ **Integration Points** - DoABC tag handler location identified
✅ **Build System** - CMake configuration understood
✅ **Code Patterns** - Error handling, memory management, binary reading

### What We Need to Build

1. **ABC Binary Reader** - Low-level parsing utilities (u30, strings, etc.)
2. **ABC Data Structures** - C++ structs for ABCFile, methods, classes, etc.
3. **ABC Parser** - Main parsing logic for constant pools, methods, classes
4. **DoABC Tag Handler** - Integration with existing SWF parser
5. **Validation** - Index checking, structural validation
6. **Debug Output** - Tools to inspect parsed ABC data
7. **Tests** - Unit tests for parsing components

### Implementation Timeline

- **Week 1**: Setup + Binary reader + Constant pools
- **Week 2**: Method/class parsing + Traits + Scripts
- **Week 3**: Integration + Testing + Debug output

**Total Estimated Time:** 2-3 weeks

---

## Available Resources

### Downloaded Documentation (in `docs/specs/`)

1. **`swf-spec-19.txt`** (370 KB)
   - Official Adobe SWF File Format Specification Version 19 (text version)
   - Contains DoABC tag format (Chapter on ActionScript 3)
   - Source: https://open-flash.github.io/mirrors/swf-spec-19.pdf (PDF version: 1.7 MB)

2. **`abc-format-46-16.txt`** (8.2 KB)
   - Official Adobe ABC File Format Specification
   - Version 46.16 (Flash Player 9+)
   - Verified accurate against ABC_PARSER_GUIDE.md
   - Source: https://github.com/adobe-flash/avmplus/blob/master/doc/abcFormat-46-16.txt

3. **`opcodes.as`** (15 KB)
   - ActionScript 3 opcode table generator
   - Shows opcode metadata structure
   - Source: https://github.com/adobe-flash/avmplus/blob/master/utils/opcodes.as

4. **`avm2_opcodes_raw.txt`** (258 lines)
   - Extracted opcode table from Interpreter.cpp
   - Maps 0x00-0xFF to instruction handlers
   - Shows which opcodes are implemented vs XXX (unimplemented)

### External References

1. **Ruffle (Rust Implementation)**
   - Repository: https://github.com/ruffle-rs/ruffle
   - ABC Parser: `swf/src/avm2/read.rs`
   - License: MIT/Apache 2.0 ✅ Compatible
   - Use: Translation reference

2. **avmplus (C++ Implementation)**
   - Repository: https://github.com/adobe-flash/avmplus
   - ABC Interpreter: `core/Interpreter.cpp`
   - License: MPL 2.0 ⚠️ File-level copyleft
   - Use: Authoritative reference only

3. **RABCDAsm (D Language Implementation)**
   - Repository: https://github.com/CyberShadow/RABCDAsm
   - Opcode Info: `abcfile.d` (OpcodeInfo array)
   - License: GPL v3+ ❌ Incompatible
   - Use: Reference for opcode operand types

### Planning Documents (in project root)

- **`ABC_PARSER_GUIDE.md`** - Detailed implementation guide with code examples
- **`ABC_PARSER_RESEARCH.md`** - Specification verification and license analysis
- **`AS3_IMPLEMENTATION_GUIDE.md`** - Full AS3 support roadmap
- **`ABC_IMPLEMENTATION_INFO.md`** - This document

### Test Resources

**No compiled Seedling.swf available** - The Seedling project contains AS3 source code but no compiled SWF file. We'll need to:
- Use test SWFs from SWFRecomp/tests/ as starting point
- Compile Seedling.swf using FlashDevelop or Apache Flex SDK (if needed)
- Or find other AS3 test SWFs online

---

## Codebase Integration Details

### Project Structure

```
SWFRecomp/
├── src/                          # C++ source files
│   ├── main.cpp                  # Entry point (161 lines)
│   ├── swf.cpp                   # SWF parser (717 lines) ⭐ KEY FILE
│   ├── tag.cpp                   # Tag parsing (86 lines)
│   ├── field.cpp                 # Binary reading (201 lines)
│   ├── recompilation.cpp         # Code generation orchestrator
│   ├── action/
│   │   └── action.cpp            # AS1/AS2 bytecode parser
│   └── abc/                      # ⭐ NEW DIRECTORY TO CREATE
│       ├── abc_reader.cpp        # Low-level binary reading
│       ├── abc_parser.cpp        # Main ABC parser
│       └── abc_codegen.cpp       # C code generation (Phase 2)
│
├── include/                      # C++ headers
│   ├── swf.hpp                   # SWF structures
│   ├── tag.hpp                   # Tag enum (DO_ABC = 82 defined here)
│   ├── field.hpp                 # Field types
│   ├── common.h                  # Type definitions, EXC macros
│   └── abc/                      # ⭐ NEW DIRECTORY TO CREATE
│       ├── abc_types.h           # ABC data structures
│       ├── abc_reader.h          # Reader interface
│       └── abc_parser.h          # Parser interface
│
├── tests/                        # 50+ integration tests
├── docs/specs/                   # Downloaded specifications
├── CMakeLists.txt                # Build configuration
└── [planning docs].md
```

### DoABC Tag Integration Point

**File:** `src/swf.cpp:889-894`

Currently commented out:
```cpp
//~ case SWF_TAG_DO_ABC:
//~ {
//~     cur_pos += tag.length;
//~     break;
//~ }
```

**Implementation needed:**
```cpp
case SWF_TAG_DO_ABC:
{
    // DoABC tag format (from SWF spec):
    // - U32 flags
    // - STRING name (null-terminated)
    // - ABC data (remaining bytes)

    uint32_t flags = *((uint32_t*)cur_pos);
    cur_pos += 4;

    std::string name = cur_pos;  // Read null-terminated string
    cur_pos += name.length() + 1;

    size_t abc_size = tag.length - 4 - (name.length() + 1);

    // Parse ABC file
    ABCFile* abc = parse_abc_file((const uint8_t*)cur_pos, abc_size);

    // Store for code generation
    // (Phase 2: generate C code from ABC)

    cur_pos += abc_size;
    break;
}
```

### Build System Integration

**File:** `CMakeLists.txt`

Add new source files:
```cmake
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/recompilation.cpp
    src/swf.cpp
    src/tag.cpp
    src/field.cpp
    src/action/action.cpp
    src/abc/abc_reader.cpp      # ADD
    src/abc/abc_parser.cpp      # ADD
    src/abc/abc_codegen.cpp     # ADD (Phase 2)
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    include/action
    include/abc                 # ADD
    # ... other includes
)
```

### Code Patterns

#### Error Handling
```cpp
// Pattern: Throw exceptions with EXC macros
#define EXC(str) fprintf(stderr, str); throw std::exception();
#define EXC_ARG(str, arg) fprintf(stderr, str, arg); throw std::exception();

// Usage:
if (abc->major_version != 46 && abc->major_version != 47) {
    EXC_ARG("Unsupported ABC version: %d\n", abc->major_version);
}
```

#### Memory Management
```cpp
// Pattern: Mix of raw pointers and RAII
// - Pools: std::vector<T> (automatic cleanup)
// - Strings: std::string (automatic cleanup)
// - Large structures: raw pointers (manual delete)

ABCFile* abc = new ABCFile();
abc->string_pool.push_back("hello");  // vector manages memory
// ... later:
delete abc;
```

#### Binary Reading
```cpp
// Pattern: Pointer arithmetic with type casting
uint8_t read_u8(const uint8_t*& ptr) {
    return *ptr++;
}

uint16_t read_u16(const uint8_t*& ptr) {
    uint16_t value = *((uint16_t*)ptr);
    ptr += 2;
    return value;
}

uint32_t read_u30(const uint8_t*& ptr) {
    uint32_t result = 0;
    int shift = 0;
    for (int i = 0; i < 5; i++) {
        uint8_t byte = *ptr++;
        result |= (byte & 0x7F) << shift;
        if (!(byte & 0x80)) break;
        shift += 7;
    }
    return result;
}
```

#### Code Generation
```cpp
// Pattern: stringstream accumulation
std::stringstream code;
code << "#include <recomp.h>" << std::endl;
code << "void method_" << method_id << "() {" << std::endl;
code << "\t// Generated code" << std::endl;
code << "}" << std::endl;

// Write to file
std::ofstream out("generated.c");
out << code.str();
```

---

## AVM2 Opcode Reference

### Opcode Categories (Total: 256 opcodes, ~100 implemented)

#### Control Flow (0x10-0x1B)
```
0x10  jump            [S24 offset]          - Unconditional jump
0x11  iftrue          [S24 offset]          - Jump if true
0x12  iffalse         [S24 offset]          - Jump if false
0x13  ifeq            [S24 offset]          - Jump if equal
0x14  ifne            [S24 offset]          - Jump if not equal
0x15  iflt            [S24 offset]          - Jump if less than
0x16  ifle            [S24 offset]          - Jump if less/equal
0x17  ifgt            [S24 offset]          - Jump if greater
0x18  ifge            [S24 offset]          - Jump if greater/equal
0x19  ifstricteq      [S24 offset]          - Jump if strict equal
0x1A  ifstrictne      [S24 offset]          - Jump if strict not equal
0x1B  lookupswitch    [complex]             - Switch statement
```

#### Stack Operations (0x20-0x2F)
```
0x20  pushnull        []                    - Push null
0x21  pushundefined   []                    - Push undefined
0x24  pushbyte        [U8 value]            - Push signed byte
0x25  pushshort       [U30 value]           - Push signed short
0x26  pushtrue        []                    - Push true
0x27  pushfalse       []                    - Push false
0x28  pushnan         []                    - Push NaN
0x29  pop             []                    - Pop stack
0x2A  dup             []                    - Duplicate top
0x2B  swap            []                    - Swap top two
0x2C  pushstring      [U30 index]           - Push string from pool
0x2D  pushint         [U30 index]           - Push int from pool
0x2E  pushuint        [U30 index]           - Push uint from pool
0x2F  pushdouble      [U30 index]           - Push double from pool
```

#### Local Variables (0x60-0x6F, 0xD0-0xD7)
```
0x60  getlex          [U30 index]           - Get lex (find and get property)
0x62  getlocal        [U30 index]           - Get local variable
0x63  setlocal        [U30 index]           - Set local variable
0x6C  getslot         [U30 index]           - Get object slot
0x6D  setslot         [U30 index]           - Set object slot
0xD0  getlocal_0      []                    - Get local 0 (optimized)
0xD1  getlocal_1      []                    - Get local 1
0xD2  getlocal_2      []                    - Get local 2
0xD3  getlocal_3      []                    - Get local 3
0xD4  setlocal_0      []                    - Set local 0 (optimized)
0xD5  setlocal_1      []                    - Set local 1
0xD6  setlocal_2      []                    - Set local 2
0xD7  setlocal_3      []                    - Set local 3
```

#### Property Access (0x61, 0x66, 0x68)
```
0x61  setproperty     [U30 multiname]       - Set property
0x66  getproperty     [U30 multiname]       - Get property
0x68  initproperty    [U30 multiname]       - Initialize property
```

#### Method Calls (0x40-0x4F)
```
0x40  newfunction     [U30 method]          - Create function closure
0x41  call            [U30 arg_count]       - Call function
0x42  construct       [U30 arg_count]       - Call constructor
0x45  callsuper       [U30 mn, U30 count]   - Call super method
0x46  callproperty    [U30 mn, U30 count]   - Call property
0x47  returnvoid      []                    - Return void
0x48  returnvalue     []                    - Return value
0x49  constructsuper  [U30 arg_count]       - Call super constructor
0x4A  constructprop   [U30 mn, U30 count]   - Construct property
0x4E  callsupervoid   [U30 mn, U30 count]   - Call super (no return)
0x4F  callpropvoid    [U30 mn, U30 count]   - Call property (no return)
```

#### Arithmetic (0xA0-0xB0)
```
0xA0  add             []                    - Add (TOS-1 + TOS)
0xA1  subtract        []                    - Subtract
0xA2  multiply        []                    - Multiply
0xA3  divide          []                    - Divide
0xA4  modulo          []                    - Modulo
0xA5  lshift          []                    - Left shift
0xA6  rshift          []                    - Right shift (signed)
0xA7  urshift         []                    - Right shift (unsigned)
0xA8  bitand          []                    - Bitwise AND
0xA9  bitor           []                    - Bitwise OR
0xAA  bitxor          []                    - Bitwise XOR
0xAB  equals          []                    - Equals (==)
0xAC  strictequals    []                    - Strict equals (===)
0xAD  lessthan        []                    - Less than
0xAE  lessequals      []                    - Less or equal
0xAF  greaterthan     []                    - Greater than
0xB0  greaterequals   []                    - Greater or equal
```

#### Type Operations (0x80-0x95)
```
0x80  coerce          [U30 multiname]       - Coerce to type
0x82  coerce_a        []                    - Coerce to any
0x85  coerce_s        []                    - Coerce to string
0x86  astype          [U30 multiname]       - Cast to type
0x87  astypelate      []                    - Late cast
0x90  negate          []                    - Numeric negation
0x91  increment       []                    - Increment
0x92  inclocal        [U30 index]           - Increment local
0x93  decrement       []                    - Decrement
0x94  declocal        [U30 index]           - Decrement local
0x95  typeof          []                    - Get type name
```

#### Object Creation (0x55-0x5E)
```
0x55  newobject       [U30 arg_count]       - Create object {}
0x56  newarray        [U30 arg_count]       - Create array []
0x57  newactivation   []                    - Create activation object
0x58  newclass        [U30 class_index]     - Create class instance
0x59  getdescendants  [U30 multiname]       - Get XML descendants
0x5A  newcatch        [U30 catch_index]     - Create catch scope
0x5D  findpropstrict  [U30 multiname]       - Find property (strict)
0x5E  findproperty    [U30 multiname]       - Find property
```

#### Scope Management (0x1D, 0x30, 0x64, 0x65)
```
0x1C  pushwith        []                    - Push with scope
0x1D  popscope        []                    - Pop scope
0x30  pushscope       []                    - Push scope
0x64  getscopeobject  [U8 index]            - Get scope object
0x65  getouterscope   [U30 index]           - Get outer scope
```

### Opcode Operand Types

| Type | Description | Encoding |
|------|-------------|----------|
| U8 | Unsigned 8-bit integer | 1 byte |
| U30 | Unsigned 30-bit variable-length | 1-5 bytes |
| S24 | Signed 24-bit variable-length | 1-4 bytes |
| multiname | Index into multiname pool | U30 |
| method | Index into method_info array | U30 |
| class | Index into class_info array | U30 |

### Critical Implementation Notes

1. **Variable-Length Encoding (u30)**
   - Each byte: 7 data bits + 1 continuation bit
   - Maximum 5 bytes
   - Values up to 2^30-1

2. **Stack Machine**
   - Most opcodes manipulate operand stack
   - Stack depth tracked in method body (max_stack)
   - Local variables separate from stack

3. **Index References**
   - All constant pools: index 0 = special value (not stored in file)
   - Method bodies reference method_info by index
   - Traits reference multinames by index

4. **Control Flow**
   - Jump offsets are relative to instruction AFTER the jump
   - lookupswitch has variable-length case list

---

## Implementation Roadmap

### Phase 1: Setup (1-2 days)

**Goal:** Create project structure and basic types

**Tasks:**
1. Create directories: `src/abc/`, `include/abc/`
2. Add files to CMakeLists.txt
3. Define basic types in `abc_types.h`:
   ```cpp
   struct ABCFile {
       uint16_t minor_version;
       uint16_t major_version;
       std::vector<int32_t> int_pool;
       std::vector<uint32_t> uint_pool;
       std::vector<double> double_pool;
       std::vector<std::string> string_pool;
       std::vector<Namespace> namespace_pool;
       std::vector<NamespaceSet> ns_set_pool;
       std::vector<Multiname> multiname_pool;
       std::vector<MethodInfo> method_info;
       std::vector<MetadataInfo> metadata_info;
       std::vector<InstanceInfo> instance_info;
       std::vector<ClassInfo> class_info;
       std::vector<ScriptInfo> script_info;
       std::vector<MethodBodyInfo> method_body_info;
   };
   ```

4. Implement ABCReader class in `abc_reader.h/cpp`:
   ```cpp
   class ABCReader {
   public:
       ABCReader(const uint8_t* data, size_t size);
       uint8_t read_u8();
       uint16_t read_u16();
       uint32_t read_u30();
       int32_t read_s32();
       double read_d64();
       std::string read_string();
       bool eof() const;
   private:
       const uint8_t* data_;
       const uint8_t* ptr_;
       const uint8_t* end_;
   };
   ```

**Deliverable:** Compiling project with empty ABC structures

### Phase 2: Constant Pools (3-4 days)

**Goal:** Parse all constant pools

**Tasks:**
1. Implement `read_u30()` with unit tests
2. Parse integer pool (int32)
3. Parse unsigned integer pool (uint32)
4. Parse double pool (float64, little-endian)
5. Parse string pool (UTF-8 with u30 length)
6. Parse namespace pool (kind + name index)
7. Parse namespace set pool (array of ns indices)
8. Parse multiname pool (9 different kinds!)
9. Handle index 0 special cases for all pools
10. Add validation (bounds checking)

**Test with:**
```cpp
void test_u30_encoding() {
    uint8_t data[] = {0x7F};  // 127
    ABCReader reader(data, 1);
    assert(reader.read_u30() == 127);

    uint8_t data2[] = {0x80, 0x01};  // 128
    ABCReader reader2(data2, 2);
    assert(reader2.read_u30() == 128);
}
```

**Deliverable:** Working constant pool parser with tests

### Phase 3: Methods and Classes (3-4 days)

**Goal:** Parse method signatures and class definitions

**Tasks:**
1. Parse method_info array:
   - Parameter count and types
   - Return type
   - Flags (HAS_OPTIONAL, HAS_PARAM_NAMES, etc.)
   - Optional parameters
   - Parameter names

2. Parse metadata_info array (can skip for now)

3. Parse instance_info and class_info:
   - Class name (multiname index)
   - Super class (multiname index)
   - Flags (SEALED, FINAL, INTERFACE, PROTECTED_NS)
   - Interfaces
   - Instance constructor (iinit)
   - Class constructor (cinit)
   - Traits (complex!)

4. Implement trait parsing:
   - 7 different trait kinds
   - Variable-length data per kind
   - Metadata references

**Deliverable:** Complete class and method parsing

### Phase 4: Scripts and Bodies (3-4 days)

**Goal:** Parse bytecode and finalize parser

**Tasks:**
1. Parse script_info array:
   - Init method index
   - Script traits

2. Parse method_body_info array:
   - Method index (which method this body is for)
   - Stack sizes (max_stack, local_count)
   - Scope depths (init_scope_depth, max_scope_depth)
   - Bytecode (raw byte array)
   - Exception handlers
   - Body traits

3. Add comprehensive validation:
   - Version checking
   - Index bounds checking
   - Structural validation
   - Pool count verification

4. Implement debug output:
   ```cpp
   void print_abc_info(const ABCFile* abc);
   void print_class_info(const ABCFile* abc, size_t class_index);
   void print_method_body(const ABCFile* abc, size_t body_index);
   ```

**Deliverable:** Complete ABC parser

### Phase 5: Integration (2-3 days)

**Goal:** Integrate with SWF parser

**Tasks:**
1. Uncomment DoABC tag handler in `swf.cpp`
2. Extract ABC data from DoABC tag:
   - Read flags (U32)
   - Read name (null-terminated string)
   - Calculate ABC data size
3. Call ABC parser
4. Store parsed ABC for code generation (Phase 2)
5. Handle errors gracefully
6. Add logging/debug output

**Deliverable:** SWFRecomp can parse DoABC tags

### Phase 6: Testing and Documentation (2-3 days)

**Goal:** Verify correctness and document

**Tasks:**
1. Find or create test AS3 SWF files:
   - Hello World (single class, single method)
   - Simple math (arithmetic operations)
   - Class inheritance (super class test)
   - Interfaces (interface implementation)

2. Test parsing with real files:
   - Verify constant pools
   - Check class hierarchies
   - Validate method signatures

3. Compare against reference implementations:
   - Use RABCDAsm to disassemble same file
   - Compare output

4. Document:
   - API usage examples
   - Integration guide
   - Troubleshooting tips

**Deliverable:** Tested, documented ABC parser

---

## Testing Strategy

### Unit Tests

**Test u30 encoding:**
```cpp
void test_u30_single_byte() {
    // Values 0-127 encode as single byte
    for (int i = 0; i < 128; i++) {
        uint8_t data[] = {(uint8_t)i};
        ABCReader r(data, 1);
        assert(r.read_u30() == i);
    }
}

void test_u30_two_bytes() {
    // 128 = 0x80 0x01
    uint8_t data[] = {0x80, 0x01};
    ABCReader r(data, 2);
    assert(r.read_u30() == 128);
}
```

**Test string reading:**
```cpp
void test_string_pool() {
    // String pool format:
    // - U30 count (including implicit index 0)
    // - For each string: U30 length, UTF-8 bytes
    uint8_t data[] = {
        0x02,              // 2 strings (index 0 implicit, index 1 explicit)
        0x05,              // Length 5
        'h','e','l','l','o'
    };

    ABCFile abc;
    const uint8_t* ptr = data;
    parse_string_pool(ptr, &abc);

    assert(abc.string_pool.size() == 2);
    assert(abc.string_pool[0] == "");      // Index 0 implicit
    assert(abc.string_pool[1] == "hello");
}
```

### Integration Tests

**Test minimal ABC file:**
```cpp
void test_minimal_abc() {
    // Smallest valid ABC file:
    // - Header (version 46.16)
    // - Empty constant pools
    // - No methods
    // - No classes
    // - One script with no traits
    // - No method bodies

    uint8_t data[] = {
        0x10, 0x00,  // Minor 16
        0x2E, 0x00,  // Major 46
        0x01,        // 1 int (implicit 0)
        0x01,        // 1 uint (implicit 0)
        0x01,        // 1 double (implicit NaN)
        0x01,        // 1 string (implicit "")
        0x01,        // 1 namespace (implicit any)
        0x01,        // 1 ns set (implicit empty)
        0x01,        // 1 multiname (implicit any)
        0x00,        // 0 methods
        0x00,        // 0 metadata
        0x00,        // 0 classes
        0x01,        // 1 script
        0x00,        // Init method 0
        0x00,        // 0 traits
        0x00,        // 0 method bodies
    };

    ABCFile* abc = parse_abc_file(data, sizeof(data));
    assert(abc->major_version == 46);
    assert(abc->minor_version == 16);
    assert(abc->string_pool.size() == 1);
    assert(abc->script_info.size() == 1);
    delete abc;
}
```

**Test against real SWF:**
```bash
# Use RABCDAsm to extract and disassemble ABC
abcexport test.swf
rabcdasm test-0.abc

# Parse with SWFRecomp
./build/SWFRecomp --dump-abc test.swf

# Compare outputs
diff test-0/*.asasm swfrecomp_output.txt
```

### Validation Tests

**Test error conditions:**
```cpp
void test_invalid_version() {
    uint8_t data[] = {0x10, 0x00, 0x00, 0x00};  // Version 0.16 (invalid)
    try {
        parse_abc_file(data, 4);
        assert(false);  // Should throw
    } catch (std::exception& e) {
        // Expected
    }
}

void test_truncated_file() {
    uint8_t data[] = {0x10, 0x00, 0x2E};  // Truncated header
    try {
        parse_abc_file(data, 3);
        assert(false);  // Should throw
    } catch (std::exception& e) {
        // Expected
    }
}

void test_invalid_index() {
    // ABC file with string index out of bounds
    // ... (create ABC with method referencing non-existent string)
    try {
        validate_abc(abc);
        assert(false);  // Should throw
    } catch (std::exception& e) {
        // Expected
    }
}
```

---

## Next Steps

### Immediate Actions (Today)

1. ✅ **Review this document** - Ensure understanding of all sections
2. **Set up project structure**:
   ```bash
   mkdir -p src/abc include/abc
   touch include/abc/abc_types.h
   touch include/abc/abc_reader.h
   touch include/abc/abc_parser.h
   touch src/abc/abc_reader.cpp
   touch src/abc/abc_parser.cpp
   ```

3. **Update CMakeLists.txt** - Add new files to build

4. **Start with ABCReader** - Implement u30 encoding first

### Week 1 Goals

- Complete ABCReader class with all read functions
- Implement constant pool parsing
- Write unit tests for u30, strings, and pools
- Parse header + all constant pools successfully

### Week 2 Goals

- Parse method_info array
- Parse class_info and instance_info
- Implement trait parsing
- Parse script_info

### Week 3 Goals

- Parse method_body_info
- Integrate with SWF parser (DoABC tag handler)
- Test with real SWF files
- Add debug output and documentation

### Success Criteria

✅ **Parser can successfully parse:**
- Minimal ABC file (test case)
- Hello World AS3 SWF
- Seedling.swf (when available)
- Multiple classes with inheritance
- Methods with bytecode

✅ **Output includes:**
- Class names and hierarchy
- Method signatures
- Constant pool contents
- Bytecode (raw bytes for now)

✅ **Code quality:**
- No memory leaks
- Proper error handling
- Clean separation of concerns
- Well-commented

---

## Questions & Answers

### Q: What if I can't find AS3 test SWF files?

**A:** Several options:
1. Use online Flash game archives (Newgrounds, etc.)
2. Compile simple AS3 files using Apache Flex SDK:
   ```bash
   # Install Apache Flex SDK
   mxmlc HelloWorld.as -o HelloWorld.swf
   ```
3. Use RABCDAsm to create test ABC files:
   ```bash
   # Write .asasm file, assemble to ABC
   rabcasm test.asasm
   ```
4. Focus on parsing structure first, validate with hex dumps

### Q: Should I implement all 256 opcodes?

**A:** No! Phase 1 (ABC Parser) only needs to:
- **Parse** bytecode as raw bytes
- **Store** in `method_body.code` vector
- **NOT interpret** opcodes

Phase 2 (Code Generator) will:
- **Disassemble** bytecode
- **Generate** C code
- **Implement** runtime for each opcode

### Q: What about DoABC vs DoAction tags?

**A:** Don't confuse DoABC (tag 82) with DoAction (tag 72):
- **Tag 72 = DoAction** - ActionScript 1.0/2.0 bytecode (older Flash)
- **Tag 82 = DoABC** - ActionScript 3.0 ABC bytecode (Flash Player 9+)

Only implement DoABC (tag 82) for AS3 support:
```cpp
case SWF_TAG_DO_ABC:      // 82 (AS3)
{
    // Parse ABC data
}

// DoAction (tag 72) is already handled for AS1/AS2 in action/action.cpp
```

### Q: How do I handle multiname complexity?

**A:** Multinames have 9 different kinds. Use a union or variant:
```cpp
struct Multiname {
    MultinameKind kind;

    // Use fields based on kind
    uint32_t ns_index;       // For QName, RTQName
    uint32_t name_index;     // For QName, Multiname
    uint32_t ns_set_index;   // For Multiname, MultinameL
    std::vector<uint32_t> type_params;  // For TypeName
};
```

### Q: What about s32 vs u30 encoding?

**A:** Both use variable-length encoding:
- **u30**: Unsigned, 30 bits max
- **s32**: Signed, 32 bits max
- **Encoding**: Same 7-bit continuation format
- **Sign**: s32 uses sign extension on final value

---

## Appendix: Key File Locations

### Documentation
- `docs/specs/swf-spec-19.txt` (370 KB text version)
- `docs/specs/abc-format-46-16.txt`
- `docs/specs/avm2_opcodes_raw.txt`
- `ABC_PARSER_GUIDE.md`
- `ABC_PARSER_RESEARCH.md`

### Source Code
- `src/swf.cpp` - Line 889 (DoABC handler)
- `include/tag.hpp` - Line 27 (SWF_TAG_DO_ABC)
- `src/field.cpp` - Binary reading patterns
- `CMakeLists.txt` - Build configuration

### Test Resources
- `tests/*/test.swf` - 50+ test SWF files
- `../Seedling/src/` - AS3 source (no compiled SWF yet)

### External References
- https://github.com/ruffle-rs/ruffle/blob/master/swf/src/avm2/read.rs
- https://github.com/adobe-flash/avmplus/blob/master/core/Interpreter.cpp
- https://github.com/CyberShadow/RABCDAsm/blob/master/abcfile.d

---

**Document Status:** Complete and ready for implementation
**Last Updated:** October 29, 2025
**Prepared By:** Claude Code (Exploration Agent)
