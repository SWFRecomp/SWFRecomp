# Phase 1: Basic Font Support - Implementation Guide

## Overview

This document provides step-by-step instructions for implementing Phase 1 of font support in SWFRecomp. This phase focuses on parsing and storing font metadata, specifically implementing DefineFontInfo (Tag 13) parsing and creating the infrastructure to store font information.

**Goal:** Enable character-to-glyph mapping by parsing DefineFontInfo tags and storing font metadata.

**Estimated Time:** 2-3 days

**Prerequisites:** Familiarity with C++, SWF tag parsing, and the existing SWFRecomp codebase.

---

## Table of Contents

1. [Current State Analysis](#current-state-analysis)
2. [Phase 1 Objectives](#phase-1-objectives)
3. [Step-by-Step Implementation](#step-by-step-implementation)
4. [Testing Strategy](#testing-strategy)
5. [Validation Checklist](#validation-checklist)
6. [Troubleshooting](#troubleshooting)

---

## Current State Analysis

### What We Have Now

**DefineFont (Tag 10) Parsing** - `src/swf.cpp:688-735`

```cpp
case SWF_TAG_DEFINE_FONT:
{
    tag.clearFields();
    tag.setFieldCount(1);
    tag.configureNextField(SWF_FIELD_UI16);
    tag.parseFields(cur_pos);

    u16 font_id = (u16) tag.fields[0].value;  // Parsed but discarded!

    char* offset_table = cur_pos;

    // Parse offset table
    tag.clearFields();
    tag.setFieldCount(1);
    tag.configureNextField(SWF_FIELD_UI16);
    tag.parseFields(cur_pos);

    std::vector<u16> entry_offsets;
    entry_offsets.push_back((u16) tag.fields[0].value);

    u16 num_entries = entry_offsets.back()/2;  // Number of glyphs

    // Parse remaining offsets
    tag.clearFields();
    tag.setFieldCount(num_entries - 1);
    for (u16 i = 0; i < num_entries - 1; ++i) {
        tag.configureNextField(SWF_FIELD_UI16);
    }
    tag.parseFields(cur_pos);

    for (u16 i = 0; i < num_entries - 1; ++i) {
        entry_offsets.push_back((u16) tag.fields[i].value);
    }

    // Parse each glyph shape
    for (u16 i = 0; i < num_entries; ++i) {
        cur_pos = offset_table + entry_offsets[i];
        interpretShape(context, tag);  // Processes glyph but doesn't store it!
    }

    break;
}
```

**Problems:**
1. ✗ Font ID is parsed but immediately discarded (line 697)
2. ✗ Glyph shapes are interpreted but not stored anywhere
3. ✗ No data structure to hold font information
4. ✗ No way to retrieve glyphs later

**DefineFontInfo (Tag 13)** - `src/swf.cpp:756-761`

```cpp
case SWF_TAG_DEFINE_FONT_INFO:
{
    cur_pos += tag.length;  // SKIPS ENTIRE TAG!
    break;
}
```

**Problem:** Completely unimplemented. This is the critical missing piece.

### Existing Patterns to Follow

**Pattern 1: Character ID Storage**

The codebase already has a pattern for storing character ID mappings (line 181, 620):

```cpp
// In swf.hpp:181
std::unordered_map<u16, size_t> char_id_to_bitmap_id;

// In swf.cpp:620
char_id_to_bitmap_id[char_id] = current_bitmap;
```

We'll follow this pattern for fonts.

**Pattern 2: Tag Field Parsing**

Example from DefineFont (lines 690-695):

```cpp
tag.clearFields();                          // Reset field state
tag.setFieldCount(1);                       // Number of fields to parse
tag.configureNextField(SWF_FIELD_UI16);     // Field type
tag.parseFields(cur_pos);                   // Parse and advance cur_pos
u16 value = (u16) tag.fields[0].value;      // Extract value
```

**Pattern 3: Variable-Length Data**

For parsing character codes, we need to handle variable-length arrays. See how glyph offsets are parsed (lines 713-726):

```cpp
tag.clearFields();
tag.setFieldCount(num_entries - 1);
for (u16 i = 0; i < num_entries - 1; ++i) {
    tag.configureNextField(SWF_FIELD_UI16);
}
tag.parseFields(cur_pos);

for (u16 i = 0; i < num_entries - 1; ++i) {
    entry_offsets.push_back((u16) tag.fields[i].value);
}
```

---

## Phase 1 Objectives

### Primary Goals

1. **Create Font Data Structure** - Store font metadata
2. **Store Font Glyphs** - Preserve glyph shapes from DefineFont
3. **Parse DefineFontInfo** - Extract character code mappings
4. **Build Character-to-Glyph Map** - Enable fast lookup

### Success Criteria

- ✅ Font data structures defined in `swf.hpp`
- ✅ DefineFont stores glyphs in Font object
- ✅ DefineFontInfo parses all fields correctly
- ✅ Character codes map to glyph indices
- ✅ Font can be retrieved by font ID
- ✅ Debug output shows parsed font information
- ✅ Test SWF with fonts parses without errors

---

## Step-by-Step Implementation

### Step 1: Add Font Data Structures to `include/swf.hpp`

**Location:** After the `LineStyle` struct (around line 130)

**Add the following structures:**

```cpp
// Font glyph structure - stores individual glyph shapes
struct FontGlyph
{
    std::vector<Tri> triangles;     // Triangulated glyph shape
    size_t first_tri_index;         // Index into global shape_data array
    size_t tri_count;               // Number of triangles for this glyph
    size_t color_index;             // Index into color_data array
};

// Font structure - stores complete font information
struct Font
{
    // Basic identification
    u16 font_id;
    std::string font_name;

    // Font flags from DefineFontInfo
    bool is_bold;
    bool is_italic;
    bool is_small_text;        // Anti-aliasing hint
    bool is_shift_jis;         // Japanese encoding
    bool is_ansi;              // ANSI encoding
    bool is_wide_codes;        // True = 16-bit codes, False = 8-bit codes

    // Glyph data from DefineFont
    std::vector<FontGlyph> glyphs;
    u16 num_glyphs;

    // Character code mapping from DefineFontInfo
    std::vector<u16> character_codes;     // character_codes[glyph_index] = char_code
    std::unordered_map<u16, size_t> char_to_glyph;  // char_code -> glyph_index

    // Status flags
    bool has_glyph_data;       // DefineFont has been parsed
    bool has_info_data;        // DefineFontInfo has been parsed

    // Constructor
    Font() : font_id(0), is_bold(false), is_italic(false),
             is_small_text(false), is_shift_jis(false), is_ansi(false),
             is_wide_codes(false), num_glyphs(0),
             has_glyph_data(false), has_info_data(false) {}
};
```

**Explanation:**
- `FontGlyph` stores the triangulated shape data for each glyph
- `Font` stores complete font metadata and glyph mappings
- `character_codes` provides glyph_index → char_code mapping
- `char_to_glyph` provides fast char_code → glyph_index lookup
- Status flags track whether DefineFont and DefineFontInfo have been parsed

### Step 2: Add Font Storage to SWF Class

**Location:** In the `SWF` class definition (around line 181, after `char_id_to_bitmap_id`)

**Add:**

```cpp
// Font storage
std::unordered_map<u16, Font*> fonts;           // font_id -> Font object
Font* current_font;                              // Font currently being parsed
```

**Explanation:**
- `fonts` map stores all fonts indexed by font ID
- `current_font` is a temporary pointer used during DefineFont parsing

### Step 3: Initialize Font Storage in Constructor

**Location:** `src/swf.cpp` - Find the `SWF::SWF(Context& context)` constructor

**Add after other initializations:**

```cpp
current_font = nullptr;
```

**Full context - find this section and add the line:**

```cpp
SWF::SWF(Context& context)
{
    // ... existing initialization code ...

    current_font = nullptr;  // ADD THIS LINE

    // ... rest of constructor ...
}
```

### Step 4: Modify DefineFont to Store Glyphs

**Location:** `src/swf.cpp:688-735` - Replace the entire DefineFont case

**Current code:**
```cpp
case SWF_TAG_DEFINE_FONT:
{
    // ... existing parsing code ...
    u16 font_id = (u16) tag.fields[0].value;
    // ... glyph parsing ...
    break;
}
```

**Replace with:**

```cpp
case SWF_TAG_DEFINE_FONT:
{
    tag.clearFields();
    tag.setFieldCount(1);
    tag.configureNextField(SWF_FIELD_UI16);
    tag.parseFields(cur_pos);

    u16 font_id = (u16) tag.fields[0].value;

    printf("DefineFont: font_id=%d\n", font_id);

    // Create or get existing font
    if (fonts.find(font_id) == fonts.end()) {
        fonts[font_id] = new Font();
        fonts[font_id]->font_id = font_id;
    }
    current_font = fonts[font_id];
    current_font->has_glyph_data = true;

    char* offset_table = cur_pos;

    // Parse offset table
    tag.clearFields();
    tag.setFieldCount(1);
    tag.configureNextField(SWF_FIELD_UI16);
    tag.parseFields(cur_pos);

    std::vector<u16> entry_offsets;
    entry_offsets.push_back((u16) tag.fields[0].value);

    u16 num_entries = entry_offsets.back()/2;

    printf("  Number of glyphs: %d\n", num_entries);

    current_font->num_glyphs = num_entries;
    current_font->glyphs.resize(num_entries);

    // Parse remaining offsets
    tag.clearFields();
    tag.setFieldCount(num_entries - 1);
    for (u16 i = 0; i < num_entries - 1; ++i) {
        tag.configureNextField(SWF_FIELD_UI16);
    }
    tag.parseFields(cur_pos);

    for (u16 i = 0; i < num_entries - 1; ++i) {
        entry_offsets.push_back((u16) tag.fields[i].value);
    }

    // Parse each glyph shape
    for (u16 i = 0; i < num_entries; ++i) {
        cur_pos = offset_table + entry_offsets[i];

        // Store current triangle index before parsing
        current_font->glyphs[i].first_tri_index = current_tri;
        current_font->glyphs[i].color_index = current_color;

        size_t tri_before = current_tri;

        interpretShape(context, tag);

        // Calculate how many triangles were added
        current_font->glyphs[i].tri_count = current_tri - tri_before;

        printf("  Glyph %d: %zu triangles, starts at %zu\n",
               i, current_font->glyphs[i].tri_count,
               current_font->glyphs[i].first_tri_index);
    }

    current_font = nullptr;  // Done parsing this font's glyphs

    break;
}
```

**Key changes:**
1. Creates Font object if it doesn't exist
2. Sets `current_font` pointer for interpretShape to use
3. Resizes glyphs vector to hold all glyphs
4. Stores triangle indices for each glyph
5. Adds debug output
6. Clears `current_font` after parsing

### Step 5: Implement DefineFontInfo Parsing

**Location:** `src/swf.cpp:756-761` - Replace the DefineFontInfo case

**Current code:**
```cpp
case SWF_TAG_DEFINE_FONT_INFO:
{
    cur_pos += tag.length;
    break;
}
```

**Replace with:**

```cpp
case SWF_TAG_DEFINE_FONT_INFO:
{
    // Parse Font ID
    tag.clearFields();
    tag.setFieldCount(1);
    tag.configureNextField(SWF_FIELD_UI16);
    tag.parseFields(cur_pos);

    u16 font_id = (u16) tag.fields[0].value;

    printf("DefineFontInfo: font_id=%d\n", font_id);

    // Get or create font
    if (fonts.find(font_id) == fonts.end()) {
        fonts[font_id] = new Font();
        fonts[font_id]->font_id = font_id;
        printf("  Warning: DefineFontInfo before DefineFont!\n");
    }

    Font* font = fonts[font_id];

    // Parse font name length
    tag.clearFields();
    tag.setFieldCount(1);
    tag.configureNextField(SWF_FIELD_UI8);
    tag.parseFields(cur_pos);

    u8 font_name_len = (u8) tag.fields[0].value;

    printf("  Font name length: %d\n", font_name_len);

    // Parse font name (not null-terminated!)
    font->font_name.clear();
    for (u8 i = 0; i < font_name_len; ++i) {
        font->font_name += *cur_pos;
        cur_pos++;
    }

    printf("  Font name: '%s'\n", font->font_name.c_str());

    // Parse font flags
    tag.clearFields();
    tag.setFieldCount(1);
    tag.configureNextField(SWF_FIELD_UI8);
    tag.parseFields(cur_pos);

    u8 flags = (u8) tag.fields[0].value;

    // Extract flag bits (SWF spec lists UB fields from MSB to LSB)
    font->is_wide_codes  = (flags & 0b00000001) != 0;  // Bit 0
    font->is_bold        = (flags & 0b00000010) != 0;  // Bit 1
    font->is_italic      = (flags & 0b00000100) != 0;  // Bit 2
    font->is_ansi        = (flags & 0b00001000) != 0;  // Bit 3
    font->is_shift_jis   = (flags & 0b00010000) != 0;  // Bit 4
    font->is_small_text  = (flags & 0b00100000) != 0;  // Bit 5
    // Bits 6-7 reserved

    printf("  Flags: wide_codes=%d, shift_jis=%d, ansi=%d, italic=%d, bold=%d, small=%d\n",
           font->is_wide_codes, font->is_shift_jis, font->is_ansi,
           font->is_italic, font->is_bold, font->is_small_text);

    // Determine number of character codes
    // Should match number of glyphs from DefineFont
    u16 num_codes = font->num_glyphs;

    if (num_codes == 0) {
        printf("  Warning: No glyphs defined yet, cannot determine code count\n");
        // We'll have to guess or skip
        break;
    }

    printf("  Parsing %d character codes (%d-bit)\n",
           num_codes, font->is_wide_codes ? 16 : 8);

    // Parse character codes
    font->character_codes.clear();
    font->character_codes.reserve(num_codes);

    if (font->is_wide_codes) {
        // 16-bit character codes (Unicode)
        tag.clearFields();
        tag.setFieldCount(num_codes);
        for (u16 i = 0; i < num_codes; ++i) {
            tag.configureNextField(SWF_FIELD_UI16);
        }
        tag.parseFields(cur_pos);

        for (u16 i = 0; i < num_codes; ++i) {
            u16 char_code = (u16) tag.fields[i].value;
            font->character_codes.push_back(char_code);
            font->char_to_glyph[char_code] = i;

            // Print printable ASCII characters
            if (char_code >= 32 && char_code < 127) {
                printf("  Code[%d] = %d ('%c') -> glyph %d\n",
                       i, char_code, (char)char_code, i);
            } else {
                printf("  Code[%d] = %d (0x%04X) -> glyph %d\n",
                       i, char_code, char_code, i);
            }
        }
    } else {
        // 8-bit character codes (ANSI or Shift-JIS)
        tag.clearFields();
        tag.setFieldCount(num_codes);
        for (u16 i = 0; i < num_codes; ++i) {
            tag.configureNextField(SWF_FIELD_UI8);
        }
        tag.parseFields(cur_pos);

        for (u16 i = 0; i < num_codes; ++i) {
            u16 char_code = (u16) tag.fields[i].value;  // Store as u16 for consistency
            font->character_codes.push_back(char_code);
            font->char_to_glyph[char_code] = i;

            // Print printable ASCII characters
            if (char_code >= 32 && char_code < 127) {
                printf("  Code[%d] = %d ('%c') -> glyph %d\n",
                       i, char_code, (char)char_code, i);
            } else {
                printf("  Code[%d] = %d (0x%02X) -> glyph %d\n",
                       i, char_code, char_code, i);
            }
        }
    }

    font->has_info_data = true;

    printf("  DefineFontInfo parsing complete: %zu character codes mapped\n",
           font->character_codes.size());

    break;
}
```

**Key features:**
1. Parses font ID to find corresponding Font object
2. Parses font name (variable length, not null-terminated)
3. Parses and decodes font flags
4. Handles both 8-bit and 16-bit character codes
5. Builds bidirectional character-to-glyph mapping
6. Extensive debug output for verification
7. Validates glyph count matches

**Flag bit layout (DefineFontInfo):**
```
Bit 0: WideCodes    (0 = 8-bit codes, 1 = 16-bit codes)
Bit 1: Bold
Bit 2: Italic
Bit 3: ANSI         (ANSI encoding)
Bit 4: ShiftJIS     (Japanese encoding)
Bit 5: SmallText    (anti-aliasing hint)
Bit 6: Reserved
Bit 7: Reserved
```

### Step 6: Add Font Cleanup to Destructor

**Location:** Find the `SWF` destructor in `src/swf.cpp`

**Add font cleanup code:**

```cpp
SWF::~SWF()
{
    // ... existing cleanup code ...

    // Clean up fonts
    for (auto& pair : fonts) {
        delete pair.second;
    }
    fonts.clear();
}
```

**Note:** If there's no destructor yet, add it to `swf.hpp` and `swf.cpp`:

```cpp
// In swf.hpp (add to SWF class):
~SWF();

// In swf.cpp (add implementation):
SWF::~SWF()
{
    // Clean up fonts
    for (auto& pair : fonts) {
        delete pair.second;
    }
    fonts.clear();
}
```

### Step 7: Add Font Information Output

**Location:** End of `parseAllTags()` in `src/swf.cpp`

**Add summary output before the function returns:**

```cpp
// At the end of SWF::parseAllTags(), before final return

printf("\n=== Font Summary ===\n");
printf("Total fonts loaded: %zu\n", fonts.size());

for (auto& pair : fonts) {
    Font* font = pair.second;
    printf("\nFont ID %d:\n", font->font_id);
    printf("  Name: %s\n", font->font_name.empty() ? "(unnamed)" : font->font_name.c_str());
    printf("  Style: %s%s\n", font->is_bold ? "Bold " : "", font->is_italic ? "Italic" : "");
    printf("  Encoding: %s\n",
           font->is_wide_codes ? "Unicode (16-bit)" :
           font->is_shift_jis ? "Shift-JIS (8-bit)" :
           font->is_ansi ? "ANSI (8-bit)" : "Unknown");
    printf("  Glyphs: %d\n", font->num_glyphs);
    printf("  Character codes: %zu\n", font->character_codes.size());
    printf("  Has glyph data: %s\n", font->has_glyph_data ? "Yes" : "No");
    printf("  Has info data: %s\n", font->has_info_data ? "Yes" : "No");

    if (!font->has_glyph_data) {
        printf("  WARNING: No glyph data (missing DefineFont)\n");
    }
    if (!font->has_info_data) {
        printf("  WARNING: No character mapping (missing DefineFontInfo)\n");
    }
    if (font->has_glyph_data && font->has_info_data) {
        if (font->num_glyphs != font->character_codes.size()) {
            printf("  ERROR: Glyph count mismatch! %d glyphs but %zu codes\n",
                   font->num_glyphs, font->character_codes.size());
        }
    }
}
printf("\n");
```

This provides a complete summary of all fonts parsed, useful for validation.

---

## Testing Strategy

### Test 1: Create Simple Font SWF

**Objective:** Verify basic parsing works

**Method:**
1. Create a simple SWF file with DefineFont + DefineFontInfo tags
2. Use a font with ASCII characters (A-Z, a-z, 0-9)
3. Run SWFRecomp on the test file
4. Check console output for correct parsing

**Expected Output:**
```
DefineFont: font_id=1
  Number of glyphs: 62
  Glyph 0: 12 triangles, starts at 0
  ...
DefineFontInfo: font_id=1
  Font name length: 5
  Font name: 'Arial'
  Flags: wide_codes=0, shift_jis=0, ansi=1, italic=0, bold=0, small=0
  Parsing 62 character codes (8-bit)
  Code[0] = 65 ('A') -> glyph 0
  Code[1] = 66 ('B') -> glyph 1
  ...

=== Font Summary ===
Total fonts loaded: 1

Font ID 1:
  Name: Arial
  Style:
  Encoding: ANSI (8-bit)
  Glyphs: 62
  Character codes: 62
  Has glyph data: Yes
  Has info data: Yes
```

### Test 2: Multiple Fonts

**Objective:** Verify multiple font handling

**Method:**
1. SWF with 2+ fonts (different IDs)
2. Each with DefineFont + DefineFontInfo
3. Verify each font is stored separately

**Expected:** Separate entries in font summary for each font ID

### Test 3: Unicode Font

**Objective:** Verify wide character code support

**Method:**
1. Create font with Unicode characters (Chinese, Japanese, etc.)
2. DefineFontInfo should have wide_codes=1
3. Verify 16-bit character codes parse correctly

**Expected:**
```
  Flags: wide_codes=1, ...
  Parsing N character codes (16-bit)
  Code[0] = 12354 (0x3042) -> glyph 0  // Japanese Hiragana
```

### Test 4: Edge Cases

**Test A: DefineFontInfo before DefineFont**
- SWF with tags in wrong order
- Should print warning but not crash
- Font created with info data, glyph data added later

**Test B: Missing DefineFontInfo**
- DefineFont only, no DefineFontInfo
- Should parse glyphs, show warning in summary
- has_info_data = false

**Test C: Missing DefineFont**
- DefineFontInfo only, no DefineFont
- Should parse info, show warning in summary
- has_glyph_data = false, num_glyphs = 0

### Test 5: Memory Leak Check

**Method:**
```bash
valgrind --leak-check=full ./SWFRecomp test_font.swf
```

**Expected:** No memory leaks from Font objects

---

## Validation Checklist

### Code Compilation

- [ ] `include/swf.hpp` compiles without errors
- [ ] `src/swf.cpp` compiles without errors
- [ ] No compiler warnings about unused variables
- [ ] Linker successfully creates executable

### Runtime Behavior

- [ ] Program runs without crashing
- [ ] DefineFont debug output appears
- [ ] DefineFontInfo debug output appears
- [ ] Font Summary appears at end
- [ ] Character code mappings are printed

### Data Validation

- [ ] Font ID matches between DefineFont and DefineFontInfo
- [ ] Glyph count matches character code count
- [ ] Character codes are reasonable (ASCII printable for ANSI fonts)
- [ ] Font name is correct (not garbage)
- [ ] Font flags decode correctly

### Edge Cases

- [ ] Multiple fonts handled correctly
- [ ] Unicode fonts (wide_codes) work
- [ ] DefineFontInfo before DefineFont doesn't crash
- [ ] Missing DefineFontInfo shows warning
- [ ] Missing DefineFont shows warning

### Memory Management

- [ ] No memory leaks (valgrind clean)
- [ ] Fonts deleted in destructor
- [ ] No dangling pointers
- [ ] No double-free errors

---

## Troubleshooting

### Problem: "Font ID mismatch"

**Symptom:** DefineFontInfo references non-existent font ID

**Possible Causes:**
1. DefineFont parsing failed before DefineFontInfo
2. Font ID parsed incorrectly
3. Tags out of order

**Debug Steps:**
1. Add breakpoint in DefineFont case
2. Verify font_id value
3. Check `fonts` map contents
4. Verify tag order in SWF file

**Solution:** Ensure DefineFont creates font in map before DefineFontInfo references it

### Problem: "Glyph count mismatch"

**Symptom:** num_glyphs != character_codes.size()

**Possible Causes:**
1. Character code array parsed incorrectly
2. Number of codes calculation wrong
3. Wide vs narrow codes confusion

**Debug Steps:**
1. Print num_glyphs value
2. Print character_codes.size()
3. Verify is_wide_codes flag
4. Check if parseFields reading correct amount

**Solution:**
- Verify num_codes = font->num_glyphs
- Check is_wide_codes flag is correct
- Ensure UI8 vs UI16 field type matches flag

### Problem: "Garbage font name"

**Symptom:** Font name contains non-printable characters

**Possible Causes:**
1. Font name length incorrect
2. Reading past font name data
3. cur_pos not advanced correctly

**Debug Steps:**
1. Print font_name_len value
2. Print each character as it's read (hex value)
3. Verify cur_pos advances by font_name_len

**Solution:** Font name is NOT null-terminated, must read exactly font_name_len bytes

### Problem: "Character codes are wrong"

**Symptom:** Character codes don't match expected values

**Possible Causes:**
1. Wide/narrow confusion (reading 8-bit as 16-bit or vice versa)
2. Byte order issues (endianness)
3. Bit field alignment issues from previous tag

**Debug Steps:**
1. Verify is_wide_codes flag
2. Print raw bytes before parsing
3. Check if previous tag was bit-aligned

**Solution:**
- Ensure UI8 vs UI16 matches is_wide_codes
- parseFields handles endianness automatically
- clearFields() resets bit alignment

### Problem: Crash in interpretShape()

**Symptom:** Segfault when parsing glyph shapes

**Possible Causes:**
1. cur_pos pointing to invalid memory
2. offset_table calculation wrong
3. entry_offsets[] out of bounds

**Debug Steps:**
1. Print offset_table address
2. Print each entry_offsets[i] value
3. Verify i < num_entries

**Solution:**
- Ensure offset_table = cur_pos AFTER reading font_id
- Verify entry_offsets.size() == num_entries
- Check tag.length to ensure enough data

### Problem: Font summary shows wrong data

**Symptom:** Summary statistics don't match console output

**Possible Causes:**
1. Font object not stored correctly in map
2. Multiple fonts overwriting each other
3. Pointer arithmetic error

**Debug Steps:**
1. Print fonts.size() after each DefineFont
2. Verify font pointer = fonts[font_id]
3. Check no duplicate font_id values

**Solution:**
- Use fonts[font_id] consistently
- Don't create new Font if one exists
- Verify current_font points to correct object

---

## Next Steps After Phase 1

Once Phase 1 is complete and validated:

1. **Phase 2 Planning:** Review FONT_IMPLEMENTATION_ANALYSIS.md Phase 2
2. **DefineText Implementation:** Begin static text support
3. **Text Layout Engine:** Design and implement glyph positioning
4. **Runtime Integration:** Add font rendering to SWFModernRuntime

But for now, focus on getting Phase 1 working correctly. The character-to-glyph mapping is the foundation for everything else.

---

## Reference: DefineFontInfo Tag Structure (SWF Specification)

```
DefineFontInfo (Tag 13)
┌─────────────────┬────────────────────────────────────┐
│ Field           │ Type                               │
├─────────────────┼────────────────────────────────────┤
│ Header          │ RECORDHEADER                       │
│ FontID          │ UI16                               │
│ FontNameLen     │ UI8                                │
│ FontName        │ UI8[FontNameLen] (not null-term)   │
│ FontFlags       │ UI8 (see bit layout below)         │
│ CodeTable       │ If WideCodes: UI16[NumGlyphs]     │
│                 │ Else:         UI8[NumGlyphs]       │
└─────────────────┴────────────────────────────────────┘

FontFlags (UI8):
  Bit 0: WideCodes    (1 = UI16, 0 = UI8)
  Bit 1: Bold
  Bit 2: Italic
  Bit 3: ANSI         (ANSI encoding)
  Bit 4: ShiftJIS     (Japanese encoding)
  Bit 5: SmallText    (anti-aliasing hint)
  Bit 6: Reserved
  Bit 7: Reserved

Note: NumGlyphs comes from corresponding DefineFont tag
      CodeTable[i] = character code for glyph i
```

---

## Document Information

**Version:** 1.1
**Date:** 2025-10-31
**Phase:** 1 of 4
**Status:** Implementation Ready
**Estimated Effort:** 2-3 days
**Revision Notes:** Fixed FontFlags bit ordering (v1.1) - corrected bits 1-4 to match SWF spec

**Files Modified:**
- `include/swf.hpp` - Add Font structures
- `src/swf.cpp` - Modify DefineFont, implement DefineFontInfo

**Files to Test:**
- Simple ASCII font SWF
- Unicode font SWF
- Multiple fonts SWF
- Edge case SWFs

---

## Conclusion

This implementation guide provides everything needed to implement Phase 1 font support. Follow the steps carefully, validate at each stage, and use the debug output to verify correct behavior.

The key insight is that DefineFont and DefineFontInfo are two parts of the same puzzle:
- **DefineFont** provides the glyph shapes
- **DefineFontInfo** provides the character mappings

Together, they enable text rendering. This phase creates the infrastructure to store and retrieve this information, which is essential for all future font work.

Good luck with the implementation!
