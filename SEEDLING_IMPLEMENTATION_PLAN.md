# Seedling Game - Targeted AS3 Implementation Plan

**Document Version:** 1.0
**Date:** October 28, 2024
**Game:** Seedling by Danny Yaroslavski (Alexander Ocias)
**Target:** Minimal AS3 implementation to run Seedling specifically

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Game Analysis](#game-analysis)
3. [Required vs Full AS3 Comparison](#required-vs-full-as3-comparison)
4. [Minimal Implementation Scope](#minimal-implementation-scope)
5. [Implementation Strategy](#implementation-strategy)
6. [Phase-by-Phase Plan](#phase-by-phase-plan)
7. [Effort Estimation](#effort-estimation)
8. [Technical Details](#technical-details)
9. [Risk Assessment](#risk-assessment)
10. [Testing Strategy](#testing-strategy)

---

## Executive Summary

### Goal

Implement the **minimum subset of AS3** required to run the game Seedling, rather than full AS3 support. This targeted approach significantly reduces implementation complexity and timeline.

### Key Findings

**Seedling Codebase:**
- **209 ActionScript 3 files** (~14,148 lines of code)
- **Uses FlashPunk engine** (48 library files included)
- **AS3 Version:** Flash Player 11 (SWF version 11)
- **Game complexity:** Medium (2D action-adventure game)
- **Platform:** Newgrounds/Kongregate/ArmorGames

**Implementation Reduction:**
- **Full AS3:** 164 opcodes, complete standard library, all Flash APIs
- **Seedling-specific:** ~80-100 opcodes, targeted Flash APIs, FlashPunk engine

### Effort Comparison

| Approach | Opcodes | Classes | Hours | Duration |
|----------|---------|---------|-------|----------|
| **Full AS3** | 164 | 200+ | 1350-2350 | 10-16 months |
| **Seedling-specific** | 80-100 | 50-80 | **600-1000** | **4-7 months** |
| **Savings** | 50% | 65% | **55%** | **50-60%** |

### Recommendation

Proceed with **Seedling-specific implementation** using the hybrid C/C++ strategy. After Seedling works, incrementally add features for other games.

---

## Game Analysis

### Project Structure

```
Seedling/
├── src/                     # 209 AS3 files (14,148 lines)
│   ├── Main.as             # Entry point (extends net.flashpunk.Engine)
│   ├── Game.as             # Main game world (huge file - main game loop)
│   ├── Player.as           # 1,967 lines - complex player logic
│   ├── Mobile.as           # Base class for moving entities
│   ├── Enemies/            # 38 enemy types
│   ├── NPCs/               # 17 NPC types
│   ├── Pickups/            # 23 item types
│   ├── Projectiles/        # 14 projectile types
│   ├── Puzzlements/        # 20 puzzle element types
│   ├── Scenery/            # 41 scenery types
│   └── net/flashpunk/      # FlashPunk engine (48 files)
├── assets/                 # Embedded graphics/audio
└── Shrum.as3proj          # Project file (Flash Player 11)
```

### Core Game Systems

**1. Game Loop & Engine**
- Uses FlashPunk `Engine` class
- 60 FPS (Main.FPS constant)
- Entity-based architecture
- World/Scene management
- Custom physics system

**2. Graphics**
- Sprite-based rendering (Spritemap with animations)
- Tile-based levels (16x16 tiles)
- Multiple render layers
- Light/darkness system
- Screen shake effects
- Color tinting (damage flash, freeze effect)
- 370+ embedded assets (PNG images)

**3. Physics/Collision**
- Custom velocity-based movement
- Pixel-perfect collision (Pixelmask)
- Rectangle collision (Hitbox)
- Multiple surface types (ice, water, lava, stairs)
- Solid collision detection
- Entity collision (player, enemies, items)

**4. Input**
- Keyboard only (no mouse for gameplay)
- Arrow keys for movement
- X, C, V keys for actions
- I key for inventory
- Number keys for cheats

**5. Audio**
- Sound effect system (82+ embedded sounds)
- Music system (12+ songs)
- Volume control
- Distance-based audio
- Looping sounds
- All sounds embedded as MP3

**6. Persistence**
- SharedObject for save system
- Saves: position, inventory, progress, achievements
- Level state persistence
- Grass cut counter (for achievement)

**7. Combat**
- Multiple weapon types (6 different weapons)
- Hit detection
- Damage system
- Knockback
- Invincibility frames
- Enemy AI (various behaviors)
- 6 unique bosses

**8. UI**
- Custom inventory rendering
- Health display
- Message system
- Text rendering
- Menu system

### Dependencies Analysis

**Flash Platform APIs Used:**

| Package | Classes Used | Critical? |
|---------|--------------|-----------|
| `flash.display.*` | BitmapData, Sprite, MovieClip, BlendMode, Graphics, Bitmap, Stage config | ✅ YES |
| `flash.geom.*` | Point, Rectangle, Matrix, ColorTransform | ✅ YES |
| `flash.media.*` | Sound, SoundChannel, SoundTransform, SoundMixer | ✅ YES |
| `flash.net.*` | SharedObject, URLLoader, URLRequest | ✅ YES |
| `flash.events.*` | Event, KeyboardEvent, MouseEvent, TimerEvent | ✅ YES |
| `flash.utils.*` | getTimer, ByteArray, Dictionary, Timer, reflection | ✅ YES |
| `flash.text.*` | TextField, TextFormat, TextLineMetrics | ✅ YES |
| `flash.ui.*` | Keyboard, Mouse, MouseCursor | ✅ YES |
| `flash.filters.*` | ColorMatrixFilter | ⚠️ OPTIONAL |
| `flash.system.*` | Security, System | ⚠️ OPTIONAL |
| `com.newgrounds.*` | Newgrounds API | ❌ NO (stub) |

**FlashPunk Engine:**
- Complete 2D game engine (~48 files)
- Must be recompiled along with game code
- Core dependency - cannot be stubbed

**Third-Party:**
- Newgrounds API - Can be stubbed out
- Kongregate API - Can be stubbed out

### AS3 Language Features Used

**Core OOP:**
- ✅ Classes (207 classes)
- ✅ Inheritance (extensive hierarchies)
- ✅ Method overriding (401+ overrides)
- ✅ Getters/setters (171 pairs)
- ✅ Static members
- ✅ Constructors
- ❌ Interfaces (NOT USED)
- ❌ Abstract classes (NOT USED)

**Type System:**
- ✅ Strong typing throughout
- ✅ `Vector.<T>` (99 occurrences - type-safe arrays)
- ✅ `Array` (traditional arrays)
- ✅ `Object` (as map/dictionary)
- ✅ `Dictionary` (in FlashPunk core)
- ✅ Primitive types: int, uint, Number, Boolean, String
- ❌ Custom generic classes (NOT USED)

**Advanced Features:**
- ✅ Embedded assets ([Embed] metadata - 370+ tags)
- ✅ Property access (get/set)
- ✅ Method overloading (via optional parameters)
- ✅ Namespaces (package structure)
- ❌ E4X/XML (NOT USED)
- ❌ Proxies (NOT USED)
- ❌ Workers (NOT USED)

---

## Required vs Full AS3 Comparison

### Opcodes Required for Seedling

Based on code analysis, Seedling uses approximately **80-100 opcodes** out of the full 164:

**Definitely Required (~60 opcodes):**

1. **Arithmetic (12):** add, add_i, subtract, subtract_i, multiply, multiply_i, divide, modulo, negate, increment, increment_i, decrement
2. **Comparison (7):** equals, strictequals, lessthan, lessequals, greaterthan, greaterequals, not
3. **Bitwise (7):** bitand, bitor, bitxor, bitnot, lshift, rshift, urshift
4. **Type Operations (8):** coerce_*, convert_*, instanceof, typeof, istype
5. **Stack (8):** dup, pop, swap, pushscope, popscope, getlocal_0-3, setlocal_0-3
6. **Constants (10):** push*, pushtrue, pushfalse, pushnull, pushundefined
7. **Control (8):** jump, ifeq, iffalse, ifge, ifgt, ifle, iflt, ifne, iftrue, ifstricteq, ifstrictne, returnvalue, returnvoid

**Likely Required (~25 opcodes):**

8. **Property Access (8):** getproperty, setproperty, initproperty, getsuper, setsuper, getslot, setslot
9. **Method Calls (10):** call, callmethod, callproperty, callsuper, construct, constructprop, constructsuper, newfunction
10. **Object Creation (4):** newobject, newarray, newactivation, newclass
11. **Local Variables (3):** getlocal, setlocal, kill

**Possibly Required (~15 opcodes):**

12. **Scope Chain (5):** findproperty, findpropstrict, getglobalscope, getscopeobject
13. **Array/Object (4):** hasnext, hasnext2, nextname, nextvalue
14. **Advanced (6):** lookupswitch, throw (exceptions), inclocal, declocal, checkfilter, applytype

**Unlikely to Need (~64 opcodes):**
- E4X/XML operations (dxns, esc_xattr, etc.) - NOT USED
- Alchemy memory ops (li8, si16, etc.) - NOT USED
- Advanced type operations - RARELY USED
- Namespace operations beyond basics - RARELY USED

### Flash APIs Required for Seedling

**Must Implement:**

**flash.display (20 classes):**
- `BitmapData` ⭐ CRITICAL - All graphics
- `Sprite` ⭐ CRITICAL - Base display object
- `MovieClip` ⭐ CRITICAL - Engine base
- `DisplayObject` - Component integration
- `Graphics` - Vector drawing
- `Bitmap` - Image rendering
- `Stage` - Root display
- `BlendMode` - Rendering modes
- `LineScaleMode`, `SpreadMethod`, `PixelSnapping` - Graphics config
- `Loader`, `LoaderInfo` - Asset loading
- `StageQuality`, `StageScaleMode`, `StageAlign`, `StageDisplayState` - Stage config

**flash.geom (5 classes):**
- `Point` ⭐ CRITICAL - 200+ uses
- `Rectangle` ⭐ CRITICAL - 75+ uses
- `Matrix` - Transformations
- `ColorTransform` - Color effects
- `Transform` - Combined transforms

**flash.media (4 classes):**
- `Sound` ⭐ CRITICAL - Audio playback
- `SoundChannel` - Channel management
- `SoundTransform` - Volume/pan
- `SoundMixer` - Global audio

**flash.net (3 classes):**
- `SharedObject` ⭐ CRITICAL - Save system
- `URLLoader` - Loading data
- `URLRequest` - Request objects

**flash.events (4 classes):**
- `Event` - Base event
- `KeyboardEvent` - Input
- `MouseEvent` - Mouse (minimal)
- `TimerEvent` - Timing

**flash.utils (7 classes):**
- `getTimer` ⭐ CRITICAL - Frame timing
- `ByteArray` - Binary data
- `Dictionary` - Advanced map
- `Timer` - Timed events
- `getDefinitionByName`, `getQualifiedClassName` - Reflection
- Various utility functions

**flash.text (3 classes):**
- `TextField` - Text rendering
- `TextFormat` - Text styling
- `TextLineMetrics` - Measurement

**flash.ui (3 classes):**
- `Keyboard` - Key constants
- `Mouse` - Mouse control
- `MouseCursor` - Cursor types

**flash.filters (1 class):**
- `ColorMatrixFilter` - Color effects (OPTIONAL)

**flash.system (2 classes):**
- `Security` - Domain checking (CAN STUB)
- `System` - Memory (CAN STUB)

**Can Stub/Skip:**
- `com.newgrounds.*` - Platform API (return fake success)
- Kongregate API - Platform API (stub)
- `flash.filters.*` (except ColorMatrixFilter) - Visual effects only
- Advanced reflection - Minimal use
- Worker threads - Not used
- Camera/Microphone - Not used
- NetConnection/NetStream - Not used

### FlashPunk Engine

**Must fully implement FlashPunk:**
- 48 source files
- ~6,000 lines of AS3 code
- Core engine functionality
- Cannot be stubbed

**FlashPunk Dependencies:**
- Same Flash APIs as Seedling
- Uses BitmapData heavily
- Entity/World architecture
- Collision system
- Graphics rendering
- Input management

---

## Minimal Implementation Scope

### What We Need to Build

**1. ABC Parser** (SAME as full AS3)
- Parse ABC file format
- Extract constant pools
- Parse method bodies
- Parse class definitions
- Parse traits
- **Effort:** 300-500 hours (same as full)

**2. Reduced Opcode Set** (50% of full)
- Implement ~80-100 opcodes (vs 164 for full)
- Focus on opcodes actually used by Seedling
- Skip E4X, Alchemy, advanced features
- **Effort:** 200-300 hours (vs 400-700 for full)

**3. Targeted Flash API Implementation** (40% of full)
- ~50 classes (vs 200+ for full)
- Focus on APIs Seedling uses
- Stub platform-specific APIs
- **Effort:** 300-500 hours (vs 800-1200 for full)

**4. Object Model** (SAME as full)
- Classes and inheritance
- Property access
- Method dispatch
- Getters/setters
- **Effort:** 250-400 hours (same as full)

**5. FlashPunk Engine Compatibility**
- Ensure FlashPunk compiles and runs
- Test all FlashPunk features used by Seedling
- **Effort:** 100-150 hours (testing/debugging)

### What We Can Skip (For Now)

**Opcodes we don't need:**
- ❌ E4X/XML operations (~15 opcodes)
- ❌ Alchemy memory operations (~14 opcodes)
- ❌ Advanced namespace operations (~10 opcodes)
- ❌ Rarely-used type operations (~10 opcodes)
- ❌ Worker/async operations (~5 opcodes)

**Flash APIs we don't need:**
- ❌ NetConnection/NetStream (video streaming)
- ❌ Camera/Microphone
- ❌ LocalConnection
- ❌ FileReference (file upload/download)
- ❌ Most flash.external.*
- ❌ Most flash.printing.*
- ❌ XML/E4X support
- ❌ Advanced filters (most)
- ❌ 3D APIs (Vector3D, Matrix3D, etc.)

**Standard library we don't need:**
- ❌ XML/XMLList
- ❌ RegExp (can add later if needed)
- ❌ Most Date functions (simple subset only)
- ❌ Advanced Array methods (some)
- ❌ Crypto APIs
- ❌ JSON (simple subset only)

---

## Implementation Strategy

### Approach: Incremental Development

**Phase 0:** AS1/2 Stabilization (2 months)
- Complete current AS1/2 work
- Fix runtime integration issues
- Establish baseline

**Phase 1:** FlashPunk Analysis (2-3 weeks)
- Deep dive into FlashPunk source
- Map FlashPunk dependencies
- Identify critical paths
- Create FlashPunk feature checklist

**Phase 2:** Core ABC + Minimal Opcodes (2-3 months)
- ABC parser (same as full plan)
- ~40 core opcodes
- Basic type system
- Simple method calls
- Test with tiny AS3 programs

**Phase 3:** Object Model + Flash APIs (2-3 months)
- Class system
- Inheritance
- Property access
- Key Flash APIs (display, geom, events)
- Test with simple FlashPunk programs

**Phase 4:** Complete Seedling Requirements (1-2 months)
- Remaining opcodes for Seedling
- All Flash APIs Seedling uses
- Sound system
- SharedObject (save system)
- Full FlashPunk support

**Phase 5:** Integration & Testing (1 month)
- Compile Seedling
- Debug issues
- Performance optimization
- Play through entire game
- Fix bugs

### Development Priorities

**Priority 1 - Critical Path:**
1. ABC parser
2. Class/inheritance system
3. BitmapData (graphics)
4. Entity system (FlashPunk core)
5. Input (keyboard)
6. Basic opcodes (arithmetic, logic, control flow)

**Priority 2 - Core Game:**
7. Spritemap (animations)
8. Collision (Pixelmask, Hitbox)
9. Sound system
10. Point/Rectangle (math)
11. Property access opcodes
12. Method call opcodes

**Priority 3 - Polish:**
13. SharedObject (saves)
14. UI rendering
15. Remaining opcodes
16. Performance optimization

### Testing Strategy

**Unit Tests:**
- Test each opcode implementation
- Test each Flash API class
- Test FlashPunk components

**Integration Tests:**
- Compile FlashPunk alone
- Compile simple games using FlashPunk
- Compile Seedling incrementally (start with just Main.as)

**Functional Tests:**
- Game boots
- Graphics render
- Input works
- Physics work
- Combat works
- Save/load works
- Can complete game

---

## Phase-by-Phase Plan

### Phase 0: Preparation (Current - 1-2 months)

**Prerequisites before starting AS3:**
- [ ] Complete AS1/2 implementation
- [ ] Fix runtime integration segfault
- [ ] Stabilize API
- [ ] Document AS1/2 architecture
- [ ] All AS1/2 tests passing

**Deliverables:**
- Stable AS1/2 support
- Clear API documentation
- Baseline performance metrics

---

### Phase 1: FlashPunk Analysis (2-3 weeks)

**Goal:** Understand FlashPunk's AS3 usage in detail

**Tasks:**

**1.1: FlashPunk Source Analysis (1 week)**
- [ ] Read all 48 FlashPunk files
- [ ] Document class hierarchy
- [ ] Map Flash API dependencies
- [ ] Identify critical code paths
- [ ] Create dependency graph

**1.2: Feature Mapping (1 week)**
- [ ] List all FlashPunk features
- [ ] Identify which features Seedling uses
- [ ] Create minimal FlashPunk feature set
- [ ] Document required opcodes for FlashPunk

**1.3: Test Plan Creation (2-3 days)**
- [ ] Create FlashPunk unit tests
- [ ] Create simple FlashPunk games for testing
- [ ] Define FlashPunk success criteria

**Deliverables:**
- FlashPunk analysis document
- Dependency graph
- Minimal feature set definition
- Test suite skeleton

---

### Phase 2: ABC Parser + Core Opcodes (2-3 months)

**Goal:** Parse ABC files and implement core opcodes

**Estimated effort:** 350-550 hours

**Milestones:**

**2.1: ABC Parser (Same as full plan) (4-6 weeks)**
- [ ] Implement ABC file parser (reuse from AS3_IMPLEMENTATION_PLAN.md)
- [ ] Parse constant pools
- [ ] Parse method info
- [ ] Parse class info
- [ ] Test with Seedling's ABC data

**2.2: Core Opcodes (3-4 weeks)**

Implement ~40 essential opcodes:

**Arithmetic (12):**
- [ ] add, add_i
- [ ] subtract, subtract_i
- [ ] multiply, multiply_i
- [ ] divide, modulo
- [ ] negate, increment, increment_i, decrement

**Comparison (7):**
- [ ] equals, strictequals
- [ ] lessthan, lessequals, greaterthan, greaterequals
- [ ] not

**Bitwise (7):**
- [ ] bitand, bitor, bitxor, bitnot
- [ ] lshift, rshift, urshift

**Stack (8):**
- [ ] dup, pop, swap
- [ ] pushscope, popscope
- [ ] getlocal_0, getlocal_1, getlocal_2, getlocal_3

**Constants (10):**
- [ ] pushbyte, pushshort, pushint, pushuint
- [ ] pushstring, pushdouble, pushnan
- [ ] pushtrue, pushfalse, pushnull, pushundefined

**Control (8):**
- [ ] jump, label
- [ ] iftrue, iffalse
- [ ] ifeq, ifne, iflt, ifgt, ifle, ifge
- [ ] returnvalue, returnvoid

**2.3: Basic Type System (2 weeks)**
- [ ] Implement AS3Value base class
- [ ] Implement primitive types (int, uint, Number, Boolean, String)
- [ ] Implement AS3Undefined, AS3Null
- [ ] Basic type conversions
- [ ] Test type system

**2.4: Simple Code Generation (2 weeks)**
- [ ] Generate C++ from simple methods
- [ ] Test with HelloWorld equivalent
- [ ] Test arithmetic/logic
- [ ] Test control flow

**Deliverables:**
- ABC parser (~4,500 lines)
- 40 opcode implementations (~2,000 lines)
- Basic type system (~1,200 lines)
- Code generator skeleton (~1,000 lines)
- Test suite (20+ tests)

---

### Phase 3: Object Model + Flash Display APIs (2-3 months)

**Goal:** Classes, inheritance, and critical Flash APIs

**Estimated effort:** 350-500 hours

**Milestones:**

**3.1: Class System (3-4 weeks)**
- [ ] Implement AS3Class structure
- [ ] Implement AS3Object base class
- [ ] Constructor generation
- [ ] Method generation
- [ ] Property access (slots)
- [ ] Test with simple class hierarchies

**3.2: Inheritance (2-3 weeks)**
- [ ] Implement prototype chain
- [ ] super() calls
- [ ] Method overriding
- [ ] Virtual dispatch
- [ ] Test with FlashPunk hierarchy (Entity → Tweener → Mobile)

**3.3: Property Access Opcodes (2 weeks)**
- [ ] getproperty, setproperty
- [ ] initproperty
- [ ] getslot, setslot
- [ ] getsuper, setsuper
- [ ] Test with Seedling classes

**3.4: Method Call Opcodes (2 weeks)**
- [ ] call, callmethod
- [ ] callproperty, callpropvoid
- [ ] callsuper
- [ ] construct, constructprop, constructsuper
- [ ] newfunction
- [ ] Test method dispatch

**3.5: flash.display.BitmapData (2 weeks)** ⭐ CRITICAL
- [ ] BitmapData class structure
- [ ] draw() method
- [ ] copyPixels()
- [ ] fillRect()
- [ ] getPixel/setPixel
- [ ] lock/unlock
- [ ] Test with simple graphics

**3.6: flash.display Core (2 weeks)**
- [ ] DisplayObject base class
- [ ] Sprite class
- [ ] MovieClip class (minimal)
- [ ] Stage class (minimal)
- [ ] Graphics class (basic drawing)
- [ ] Bitmap class
- [ ] Test display list

**3.7: flash.geom (1 week)**
- [ ] Point class
- [ ] Rectangle class
- [ ] Matrix class
- [ ] ColorTransform class
- [ ] Test geometry operations

**3.8: flash.events (1 week)**
- [ ] Event base class
- [ ] KeyboardEvent
- [ ] MouseEvent (minimal)
- [ ] Event dispatching
- [ ] addEventListener/removeEventListener

**Deliverables:**
- Class system (~2,500 lines)
- Object model (~2,000 lines)
- Property/method opcodes (~1,500 lines)
- flash.display (~3,000 lines)
- flash.geom (~800 lines)
- flash.events (~600 lines)
- Test suite (50+ tests)

---

### Phase 4: Complete Seedling Requirements (1-2 months)

**Goal:** All remaining features Seedling needs

**Estimated effort:** 300-450 hours

**Milestones:**

**4.1: Remaining Core Opcodes (2 weeks)**
- [ ] Object creation: newobject, newarray
- [ ] Scope: findproperty, findpropstrict
- [ ] Locals: getlocal, setlocal
- [ ] Iteration: hasnext, hasnext2, nextname, nextvalue
- [ ] Type ops: instanceof, typeof, coerce_*, convert_*
- [ ] Test all opcodes with Seedling code

**4.2: Collections (1 week)**
- [ ] Array class
- [ ] Vector.<T> class (generic)
- [ ] Dictionary class
- [ ] Test collection operations

**4.3: Sound System (2 weeks)** ⭐ CRITICAL
- [ ] flash.media.Sound
- [ ] SoundChannel
- [ ] SoundTransform
- [ ] SoundMixer
- [ ] Load embedded MP3s
- [ ] Playback, volume, looping
- [ ] Test with Seedling's 82+ sounds

**4.4: Input System (1 week)**
- [ ] flash.ui.Keyboard constants
- [ ] KeyboardEvent handling
- [ ] flash.utils.Input (FlashPunk wrapper)
- [ ] Test keyboard input

**4.5: SharedObject (Save System) (1 week)** ⭐ CRITICAL
- [ ] flash.net.SharedObject
- [ ] getLocal() method
- [ ] data property (Object storage)
- [ ] flush() method
- [ ] Test save/load

**4.6: Utility Classes (1 week)**
- [ ] flash.utils.getTimer
- [ ] flash.utils.Timer
- [ ] flash.utils.ByteArray (basic)
- [ ] Reflection methods (getDefinitionByName, etc.)

**4.7: Text Rendering (1 week)**
- [ ] flash.text.TextField
- [ ] TextFormat
- [ ] TextLineMetrics
- [ ] Test text rendering

**4.8: FlashPunk Integration (2 weeks)**
- [ ] Compile all 48 FlashPunk files
- [ ] Fix compilation errors
- [ ] Test FlashPunk components
- [ ] Verify Entity system works
- [ ] Verify collision works
- [ ] Verify graphics rendering works

**4.9: Embedded Assets (1 week)**
- [ ] [Embed] metadata parsing
- [ ] Asset loading system
- [ ] Integrate assets into compiled code
- [ ] Test with Seedling's 370+ embedded assets

**Deliverables:**
- Remaining opcodes (~2,000 lines)
- Collections (~1,500 lines)
- Sound system (~1,200 lines)
- Input system (~400 lines)
- SharedObject (~600 lines)
- Utils (~800 lines)
- Text rendering (~600 lines)
- Embedded asset system (~500 lines)
- FlashPunk fully compiling
- Test suite (100+ tests)

---

### Phase 5: Seedling Integration & Testing (1 month)

**Goal:** Get Seedling fully playable

**Estimated effort:** 150-250 hours

**Milestones:**

**5.1: Compilation (1 week)**
- [ ] Compile Seedling source (all 209 files)
- [ ] Fix compilation errors
- [ ] Generate C++ code
- [ ] Link with runtime
- [ ] Get executable building

**5.2: Boot & Initialization (3-5 days)**
- [ ] Game launches
- [ ] FlashPunk initializes
- [ ] Splash screen displays
- [ ] Music plays
- [ ] Input responsive

**5.3: Core Gameplay (1 week)**
- [ ] Player movement works
- [ ] Graphics render correctly
- [ ] Collision detection works
- [ ] Enemies function
- [ ] Combat works
- [ ] Items can be picked up

**5.4: Complete Features (1 week)**
- [ ] Save/load works
- [ ] All weapons work
- [ ] All enemies work
- [ ] All puzzles work
- [ ] UI renders correctly
- [ ] Sound effects play correctly

**5.5: Full Playthrough (3-5 days)**
- [ ] Play from start to end
- [ ] Test all dungeons
- [ ] Test all bosses
- [ ] Test all items
- [ ] Test edge cases

**5.6: Bug Fixing (1 week)**
- [ ] Fix crashes
- [ ] Fix graphical glitches
- [ ] Fix audio issues
- [ ] Fix save/load issues
- [ ] Fix physics bugs
- [ ] Performance optimization

**5.7: Platform Testing (2-3 days)**
- [ ] Test on Linux
- [ ] Test on Windows
- [ ] Test on macOS
- [ ] Fix platform-specific issues

**Deliverables:**
- Playable Seedling game
- Full feature parity with Flash version
- Bug fixes
- Performance optimizations
- Platform compatibility
- Documentation of issues/workarounds

---

## Effort Estimation

### Summary by Phase

| Phase | Duration | Hours | Key Deliverable |
|-------|----------|-------|-----------------|
| **Phase 0** | 1-2 months | - | AS1/2 stable |
| **Phase 1** | 2-3 weeks | 60-80 | FlashPunk analysis |
| **Phase 2** | 2-3 months | 350-550 | ABC + core opcodes |
| **Phase 3** | 2-3 months | 350-500 | Object model + Flash APIs |
| **Phase 4** | 1-2 months | 300-450 | Seedling requirements |
| **Phase 5** | 1 month | 150-250 | Integration & testing |
| **TOTAL** | **8-12 months** | **1210-1830** | **Playable Seedling** |

### Comparison to Full AS3

| Metric | Full AS3 | Seedling-Specific | Savings |
|--------|----------|-------------------|---------|
| **Opcodes** | 164 | 80-100 | 39-51% |
| **Flash APIs** | 200+ classes | 50-80 classes | 60-75% |
| **Standard Lib** | Complete | Minimal subset | 70-80% |
| **Total Hours** | 2460-3490 | 1210-1830 | 51-48% |
| **Duration** | 12-18 months | 8-12 months | 33-40% |
| **Lines of Code** | ~25,000 | ~12,000-15,000 | 40-52% |

### Developer Velocity

**Assumptions:**
- Single experienced developer
- Part-time work (20-30 hours/week)
- Based on LittleCube's current pace

**Timeline:**
- **Full-time (40h/week):** 6-9 months
- **Part-time (20h/week):** 12-18 months
- **Team of 2-3:** 4-6 months

---

## Technical Details

### Critical Code Paths in Seedling

**1. Game Loop:**
```
Main.update()
  → FP.world.update()
    → Game.update()
      → Player.update()
        → Input.check()
        → Physics.calculate()
        → Collision.detect()
      → Enemy.update() (for each)
      → Entity.update() (for each)
      → Music.update()
  → FP.world.render()
    → Game.render()
      → Entity.render() (for each)
        → Graphic.render()
          → BitmapData.draw()
```

**2. Graphics Rendering:**
```
FlashPunk.Engine
  → Screen.render()
    → BitmapData.fillRect() (clear)
    → World.render()
      → Entity.render() (sorted by layer)
        → Graphic.render()
          → Image.render() or Spritemap.render()
            → BitmapData.copyPixels() or draw()
    → Display to screen (scale 3x)
```

**3. Collision Detection:**
```
Entity.collide(type, x, y)
  → Check mask type
  → Pixelmask vs Pixelmask (pixel-perfect)
    → BitmapData.hitTest()
  → Hitbox vs Hitbox (rectangle)
    → Rectangle.intersects()
  → Grid vs Hitbox (tile collision)
    → Grid.collideRect()
```

**4. Sound Playback:**
```
Music.playSong(name)
  → Look up Sound in array
  → Sound.play()
    → Returns SoundChannel
  → Store SoundChannel
  → Apply SoundTransform (volume)

Sfx.play()
  → Sound.play()
  → Cache SoundChannel
  → Distance-based volume calculation
```

**5. Save/Load:**
```
Save:
  Main.SAVE_FILE.data.playerPositionX = value;
  Main.SAVE_FILE.data.level = value;
  Main.SAVE_FILE.data.hasSword = value;
  // ... more properties
  Main.SAVE_FILE.flush();

Load:
  var saveData = Main.SAVE_FILE.data;
  if (saveData.playerPositionX != undefined) {
    Main.playerPositionX = saveData.playerPositionX;
  }
```

### Opcodes by Priority for Seedling

**Tier 1 - Absolutely Critical (30 opcodes):**
```
Arithmetic: add, add_i, subtract, subtract_i, multiply, divide
Comparison: equals, lessthan, lessequals, greaterthan, greaterequals
Logic: not, iftrue, iffalse
Stack: dup, pop, getlocal_0-3, setlocal_0-3
Constants: pushint, pushdouble, pushstring, pushtrue, pushfalse, pushnull
Control: jump, returnvalue, returnvoid
```

**Tier 2 - Very Important (25 opcodes):**
```
Bitwise: bitand, bitor, bitxor, lshift, rshift
Type: instanceof, typeof, coerce_a, coerce_i, coerce_s
Property: getproperty, setproperty, getslot, setslot
Methods: callproperty, callpropvoid, construct, newfunction
Objects: newobject, newarray
Scope: pushscope, popscope, findpropstrict
```

**Tier 3 - Important (20 opcodes):**
```
Advanced Control: ifeq, ifne, iflt, ifge, strictequals
More Types: coerce_b, coerce_d, coerce_o, convert_*
Advanced Properties: initproperty, getsuper, setsuper
Advanced Methods: callsuper, constructprop, constructsuper
Locals: getlocal, setlocal, inclocal, declocal
Iteration: hasnext, hasnext2
```

**Tier 4 - Nice to Have (15 opcodes):**
```
More Arithmetic: modulo, negate, increment, decrement
More Bitwise: bitnot, urshift
More Stack: swap
More Scope: getglobalscope, getscopeobject
Advanced: lookupswitch, throw, checkfilter, applytype
```

### Flash API Implementation Priorities

**Tier 1 - Critical (Must have for basic functionality):**
```
flash.display.BitmapData ⭐⭐⭐ - ALL graphics depend on this
flash.geom.Point ⭐⭐⭐ - 200+ uses
flash.geom.Rectangle ⭐⭐⭐ - 75+ uses
flash.utils.getTimer ⭐⭐⭐ - Frame timing
flash.events.Event ⭐⭐⭐ - Event system
```

**Tier 2 - Very Important (Needed for core gameplay):**
```
flash.display.Sprite ⭐⭐ - Display hierarchy
flash.display.MovieClip ⭐⭐ - Engine base
flash.media.Sound ⭐⭐ - Audio
flash.media.SoundChannel ⭐⭐ - Audio control
flash.events.KeyboardEvent ⭐⭐ - Input
flash.utils.Dictionary ⭐⭐ - FlashPunk core
```

**Tier 3 - Important (Needed for complete experience):**
```
flash.net.SharedObject ⭐ - Save system
flash.text.TextField ⭐ - Text rendering
flash.geom.Matrix ⭐ - Transformations
flash.geom.ColorTransform ⭐ - Color effects
flash.display.Graphics ⭐ - Vector drawing
flash.utils.ByteArray ⭐ - Binary data
```

**Tier 4 - Nice to Have:**
```
flash.display.BlendMode - Special effects
flash.filters.ColorMatrixFilter - Advanced effects
flash.ui.Mouse - Cursor control
flash.text.TextFormat - Text styling
```

---

## Risk Assessment

### High-Risk Items

**1. FlashPunk Complexity ⚠️ HIGH**
- **Risk:** FlashPunk may use AS3 features we haven't accounted for
- **Mitigation:** Phase 1 deep analysis, early FlashPunk compilation tests
- **Impact:** +1-3 months if major issues found

**2. BitmapData Operations ⚠️ HIGH**
- **Risk:** BitmapData is complex and performance-critical
- **Mitigation:** Focus heavily on BitmapData implementation, optimize early
- **Impact:** Performance issues if not implemented well

**3. Sound System ⚠️ MEDIUM**
- **Risk:** Audio might have platform-specific issues
- **Mitigation:** Use SDL3's audio support, test early
- **Impact:** +2-4 weeks for debugging

**4. Embedded Assets ⚠️ MEDIUM**
- **Risk:** 370+ embedded assets need special handling
- **Mitigation:** Study how Flash Player handles embedded assets, implement carefully
- **Impact:** +1-2 weeks if issues

**5. Save System ⚠️ MEDIUM**
- **Risk:** SharedObject format compatibility
- **Mitigation:** Document Flash's SharedObject format, test thoroughly
- **Impact:** +1 week for implementation issues

### Medium-Risk Items

**6. Performance 📊 MEDIUM**
- **Risk:** C++ version might be slower than Flash
- **Mitigation:** Profile early, optimize BitmapData operations
- **Impact:** +2-4 weeks optimization

**7. Graphics Rendering 📊 MEDIUM**
- **Risk:** Rendering artifacts or glitches
- **Mitigation:** Pixel-perfect testing, compare screenshots
- **Impact:** +1-2 weeks debugging

**8. Physics/Collision 📊 MEDIUM**
- **Risk:** Subtle physics differences
- **Mitigation:** Test thoroughly, record Flash behavior
- **Impact:** +1-2 weeks tuning

### Low-Risk Items

**9. Opcode Implementation ✅ LOW**
- **Risk:** Individual opcodes straightforward
- **Mitigation:** Test each opcode, follow AVM2 spec
- **Impact:** Minimal

**10. Basic Flash APIs ✅ LOW**
- **Risk:** Most APIs well-documented
- **Mitigation:** Follow Flash documentation
- **Impact:** Minimal

---

## Testing Strategy

### Test Pyramid

**Level 1: Unit Tests**
- Test each opcode implementation
- Test each Flash API class/method
- Test type conversions
- Test object model operations
- **Target:** 200+ unit tests

**Level 2: Integration Tests**
- FlashPunk components compile
- FlashPunk features work
- Flash APIs interact correctly
- **Target:** 50+ integration tests

**Level 3: System Tests**
- Compile Seedling
- Game boots
- Core systems work
- **Target:** 20+ system tests

**Level 4: Acceptance Tests**
- Full playthrough
- All features work
- Performance acceptable
- **Target:** Complete game playthrough

### Continuous Testing

**Per Phase:**
- Phase 2: Test core opcodes with tiny programs
- Phase 3: Test object model with simple classes
- Phase 4: Test with FlashPunk examples
- Phase 5: Test with Seedling

**Regression Testing:**
- Keep all unit/integration tests passing
- Add test for each bug found
- Automate testing with CI

### Performance Testing

**Metrics to Track:**
- FPS (target: 60 FPS)
- Memory usage
- Load time
- Compilation time

**Benchmarks:**
- Compare to Flash Player
- Compare to Ruffle
- Profile hotspots
- Optimize critical paths

---

## Success Criteria

### Phase 1 Success
- [ ] FlashPunk dependencies mapped
- [ ] Required features list complete
- [ ] Test plan created

### Phase 2 Success
- [ ] ABC parser working
- [ ] 40 core opcodes implemented
- [ ] Basic type system functional
- [ ] Simple programs compile and run

### Phase 3 Success
- [ ] Class system working
- [ ] Inheritance working
- [ ] flash.display core implemented
- [ ] Simple FlashPunk programs work

### Phase 4 Success
- [ ] All Seedling-required opcodes implemented
- [ ] All Seedling-required Flash APIs implemented
- [ ] FlashPunk compiles
- [ ] Sound system working
- [ ] Save system working

### Phase 5 Success
- [ ] Seedling compiles without errors
- [ ] Seedling boots and runs
- [ ] All game systems functional
- [ ] Can complete entire game
- [ ] Performance acceptable (>30 FPS)
- [ ] No major bugs

### Overall Success
- [ ] Seedling is fully playable
- [ ] Save/load works
- [ ] Performance comparable to Flash
- [ ] Works on all target platforms
- [ ] Code is maintainable
- [ ] Path forward for other games clear

---

## Next Steps

### Immediate Actions (After AS1/2 Complete)

1. **FlashPunk Analysis** (Week 1-3)
   - Deep dive into FlashPunk source
   - Map dependencies
   - Create feature checklist

2. **Prototype ABC Parser** (Week 4-8)
   - Parse simple ABC files
   - Validate structure
   - Test with Seedling's ABC

3. **Implement Core Opcodes** (Week 9-16)
   - Start with arithmetic/logic
   - Add control flow
   - Test with tiny programs

4. **First FlashPunk Test** (Week 17-20)
   - Compile minimal FlashPunk subset
   - Get simple Entity working
   - Display "Hello World" sprite

### Decision Points

**After Phase 1:**
- Is FlashPunk feasible? Continue or pivot?

**After Phase 2:**
- Are opcodes working correctly? Performance acceptable?

**After Phase 3:**
- Does object model support FlashPunk? Continue or refactor?

**After Phase 4:**
- Does Seedling compile? All features available?

### Long-term Vision

**Once Seedling works:**
1. Add missing opcodes for other games
2. Expand Flash API coverage
3. Support more FlashPunk games
4. Support other frameworks (Flixel, Starling)
5. Eventually: Full AS3 support

**Incremental Expansion:**
- Seedling → Other FlashPunk games
- FlashPunk games → Flixel games
- Simple games → Complex games
- Game-by-game feature additions

---

## Appendix A: Seedling Class Hierarchy

```
Main (net.flashpunk.Engine)

Game (net.flashpunk.World)
  ├─ Player (Mobile)
  ├─ Enemies/* (Enemy → Mobile)
  │   ├─ Bob, BobBoss
  │   ├─ Flyer, WallFlyer
  │   ├─ Turret, IceTurret
  │   ├─ Bulb, Cactus, Squishle, Jumper, Spinner
  │   ├─ BossTotem, LightBoss, LavaBoss, ShieldBoss, FinalBoss
  │   └─ LightBossController
  ├─ NPCs/* (NPC → Mobile)
  │   ├─ Sign, Statue, Totem, Watcher, Oracle
  │   ├─ Karlore, Hermit, Sensei, Witch, Yeti, Rekcahdam
  │   ├─ IntroCharacter, ForestCharacter, AdnanCharacter
  │   ├─ BobBossNPC, Help
  │   └─ BossPlatform
  ├─ Pickups/* (Pickup → Mobile)
  │   ├─ Coin, HealthPickup, Seed
  │   ├─ BossKey, SealPiece
  │   ├─ Stick, Wand, Fire, DarkSword
  │   └─ DarkShield, DarkSuit
  ├─ Projectiles/* (Mobile)
  │   ├─ Arrow, Bomb, Explosion
  │   ├─ WandShot, RayShot
  │   ├─ TurretSpit, IceTurretBlast, BossTotemShot
  │   ├─ LavaBall, LightBossShot
  │   └─ various enemy projectiles
  ├─ Puzzlements/* (net.flashpunk.Entity)
  │   ├─ Button, ButtonRoom, Activators
  │   ├─ MagicalLock, RockLock
  │   ├─ Whirlpool, Wire
  │   └─ various puzzle elements
  └─ Scenery/* (net.flashpunk.Entity)
      ├─ Tile (collision tiles)
      ├─ Grass, Tree, BurnableTree
      ├─ Light, LightPole, Moonrock
      ├─ RockFall, Pod, SlashHit
      └─ various scenery objects

Mobile (net.flashpunk.Entity)
  └─ Custom physics/movement base class

Splash (net.flashpunk.World)
  └─ Menu/splash screen

FlashPunk Core:
  net.flashpunk.Engine
  net.flashpunk.World
  net.flashpunk.Entity
  net.flashpunk.Tweener (base of Entity)
  net.flashpunk.Graphic (base for graphics)
  net.flashpunk.Mask (base for collision)
```

---

## Appendix B: Required Flash APIs Reference

### flash.display.*

```actionscript
class BitmapData {
    function BitmapData(width:int, height:int, transparent:Boolean, fillColor:uint);
    function draw(source:IBitmapDrawable, matrix:Matrix, ...):void;
    function copyPixels(sourceBitmapData:BitmapData, sourceRect:Rectangle, destPoint:Point, ...):void;
    function fillRect(rect:Rectangle, color:uint):void;
    function getPixel(x:int, y:int):uint;
    function setPixel(x:int, y:int, color:uint):void;
    function lock():void;
    function unlock():void;
    function hitTest(firstPoint:Point, ...):Boolean;
    // + width, height properties
}

class Sprite extends DisplayObjectContainer {
    var graphics:Graphics;
    // + display properties
}

class MovieClip extends Sprite {
    function gotoAndStop(frame:Object):void;
    function gotoAndPlay(frame:Object):void;
    // + timeline properties
}

class Graphics {
    function clear():void;
    function lineStyle(...):void;
    function moveTo(x:Number, y:Number):void;
    function lineTo(x:Number, y:Number):void;
    function drawRect(x:Number, y:Number, width:Number, height:Number):void;
    function drawCircle(x:Number, y:Number, radius:Number):void;
    function beginFill(color:uint, alpha:Number):void;
    function endFill():void;
}
```

### flash.geom.*

```actionscript
class Point {
    var x:Number;
    var y:Number;
    function Point(x:Number, y:Number);
    function distance(pt1:Point, pt2:Point):Number; // static
    function add(v:Point):Point;
    function subtract(v:Point):Point;
    function normalize(thickness:Number):void;
    var length:Number; // getter
}

class Rectangle {
    var x:Number, y:Number, width:Number, height:Number;
    function Rectangle(x:Number, y:Number, width:Number, height:Number);
    function contains(x:Number, y:Number):Boolean;
    function intersects(toIntersect:Rectangle):Boolean;
    function union(toUnion:Rectangle):Rectangle;
    // + left, right, top, bottom getters
}

class Matrix {
    var a:Number, b:Number, c:Number, d:Number, tx:Number, ty:Number;
    function Matrix(a:Number, ...);
    function identity():void;
    function translate(dx:Number, dy:Number):void;
    function rotate(angle:Number):void;
    function scale(sx:Number, sy:Number):void;
}
```

### flash.media.*

```actionscript
class Sound {
    function Sound(stream:URLRequest);
    function play(startTime:Number, loops:int, sndTransform:SoundTransform):SoundChannel;
    // For embedded: [Embed(source="...")] var snd:Class;
}

class SoundChannel {
    function stop():void;
    var soundTransform:SoundTransform;
    var position:Number; // getter
}

class SoundTransform {
    var volume:Number;
    var pan:Number;
    function SoundTransform(vol:Number, panning:Number);
}

class SoundMixer {
    static var soundTransform:SoundTransform;
    static function stopAll():void;
}
```

### flash.net.*

```actionscript
class SharedObject {
    static function getLocal(name:String):SharedObject;
    var data:Object;
    function flush():String;
}
```

---

**END OF DOCUMENT**
