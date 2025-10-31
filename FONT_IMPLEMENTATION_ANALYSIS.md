# Font Implementation Analysis for SWFRecomp

## Executive Summary

This document provides a comprehensive analysis of the current state of font support in SWFRecomp and outlines the work required to achieve full font and text rendering functionality.

**Current Status:** Basic glyph shape parsing only
**Completion:** ~15% of full font support
**Critical Gap:** No character-to-glyph mapping (DefineFontInfo not implemented)

---

## Table of Contents

1. [Current Implementation](#current-implementation)
2. [Missing Components](#missing-components)
3. [SWF Font System Overview](#swf-font-system-overview)
4. [Implementation Roadmap](#implementation-roadmap)
5. [Technical Specifications](#technical-specifications)
6. [Code Locations](#code-locations)

---

## Current Implementation

### What Works Now

**DefineFont (Tag 10) Parsing** - `src/swf.cpp:688-735`

The current implementation successfully:
- ✅ Parses the font ID from the tag
- ✅ Reads the glyph offset table
- ✅ Calculates the number of glyphs (num_entries)
- ✅ Extracts individual glyph shapes
- ✅ Processes each glyph through `interpretShape()`
- ✅ Renders glyphs as vector shapes with white fill

**Glyph Shape Interpretation** - `src/swf.cpp:1141-1235`

When processing font glyphs:
- Sets `is_font = true` flag
- Creates a simple white fill style (RGB: 0xFF, 0xFF, 0xFF)
- Uses simplified rendering path (no line styles)
- Stores glyph geometry as triangulated shapes

### What Doesn't Work

**DefineFontInfo (Tag 13)** - `src/swf.cpp:756-761`
```cpp
case SWF_TAG_DEFINE_FONT_INFO:
{
    cur_pos += tag.length;  // Just skips the entire tag!
    break;
}
```

This is **critically broken** because:
- No character code mappings are stored
- No font metadata is preserved
- Cannot determine which glyph represents which character
- Makes text rendering impossible

**Font Storage:**
- Font ID is parsed but immediately discarded (line 697)
- No data structure exists to store font information
- No registry to look up fonts by ID
- Glyphs are rendered but not associated with any font

**Text Rendering:**
- No DefineText/DefineText2 support
- No DefineEditText support
- No text layout engine
- No glyph positioning logic

---

## Missing Components

### 1. DefineFontInfo Implementation (CRITICAL PRIORITY)

**Tag 13 Structure:**
```
UI16    FontID           // Which font this info applies to
UI8     FontNameLen      // Length of font name
UI8[]   FontName         // Font name string (NOT null-terminated!)
UI8     FontFlags        // Bit flags (see below)
UI16[]  CodeTable        // Character codes for each glyph (if WideCodes=1)
UI8[]   CodeTable        // Character codes for each glyph (if WideCodes=0)
```

**Important:** The font name is NOT null-terminated and must be parsed byte-by-byte. The existing `SWF_FIELD_STRING` field type is defined but not implemented, so manual parsing is required.

**Font Flags (Byte Layout):**

The SWF spec defines FontFlags as a series of UB (unsigned bit) fields read MSB-first. The byte is structured as:
- UB[2]: Reserved (bits 6-7, MSB)
- UB[1]: SmallText - Anti-aliasing hint (bit 5)
- UB[1]: ShiftJIS - Japanese encoding (bit 4)
- UB[1]: ANSI - ANSI encoding (bit 3)
- UB[1]: Italic (bit 2)
- UB[1]: Bold (bit 1)
- UB[1]: WideCodes - If 1, CodeTable is UI16[]; if 0, UI8[] (bit 0, LSB)

When reading the flags byte directly (LSB to MSB, bit 0 to 7):
- Bit 0 (0x01): WideCodes (1 = 16-bit codes, 0 = 8-bit codes)
- Bit 1 (0x02): Bold
- Bit 2 (0x04): Italic
- Bit 3 (0x08): ANSI encoding
- Bit 4 (0x10): ShiftJIS encoding (Japanese)
- Bit 5 (0x20): SmallText (anti-aliasing hint, SWF 7+)
- Bits 6-7 (0xC0): Reserved

**Note:** The "Has layout info" flag only appears in DefineFont2, not DefineFontInfo.

**Why This Is Critical:**

Without DefineFontInfo, you have glyphs but don't know:
- Which glyph represents 'A' vs 'B' vs '1'
- Font name for debugging/logging
- Style information (bold, italic)
- Character encoding (Unicode vs ANSI)

### 2. Additional Font Tag Support

#### DefineFont2 (Tag 48) - Enhanced Font Format

Combines DefineFont + DefineFontInfo + additional metrics in one tag:

```
UI16    FontID
UI8     FontFlags        // Has layout, ShiftJIS, small, ANSI, wide codes, italic, bold
UI8     LanguageCode     // 1=Latin, 2=Japanese, 3=Korean, etc.
UI8     FontNameLen
UI8[]   FontName
UI16    NumGlyphs
UI32[]  OffsetTable      // Offsets to each glyph shape
UI16    CodeTableOffset  // Offset to code table
SHAPE[] GlyphShapeTable  // Shape for each glyph
UI16[]  CodeTable        // Character code for each glyph

// If HasLayout flag is set:
SI16    FontAscent       // Ascender height in EM units
SI16    FontDescent      // Descender depth
SI16    FontLeading      // Line spacing
SI16[]  FontAdvanceTable // Advance width for each glyph
RECT[]  FontBoundsTable  // Bounding box for each glyph
UI16    KerningCount
KERNINGRECORD[] KerningTable
```

**Advantages over DefineFont + DefineFontInfo:**
- All data in one place (easier to parse)
- Includes font metrics (ascent, descent, leading)
- Includes advance widths (proper spacing)
- Includes bounding boxes (accurate positioning)
- Includes kerning pairs (better typography)

#### DefineFont3 (Tag 75) - Enhanced DefineFont2

Same structure as DefineFont2 but with:
- Always uses 32-bit offsets (supports more glyphs)
- Always uses wide (16-bit) character codes
- Better support for Unicode and large character sets

#### DefineFont4 (Tag 91) - Embedded OpenType/CFF Fonts

**Introduced in:** SWF 10 (Flash Player 10+)
**Purpose:** Supports the Flash Text Engine with OpenType CFF font embedding

```
UI16    FontID              // ID for this font character
UB[5]   FontFlagsReserved   // Reserved bits
UB[1]   FontFlagsHasFontData // Font is embedded (includes SFNT data)
UB[1]   FontFlagsItalic     // Italic font
UB[1]   FontFlagsBold       // Bold font
STRING  FontName            // Name of the font (null-terminated)
FONTDATA FontData            // OpenType CFF font data (optional, based on HasFontData flag)
```

**FontData Structure:**
When present, contains a complete OpenType CFF font as defined in the OpenType specification. Required tables:
- **Required:** 'CFF ', 'cmap', 'head', 'maxp', 'OS/2', 'post'
- **Required (either):** ('hhea' and 'hmtx') OR ('vhea', 'vmtx', and 'VORG')
- **Optional:** 'GSUB', 'GPOS', 'GDEF', 'BASE'

The 'cmap' table must include one of these Unicode subtables:
- (platform 0, encoding 4)
- (platform 0, encoding 3)
- (platform 3, encoding 10)
- (platform 3, encoding 1)
- (platform 3, encoding 0)

**Key Differences from DefineFont/DefineFont2/DefineFont3:**
- Uses CFF (Compact Font Format) instead of SWF shape definitions
- Embeds complete OpenType font files
- Designed specifically for Flash Text Engine (not classic TextField)
- More efficient for complex fonts with many glyphs
- Supports advanced typography features (ligatures, kerning via GPOS, etc.)

#### DefineFontInfo2 (Tag 62) - Extended Font Info

Similar to DefineFontInfo (tag 13) but adds:
- Language code field (0=Latin, 1=Japanese, 2=Korean, 3=Simplified Chinese, 4=Traditional Chinese)
- Otherwise identical structure to DefineFontInfo

```
UI16    FontID
UI8     FontNameLen
UI8[]   FontName
UI8     FontFlags
UI8     LanguageCode     // New in DefineFontInfo2
UI16[]  CodeTable        // or UI8[] depending on WideCodes flag
```

### 3. Text Rendering Tags

#### DefineText (Tag 11) - Static Text

```
UI16    CharacterID      // ID for this text object
RECT    TextBounds       // Bounding rectangle
MATRIX  TextMatrix       // Transformation matrix
UI8     GlyphBits        // Bits per glyph index
UI8     AdvanceBits      // Bits per advance value
TEXTRECORD[] TextRecords // Array of text runs
```

**TEXTRECORD Structure:**
```
UI8     TextRecordType   // Type flags
UI16    FontID           // (if font change)
RGB     TextColor        // (if color change)
SI16    XOffset          // (if position change)
SI16    YOffset          // (if position change)
UI16    TextHeight       // (if font change)
UI8     GlyphCount       // Number of glyphs in this record
GLYPHENTRY[] GlyphEntries
```

**GLYPHENTRY:**
```
UB[GlyphBits]    GlyphIndex    // Index into font's glyph table
SB[AdvanceBits]  GlyphAdvance  // Horizontal advance for this glyph
```

#### DefineText2 (Tag 33) - Static Text with Alpha

Identical to DefineText but uses RGBA instead of RGB for colors.

#### DefineEditText (Tag 37) - Dynamic/Input Text

```
UI16    CharacterID
RECT    Bounds           // Text field boundaries
UI8     Flags1           // HasText, WordWrap, Multiline, Password, etc.
UI8     Flags2           // ReadOnly, HTML, UsesOutlines, etc.
UI16    FontID           // Which font to use
UI16    FontHeight       // Font size in twips
RGBA    TextColor        // Text color
UI16    MaxLength        // Max characters (if HasMaxLength flag)
UI8     Align            // 0=left, 1=right, 2=center, 3=justify
UI16    LeftMargin       // In twips
UI16    RightMargin
UI16    Indent
SI16    Leading          // Line spacing
STRING  VariableName     // ActionScript variable name
STRING  InitialText      // (if HasText flag)
```

**Critical for:**
- User input fields
- Dynamic text that changes via ActionScript
- Variables like score displays, usernames, etc.
- HTML text rendering

### 4. Data Structures (Need to Add to `swf.hpp`)

#### Font Structure

```cpp
struct KerningRecord
{
    u16 left_char;       // First character code
    u16 right_char;      // Second character code
    s16 adjustment;      // Kerning adjustment in EM units
};

struct Font
{
    // Basic info (DefineFont + DefineFontInfo)
    u16 font_id;
    std::string font_name;
    bool is_bold;
    bool is_italic;
    bool is_unicode;      // True if 16-bit codes, false if 8-bit
    bool shift_jis;
    bool small_text;      // Anti-aliasing hint

    // Glyph data
    std::vector<Shape*> glyphs;           // Glyph shapes (already have this)
    std::vector<u16> character_codes;     // Maps glyph index -> character code
    std::unordered_map<u16, size_t> char_to_glyph;  // Fast lookup: char -> glyph index

    // Layout info (DefineFont2/3 only)
    bool has_layout;
    s16 ascent;          // Distance from baseline to top
    s16 descent;         // Distance from baseline to bottom (negative)
    s16 leading;         // Line spacing
    std::vector<s16> advance_widths;      // Advance width per glyph
    std::vector<RECT> glyph_bounds;       // Bounding box per glyph
    std::vector<KerningRecord> kerning_table;

    // Language
    u8 language_code;    // 0=none, 1=Latin, 2=Japanese, etc.
};

struct TextRecord
{
    bool has_font;
    u16 font_id;
    bool has_color;
    u8 r, g, b, a;
    bool has_position;
    s16 x_offset;
    s16 y_offset;
    u16 text_height;     // Font size in twips
    std::vector<u16> glyph_indices;
    std::vector<s16> glyph_advances;
};

struct StaticTextField
{
    u16 character_id;
    RECT bounds;
    MATRIX transform;
    std::vector<TextRecord> text_records;
};

struct DynamicTextField
{
    u16 character_id;
    RECT bounds;
    u16 font_id;
    u16 font_height;
    u8 r, g, b, a;
    bool word_wrap;
    bool multiline;
    bool password;
    bool read_only;
    bool auto_size;
    bool no_select;
    bool border;
    bool html;
    u8 align;            // 0=left, 1=right, 2=center, 3=justify
    u16 left_margin;
    u16 right_margin;
    u16 indent;
    s16 leading;
    std::string variable_name;
    std::string initial_text;
    u16 max_length;
};
```

#### Add to SWF Class

```cpp
class SWF
{
public:
    // ... existing members ...

    // Font storage (follows existing pattern from char_id_to_bitmap_id at line 181)
    std::unordered_map<u16, Font*> fonts;              // font_id -> Font
    std::unordered_map<u16, StaticTextField*> static_text_fields;
    std::unordered_map<u16, DynamicTextField*> dynamic_text_fields;

    // ... existing methods ...

    void parseDefineFont(Context& context, SWFTag& tag);
    void parseDefineFontInfo(Context& context, SWFTag& tag);
    void parseDefineFont2(Context& context, SWFTag& tag);
    void parseDefineFont3(Context& context, SWFTag& tag);
    void parseDefineText(Context& context, SWFTag& tag);
    void parseDefineText2(Context& context, SWFTag& tag);
    void parseDefineEditText(Context& context, SWFTag& tag);
};
```

**Note:** The SWF class currently has no destructor. One must be added to properly clean up Font objects (see Phase 1 implementation guide).

### 5. Text Layout Engine (Needs Implementation)

Core functionality required:

```cpp
class TextLayoutEngine
{
public:
    struct LayoutGlyph
    {
        u16 glyph_index;
        s32 x;           // Position in twips
        s32 y;
        u16 font_id;
        u16 font_size;
        u8 r, g, b, a;
    };

    struct LayoutLine
    {
        std::vector<LayoutGlyph> glyphs;
        s32 baseline_y;
        s32 width;
        s32 height;
    };

    // Layout a text field
    std::vector<LayoutLine> layoutText(
        const std::string& text,
        Font* font,
        u16 font_size,
        RECT bounds,
        u8 align,
        bool word_wrap,
        bool multiline
    );

    // Apply kerning between two glyphs
    s16 getKerningAdjustment(Font* font, u16 left_char, u16 right_char);

    // Get advance width for a character
    s16 getAdvanceWidth(Font* font, u16 char_code, u16 font_size);

    // Convert character to glyph index
    size_t getGlyphIndex(Font* font, u16 char_code);

    // Word wrap calculation
    std::vector<std::string> wrapText(
        const std::string& text,
        Font* font,
        u16 font_size,
        s32 max_width
    );
};
```

**Key Algorithms:**

1. **Character to Glyph Mapping:**
   ```cpp
   size_t glyph_index = font->char_to_glyph[character_code];
   ```

2. **Glyph Positioning:**
   ```cpp
   current_x += (advance_width * font_size) / 1024;  // EM units to twips
   current_x += getKerningAdjustment(font, prev_char, current_char);
   ```

3. **Line Breaking:**
   - Track current line width
   - Break at word boundaries if word_wrap enabled
   - Break at newlines if multiline enabled
   - Apply alignment (left/right/center/justify) per line

4. **Vertical Positioning:**
   ```cpp
   baseline_y = bounds.ymin + font->ascent;
   next_line_y = baseline_y + font_size + font->leading;
   ```

### 6. Runtime Support (SWFModernRuntime)

Currently, the runtime has **zero font-specific code**. Needs:

#### Font Atlas Generation

For efficient GPU rendering:
```cpp
struct FontAtlas
{
    GLuint texture_id;
    int width;
    int height;
    std::unordered_map<u16, GlyphUV> glyph_uvs;
};

struct GlyphUV
{
    float u0, v0;  // Top-left UV
    float u1, v1;  // Bottom-right UV
    float width;   // Glyph width in pixels
    float height;  // Glyph height
};
```

#### Text Rendering Functions

```cpp
void renderStaticText(u16 character_id);
void renderDynamicText(u16 character_id);
void updateDynamicText(u16 character_id, const char* new_text);
void setTextColor(u16 character_id, u8 r, u8 g, u8 b, u8 a);
```

#### Variable Binding

For DefineEditText with variable names:
```cpp
void bindTextVariable(u16 character_id, const char* var_name);
const char* getTextVariable(const char* var_name);
void setTextVariable(const char* var_name, const char* value);
```

---

## SWF Font System Overview

### How Fonts Work in SWF Files

1. **Font Definition Phase:**
   - `DefineFont` or `DefineFont2/3` declares a font and its glyphs
   - `DefineFontInfo` (if separate) provides metadata and character mappings
   - Font is registered by ID for later use

2. **Text Placement Phase:**
   - `DefineText/DefineText2` creates static text display objects
   - `DefineEditText` creates dynamic/input text fields
   - Both reference fonts by ID

3. **Runtime Phase:**
   - Text objects are placed on stage via `PlaceObject2`
   - Dynamic text can be updated via ActionScript
   - Text fields can be edited by user input (if configured)

### Font Glyph Coordinate System

- **EM Square:** Glyphs are defined in a 1024×1024 EM square
- **Baseline:** Y=0 is the baseline (not the bottom of the glyph)
- **Ascent:** Distance from baseline to top of capital letters
- **Descent:** Distance from baseline to bottom of descenders (negative)
- **Advance Width:** Horizontal distance to next glyph position

**Scaling to Twips:**
```
glyph_size_in_twips = (em_units * font_size_in_twips) / 1024
```

### Character Encoding

**SWF supports multiple encodings:**
- **8-bit ANSI:** Characters 0-255 (Western languages)
- **16-bit Unicode:** Full Unicode support
- **Shift-JIS:** Japanese character encoding

The `wide_codes` flag in FontFlags determines 8-bit vs 16-bit.

---

## Implementation Roadmap

### Phase 1: Basic Font Support (Critical)

**Priority: URGENT**
**Estimated Effort:** 2-3 days

**Tasks:**

1. **Implement DefineFontInfo Parsing**
   - Parse font ID, name, flags
   - Parse character code array (8-bit or 16-bit)
   - Create Font data structure
   - Store in fonts registry

2. **Add Font Data Structures**
   - Add `Font` struct to `swf.hpp`
   - Add `fonts` map to `SWF` class
   - Modify `DefineFont` parsing to store glyphs in Font object

3. **Create Character-to-Glyph Mapping**
   - Build `char_to_glyph` map when parsing DefineFontInfo
   - Validate glyph count matches character code count

4. **Testing**
   - Create test SWF with DefineFont + DefineFontInfo
   - Verify correct parsing of character codes
   - Verify font metadata is stored correctly

**Success Criteria:**
- DefineFontInfo data is parsed and stored
- Can look up glyph index by character code
- Font name and style flags are accessible

### Phase 2: Static Text Support

**Priority: HIGH**
**Estimated Effort:** 1 week

**Tasks:**

1. **Implement DefineText/DefineText2 Parsing**
   - Parse text bounds and matrix
   - Parse text records with glyph entries
   - Store in `static_text_fields` map

2. **Build Text Layout Engine (Basic)**
   - Implement glyph positioning from TextRecords
   - Calculate absolute positions based on advances
   - No word wrapping yet (DefineText is pre-laid-out)

3. **Generate Rendering Code**
   - Output glyph positions and transformations
   - Reference font glyph shapes
   - Generate draw calls in `tagMain.c`

4. **Runtime Rendering**
   - Implement `renderStaticText()` in runtime
   - Render glyphs using existing shape rendering
   - Apply text colors and transformations

5. **Testing**
   - Create test SWF with static text
   - Verify text renders correctly
   - Test multiple fonts and colors

**Success Criteria:**
- Static text fields render correctly
- Text color and positioning is accurate
- Multiple text objects work simultaneously

### Phase 3: Enhanced Font Support

**Priority: MEDIUM**
**Estimated Effort:** 1-2 weeks

**Tasks:**

1. **Implement DefineFont2/3 Parsing**
   - Parse complete font definition
   - Parse layout information (ascent, descent, leading)
   - Parse advance widths
   - Parse bounding boxes
   - Parse kerning table

2. **Enhanced Text Layout**
   - Use proper advance widths instead of fixed spacing
   - Apply kerning adjustments
   - Use ascent/descent for vertical positioning
   - Calculate accurate line heights

3. **Font Metrics Validation**
   - Add debugging output for font metrics
   - Verify advance widths match visual spacing
   - Test kerning with character pairs like "AV", "To"

4. **Testing**
   - Create test SWFs with DefineFont2
   - Test fonts with kerning
   - Verify metrics match Flash Player output

**Success Criteria:**
- DefineFont2/3 fonts parse correctly
- Text spacing matches Flash Player exactly
- Kerning is applied correctly
- Line spacing uses proper metrics

### Phase 4: Dynamic Text Support

**Priority: MEDIUM-HIGH**
**Estimated Effort:** 2-3 weeks

**Tasks:**

1. **Implement DefineEditText Parsing**
   - Parse all text field properties
   - Store variable names
   - Store initial text

2. **Text Layout Engine (Advanced)**
   - Implement word wrapping
   - Implement text alignment (left/center/right/justify)
   - Implement multiline layout
   - Implement scrolling regions

3. **Runtime Dynamic Text**
   - Implement `updateDynamicText()`
   - Re-layout text when content changes
   - Implement text field bounds clipping
   - Add scrolling support

4. **Variable Binding**
   - Create variable registry
   - Link text fields to ActionScript variables
   - Update text when variables change
   - Update variables when text is edited

5. **Input Handling (if needed)**
   - Text cursor positioning
   - Character insertion/deletion
   - Selection handling
   - Keyboard event handling

6. **Testing**
   - Test dynamic text updates
   - Test variable binding
   - Test word wrap and alignment
   - Test multiline text fields

**Success Criteria:**
- Dynamic text fields render correctly
- Text updates when ActionScript changes variables
- Word wrap and alignment work correctly
- Input fields accept user input (if implemented)

### Phase 5: Advanced Features (Future)

**Priority: LOW**
**Estimated Effort:** Ongoing

**Tasks:**

- **DefineFont4 (Tag 91)** - Embedded OpenType CFF fonts
  - Requires CFF font parser
  - Requires OpenType table parsing
  - Only needed for Flash Text Engine (FTE) support
  - Most Flash content uses DefineFont2/3, not DefineFont4
  - **Recommendation:** Defer until DefineFont/DefineFont2/DefineFont3 are fully working
- HTML text rendering
- Advanced typography (ligatures via OpenType GSUB, etc.)
- Font subsetting/optimization
- Font fallback chains
- Emoji support
- Vertical text (Asian languages)
- Right-to-left text (Arabic, Hebrew)

---

## Technical Specifications

### Memory Considerations

**Font Storage:**
- Each glyph shape: ~100-500 bytes (depends on complexity)
- Font with 256 glyphs: ~25-125 KB
- Character code array: 256-512 bytes
- Total per font: ~30-150 KB

**Text Fields:**
- Static text: ~50-200 bytes per field
- Dynamic text: ~100-500 bytes per field
- Layout data: ~20 bytes per glyph

### Performance Considerations

**Font Atlas Benefits:**
- Single texture bind per font
- Efficient GPU rendering
- Cache-friendly memory access

**Without Font Atlas:**
- One draw call per glyph
- Slower for large amounts of text
- More CPU overhead

**Recommendation:** Implement font atlas for any project with more than 10 text fields.

### Edge Cases to Handle

1. **Missing Glyphs:**
   - Character code in text but not in font
   - Solution: Use fallback glyph (usually space or '?')

2. **Zero-Width Glyphs:**
   - Some fonts have glyphs with zero advance width
   - Solution: Use minimum advance or skip rendering

3. **Overlapping Glyphs:**
   - Kerning can cause glyphs to overlap
   - Solution: This is intentional, allow overlap

4. **Empty Text Fields:**
   - Text field with no text
   - Solution: Render bounds only (if border enabled)

5. **Unicode Normalization:**
   - Same character, different encodings (é vs e + ´)
   - Solution: Normalize to NFC before lookup

6. **Right-to-Left Text:**
   - Arabic, Hebrew require reversed layout
   - Solution: Detect RTL script, reverse glyph order

---

## SWF Tag Reference

### Font and Text Tag Numbers

Complete list of font and text-related SWF tags with their tag numbers:

| Tag Name | Number | SWF Version | Description |
|----------|--------|-------------|-------------|
| DefineFont | 10 | 1 | Basic font with glyph shapes only |
| DefineText | 11 | 1 | Static text field |
| DefineFontInfo | 13 | 1 | Font metadata and character mapping |
| DefineText2 | 33 | 3 | Static text with RGBA colors |
| DefineEditText | 37 | 4 | Dynamic/input text field |
| DefineFont2 | 48 | 3 | Complete font with metrics and layout |
| DefineFontInfo2 | 62 | 6 | Font metadata with language code |
| DefineFont3 | 75 | 8 | DefineFont2 with 32-bit offsets |
| DefineFont4 | 91 | 10 | Embedded OpenType CFF fonts for Flash Text Engine |

**Notes:**
- DefineFont (10) requires DefineFontInfo (13) or DefineFontInfo2 (62) for character mapping
- DefineFont2 (48) and DefineFont3 (75) include all metadata in one tag
- DefineText2 (33) is identical to DefineText (11) except uses RGBA instead of RGB
- DefineFont4 (91) uses OpenType CFF format instead of SWF shapes, designed for Flash Text Engine
- Tag numbers verified against official SWF specification v19

---

## Code Locations

### Current Font Code

**SWFRecomp:**
- `include/tag.hpp:16-18` - Font tag enum definitions (DefineFont=10, DefineFontInfo=13)
- `src/swf.cpp:688-735` - DefineFont parsing
- `src/swf.cpp:756-761` - DefineFontInfo (currently skipped)
- `src/swf.cpp:1141-1235` - Font glyph shape interpretation (is_font flag, white fill)

**Note:** Text tags (11, 33, 37) and additional font tags (48, 62, 75) need to be added to tag.hpp enum

**SWFModernRuntime:**
- No font-specific code exists

### Where to Add New Code

**Font Data Structures:**
- `include/swf.hpp` - Add Font, TextRecord, TextField structs

**Font Parsing:**
- `src/swf.cpp` - Expand existing DefineFont handler
- `src/swf.cpp` - Implement DefineFontInfo handler
- `src/swf.cpp` - Add DefineFont2/3 handlers
- `src/swf.cpp` - Add DefineText/DefineText2 handlers
- `src/swf.cpp` - Add DefineEditText handler

**Text Layout:**
- New file: `src/text_layout.cpp`
- New header: `include/text_layout.hpp`

**Runtime Rendering:**
- `SWFModernRuntime/src/text_render.c` (new file)
- `SWFModernRuntime/include/text_render.h` (new file)

---

## References

### SWF Specification
- Adobe SWF File Format Specification v19
- Section 8: Fonts and Text (pages 135-167)

### Useful Resources
- [SWF File Format Specification](https://web.archive.org/web/20170623054207/http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf)
- [OpenType Font Specification](https://docs.microsoft.com/en-us/typography/opentype/spec/)
- [Unicode Text Segmentation](https://unicode.org/reports/tr29/)
- [HarfBuzz Text Shaping Library](https://harfbuzz.github.io/)

### Related Code
- `stb_truetype.h` in `lib/stb/` - Can be used as reference for font metrics
- FreeType library - Industry standard for font rendering (could be integrated)

---

## Conclusion

Full font support in SWFRecomp requires significant additional work beyond the current basic glyph parsing. The most critical immediate need is implementing DefineFontInfo parsing to establish character-to-glyph mappings, without which text rendering is impossible.

The roadmap above provides a phased approach that delivers incremental value:
- Phase 1 enables basic font support
- Phase 2 enables static text rendering
- Phase 3 improves text quality
- Phase 4 enables dynamic text and interactivity

Estimated total effort: **6-8 weeks** for full implementation through Phase 4.

---

**Document Version:** 1.0
**Last Updated:** 2025-10-30
**Author:** Analysis by Claude Code
**Status:** Draft
