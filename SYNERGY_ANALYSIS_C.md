# Synergy Analysis: Manual C Conversion + SWFRecomp (Pure C)

**Document Version:** 1.0
**Date:** October 28, 2025
**Purpose:** Identify synergies between manual Seedling C conversion and SWFRecomp AS3 implementation (both using pure C)
**Language:** Pure C (per LittleCube's guidance)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Why Pure C Changes the Synergies](#why-pure-c-changes-the-synergies)
3. [Shared Components](#shared-components)
4. [Knowledge Transfer](#knowledge-transfer)
5. [Testing & Validation](#testing--validation)
6. [Incremental Migration Strategy](#incremental-migration-strategy)
7. [Hybrid Runtime Architecture](#hybrid-runtime-architecture)
8. [Development Workflow](#development-workflow)
9. [Cost-Benefit Analysis](#cost-benefit-analysis)
10. [Long-term Evolution](#long-term-evolution)

---

## Executive Summary

### Key Insight

The manual C conversion and SWFRecomp (C) projects have **strong but different synergies** compared to C++. Pure C's philosophy (explicit, minimal, direct) means shared code must be carefully designed for reusability.

### Top Synergies (Pure C)

1. **Shared C Runtime Core** - Common type system, memory management, core data structures
2. **Flash API Implementation** - Handwritten Flash APIs used by both (50+ classes)
3. **Reference Implementation** - Manual conversion validates SWFRecomp's behavior
4. **Test Oracle** - Parallel execution catches bugs in both projects
5. **Knowledge Transfer** - Learnings about AS3 semantics flow both ways
6. **Build System** - Shared CMake + Emscripten configuration

### Differences from C++ Approach

| Aspect | C++ Synergy | Pure C Synergy | Impact |
|--------|-------------|----------------|--------|
| **Code reuse** | High (templates, inheritance) | Medium (manual patterns) | Need careful design |
| **FlashPunk port** | Share directly | Share concepts, not code | Less direct reuse |
| **Flash APIs** | Share classes | Share C implementations | Strong synergy |
| **Type system** | std::shared_ptr, etc. | Manual refcounting | Different implementations |
| **Polymorphism** | Virtual functions | Function pointers | Same concepts, different syntax |

### Strategic Recommendation

**Do both approaches in parallel with shared C infrastructure, but expect less direct code sharing than C++.**

This approach:
- ✅ Gets Seedling playable quickly (manual, 3-5 months)
- ✅ Builds reusable AS3 tooling (SWFRecomp, 6-10 months)
- ✅ Validates both implementations against each other
- ✅ Shares Flash API implementations (300-500 hours saved)
- ✅ Shares core runtime patterns (200-300 hours saved)
- ⚠️ Less direct code reuse than C++ (different architectures)

**Total Effort:**
- Manual alone: 480-960 hours
- SWFRecomp alone: 900-1600 hours
- **Both with synergy: 1100-1900 hours** (vs 1380-2560 separate)
- **Savings: 280-660 hours (20-25%)**

---

## Why Pure C Changes the Synergies

### C++ Synergy Model

With C++, you can share code directly:

```cpp
// FlashPunk C++ (manual conversion)
class Entity {
public:
    virtual void update() = 0;
    virtual void render() = 0;
protected:
    float x, y;
};

class Player : public Entity {
    void update() override { /* handwritten */ }
    void render() override { /* handwritten */ }
};

// SWFRecomp generated code (uses same base classes)
class GeneratedEnemy : public Entity {
    void update() override { /* generated */ }
    void render() override { /* generated */ }
};
```

**Easy sharing** - Same base classes, same inheritance, same virtual functions.

### Pure C Synergy Model

With pure C, sharing is more complex:

```c
// Manual conversion approach (optimized for Seedling)
typedef struct Entity {
    float x, y;
    void (*update)(struct Entity*);
    void (*render)(struct Entity*);
    void* data;  // Simple type-specific data
} Entity;

typedef struct {
    Entity base;
    int health;
    float vx, vy;
} Player;

// SWFRecomp generated approach (generic AS3 support)
typedef struct AS3Object {
    AS3Class* klass;
    AS3Object* prototype;
    HashMap* properties;
    AS3Value** slots;
    uint32_t slot_count;
} AS3Object;

typedef struct AS3Class {
    const char* name;
    AS3Class* super_class;
    AS3Function** methods;
    // ... full AS3 semantics
} AS3Class;
```

**Different architectures**:
- Manual: Lightweight, game-specific
- SWFRecomp: Generic, AS3-compliant

**Can still share:**
- Flash API implementations (common ground)
- Core algorithms (collision, rendering)
- Asset loading
- Build system

**Cannot share as easily:**
- Entity architecture (different complexity levels)
- Memory management (manual vs refcounting)
- Type systems (simple vs AS3Value)

---

## Shared Components

### 1. Flash API C Implementations

**Problem:** Both projects need Flash APIs (BitmapData, Point, Rectangle, Sound, etc.)

**Solution:** Share a single C implementation library

**Architecture:**

```
libflash-c/
├── include/
│   ├── flash/
│   │   ├── display/
│   │   │   ├── bitmap_data.h
│   │   │   ├── sprite.h
│   │   │   ├── graphics.h
│   │   │   └── ...
│   │   ├── geom/
│   │   │   ├── point.h
│   │   │   ├── rectangle.h
│   │   │   ├── matrix.h
│   │   │   └── ...
│   │   ├── media/
│   │   │   ├── sound.h
│   │   │   └── sound_channel.h
│   │   └── net/
│   │       └── shared_object.h
│   └── common/
│       ├── refcount.h       # Shared refcounting
│       └── types.h          # Common types
├── src/
│   └── ... (implementations)
└── CMakeLists.txt

Manual Seedling:
  seedling.c
    ↓ uses
  libflash-c (SHARED)
    ↓ uses
  SDL2

SWFRecomp Runtime:
  generated_seedling.c
    ↓ uses
  avm2_runtime.c
    ↓ uses
  libflash-c (SAME SHARED LIBRARY)
    ↓ uses
  SDL2
```

**Example: Point (Pure C)**

```c
// flash/geom/point.h
#ifndef FLASH_GEOM_POINT_H
#define FLASH_GEOM_POINT_H

typedef struct Point {
    double x;
    double y;
} Point;

// Constructors
Point point_create(double x, double y);
Point point_zero(void);

// Methods
double point_distance(Point a, Point b);
Point point_add(Point a, Point b);
Point point_subtract(Point a, Point b);
double point_length(Point p);
Point point_normalize(Point p);
void point_offset(Point* p, double dx, double dy);

#endif
```

```c
// flash/geom/point.c
#include "flash/geom/point.h"
#include <math.h>

Point point_create(double x, double y) {
    Point p = {x, y};
    return p;
}

Point point_zero(void) {
    return point_create(0.0, 0.0);
}

double point_distance(Point a, Point b) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    return sqrt(dx * dx + dy * dy);
}

Point point_add(Point a, Point b) {
    return point_create(a.x + b.x, a.y + b.y);
}

// ... more implementations
```

**Usage in Manual Conversion:**
```c
// player.c (manual)
#include "flash/geom/point.h"

void player_update(Player* player) {
    Point pos = point_create(player->entity.x, player->entity.y);
    Point vel = point_create(player->vx, player->vy);
    Point new_pos = point_add(pos, vel);

    player->entity.x = new_pos.x;
    player->entity.y = new_pos.y;
}
```

**Usage in SWFRecomp Generated Code:**
```c
// generated_player.c (from SWFRecomp)
#include "flash/geom/point.h"
#include "avm2_runtime.h"

void Player_update(AVM2Context* ctx, AS3Value* this_obj) {
    // Extract position from AS3 object
    AS3Value* x_val = getProperty(this_obj, "x");
    AS3Value* y_val = getProperty(this_obj, "y");
    double x = toNumber(x_val);
    double y = toNumber(y_val);

    // Use same Point implementation
    Point pos = point_create(x, y);
    Point vel = point_create(/* vx */, /* vy */);
    Point new_pos = point_add(pos, vel);

    // Store back in AS3 object
    setProperty(this_obj, "x", createNumber(new_pos.x));
    setProperty(this_obj, "y", createNumber(new_pos.y));
}
```

**Shared Flash API Classes (Pure C):**

| Class | Complexity | Shared Lines | Effort Savings |
|-------|------------|--------------|----------------|
| Point | Low | 150 | 5-8h |
| Rectangle | Low | 200 | 6-10h |
| Matrix | Medium | 300 | 10-15h |
| BitmapData | High | 800 | 30-50h |
| Sprite | High | 600 | 25-40h |
| Sound | Medium | 400 | 15-25h |
| SoundChannel | Low | 200 | 8-12h |
| SharedObject | Medium | 500 | 20-30h |
| Graphics | High | 700 | 30-45h |
| ColorTransform | Medium | 250 | 10-15h |
| **Total (50 classes)** | | **~15,000** | **400-700h** |

**Effort Savings:**
- Manual implementation: 400-700 hours (one-time cost)
- Reused by SWFRecomp: Saves 400-700 hours
- **Net benefit: Break-even at 1 game, major savings at 2+ games**

---

### 2. Core Runtime Utilities

**Problem:** Both need common C utilities

**Solution:** Shared utility library

**common/refcount.h:**
```c
// Reference counting helpers
typedef struct Refcounted {
    uint32_t refcount;
} Refcounted;

#define REFCOUNT_INIT(obj) ((obj)->refcount = 1)

static inline void* retain(void* ptr) {
    if (ptr) {
        ((Refcounted*)ptr)->refcount++;
    }
    return ptr;
}

static inline void release(void* ptr, void (*destructor)(void*)) {
    if (ptr) {
        Refcounted* rc = (Refcounted*)ptr;
        if (--rc->refcount == 0) {
            if (destructor) destructor(ptr);
            free(ptr);
        }
    }
}
```

**common/array.h:**
```c
// Dynamic array (like std::vector)
typedef struct {
    void** items;
    uint32_t count;
    uint32_t capacity;
} Array;

Array* array_create(uint32_t initial_capacity);
void array_push(Array* arr, void* item);
void* array_get(Array* arr, uint32_t index);
void array_destroy(Array* arr, void (*item_destructor)(void*));
```

**common/hashmap.h:**
```c
// Already exists: c-hashmap library
// Used for dynamic properties in both projects
```

**Effort Savings:**
- Implement once: 40-80 hours
- Reused by both: Saves 40-80 hours

---

### 3. Asset Pipeline

**Problem:** Both need to load PNG, MP3, OEL files

**Solution:** Shared asset loading code

**assets/asset_loader.h:**
```c
typedef struct {
    SDL_Texture** textures;
    uint32_t texture_count;

    Mix_Chunk** sounds;
    uint32_t sound_count;

    Mix_Music** music;
    uint32_t music_count;
} AssetManager;

AssetManager* asset_manager_create(SDL_Renderer* renderer);
SDL_Texture* asset_load_texture(AssetManager* mgr, const char* path);
Mix_Chunk* asset_load_sound(AssetManager* mgr, const char* path);
Mix_Music* asset_load_music(AssetManager* mgr, const char* path);
void asset_manager_destroy(AssetManager* mgr);
```

**Effort Savings:**
- Implement once: 20-40 hours
- Reused by both: Saves 20-40 hours

---

### 4. Build System

**Problem:** Both need CMake + Emscripten builds

**Solution:** Shared CMakeLists.txt templates

**CMakeLists.txt (shared pattern):**
```cmake
cmake_minimum_required(VERSION 3.10)
project(${PROJECT_NAME} C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Common compiler flags
if(NOT EMSCRIPTEN)
    # Native build
    add_compile_options(-Wall -Wextra -Werror)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Source files
file(GLOB_RECURSE SOURCES "src/*.c")

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
if(EMSCRIPTEN)
    # WASM build
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_MIXER=2 -s ALLOW_MEMORY_GROWTH=1 --preload-file assets"
    )
else()
    # Native build
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2 SDL2_image::SDL2_image SDL2_mixer::SDL2_mixer m)
endif()
```

**Effort Savings:**
- Create template: 10-20 hours
- Reused by both: Saves 10-20 hours each

---

## Knowledge Transfer

### 1. AS3 Semantics Understanding

**Flow: Manual → SWFRecomp**

When manually converting Seedling, you discover:
- How AS3 type coercion works in practice
- Edge cases in collision detection
- How FlashPunk's entity system actually works
- Performance bottlenecks in Flash APIs

**Benefits for SWFRecomp:**
- Know which AS3 features are actually used
- Know which optimizations matter
- Know which Flash APIs are critical
- Know common patterns to optimize

**Example:**

During manual conversion, discover:
```c
// AS3: var x:Number = player.x + enemy.x;
// Simple in AS3, but what about edge cases?

// Testing reveals:
// - Usually just: double add (fast path)
// - Sometimes: Number + int requires coercion
// - Rarely: Number + String requires toString

// SWFRecomp can optimize for common case:
double add_numbers(AS3Value* a, AS3Value* b) {
    // Fast path (90% of cases in Seedling)
    if (a->type == TYPE_NUMBER && b->type == TYPE_NUMBER) {
        return a->value.d + b->value.d;
    }

    // Slow path (10% of cases)
    return toNumber(a) + toNumber(b);
}
```

**Effort Savings:** 50-100 hours (better design decisions)

### 2. Flash API Requirements

**Flow: Manual → SWFRecomp**

Manual conversion reveals:
- Which Flash API methods are actually used
- Which methods can be stubbed
- Which methods are performance-critical

**Example:**

```c
// BitmapData in Seedling uses:
// - copyPixels() - CRITICAL (used 100+ times per frame)
// - draw() - USED (10-20 times per frame)
// - getPixel() - RARELY (only in collision)
// - setPixel() - NEVER (not used)

// SWFRecomp can prioritize:
// 1. Implement copyPixels() first (critical path)
// 2. Implement draw() second
// 3. Implement getPixel() later
// 4. Skip setPixel() entirely for Seedling
```

**Effort Savings:** 100-200 hours (avoid implementing unused features)

### 3. Performance Insights

**Flow: Manual → SWFRecomp**

Manual conversion with profiling reveals:
- Entity updates are called 60x/second (hot path)
- Collision detection is expensive (needs spatial partitioning)
- Rendering can be batched (don't flush per sprite)

**SWFRecomp can generate optimized code:**
```c
// Instead of naive generation:
void entity_update_all(Entity** entities, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        // Generic call (slow)
        entities[i]->update(entities[i]);
    }
}

// Generate optimized version:
void entity_update_all_optimized(Entity** entities, uint32_t count) {
    // Batch by type for better cache locality
    for (uint32_t i = 0; i < count; i++) {
        if (entities[i]->type == ENTITY_PLAYER) {
            player_update_batch(entities, count, i);
        }
        // ... more batching
    }
}
```

**Effort Savings:** 50-100 hours (better code generation)

### 4. Reference Behavior

**Flow: SWFRecomp → Manual**

When SWFRecomp implements AS3 spec faithfully, manual conversion can learn:
- Correct AS3 edge case behavior
- Subtle type coercion rules
- Proper exception handling

**Example:**

```c
// Manual conversion might do:
int result = (int)(a + b);  // Naive

// SWFRecomp spec-compliant:
int result = toInt32(add(a, b));  // Correct (handles NaN, Infinity, overflow)

// Learn from SWFRecomp and fix manual version
```

**Effort Savings:** 20-40 hours (fewer bugs)

---

## Testing & Validation

### 1. Differential Testing

**Concept:** Run both implementations side-by-side, compare outputs

**Architecture:**

```
Test Framework:
  ┌─────────────────────────────────────┐
  │  Seedling Test Inputs               │
  │  - Input sequences (recorded)       │
  │  - Random seeds                     │
  │  - Level data                       │
  └─────────────┬───────────────────────┘
                │
        ┌───────┴────────┐
        ▼                ▼
  ┌──────────┐    ┌──────────────┐
  │ Manual C │    │ SWFRecomp C  │
  │ Version  │    │ Generated    │
  └────┬─────┘    └──────┬───────┘
       │                 │
       │  Frame-by-frame │
       │  state capture  │
       │                 │
       ▼                 ▼
  ┌─────────────────────────────┐
  │  Diff Tool                  │
  │  - Compare positions        │
  │  - Compare health/state     │
  │  - Compare render output    │
  │  - Report differences       │
  └─────────────────────────────┘
```

**Implementation:**

```c
// test_differential.c
typedef struct GameState {
    float player_x, player_y;
    int player_health;
    uint32_t enemy_count;
    // ... more state
} GameState;

GameState manual_run_frame(Input input);
GameState swfrecomp_run_frame(Input input);

void test_differential(void) {
    Input inputs[1000];  // Recorded input sequence

    for (int frame = 0; frame < 1000; frame++) {
        GameState manual = manual_run_frame(inputs[frame]);
        GameState swfrecomp = swfrecomp_run_frame(inputs[frame]);

        // Compare states
        if (fabs(manual.player_x - swfrecomp.player_x) > 0.01) {
            printf("Frame %d: player_x differs: %.2f vs %.2f\n",
                   frame, manual.player_x, swfrecomp.player_x);
        }

        // ... compare more fields
    }
}
```

**Benefits:**
- Catches bugs in both implementations
- Validates SWFRecomp behavior against known-good manual version
- Validates manual version against spec-compliant SWFRecomp

**Effort:** 40-80 hours to set up, ongoing benefits

### 2. Performance Benchmarking

**Compare performance characteristics:**

```c
// benchmark.c
void benchmark_both(void) {
    // Manual version
    uint64_t manual_start = get_time_us();
    for (int i = 0; i < 1000; i++) {
        manual_run_frame(/* ... */);
    }
    uint64_t manual_end = get_time_us();

    // SWFRecomp version
    uint64_t swfrecomp_start = get_time_us();
    for (int i = 0; i < 1000; i++) {
        swfrecomp_run_frame(/* ... */);
    }
    uint64_t swfrecomp_end = get_time_us();

    printf("Manual:    %.2f ms/frame\n",
           (manual_end - manual_start) / 1000.0 / 1000.0);
    printf("SWFRecomp: %.2f ms/frame\n",
           (swfrecomp_end - swfrecomp_start) / 1000.0 / 1000.0);
}
```

**Insights:**
- If manual is faster: Learn what optimizations to add to SWFRecomp
- If SWFRecomp is faster: Learn what to improve in manual

**Effort:** 20-40 hours

---

## Incremental Migration Strategy

### Strategy: Start Manual, Migrate to Generated

**Phase 1: Manual Seedling (Month 1-3)**
- Implement entire game manually in C
- Get it working, playable, optimized

**Phase 2: SWFRecomp Development (Month 4-9)**
- Build AS3 support in SWFRecomp
- Use manual Seedling as reference

**Phase 3: Partial Generation (Month 10-11)**
- Keep handwritten FlashPunk
- Generate game entity code only
- Mix handwritten and generated code

**Phase 4: Full Generation (Month 12+)**
- Generate everything except Flash APIs
- Manual Flash APIs become the runtime

**Hybrid Executable:**

```
Final Seedling Binary:
├── Flash APIs (handwritten C, 15,000 lines)
│   └── Shared by both
├── FlashPunk (handwritten C, 3,000 lines)
│   └── Highest quality, handwritten
├── Core game (EITHER)
│   ├── Handwritten (3,000 lines, fast development)
│   └── OR Generated (from ABC, automated)
└── Entity code (MIXED)
    ├── Complex entities (handwritten, optimized)
    └── Simple entities (generated, automated)
```

**Benefits:**
- Not all-or-nothing
- Can keep best of both
- Gradual migration as SWFRecomp matures

---

## Hybrid Runtime Architecture

### Concept: Single Binary, Mixed Code

```c
// Handwritten entity (manual conversion)
typedef struct {
    Entity base;
    int health;
    float vx, vy;
} Player;

void player_update(Entity* ent) {
    Player* p = (Player*)ent;
    // Handwritten, optimized code
    // ...
}

// Generated entity (from SWFRecomp)
typedef struct {
    AS3Object base;
    // Generated structure from ABC
} GeneratedEnemy;

void generated_enemy_update(AS3Object* obj) {
    // Generated code from ABC bytecode
    // Uses AS3Value, generic but correct
    // ...
}

// Unified entity list
Entity* entities[1000];  // Mix of handwritten and generated

void update_all_entities(void) {
    for (int i = 0; i < entity_count; i++) {
        // Works for both!
        entities[i]->update(entities[i]);
    }
}
```

**Challenge:** Different type systems

**Solution:** Adapter layer

```c
// adapter.c
Entity* wrap_as3_object(AS3Object* obj) {
    Entity* ent = entity_create();
    ent->data = obj;
    ent->update = as3_object_update_wrapper;
    return ent;
}

void as3_object_update_wrapper(Entity* ent) {
    AS3Object* obj = (AS3Object*)ent->data;
    // Call AS3 update method
    call_as3_method(obj, "update");
}
```

**Effort:** 40-80 hours for adapter layer

---

## Cost-Benefit Analysis

### Scenario 1: Manual Only

**Effort:**
- Manual conversion: 480-960 hours
- **Total: 480-960 hours**

**Deliverables:**
- Seedling playable (1 game)

**Pros:**
- Fast (3-5 months)
- Smallest binary
- Best performance

**Cons:**
- No other games
- No automation
- No Flash preservation

---

### Scenario 2: SWFRecomp Only

**Effort:**
- SWFRecomp AS3: 900-1600 hours
- **Total: 900-1600 hours**

**Deliverables:**
- Seedling playable (1 game)
- Tool for other games

**Pros:**
- Reusable for other games
- Automated

**Cons:**
- Slow (6-10 months)
- Larger binary than manual
- Slower runtime than manual

---

### Scenario 3: Both with Synergy (RECOMMENDED)

**Effort:**
- Manual conversion: 480-960 hours
- Flash API implementation: **Already done in manual** (save 400-700h)
- SWFRecomp AS3: 900-1600 hours
- **Minus shared Flash APIs:** -400-700h
- **Minus knowledge transfer:** -100-200h
- **Plus adapter layer:** +40-80h
- **Total: 920-1740 hours**

**Compared to separate:**
- Separate: 480 + 900 = 1380 (best case) to 960 + 1600 = 2560 (worst case)
- Synergy: 920-1740 hours
- **Savings: 280-820 hours (20-40%)**

**Deliverables:**
- Seedling playable quickly (manual, month 3)
- Seedling playable via SWFRecomp (month 10)
- Tool for other games
- Both implementations validate each other

**Pros:**
- ✅ Best of both worlds
- ✅ Significant time savings
- ✅ Risk mitigation (two approaches)
- ✅ Quality improvement (mutual validation)

**Cons:**
- ❌ More complex project management
- ❌ Need discipline to share code

---

### Break-Even Analysis

**Games needed to justify SWFRecomp:**

| Approach | 1 Game | 2 Games | 5 Games | 10 Games |
|----------|--------|---------|---------|----------|
| **Manual only** | 480-960h | 960-1920h | 2400-4800h | 4800-9600h |
| **SWFRecomp only** | 900-1600h | 1000-1700h | 1200-2000h | 1400-2400h |
| **Both (synergy)** | 920-1740h | 1020-1840h | 1220-2140h | 1420-2540h |

**Break-even:**
- **1 game:** Manual is fastest
- **2 games:** Synergy becomes competitive
- **3+ games:** Synergy or SWFRecomp best

**Recommendation:** If you want to preserve 3+ Flash games, do both with synergy.

---

## Development Workflow

### Parallel Development Schedule

**Months 1-3: Focus on Manual**
- Week 1-2: Setup + FlashPunk Entity system
- Week 3-6: FlashPunk Graphics + Collision
- Week 7-10: Core game systems
- Week 11-16: Game entities
- Week 17-20: Polish

**Output:** Playable Seedling (manual)

**Months 4-9: Focus on SWFRecomp**
- Month 4: ABC parser (C++)
- Month 5: Code generator (C++)
- Month 6-7: Type system + Core opcodes (C)
- Month 8-9: Object model (C)

**Months 10-12: Integration**
- Month 10: Generate Seedling code
- Month 11: Test SWFRecomp version
- Month 12: Differential testing, benchmarks

### Team Organization (if multiple people)

**Developer A: Manual Conversion Specialist**
- Implements Flash APIs (shared)
- Implements FlashPunk (handwritten)
- Implements game entities
- **Output:** Shared C library + Seedling manual

**Developer B: SWFRecomp Specialist**
- Implements ABC parser (C++)
- Implements code generator (C++)
- Implements AVM2 runtime (C)
- **Uses:** Flash APIs from Developer A

**Synergy Points:**
- Weekly: Discuss Flash API needs
- Monthly: Share findings about AS3 semantics
- End: Differential testing together

---

## Long-term Evolution

### Evolution Path

**Year 1:**
- Manual Seedling complete
- SWFRecomp AS3 complete
- Flash API library mature

**Year 2:**
- Port 5-10 more games using SWFRecomp
- Discover which manual optimizations to add to generator
- Expand Flash API library

**Year 3:**
- Mature SWFRecomp can handle most games automatically
- Manual conversion only for games needing special optimization
- Flash API library supports 90% of Flash games

### Maintenance Strategy

**Shared Flash APIs:**
- Maintained by both projects
- Bug fixes benefit both
- New features added as needed

**Manual Seedling:**
- Keep as reference implementation
- Use for regression testing SWFRecomp
- Update as needed for new features

**SWFRecomp:**
- Primary focus for new games
- Learn from manual conversion patterns
- Gradually approach manual's performance

---

## Conclusion

### Summary

Doing **both manual C conversion and SWFRecomp** in parallel with shared infrastructure provides:

**Quantified Benefits:**
- ✅ **280-820 hours saved** (20-40%) through code sharing
- ✅ **3 months** for playable Seedling (manual)
- ✅ **10 months** for automated Flash preservation (SWFRecomp)
- ✅ **Mutual validation** catches bugs in both
- ✅ **Risk mitigation** - two working approaches

**Shared Components:**
- Flash API library (400-700h, used by both)
- Asset pipeline (20-40h, used by both)
- Build system (10-20h, used by both)
- Knowledge about AS3 (150-300h value)

**Total Effort:**
- Manual alone: 480-960h
- SWFRecomp alone: 900-1600h
- **Both with synergy: 920-1740h** (vs 1380-2560 separate)

### Recommendation

**For Seedling + future Flash games: Do both.**

The synergies are strong enough to justify parallel development, especially:
1. **Flash API library** - 400-700 hours saved
2. **Knowledge transfer** - Better design for both
3. **Testing** - Mutual validation
4. **Flexibility** - Can choose best approach per game

This is not "twice the work" - it's **1.5x the work for 2x the benefit**.