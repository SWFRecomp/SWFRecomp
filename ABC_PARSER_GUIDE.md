# ABC Parser Implementation Guide - Phase 1

**Document Version:** 1.0
**Date:** October 29, 2025
**Phase:** ABC Parser (First implementation step for AS3 support)
**Language:** C++ (build-time tool only)

---

## Table of Contents

1. [Overview](#overview)
2. [ABC File Format](#abc-file-format)
3. [Implementation Steps](#implementation-steps)
4. [Data Structures](#data-structures)
5. [Parsing Details](#parsing-details)
6. [Validation](#validation)
7. [Testing](#testing)
8. [Example Code](#example-code)

---

## Overview

### What is the ABC Parser?

The ABC (ActionScript Byte Code) parser is the **first component** you need to build for AS3 support. It reads the binary ABC format embedded in DoABC tags within SWF files and extracts:

- **Constant pools** - strings, numbers, namespaces, multinames
- **Method signatures** - parameter types, return types, flags
- **Class definitions** - inheritance, traits, slots
- **Method bodies** - bytecode, exception handlers, stack sizes
- **Scripts** - initialization code

### Why C++ for the Parser?

The ABC parser is a **build-time tool** that runs during recompilation, not at runtime. C++ is appropriate here because:

- It doesn't affect runtime performance
- STL containers simplify parsing (vectors, maps, strings)
- Exception handling makes error reporting easier
- It integrates with the existing SWFRecomp C++ codebase

The parsed data will be used to **generate C runtime code** in Phase 2.

### Input and Output

**Input:** SWF file with DoABC tags
**Output:** In-memory ABC structures (later used for code generation)

```
SWF file (binary)
    ↓
DoABC tag extraction
    ↓
ABC binary data
    ↓
ABC Parser (this phase) ← YOU ARE HERE
    ↓
ABCFile structure (in-memory)
    ↓
Code Generator (Phase 2)
    ↓
Generated C code
```

---

## ABC File Format

### File Structure Overview

An ABC file contains:

```
ABC File:
├── Header (minor_version, major_version)
├── Constant Pools
│   ├── Integer pool (int32[])
│   ├── Unsigned int pool (uint32[])
│   ├── Double pool (double[])
│   ├── String pool (UTF-8 strings)
│   ├── Namespace pool
│   ├── Namespace set pool
│   └── Multiname pool
├── Method Info Array
├── Metadata Info Array
├── Class Info Array
│   ├── Instance Info (per class)
│   └── Class Info (static members)
├── Script Info Array
└── Method Body Array
```

### ABC Version

```c
typedef struct {
    uint16_t minor_version;  // Typically 16
    uint16_t major_version;  // Typically 46 (for Flash Player 10+)
} ABCHeader;
```

**Version history:**
- 46.16 - Flash Player 9+ (AS3)
- 46.17 - Flash Player 10.3+ (added some features)
- 47.x - Flash Player 11+ (experimental)

Most AS3 content uses version 46.16.

### Variable-Length Encoding (u30)

ABC uses a variable-length encoding for integers called **u30** or **u32**:

- Integers 0-127: 1 byte
- Integers 128-16383: 2 bytes
- Larger integers: up to 5 bytes

**Encoding format:**
- Each byte has 7 data bits + 1 continuation bit
- Continuation bit = 1 means "more bytes follow"
- Little-endian bit order

**Implementation:**
```cpp
uint32_t read_u30(const uint8_t*& ptr) {
    uint32_t result = 0;
    int shift = 0;

    for (int i = 0; i < 5; i++) {
        uint8_t byte = *ptr++;
        result |= (byte & 0x7F) << shift;

        if (!(byte & 0x80)) {  // No continuation bit
            break;
        }
        shift += 7;
    }

    return result;
}
```

### String Pool

Strings are stored as UTF-8 with length prefix:

```
String entry:
├── Length (u30) - number of bytes
└── Data (UTF-8 bytes)
```

**Index 0 is special:** It represents the empty string `""` and is not stored in the pool.

**Implementation:**
```cpp
std::string read_string(const uint8_t*& ptr) {
    uint32_t length = read_u30(ptr);
    if (length == 0) {
        return "";  // Empty string
    }

    std::string result((const char*)ptr, length);
    ptr += length;
    return result;
}

void parse_string_pool(const uint8_t*& ptr, ABCFile* abc) {
    uint32_t count = read_u30(ptr);

    // Index 0 is implicit empty string
    abc->string_pool.push_back("");

    // Parse remaining strings
    for (uint32_t i = 1; i < count; i++) {
        abc->string_pool.push_back(read_string(ptr));
    }
}
```

---

## Implementation Steps

### Step 1: Set Up Project Structure

Create the following files:

```
src/abc/
├── abc_parser.cpp         # Main parser implementation
├── abc_parser.h           # Parser interface
├── abc_types.h            # ABC data structures
├── abc_reader.cpp         # Low-level binary reading
└── abc_reader.h           # Reader utilities
```

### Step 2: Implement Binary Reader

Start with basic reading utilities:

```cpp
// abc_reader.h
class ABCReader {
public:
    ABCReader(const uint8_t* data, size_t size);

    // Basic reads
    uint8_t read_u8();
    uint16_t read_u16();
    uint32_t read_u30();
    int32_t read_s32();
    double read_d64();
    std::string read_string();

    // Position management
    size_t position() const { return ptr_ - data_; }
    bool eof() const { return ptr_ >= end_; }

private:
    const uint8_t* data_;
    const uint8_t* ptr_;
    const uint8_t* end_;
};
```

### Step 3: Parse ABC Header

```cpp
void parse_header(ABCReader& reader, ABCFile* abc) {
    abc->minor_version = reader.read_u16();
    abc->major_version = reader.read_u16();

    // Validate version
    if (abc->major_version != 46 && abc->major_version != 47) {
        throw std::runtime_error(
            "Unsupported ABC version: " +
            std::to_string(abc->major_version) + "." +
            std::to_string(abc->minor_version)
        );
    }
}
```

### Step 4: Parse Constant Pools

Parse pools in this order (per ABC spec):

```cpp
void parse_constant_pools(ABCReader& reader, ABCFile* abc) {
    // 1. Integer pool
    uint32_t int_count = reader.read_u30();
    abc->int_pool.push_back(0);  // Index 0 implicit
    for (uint32_t i = 1; i < int_count; i++) {
        abc->int_pool.push_back(reader.read_s32());
    }

    // 2. Unsigned integer pool
    uint32_t uint_count = reader.read_u30();
    abc->uint_pool.push_back(0);  // Index 0 implicit
    for (uint32_t i = 1; i < uint_count; i++) {
        abc->uint_pool.push_back(reader.read_u30());
    }

    // 3. Double pool
    uint32_t double_count = reader.read_u30();
    abc->double_pool.push_back(NAN);  // Index 0 implicit
    for (uint32_t i = 1; i < double_count; i++) {
        abc->double_pool.push_back(reader.read_d64());
    }

    // 4. String pool
    parse_string_pool(reader, abc);

    // 5. Namespace pool
    parse_namespace_pool(reader, abc);

    // 6. Namespace set pool
    parse_ns_set_pool(reader, abc);

    // 7. Multiname pool
    parse_multiname_pool(reader, abc);
}
```

### Step 5: Parse Namespace Pool

Namespaces have a kind and a name:

```cpp
enum NamespaceKind {
    NAMESPACE = 0x08,
    PACKAGE_NAMESPACE = 0x16,
    PACKAGE_INTERNAL_NS = 0x17,
    PROTECTED_NAMESPACE = 0x18,
    EXPLICIT_NAMESPACE = 0x19,
    STATIC_PROTECTED_NS = 0x1A,
    PRIVATE_NS = 0x05,
};

struct Namespace {
    NamespaceKind kind;
    uint32_t name_index;  // Index into string pool
};

void parse_namespace_pool(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    // Index 0 is special: any namespace (wildcard)
    abc->namespace_pool.push_back({NAMESPACE, 0});

    for (uint32_t i = 1; i < count; i++) {
        Namespace ns;
        ns.kind = (NamespaceKind)reader.read_u8();
        ns.name_index = reader.read_u30();
        abc->namespace_pool.push_back(ns);
    }
}
```

### Step 6: Parse Namespace Set Pool

Namespace sets are arrays of namespace indices:

```cpp
struct NamespaceSet {
    std::vector<uint32_t> namespace_indices;
};

void parse_ns_set_pool(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    // Index 0 is implicit empty set
    abc->ns_set_pool.push_back({});

    for (uint32_t i = 1; i < count; i++) {
        NamespaceSet ns_set;
        uint32_t ns_count = reader.read_u30();

        for (uint32_t j = 0; j < ns_count; j++) {
            ns_set.namespace_indices.push_back(reader.read_u30());
        }

        abc->ns_set_pool.push_back(ns_set);
    }
}
```

### Step 7: Parse Multiname Pool

Multinames are complex - they represent names with namespace qualification:

```cpp
enum MultinameKind {
    QNAME = 0x07,           // Qualified name (ns + name)
    QNAME_A = 0x0D,         // QName with attribute flag
    RTQNAME = 0x0F,         // Runtime qualified name
    RTQNAME_A = 0x10,       // Runtime QName with attribute
    RTQNAME_L = 0x11,       // Late-bound runtime QName
    RTQNAME_LA = 0x12,      // Late-bound runtime QName attr
    MULTINAME = 0x09,       // Multiname (name + ns set)
    MULTINAME_A = 0x0E,     // Multiname with attribute
    MULTINAME_L = 0x1B,     // Late-bound multiname
    MULTINAME_LA = 0x1C,    // Late-bound multiname attr
    TYPENAME = 0x1D,        // Generic type (Vector.<T>)
};

struct Multiname {
    MultinameKind kind;
    uint32_t ns_index;      // For QName
    uint32_t name_index;    // For QName, Multiname
    uint32_t ns_set_index;  // For Multiname
    std::vector<uint32_t> type_params;  // For TypeName
};

void parse_multiname_pool(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    // Index 0 is implicit wildcard
    abc->multiname_pool.push_back({});

    for (uint32_t i = 1; i < count; i++) {
        Multiname mn;
        mn.kind = (MultinameKind)reader.read_u8();

        switch (mn.kind) {
            case QNAME:
            case QNAME_A:
                mn.ns_index = reader.read_u30();
                mn.name_index = reader.read_u30();
                break;

            case RTQNAME:
            case RTQNAME_A:
                mn.name_index = reader.read_u30();
                break;

            case RTQNAME_L:
            case RTQNAME_LA:
                // No data
                break;

            case MULTINAME:
            case MULTINAME_A:
                mn.name_index = reader.read_u30();
                mn.ns_set_index = reader.read_u30();
                break;

            case MULTINAME_L:
            case MULTINAME_LA:
                mn.ns_set_index = reader.read_u30();
                break;

            case TYPENAME: {
                mn.name_index = reader.read_u30();  // Base type
                uint32_t param_count = reader.read_u30();
                for (uint32_t j = 0; j < param_count; j++) {
                    mn.type_params.push_back(reader.read_u30());
                }
                break;
            }
        }

        abc->multiname_pool.push_back(mn);
    }
}
```

### Step 8: Parse Method Info Array

Method signatures:

```cpp
enum MethodFlags {
    NEED_ARGUMENTS = 0x01,      // need_arguments
    NEED_ACTIVATION = 0x02,     // need_activation
    NEED_REST = 0x04,           // need_rest
    HAS_OPTIONAL = 0x08,        // has_optional
    IGNORE_REST = 0x10,         // ignore_rest
    EXPLICIT = 0x20,            // explicit
    SET_DXNS = 0x40,            // setsdxns
    HAS_PARAM_NAMES = 0x80,     // has_paramnames
};

struct OptionDetail {
    uint32_t value_index;
    uint8_t value_kind;  // Constant pool type
};

struct MethodInfo {
    uint32_t param_count;
    uint32_t return_type;        // Multiname index
    std::vector<uint32_t> param_types;
    uint32_t name_index;
    uint8_t flags;
    std::vector<OptionDetail> options;
    std::vector<uint32_t> param_names;
};

void parse_method_info(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    for (uint32_t i = 0; i < count; i++) {
        MethodInfo method;

        method.param_count = reader.read_u30();
        method.return_type = reader.read_u30();

        for (uint32_t j = 0; j < method.param_count; j++) {
            method.param_types.push_back(reader.read_u30());
        }

        method.name_index = reader.read_u30();
        method.flags = reader.read_u8();

        // Optional parameters
        if (method.flags & HAS_OPTIONAL) {
            uint32_t option_count = reader.read_u30();
            for (uint32_t j = 0; j < option_count; j++) {
                OptionDetail opt;
                opt.value_index = reader.read_u30();
                opt.value_kind = reader.read_u8();
                method.options.push_back(opt);
            }
        }

        // Parameter names (debugging info)
        if (method.flags & HAS_PARAM_NAMES) {
            for (uint32_t j = 0; j < method.param_count; j++) {
                method.param_names.push_back(reader.read_u30());
            }
        }

        abc->method_info.push_back(method);
    }
}
```

### Step 9: Parse Metadata (Optional)

Metadata can be skipped for now, but here's how to parse it:

```cpp
struct MetadataItem {
    uint32_t key_index;
    uint32_t value_index;
};

struct MetadataInfo {
    uint32_t name_index;
    std::vector<MetadataItem> items;
};

void parse_metadata(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    for (uint32_t i = 0; i < count; i++) {
        MetadataInfo meta;
        meta.name_index = reader.read_u30();

        uint32_t item_count = reader.read_u30();
        for (uint32_t j = 0; j < item_count; j++) {
            MetadataItem item;
            item.key_index = reader.read_u30();
            item.value_index = reader.read_u30();
            meta.items.push_back(item);
        }

        abc->metadata_info.push_back(meta);
    }
}
```

### Step 10: Parse Class Definitions

Classes have instance info and class (static) info:

```cpp
enum ClassFlags {
    CLASS_SEALED = 0x01,
    CLASS_FINAL = 0x02,
    CLASS_INTERFACE = 0x04,
    CLASS_PROTECTED_NS = 0x08,
};

struct InstanceInfo {
    uint32_t name_index;         // Multiname
    uint32_t super_name_index;   // Multiname
    uint8_t flags;
    uint32_t protected_ns_index;
    std::vector<uint32_t> interface_indices;
    uint32_t iinit_index;        // Instance constructor
    std::vector<Trait> traits;
};

struct ClassInfo {
    uint32_t cinit_index;        // Class constructor
    std::vector<Trait> traits;
};

void parse_classes(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    // Parse instance info
    for (uint32_t i = 0; i < count; i++) {
        InstanceInfo inst;

        inst.name_index = reader.read_u30();
        inst.super_name_index = reader.read_u30();
        inst.flags = reader.read_u8();

        if (inst.flags & CLASS_PROTECTED_NS) {
            inst.protected_ns_index = reader.read_u30();
        }

        uint32_t interface_count = reader.read_u30();
        for (uint32_t j = 0; j < interface_count; j++) {
            inst.interface_indices.push_back(reader.read_u30());
        }

        inst.iinit_index = reader.read_u30();
        parse_traits(reader, inst.traits);

        abc->instance_info.push_back(inst);
    }

    // Parse class info
    for (uint32_t i = 0; i < count; i++) {
        ClassInfo cls;
        cls.cinit_index = reader.read_u30();
        parse_traits(reader, cls.traits);
        abc->class_info.push_back(cls);
    }
}
```

### Step 11: Parse Traits

Traits are properties, methods, getters, setters:

```cpp
enum TraitKind {
    TRAIT_SLOT = 0,
    TRAIT_METHOD = 1,
    TRAIT_GETTER = 2,
    TRAIT_SETTER = 3,
    TRAIT_CLASS = 4,
    TRAIT_FUNCTION = 5,
    TRAIT_CONST = 6,
};

enum TraitAttributes {
    ATTR_FINAL = 0x10,
    ATTR_OVERRIDE = 0x20,
    ATTR_METADATA = 0x40,
};

struct Trait {
    uint32_t name_index;
    uint8_t kind;
    uint8_t attributes;

    // For TRAIT_SLOT, TRAIT_CONST
    uint32_t slot_id;
    uint32_t type_name;
    uint32_t vindex;
    uint8_t vkind;

    // For TRAIT_METHOD, TRAIT_GETTER, TRAIT_SETTER, TRAIT_FUNCTION
    uint32_t disp_id;
    uint32_t method_index;

    // For TRAIT_CLASS
    uint32_t class_index;

    std::vector<uint32_t> metadata_indices;
};

void parse_traits(ABCReader& reader, std::vector<Trait>& traits) {
    uint32_t count = reader.read_u30();

    for (uint32_t i = 0; i < count; i++) {
        Trait trait;

        trait.name_index = reader.read_u30();
        uint8_t kind_and_attrs = reader.read_u8();

        trait.kind = kind_and_attrs & 0x0F;
        trait.attributes = (kind_and_attrs >> 4) & 0x0F;

        switch (trait.kind) {
            case TRAIT_SLOT:
            case TRAIT_CONST:
                trait.slot_id = reader.read_u30();
                trait.type_name = reader.read_u30();
                trait.vindex = reader.read_u30();
                if (trait.vindex != 0) {
                    trait.vkind = reader.read_u8();
                }
                break;

            case TRAIT_METHOD:
            case TRAIT_GETTER:
            case TRAIT_SETTER:
                trait.disp_id = reader.read_u30();
                trait.method_index = reader.read_u30();
                break;

            case TRAIT_CLASS:
                trait.slot_id = reader.read_u30();
                trait.class_index = reader.read_u30();
                break;

            case TRAIT_FUNCTION:
                trait.slot_id = reader.read_u30();
                trait.method_index = reader.read_u30();
                break;
        }

        if (trait.attributes & ATTR_METADATA) {
            uint32_t metadata_count = reader.read_u30();
            for (uint32_t j = 0; j < metadata_count; j++) {
                trait.metadata_indices.push_back(reader.read_u30());
            }
        }

        traits.push_back(trait);
    }
}
```

### Step 12: Parse Scripts

Scripts are initialization code:

```cpp
struct ScriptInfo {
    uint32_t init_index;  // Method index
    std::vector<Trait> traits;
};

void parse_scripts(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    for (uint32_t i = 0; i < count; i++) {
        ScriptInfo script;
        script.init_index = reader.read_u30();
        parse_traits(reader, script.traits);
        abc->script_info.push_back(script);
    }
}
```

### Step 13: Parse Method Bodies

Method bodies contain bytecode:

```cpp
struct ExceptionInfo {
    uint32_t from;
    uint32_t to;
    uint32_t target;
    uint32_t exc_type_index;
    uint32_t var_name_index;
};

struct MethodBodyInfo {
    uint32_t method_index;
    uint32_t max_stack;
    uint32_t max_regs;          // Number of local registers
    uint32_t scope_depth;       // Initial scope depth
    uint32_t max_scope_depth;   // Maximum scope depth
    std::vector<uint8_t> code;
    std::vector<ExceptionInfo> exceptions;
    std::vector<Trait> traits;
};

void parse_method_bodies(ABCReader& reader, ABCFile* abc) {
    uint32_t count = reader.read_u30();

    for (uint32_t i = 0; i < count; i++) {
        MethodBodyInfo body;

        body.method_index = reader.read_u30();
        body.max_stack = reader.read_u30();
        body.max_regs = reader.read_u30();
        body.scope_depth = reader.read_u30();
        body.max_scope_depth = reader.read_u30();

        // Read bytecode
        uint32_t code_length = reader.read_u30();
        body.code.resize(code_length);
        for (uint32_t j = 0; j < code_length; j++) {
            body.code[j] = reader.read_u8();
        }

        // Read exception handlers
        uint32_t exception_count = reader.read_u30();
        for (uint32_t j = 0; j < exception_count; j++) {
            ExceptionInfo ex;
            ex.from = reader.read_u30();
            ex.to = reader.read_u30();
            ex.target = reader.read_u30();
            ex.exc_type_index = reader.read_u30();
            ex.var_name_index = reader.read_u30();
            body.exceptions.push_back(ex);
        }

        // Read traits
        parse_traits(reader, body.traits);

        abc->method_body_info.push_back(body);
    }
}
```

---

## Data Structures

### Complete ABCFile Structure

```cpp
// abc_types.h

struct ABCFile {
    // Header
    uint16_t minor_version;
    uint16_t major_version;

    // Constant pools
    std::vector<int32_t> int_pool;
    std::vector<uint32_t> uint_pool;
    std::vector<double> double_pool;
    std::vector<std::string> string_pool;
    std::vector<Namespace> namespace_pool;
    std::vector<NamespaceSet> ns_set_pool;
    std::vector<Multiname> multiname_pool;

    // Method info
    std::vector<MethodInfo> method_info;

    // Metadata
    std::vector<MetadataInfo> metadata_info;

    // Class definitions
    std::vector<InstanceInfo> instance_info;
    std::vector<ClassInfo> class_info;

    // Scripts
    std::vector<ScriptInfo> script_info;

    // Method bodies
    std::vector<MethodBodyInfo> method_body_info;
};
```

---

## Validation

### What to Validate

1. **Version checking**
   - Ensure major version is 46 or 47
   - Warn on unknown versions

2. **Index bounds checking**
   - All constant pool indices must be valid
   - String indices < string_pool.size()
   - Namespace indices < namespace_pool.size()
   - Method indices < method_info.size()

3. **Structural validation**
   - Pool counts match actual entries
   - Trait counts match parsed traits
   - Code length matches actual bytecode length

4. **Semantic validation** (optional)
   - Class inheritance forms a valid tree
   - No circular dependencies
   - Method signatures are valid

### Example Validation

```cpp
void validate_abc(const ABCFile* abc) {
    // Validate version
    if (abc->major_version < 46 || abc->major_version > 47) {
        throw std::runtime_error("Invalid ABC version");
    }

    // Validate class count matches
    if (abc->instance_info.size() != abc->class_info.size()) {
        throw std::runtime_error("Mismatch between instance and class count");
    }

    // Validate method indices
    for (const auto& body : abc->method_body_info) {
        if (body.method_index >= abc->method_info.size()) {
            throw std::runtime_error("Invalid method index in body");
        }
    }

    // More validation...
}
```

---

## Testing

### Test Strategy

1. **Unit tests for binary reading**
   - Test u30 encoding/decoding
   - Test string reading
   - Test double reading

2. **Parse simple ABC files**
   - Single class with one method
   - Empty method body
   - Simple constant pools

3. **Parse real SWF files**
   - Hello world AS3 program
   - Simple FlashPunk program
   - Seedling game

4. **Validation tests**
   - Malformed ABC data
   - Invalid indices
   - Truncated files

### Test Cases

```cpp
// Test 1: Parse empty ABC
void test_empty_abc() {
    // Minimal ABC with no classes, no methods
    uint8_t data[] = {
        0x10, 0x00,  // Minor version 16
        0x2E, 0x00,  // Major version 46
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
        0x00,        // Script init method 0
        0x00,        // 0 script traits
        0x00,        // 0 method bodies
    };

    ABCFile abc;
    parse_abc(data, sizeof(data), &abc);

    assert(abc.major_version == 46);
    assert(abc.minor_version == 16);
    assert(abc.string_pool.size() == 1);
    assert(abc.string_pool[0] == "");
}

// Test 2: Parse simple class
void test_simple_class() {
    // Create ABC with one class "Hello" extending Object
    // with one method "sayHello"
    // ... (more complex test data)
}
```

---

## Example Code

### Complete Parser Main Function

```cpp
// abc_parser.cpp

#include "abc_parser.h"
#include "abc_reader.h"
#include "abc_types.h"

ABCFile* parse_abc(const uint8_t* data, size_t size) {
    ABCFile* abc = new ABCFile();
    ABCReader reader(data, size);

    try {
        // Parse header
        parse_header(reader, abc);

        // Parse constant pools
        parse_constant_pools(reader, abc);

        // Parse method info
        parse_method_info(reader, abc);

        // Parse metadata
        parse_metadata(reader, abc);

        // Parse classes
        parse_classes(reader, abc);

        // Parse scripts
        parse_scripts(reader, abc);

        // Parse method bodies
        parse_method_bodies(reader, abc);

        // Validate
        validate_abc(abc);

        return abc;

    } catch (const std::exception& e) {
        delete abc;
        throw;
    }
}

void print_abc_info(const ABCFile* abc) {
    std::cout << "ABC Version: " << abc->major_version << "."
              << abc->minor_version << std::endl;
    std::cout << "Strings: " << abc->string_pool.size() << std::endl;
    std::cout << "Methods: " << abc->method_info.size() << std::endl;
    std::cout << "Classes: " << abc->instance_info.size() << std::endl;
    std::cout << "Scripts: " << abc->script_info.size() << std::endl;
    std::cout << "Method Bodies: " << abc->method_body_info.size() << std::endl;

    // Print class names
    std::cout << "\nClasses:" << std::endl;
    for (const auto& inst : abc->instance_info) {
        const Multiname& mn = abc->multiname_pool[inst.name_index];
        const std::string& name = abc->string_pool[mn.name_index];
        std::cout << "  - " << name << std::endl;
    }
}
```

### Integration with SWFRecomp

```cpp
// In swf.cpp, add DoABC tag handling

void process_doabc_tag(const uint8_t* tag_data, size_t tag_size) {
    // DoABC format:
    // - u32 flags
    // - string name
    // - ABC data

    const uint8_t* ptr = tag_data;
    uint32_t flags = read_u32(ptr);
    ptr += 4;

    std::string name = read_string(ptr);

    // Remaining data is ABC
    size_t abc_size = tag_size - (ptr - tag_data);

    // Parse ABC
    ABCFile* abc = parse_abc(ptr, abc_size);

    // Store for later code generation
    swf->abc_files.push_back(abc);
}
```

---

## Next Steps

After completing the ABC parser:

1. **Verify parsing** - Successfully parse Seedling.swf and extract all ABC data
2. **Print structures** - Implement debug output to inspect parsed data
3. **Move to Phase 2** - Start code generator to convert ABC → C code

The ABC parser is the foundation for everything else. Take time to test it thoroughly before proceeding!
