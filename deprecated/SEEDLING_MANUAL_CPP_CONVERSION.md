# Seedling Manual C++ Conversion Plan

**Document Version:** 1.0
**Date:** October 28, 2024
**Game:** Seedling by Danny Yaroslavski (Alexander Ocias)
**Target:** Manual conversion from ActionScript 3 to C++, then compile to WASM

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Comparison: SWFRecomp vs Manual Conversion](#comparison-swfrecomp-vs-manual-conversion)
3. [Project Overview](#project-overview)
4. [Conversion Strategy](#conversion-strategy)
5. [Phase-by-Phase Plan](#phase-by-phase-plan)
6. [Technology Stack](#technology-stack)
7. [AS3 to C++ Mapping](#as3-to-c-mapping)
8. [FlashPunk to C++ Port](#flashpunk-to-c-port)
9. [Asset Management](#asset-management)
10. [Build System](#build-system)
11. [Effort Estimation](#effort-estimation)
12. [Risk Assessment](#risk-assessment)
13. [Testing Strategy](#testing-strategy)
14. [Success Criteria](#success-criteria)

---

## Executive Summary

### Goal

Manually convert the Seedling game from ActionScript 3 to C++, then compile to native and WebAssembly, bypassing the SWFRecomp approach entirely.

### Why Consider Manual Conversion?

**Advantages:**
- No dependency on SWFRecomp AS3 implementation (saves 4-7 months)
- Direct control over architecture and performance
- Can use modern C++ patterns and libraries
- Cleaner codebase without Flash legacy baggage
- Easier to optimize and maintain
- Can target more platforms (native Windows/Mac/Linux + WASM)

**Disadvantages:**
- Must manually translate all 209 AS3 files
- Must manually port FlashPunk framework (48 files)
- Cannot automatically convert other Flash games
- No preservation of original bytecode

### Key Findings

**Seedling Codebase:**
- **209 AS3 files** (~61,000 lines total)
  - 17 core files (6,987 lines)
  - 161 game object files (49,000 lines)
  - 48 FlashPunk files (12,000 lines)
- **Assets:** 100+ graphics, 115+ levels, 50+ sounds
- **Architecture:** Entity-component system via FlashPunk
- **Complexity:** Medium (well-structured, minimal AS3-specific features)

### Effort Comparison

| Approach | Duration | Total Effort | Main Work |
|----------|----------|--------------|-----------|
| **SWFRecomp AS3** | 4-7 months | 600-1000 hours | Build AS3 recompiler |
| **Manual Conversion** | **2-4 months** | **320-640 hours** | **Port code + assets** |
| **Savings** | **50%** | **40-45%** | Different skillset |

### Recommendation

**Manual conversion is faster for a single game** if:
- You only want to port Seedling (not other Flash games)
- You have C++ experience
- You want full control over the result
- You want optimal performance

**Use SWFRecomp if:**
- You want to preserve many Flash games
- You want automatic conversion
- You prefer Flash-faithful behavior
- You're willing to invest in tooling first

---

## Comparison: SWFRecomp vs Manual Conversion

### Timeline Comparison

| Phase | SWFRecomp Approach | Manual Approach | Winner |
|-------|-------------------|-----------------|--------|
| **Phase 0: Prerequisites** | 1-2 months (AS1/2 stable) | 0 weeks | ✅ Manual |
| **Phase 1: Analysis** | 2-3 weeks (FlashPunk) | 1 week (codebase) | ✅ Manual |
| **Phase 2: Core Implementation** | 2-3 months (ABC parser + opcodes) | 2-3 weeks (FlashPunk port) | ✅ Manual |
| **Phase 3: Object Model** | 2-3 months (classes + Flash APIs) | 2-3 weeks (Core systems) | ✅ Manual |
| **Phase 4: Game Features** | 1-2 months (Seedling requirements) | 3-5 weeks (Game code) | ≈ Tie |
| **Phase 5: Integration** | 1 month (testing) | 1-2 weeks (polish) | ✅ Manual |
| **TOTAL** | **8-12 months** | **2-4 months** | ✅ Manual (3x faster) |

### Effort Comparison (Hours)

| Task Category | SWFRecomp | Manual | Difference |
|--------------|-----------|--------|------------|
| Analysis | 60-80 | 40-60 | -20 |
| ABC Parser | 300-500 | 0 | -400 |
| Opcodes | 200-300 | 0 | -250 |
| Object Model | 250-400 | 0 | -325 |
| Flash APIs | 300-500 | 0 | -400 |
| **FlashPunk** | (included in Flash APIs) | 80-160 | +120 |
| **Game Code** | (automatic) | 160-320 | +240 |
| **Assets** | (automatic) | 40-80 | +80 |
| **TOTAL** | **1210-1830** | **320-640** | **-990 avg** |

### Reusability Comparison

| Aspect | SWFRecomp | Manual | Notes |
|--------|-----------|--------|-------|
| **Can port other Flash games?** | ✅ YES (automatic) | ❌ NO (manual work) | SWFRecomp wins long-term |
| **Preserves Flash behavior?** | ✅ YES (faithful) | ⚠️ PARTIAL (approximation) | SWFRecomp more accurate |
| **Optimal performance?** | ⚠️ MAYBE (overhead) | ✅ YES (native C++) | Manual wins |
| **Maintainability?** | ⚠️ COMPLEX (generated) | ✅ CLEAN (handwritten) | Manual wins |
| **Can modify game?** | ⚠️ HARD (regenerate) | ✅ EASY (edit C++) | Manual wins |
| **Platform support?** | Limited by runtime | Full control | Manual wins |

### Conclusion

**For Seedling specifically: Manual conversion is 3x faster and produces better code.**

**For Flash preservation generally: SWFRecomp is better for archiving many games.**

---

## Project Overview

### Seedling Codebase Structure

From the codebase analysis in ~/projects/Seedling:

```
Seedling/
├── src/                           # 209 AS3 source files
│   ├── Main.as                   # Entry point (241 lines)
│   ├── Game.as                   # Main game world (1,874 lines) ⭐ LARGEST
│   ├── Player.as                 # Player logic (1,967 lines) ⭐ COMPLEX
│   ├── Mobile.as                 # Movement base class (1,089 lines)
│   ├── Enemies/                  # 30 enemy types (12,500 lines)
│   ├── NPCs/                     # 17 NPC types (4,800 lines)
│   ├── Pickups/                  # 21 pickup types (3,200 lines)
│   ├── Projectiles/              # 10 projectile types (2,800 lines)
│   ├── Puzzlements/              # 24 puzzle types (7,100 lines)
│   ├── Scenery/                  # 42 scenery types (6,500 lines)
│   └── net/flashpunk/            # FlashPunk framework (48 files, ~12,000 lines)
│       ├── Engine.as             # Main engine loop
│       ├── World.as              # Scene container
│       ├── Entity.as             # GameObject base
│       ├── graphics/             # Rendering (Image, Spritemap, etc.)
│       ├── masks/                # Collision (Hitbox, Pixelmask, Grid)
│       └── utils/                # Input, Draw, Key, etc.
├── assets/                       # External assets
│   ├── gfx/                      # 100+ PNG images
│   │   ├── player.png
│   │   ├── enemies.png
│   │   ├── tiles.png
│   │   └── ...
│   ├── levels/                   # 115+ OEL level files (XML)
│   ├── sfx/                      # 50+ MP3 sound effects
│   └── music/                    # 12+ MP3 music tracks
└── Shrum.as3proj                 # FlashDevelop project file

Total: ~61,000 lines of AS3 code
```

### Core Systems

**1. Game Loop (Main.as + Engine.as)**
- 60 FPS fixed timestep
- Update → Render cycle
- World/Scene management

**2. Entity System (Entity.as + Mobile.as)**
- Entity base class (position, type, layer, active, visible)
- Mobile extends Entity (velocity, acceleration, physics)
- Component-like graphics and masks

**3. Graphics System (graphics/*)**
- BitmapData-based rendering (software rendering)
- Spritemap for animations
- Image for static sprites
- Tilemap for levels
- Multiple render layers

**4. Collision System (masks/*)**
- Hitbox (rectangle collision)
- Pixelmask (pixel-perfect collision)
- Grid (tile-based collision)
- Collision callbacks

**5. Input System (utils/Input.as + utils/Key.as)**
- Keyboard state tracking
- Key mapping (arrow keys, X/C/V actions)

**6. Audio System (Sfx.as + Music.as)**
- Sound effect playback
- Music streaming
- Volume control

**7. Level System (Game.as)**
- OEL (Ogmo Editor) XML level format
- Tile-based levels (16x16 tiles)
- Entity placement from XML
- Level state persistence

**8. Save System (Main.as + SharedObject)**
- Player progress
- Inventory state
- Achievement tracking
- Level completion flags

### Conversion Approach

**Core Strategy: Port FlashPunk first, then game code**

1. **Phase 1: Foundation** (2-3 weeks)
   - Set up C++ project structure
   - Port FlashPunk core (Engine, World, Entity)
   - Implement graphics system (SDL2 + textures)
   - Implement input system

2. **Phase 2: Core Game Systems** (2-3 weeks)
   - Port Player.as
   - Port Mobile.as (physics base)
   - Port collision system
   - Port level loading (OEL XML)
   - Port save system

3. **Phase 3: Game Content** (3-5 weeks)
   - Port all enemy types (30 classes)
   - Port all NPC types (17 classes)
   - Port all pickup types (21 classes)
   - Port all projectile types (10 classes)
   - Port all puzzle types (24 classes)
   - Port all scenery types (42 classes)

4. **Phase 4: Polish** (1-2 weeks)
   - Asset integration
   - Sound system
   - UI rendering
   - Testing and bug fixes
   - Performance optimization
   - WASM build

---

## Conversion Strategy

### Manual Conversion Workflow

**Per File:**

1. **Read AS3 source** (understand logic)
2. **Create C++ header** (.h file with class declaration)
3. **Create C++ implementation** (.cpp file with methods)
4. **Convert AS3 patterns to C++** (see mapping section)
5. **Replace Flash APIs with C++ equivalents**
6. **Test incrementally**

### Translation Tools

**Semi-Automated Approach:**
- Use regex/scripts to handle mechanical conversions
- Manual review and adjustment
- Focus human effort on complex logic

**Example Script (Python):**
```python
# Convert AS3 class skeleton to C++
import re

def convert_class(as3_code):
    # Extract class name
    match = re.search(r'class (\w+)', as3_code)
    classname = match.group(1)

    # Generate header
    header = f"class {classname} {{\npublic:\n"

    # Extract methods
    methods = re.findall(r'(public|private) function (\w+)\((.*?)\):(.*?)\{', as3_code)
    for visibility, name, params, returntype in methods:
        header += f"    {returntype} {name}({params});\n"

    header += "};"
    return header
```

### Directory Structure (C++ Version)

```
SeedlingCpp/
├── CMakeLists.txt                # Main build file
├── src/
│   ├── main.cpp                  # Entry point
│   ├── core/                     # FlashPunk port
│   │   ├── Engine.h/cpp
│   │   ├── World.h/cpp
│   │   ├── Entity.h/cpp
│   │   ├── Tweener.h/cpp
│   │   ├── Graphics.h/cpp        # Base graphic class
│   │   ├── Mask.h/cpp            # Base mask class
│   │   └── ...
│   ├── graphics/                 # Graphics components
│   │   ├── Image.h/cpp
│   │   ├── Spritemap.h/cpp
│   │   ├── Tilemap.h/cpp
│   │   ├── Text.h/cpp
│   │   └── ...
│   ├── masks/                    # Collision components
│   │   ├── Hitbox.h/cpp
│   │   ├── Pixelmask.h/cpp
│   │   ├── Grid.h/cpp
│   │   └── ...
│   ├── utils/                    # Utilities
│   │   ├── Input.h/cpp
│   │   ├── Key.h/cpp
│   │   ├── Draw.h/cpp
│   │   └── ...
│   ├── game/                     # Seedling game code
│   │   ├── Main.h/cpp
│   │   ├── Game.h/cpp
│   │   ├── Player.h/cpp
│   │   ├── Mobile.h/cpp
│   │   ├── enemies/
│   │   ├── npcs/
│   │   ├── pickups/
│   │   ├── projectiles/
│   │   ├── puzzlements/
│   │   └── scenery/
│   └── flash/                    # Flash API subset (minimal)
│       ├── geom/
│       │   ├── Point.h/cpp
│       │   └── Rectangle.h/cpp
│       └── utils/
│           └── SharedObject.h/cpp
├── assets/                       # Asset files
│   ├── gfx/
│   ├── levels/
│   ├── sfx/
│   └── music/
├── external/                     # Third-party libraries
│   ├── SDL2/
│   ├── SDL2_mixer/
│   ├── pugixml/                  # XML parsing
│   └── nlohmann_json/            # JSON for saves
└── wasm/                         # WASM-specific code
    ├── CMakeLists.txt
    ├── index.html
    └── shell.js
```

---

## Phase-by-Phase Plan

### Phase 0: Setup (2-3 days)

**Goal:** Project infrastructure ready

**Tasks:**

**0.1: Development Environment**
- [ ] Install C++ compiler (g++/clang)
- [ ] Install CMake 3.20+
- [ ] Install SDL2 development libraries
- [ ] Install Emscripten (for WASM)
- [ ] Set up IDE (VSCode/CLion/Visual Studio)

**0.2: Project Structure**
- [ ] Create directory structure
- [ ] Create root CMakeLists.txt
- [ ] Create subdirectory CMakeLists.txt
- [ ] Set up Git repository
- [ ] Create .gitignore

**0.3: Third-Party Dependencies**
- [ ] Add SDL2 (graphics/input/audio)
- [ ] Add SDL2_mixer (audio playback)
- [ ] Add pugixml (XML parsing for OEL levels)
- [ ] Add nlohmann/json (JSON for save files)
- [ ] Configure CMake to find libraries

**0.4: Test Build**
- [ ] Create minimal main.cpp (SDL2 window)
- [ ] Build native executable
- [ ] Build WASM (emcc)
- [ ] Verify both targets work

**Deliverables:**
- Working build system
- Test executable (opens window)
- WASM test page

---

### Phase 1: FlashPunk Core (2-3 weeks)

**Goal:** Port FlashPunk framework to C++

**Estimated effort:** 80-160 hours

**Priority: CRITICAL - Everything depends on this**

#### 1.1: Core Classes (1 week)

**Engine.as → Engine.h/cpp**
- [ ] Main loop (init, update, render)
- [ ] Fixed timestep (60 FPS)
- [ ] World management
- [ ] Input handling
- [ ] Rendering setup
- [ ] ~200 lines C++

**World.as → World.h/cpp**
- [ ] Entity container
- [ ] Add/remove entities
- [ ] Update all entities
- [ ] Render all entities (sorted by layer)
- [ ] Entity type lists
- [ ] Collision detection queries
- [ ] ~300 lines C++

**Entity.as → Entity.h/cpp**
- [ ] Position (x, y)
- [ ] Type string
- [ ] Layer (render order)
- [ ] Active/visible flags
- [ ] Graphic component
- [ ] Mask component (collision)
- [ ] World reference
- [ ] Lifecycle (added, removed, update, render)
- [ ] ~250 lines C++

**Tweener.as → Tweener.h/cpp** (base of Entity)
- [ ] Tween system (not critical for Seedling)
- [ ] Can stub initially
- [ ] ~50 lines C++

#### 1.2: Graphics System (1 week)

**Graphic.as → Graphics.h/cpp** (base class)
- [ ] Position offset
- [ ] Scrolling factors
- [ ] Visibility
- [ ] Render method (virtual)
- [ ] ~100 lines C++

**Image.as → Image.h/cpp**
- [ ] Load texture from file
- [ ] Render at position
- [ ] Scale, rotation, color tinting
- [ ] Flipping (horizontal/vertical)
- [ ] Alpha blending
- [ ] ~200 lines C++

**Spritemap.as → Spritemap.h/cpp** ⭐ CRITICAL
- [ ] Sprite sheet (texture atlas)
- [ ] Frame data (grid of frames)
- [ ] Animation system
  - [ ] Define animations (name, frame array, FPS, loop)
  - [ ] Play animation
  - [ ] Animation completion callbacks
- [ ] Current frame tracking
- [ ] Render current frame
- [ ] ~350 lines C++

**Tilemap.as → Tilemap.h/cpp**
- [ ] Tile grid (2D array)
- [ ] Load from OEL XML
- [ ] Render tiles (culled to screen)
- [ ] Tile properties (solid, etc.)
- [ ] ~250 lines C++

**Text.as → Text.h/cpp**
- [ ] Simple text rendering
- [ ] Can use SDL_ttf or bitmap font
- [ ] ~150 lines C++

#### 1.3: Collision System (3-4 days)

**Mask.as → Mask.h/cpp** (base class)
- [ ] Collision interface
- [ ] Position
- [ ] Parent entity reference
- [ ] ~80 lines C++

**Hitbox.as → Hitbox.h/cpp**
- [ ] Rectangle collision (x, y, width, height)
- [ ] Intersection test (rectangle vs rectangle)
- [ ] ~120 lines C++

**Pixelmask.as → Pixelmask.h/cpp** ⭐ IMPORTANT
- [ ] Pixel-perfect collision
- [ ] Bitmask data (per-pixel hit detection)
- [ ] Load from texture alpha channel
- [ ] Intersection test (pixelmask vs pixelmask)
- [ ] Intersection test (pixelmask vs rectangle)
- [ ] ~200 lines C++

**Grid.as → Grid.h/cpp**
- [ ] Tile-based collision grid
- [ ] Load from level data
- [ ] Tile collision queries
- [ ] Rectangle vs grid collision
- [ ] ~180 lines C++

#### 1.4: Utilities (2-3 days)

**Input.as → Input.h/cpp**
- [ ] Keyboard state tracking
- [ ] check(keycode) - is key pressed
- [ ] pressed(keycode) - was key just pressed
- [ ] released(keycode) - was key just released
- [ ] Update each frame
- [ ] ~100 lines C++

**Key.as → Key.h/cpp**
- [ ] Key code constants (UP, DOWN, LEFT, RIGHT, X, C, V, etc.)
- [ ] Map SDL keycodes to constants
- [ ] ~50 lines C++ (mostly enums)

**Draw.as → Draw.h/cpp**
- [ ] Simple drawing primitives
- [ ] Line, rectangle, circle
- [ ] Uses SDL_Renderer or direct pixel manipulation
- [ ] ~150 lines C++

**FP.as → FP.h/cpp** (global state)
- [ ] Current world
- [ ] Screen dimensions
- [ ] Random number generator
- [ ] Global functions (distance, clamp, etc.)
- [ ] ~100 lines C++

#### 1.5: Audio System (2 days)

**Sfx.as → Sfx.h/cpp**
- [ ] Load sound effect
- [ ] Play sound
- [ ] Volume control
- [ ] Uses SDL_mixer
- [ ] ~100 lines C++

**Music.as → Music.h/cpp** (custom for Seedling)
- [ ] Load music track
- [ ] Play/stop/pause
- [ ] Volume control
- [ ] Looping
- [ ] ~80 lines C++

**Deliverables:**
- FlashPunk C++ port (~2,500 lines)
- All core systems functional
- Test program (entity rendering, collision, input)

---

### Phase 2: Core Game Systems (2-3 weeks)

**Goal:** Port Seedling's core classes

**Estimated effort:** 120-200 hours

#### 2.1: Mobile Base Class (3 days)

**Mobile.as → Mobile.h/cpp**
- [ ] Extends Entity
- [ ] Velocity (velX, velY)
- [ ] Acceleration
- [ ] Friction
- [ ] Movement integration
- [ ] Solid collision detection
- [ ] Surface types (ice, water, lava, stairs)
- [ ] Health system
- [ ] Damage/knockback
- [ ] ~400 lines C++

#### 2.2: Player (1 week)

**Player.as → Player.h/cpp** ⭐ COMPLEX (1,967 lines AS3)
- [ ] Extends Mobile
- [ ] Input handling (8-direction movement)
- [ ] State machine (normal, attacking, hurt, dying, etc.)
- [ ] Animation control (idle, walk, attack, hurt)
- [ ] Weapon system (6 weapon types)
  - [ ] Stick (melee)
  - [ ] Wand (ranged)
  - [ ] Fire wand (fire shots)
  - [ ] Bow (arrows)
  - [ ] Dark sword (powerful melee)
  - [ ] Dark shield (defense)
- [ ] Attack logic per weapon
- [ ] Inventory system (items, seeds)
- [ ] Health/damage system
- [ ] Invincibility frames
- [ ] Interaction with NPCs/objects
- [ ] ~700 lines C++

#### 2.3: Main/Game (1 week)

**Main.as → Main.h/cpp**
- [ ] Extends Engine
- [ ] Initialization
- [ ] Load save file (SharedObject)
- [ ] Create initial world
- [ ] Global state (player stats, progress flags)
- [ ] ~150 lines C++

**Game.as → Game.h/cpp** ⭐ LARGEST (1,874 lines AS3)
- [ ] Extends World
- [ ] Level loading (OEL XML)
  - [ ] Parse level XML
  - [ ] Load tilemap
  - [ ] Instantiate entities from XML
- [ ] Player spawning
- [ ] Camera system (follow player)
- [ ] Level transitions
- [ ] Music management
- [ ] UI rendering (health, inventory)
- [ ] Pause menu
- [ ] Message system
- [ ] Save game
- [ ] 115+ level files
- [ ] ~800 lines C++

#### 2.4: Level System (3 days)

**OEL Level Loading**
- [ ] Parse XML (pugixml)
- [ ] Read tilemap layer
- [ ] Read entity layer
- [ ] Entity factory (create entity by name)
- [ ] Load all 115+ level files
- [ ] ~200 lines C++

#### 2.5: Save System (2 days)

**SharedObject.h/cpp**
- [ ] Save player state to JSON file
- [ ] Load player state from JSON
- [ ] Fields:
  - [ ] Player position (x, y, level)
  - [ ] Health, max health
  - [ ] Inventory (items, seeds)
  - [ ] Weapon equipped
  - [ ] Progress flags (dungeons completed, NPCs met)
  - [ ] Achievements
- [ ] Uses nlohmann/json
- [ ] ~150 lines C++

**Deliverables:**
- Mobile base class
- Player fully functional
- Level loading working
- Save/load working
- Main menu functional
- ~2,450 lines C++

---

### Phase 3: Game Content (3-5 weeks)

**Goal:** Port all game entities

**Estimated effort:** 160-320 hours

This is the most time-consuming but straightforward phase. Each entity class follows a similar pattern.

#### 3.1: Enemies (1.5-2 weeks)

**30 enemy types** (~12,500 lines AS3 → ~5,000 lines C++)

**Strategy:** Port by complexity tier

**Tier 1: Simple Enemies (1 week)**
- [ ] Bob.as (basic walker)
- [ ] Flyer.as (flying enemy)
- [ ] Turret.as (stationary shooter)
- [ ] Bulb.as (plant enemy)
- [ ] Squishle.as (bouncing enemy)
- [ ] Total: ~1,200 lines C++

**Tier 2: Medium Enemies (3-4 days)**
- [ ] WallFlyer.as
- [ ] Jumper.as
- [ ] Spinner.as
- [ ] IceTurret.as
- [ ] Cactus.as
- [ ] Total: ~1,000 lines C++

**Tier 3: Complex Enemies (2-3 days)**
- [ ] BossTotem.as
- [ ] BobBoss.as (miniboss)
- [ ] Total: ~800 lines C++

**Tier 4: Bosses (3-4 days)**
- [ ] LightBoss.as + LightBossController.as
- [ ] LavaBoss.as
- [ ] ShieldBoss.as
- [ ] FinalBoss.as
- [ ] Total: ~2,000 lines C++

**Each enemy class typically includes:**
- Extends Mobile
- State machine (idle, chase, attack, hurt, dying)
- AI logic (movement patterns, attack patterns)
- Animation control
- Collision/damage logic
- Projectile spawning (if ranged)
- Drop items on death

#### 3.2: NPCs (4-5 days)

**17 NPC types** (~4,800 lines AS3 → ~2,000 lines C++)

**Categories:**
- Interactive NPCs (dialogue)
- Statues (lore)
- Signs (hints)
- Quest givers

**Key NPCs:**
- [ ] Karlore.as (tutorial NPC)
- [ ] Hermit.as (item seller)
- [ ] Sensei.as (teaches moves)
- [ ] Witch.as (upgrades)
- [ ] Yeti.as (quest)
- [ ] Sign.as (generic sign)
- [ ] Statue.as (lore)
- [ ] Total: ~2,000 lines C++

**Each NPC class typically includes:**
- Extends Mobile (or Entity if stationary)
- Interaction trigger
- Dialogue system (text display)
- State tracking (met, quest completed, etc.)
- Animation

#### 3.3: Pickups (3-4 days)

**21 pickup types** (~3,200 lines AS3 → ~1,200 lines C++)

**Categories:**
- Currency (coins)
- Health (hearts)
- Keys (dungeon keys)
- Quest items (seal pieces, seeds)
- Weapons (stick, wand, sword, shield, bow)

**Key Pickups:**
- [ ] Coin.as
- [ ] HealthPickup.as
- [ ] Seed.as (collectible)
- [ ] BossKey.as
- [ ] SealPiece.as
- [ ] Stick.as (weapon)
- [ ] Wand.as (weapon)
- [ ] Fire.as (fire wand)
- [ ] DarkSword.as (weapon)
- [ ] DarkShield.as (weapon)
- [ ] Total: ~1,200 lines C++

**Each pickup class typically includes:**
- Extends Mobile (or Entity)
- Collision with player
- Effect on player (add to inventory, heal, unlock, etc.)
- Animation (idle, collected)
- Sound effect

#### 3.4: Projectiles (2 days)

**10 projectile types** (~2,800 lines AS3 → ~1,000 lines C++)

**Projectiles:**
- [ ] Arrow.as (player bow)
- [ ] Bomb.as (player/enemy)
- [ ] Explosion.as (visual effect)
- [ ] WandShot.as (player wand)
- [ ] RayShot.as (fire wand)
- [ ] TurretSpit.as (enemy)
- [ ] IceTurretBlast.as (enemy)
- [ ] BossTotemShot.as (enemy)
- [ ] LavaBall.as (boss)
- [ ] LightBossShot.as (boss)
- [ ] Total: ~1,000 lines C++

**Each projectile class typically includes:**
- Extends Mobile
- Movement pattern (straight line, arc, homing, etc.)
- Collision detection (hit player/enemy)
- Damage value
- Lifetime/despawn
- Visual effect (sprite, trail)

#### 3.5: Puzzlements (4-5 days)

**24 puzzle types** (~7,100 lines AS3 → ~2,500 lines C++)

**Puzzle Elements:**
- Buttons/Pressure plates
- Locked doors
- Activators
- Whirlpools (teleporters)
- Wires (logic connections)
- Moving platforms
- etc.

**Key Puzzles:**
- [ ] Button.as
- [ ] ButtonRoom.as (activator)
- [ ] MagicalLock.as
- [ ] RockLock.as
- [ ] Whirlpool.as
- [ ] Wire.as
- [ ] Total: ~2,500 lines C++

**Each puzzle class typically includes:**
- Extends Entity
- State (activated, locked, etc.)
- Interaction logic
- Animation
- Trigger effects (open door, activate mechanism)

#### 3.6: Scenery (3-4 days)

**42 scenery types** (~6,500 lines AS3 → ~2,000 lines C++)

**Scenery Objects:**
- Collision tiles
- Destructible objects (grass, trees)
- Visual effects (lights, particles)
- Decorations

**Key Scenery:**
- [ ] Tile.as (collision tile)
- [ ] Grass.as (cuttable)
- [ ] Tree.as
- [ ] BurnableTree.as
- [ ] Light.as (light source)
- [ ] RockFall.as (falling hazard)
- [ ] Total: ~2,000 lines C++

**Each scenery class typically includes:**
- Extends Entity
- Collision mask (if solid)
- Interaction (if destructible)
- Animation (if animated)
- Visual effect (if light source)

**Deliverables:**
- All 30 enemy types functional
- All 17 NPC types functional
- All 21 pickup types functional
- All 10 projectile types functional
- All 24 puzzle types functional
- All 42 scenery types functional
- ~13,700 lines C++

---

### Phase 4: Polish & Integration (1-2 weeks)

**Goal:** Complete the game

**Estimated effort:** 80-120 hours

#### 4.1: Asset Integration (2-3 days)

**Graphics Assets**
- [ ] Embed assets or load from files
- [ ] Test all sprite sheets load correctly
- [ ] Verify all animations work
- [ ] ~100 PNG files

**Audio Assets**
- [ ] Load all sound effects (~50 MP3s)
- [ ] Load all music tracks (~12 MP3s)
- [ ] Test audio playback
- [ ] Volume mixing

**Level Assets**
- [ ] Verify all 115+ levels load
- [ ] Test level transitions
- [ ] Fix any entity spawn issues

#### 4.2: UI System (2-3 days)

**HUD**
- [ ] Health display
- [ ] Coin counter
- [ ] Seed counter
- [ ] Inventory display

**Menus**
- [ ] Main menu
- [ ] Pause menu
- [ ] Inventory screen
- [ ] Game over screen

**Text Rendering**
- [ ] Dialogue boxes
- [ ] Signs/NPC text
- [ ] Menu text

#### 4.3: Testing & Bug Fixes (1 week)

**Functional Testing**
- [ ] Play through first dungeon
- [ ] Test all weapons
- [ ] Test all enemies
- [ ] Test all bosses
- [ ] Test save/load
- [ ] Test full playthrough (3-5 hours)

**Bug Fixes**
- [ ] Collision bugs
- [ ] Animation bugs
- [ ] Physics bugs
- [ ] Audio bugs
- [ ] Save/load bugs
- [ ] Performance issues

#### 4.4: Performance Optimization (2-3 days)

**Profiling**
- [ ] Identify bottlenecks
- [ ] Profile with profiler (gprof, perf, Tracy)

**Optimizations**
- [ ] Optimize rendering (reduce draw calls)
- [ ] Optimize collision detection (spatial partitioning)
- [ ] Optimize entity updates (skip off-screen entities)
- [ ] Memory optimization (object pooling)

**Target:** 60 FPS on target hardware

#### 4.5: WASM Build (2-3 days)

**Emscripten Port**
- [ ] Create WASM CMakeLists.txt
- [ ] Port SDL2 code to WASM (use Emscripten SDL2)
- [ ] Handle file loading (preload assets)
- [ ] Handle audio (Web Audio API)
- [ ] Create HTML shell
- [ ] Test in browser

**WASM Optimizations**
- [ ] Minimize binary size
- [ ] Optimize load time
- [ ] Test on different browsers

**Deliverables:**
- Fully playable Seedling game (native)
- Fully playable Seedling game (WASM)
- All features working
- Performance optimized
- Ready for release

---

## Technology Stack

### Core Technologies

**C++ Standard:** C++17 or later
- Modern C++ features (smart pointers, lambdas, auto, etc.)
- Standard library (std::vector, std::map, std::string, etc.)

**Build System:** CMake 3.20+
- Cross-platform build configuration
- Easy dependency management
- Supports both native and WASM targets

**Compiler:**
- Native: g++ or clang++
- WASM: emcc (Emscripten)

### Graphics & Window

**SDL2 2.0.20+** (recommended)
- Cross-platform (Windows, Mac, Linux, WASM)
- 2D rendering (SDL_Renderer)
- Texture management
- Input handling (keyboard, mouse)
- Audio support (SDL_mixer)
- Well-documented
- WASM support via Emscripten

**Alternative: SFML 2.5+**
- Similar to SDL2
- More C++-friendly API
- Also supports WASM (with some effort)

### Audio

**SDL_mixer 2.0+**
- Audio playback (MP3, OGG, WAV)
- Multiple channels
- Volume control
- Works with SDL2
- WASM support

**Alternative: OpenAL**
- 3D audio (overkill for Seedling)
- More complex

### XML Parsing

**pugixml**
- Lightweight C++ XML parser
- Header-only option
- Easy to use
- Perfect for OEL level files

### JSON Parsing

**nlohmann/json**
- Modern C++ JSON library
- Header-only
- Easy to use
- Perfect for save files

### Optional: Physics

**Box2D** (if you want a full physics engine)
- Not strictly necessary for Seedling
- Manual physics is simpler and sufficient

### WASM

**Emscripten**
- C++ to WASM compiler
- Provides SDL2 port
- File system emulation
- Web Audio API integration

---

## AS3 to C++ Mapping

### Language Features

#### Classes and Inheritance

**AS3:**
```actionscript
package {
    public class Player extends Mobile {
        private var health:int = 100;

        public function Player(x:Number, y:Number) {
            super(x, y);
        }

        override public function update():void {
            super.update();
            // Player update logic
        }
    }
}
```

**C++:**
```cpp
// Player.h
#pragma once
#include "Mobile.h"

class Player : public Mobile {
private:
    int health = 100;

public:
    Player(float x, float y);
    void update() override;
};

// Player.cpp
#include "Player.h"

Player::Player(float x, float y) : Mobile(x, y) {
}

void Player::update() {
    Mobile::update();
    // Player update logic
}
```

#### Properties (Getters/Setters)

**AS3:**
```actionscript
private var _speed:Number = 5;

public function get speed():Number {
    return _speed;
}

public function set speed(value:Number):void {
    _speed = value;
}
```

**C++:**
```cpp
private:
    float m_speed = 5.0f;

public:
    float getSpeed() const { return m_speed; }
    void setSpeed(float value) { m_speed = value; }

    // Or use property-like syntax (less common):
    __declspec(property(get=getSpeed, put=setSpeed)) float speed;
```

#### Vectors

**AS3:**
```actionscript
var enemies:Vector.<Enemy> = new Vector.<Enemy>();
enemies.push(new Enemy());
for each (var enemy:Enemy in enemies) {
    enemy.update();
}
```

**C++:**
```cpp
std::vector<std::unique_ptr<Enemy>> enemies;
enemies.push_back(std::make_unique<Enemy>());
for (auto& enemy : enemies) {
    enemy->update();
}
```

#### Dictionaries/Maps

**AS3:**
```actionscript
var data:Object = {};
data["key"] = "value";
trace(data["key"]);
```

**C++:**
```cpp
std::unordered_map<std::string, std::string> data;
data["key"] = "value";
std::cout << data["key"] << std::endl;
```

#### Type Casting

**AS3:**
```actionscript
var entity:Entity = getEntity();
var player:Player = entity as Player;
if (player != null) {
    player.jump();
}
```

**C++:**
```cpp
Entity* entity = getEntity();
Player* player = dynamic_cast<Player*>(entity);
if (player != nullptr) {
    player->jump();
}
```

#### Anonymous Functions (Callbacks)

**AS3:**
```actionscript
button.addEventListener(MouseEvent.CLICK, function(e:MouseEvent):void {
    trace("Clicked!");
});
```

**C++:**
```cpp
button.setOnClick([this]() {
    std::cout << "Clicked!" << std::endl;
});
```

### Flash API Replacements

#### Point

**AS3:**
```actionscript
var p:Point = new Point(10, 20);
p.x += 5;
var dist:Number = Point.distance(p1, p2);
```

**C++:**
```cpp
struct Point {
    float x, y;

    Point(float x = 0, float y = 0) : x(x), y(y) {}

    static float distance(const Point& p1, const Point& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        return std::sqrt(dx*dx + dy*dy);
    }
};

Point p(10, 20);
p.x += 5;
float dist = Point::distance(p1, p2);
```

#### Rectangle

**AS3:**
```actionscript
var rect:Rectangle = new Rectangle(0, 0, 100, 50);
if (rect.contains(x, y)) {
    // ...
}
```

**C++:**
```cpp
struct Rectangle {
    float x, y, width, height;

    Rectangle(float x = 0, float y = 0, float w = 0, float h = 0)
        : x(x), y(y), width(w), height(h) {}

    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    bool intersects(const Rectangle& other) const {
        return !(other.x >= x + width || other.x + other.width <= x ||
                 other.y >= y + height || other.y + other.height <= y);
    }
};
```

#### BitmapData → SDL_Texture

**AS3:**
```actionscript
var bitmapData:BitmapData = new BitmapData(100, 100);
bitmapData.fillRect(new Rectangle(0, 0, 100, 100), 0xFF0000);
bitmapData.draw(sprite);
```

**C++:**
```cpp
SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_TARGET, 100, 100);
SDL_SetRenderTarget(renderer, texture);
SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
SDL_RenderClear(renderer);
// Draw sprite...
SDL_SetRenderTarget(renderer, nullptr);
```

#### Sound → SDL_mixer

**AS3:**
```actionscript
var sound:Sound = new Sound(new URLRequest("sound.mp3"));
var channel:SoundChannel = sound.play();
channel.soundTransform = new SoundTransform(0.5); // volume
```

**C++:**
```cpp
Mix_Chunk* sound = Mix_LoadWAV("sound.mp3");
int channel = Mix_PlayChannel(-1, sound, 0);
Mix_Volume(channel, MIX_MAX_VOLUME / 2); // 50% volume
```

#### SharedObject → JSON File

**AS3:**
```actionscript
var save:SharedObject = SharedObject.getLocal("seedling_save");
save.data.playerX = 100;
save.data.playerY = 200;
save.flush();
```

**C++:**
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json save;
save["playerX"] = 100;
save["playerY"] = 200;

std::ofstream file("save.json");
file << save.dump(4);
file.close();
```

#### getTimer → SDL_GetTicks

**AS3:**
```actionscript
var time:int = getTimer(); // milliseconds
```

**C++:**
```cpp
uint32_t time = SDL_GetTicks(); // milliseconds
```

---

## FlashPunk to C++ Port

### Architecture Comparison

**FlashPunk (AS3):**
```
Engine (MovieClip)
  └─ World (extends Tweener)
      └─ Entity (extends Tweener)
          ├─ Graphic (Image, Spritemap, etc.)
          └─ Mask (Hitbox, Pixelmask, Grid)
```

**FlashPunk C++ Port:**
```
Engine (class)
  └─ World (class)
      └─ Entity (class)
          ├─ Graphic* (polymorphic)
          └─ Mask* (polymorphic)
```

### Key Design Decisions

**1. Memory Management**
- Use `std::unique_ptr` for entity ownership
- World owns entities
- Entities own graphics/masks
- Use raw pointers for references (non-owning)

**2. Polymorphism**
- Virtual functions for update/render
- Dynamic dispatch for entity types
- Base Graphic/Mask classes with derived types

**3. Rendering**
- Software rendering (like FlashPunk) using SDL_Renderer
- Each graphic renders to screen texture
- Layered rendering (sort entities by layer before render)

**4. Collision**
- Keep FlashPunk's collision system design
- Mask types (Hitbox, Pixelmask, Grid)
- Query functions (collide, collideTypes, collideRect, etc.)

### Core Class Comparison

#### Engine Class

**FlashPunk AS3:**
```actionscript
public class Engine extends MovieClip {
    public function Engine() {
        // Init
        addEventListener(Event.ENTER_FRAME, onEnterFrame);
    }

    private function onEnterFrame(e:Event):void {
        FP.elapsed = 1.0 / FP.frameRate;
        update();
        render();
    }

    public function update():void {
        if (FP.world) FP.world.update();
    }

    public function render():void {
        if (FP.world) FP.world.render();
    }
}
```

**C++ Port:**
```cpp
class Engine {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    World* currentWorld = nullptr;
    bool running = true;

    const int FPS = 60;
    const float FRAME_TIME = 1.0f / FPS;

public:
    Engine(int width, int height, const std::string& title);
    ~Engine();

    void run() {
        uint32_t lastTime = SDL_GetTicks();

        while (running) {
            uint32_t currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            handleEvents();
            update(deltaTime);
            render();

            // Frame rate limiting
            uint32_t frameTime = SDL_GetTicks() - currentTime;
            if (frameTime < FRAME_TIME * 1000) {
                SDL_Delay((FRAME_TIME * 1000) - frameTime);
            }
        }
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            // Handle input...
        }
    }

    void update(float deltaTime) {
        if (currentWorld) {
            currentWorld->update();
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (currentWorld) {
            currentWorld->render(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    void setWorld(World* world) {
        currentWorld = world;
    }
};
```

#### Entity Class

**FlashPunk AS3:**
```actionscript
public class Entity extends Tweener {
    public var x:Number = 0;
    public var y:Number = 0;
    public var type:String = "";
    public var layer:int = 0;
    public var graphic:Graphic;
    public var mask:Mask;
    public var world:World;

    public function update():void { }
    public function render():void {
        if (graphic && graphic.visible) {
            graphic.render(/* ... */);
        }
    }

    public function collide(type:String, x:Number, y:Number):Entity {
        // Collision detection...
    }
}
```

**C++ Port:**
```cpp
class Entity {
protected:
    float x = 0;
    float y = 0;
    std::string type = "";
    int layer = 0;

    std::unique_ptr<Graphic> graphic;
    std::unique_ptr<Mask> mask;

    World* world = nullptr; // Non-owning pointer

public:
    virtual ~Entity() = default;

    // Getters/setters
    float getX() const { return x; }
    float getY() const { return y; }
    void setPosition(float newX, float newY) { x = newX; y = newY; }

    const std::string& getType() const { return type; }
    void setType(const std::string& newType) { type = newType; }

    int getLayer() const { return layer; }
    void setLayer(int newLayer) { layer = newLayer; }

    // Lifecycle
    virtual void added() { }
    virtual void removed() { }
    virtual void update() { }
    virtual void render(SDL_Renderer* renderer) {
        if (graphic) {
            graphic->render(renderer, x, y);
        }
    }

    // Collision
    Entity* collide(const std::string& type, float checkX, float checkY);
    bool collideRect(float checkX, float checkY, float rectWidth, float rectHeight);

    // Graphic/Mask management
    void setGraphic(std::unique_ptr<Graphic> g) {
        graphic = std::move(g);
    }

    void setMask(std::unique_ptr<Mask> m) {
        mask = std::move(m);
    }

    Mask* getMask() const { return mask.get(); }

    friend class World;
};
```

---

## Asset Management

### Graphics Assets

**Approach 1: Load from files (easier for development)**

```cpp
SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << path << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

// Usage:
SDL_Texture* playerTexture = loadTexture("assets/gfx/player.png", renderer);
```

**Approach 2: Embed in binary (better for distribution)**

Use a tool like `xxd` or custom script to convert PNG to C array:

```bash
xxd -i assets/gfx/player.png > src/assets/player_png.h
```

```cpp
#include "assets/player_png.h"

SDL_Texture* loadEmbeddedTexture(const unsigned char* data, int size, SDL_Renderer* renderer) {
    SDL_RWops* rw = SDL_RWFromConstMem(data, size);
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Usage:
SDL_Texture* playerTexture = loadEmbeddedTexture(player_png, player_png_len, renderer);
```

**For WASM: Preload files**

```cmake
# CMakeLists.txt for WASM
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(Seedling PROPERTIES LINK_FLAGS "--preload-file assets")
endif()
```

### Audio Assets

**SDL_mixer:**

```cpp
// Load sound effect
Mix_Chunk* sfx = Mix_LoadWAV("assets/sfx/hit.mp3");

// Load music
Mix_Music* music = Mix_LoadMUS("assets/music/overworld.mp3");

// Play
Mix_PlayChannel(-1, sfx, 0); // -1 = first available channel
Mix_PlayMusic(music, -1); // -1 = loop forever
```

### Level Assets (OEL XML)

**Example OEL file:**
```xml
<level width="320" height="240">
  <tiles tileset="tiles.png" exportMode="CSV">
    1,1,1,1,1,...
    1,0,0,0,1,...
    ...
  </tiles>
  <entities>
    <Player x="80" y="80"/>
    <Enemy x="160" y="120" type="Bob"/>
    <Coin x="100" y="100"/>
  </entities>
</level>
```

**Parser:**

```cpp
#include <pugixml.hpp>

void Game::loadLevel(const std::string& levelPath) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(levelPath.c_str());

    if (!result) {
        std::cerr << "Failed to load level: " << levelPath << std::endl;
        return;
    }

    pugi::xml_node level = doc.child("level");

    // Load tiles
    pugi::xml_node tiles = level.child("tiles");
    std::string tileData = tiles.text().as_string();
    parseTiles(tileData);

    // Load entities
    pugi::xml_node entities = level.child("entities");
    for (pugi::xml_node entity : entities.children()) {
        std::string name = entity.name();
        float x = entity.attribute("x").as_float();
        float y = entity.attribute("y").as_float();

        // Entity factory
        if (name == "Player") {
            player = std::make_unique<Player>(x, y);
            add(player.get());
        } else if (name == "Enemy") {
            std::string type = entity.attribute("type").as_string();
            add(createEnemy(type, x, y));
        } else if (name == "Coin") {
            add(std::make_unique<Coin>(x, y));
        }
        // ... more entity types
    }
}
```

---

## Build System

### CMakeLists.txt (Root)

```cmake
cmake_minimum_required(VERSION 3.20)
project(Seedling)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
if (EMSCRIPTEN)
    # Emscripten provides SDL2
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_SDL=2 -s USE_SDL_MIXER=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[\"png\"]")
else()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(SDL2_image REQUIRED)
endif()

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/core/*.cpp
    src/graphics/*.cpp
    src/masks/*.cpp
    src/utils/*.cpp
    src/game/*.cpp
    src/game/enemies/*.cpp
    src/game/npcs/*.cpp
    src/game/pickups/*.cpp
    src/game/projectiles/*.cpp
    src/game/puzzlements/*.cpp
    src/game/scenery/*.cpp
)

# Create executable
add_executable(Seedling ${SOURCES})

# Include directories
target_include_directories(Seedling PRIVATE
    src
    src/core
    src/graphics
    src/masks
    src/utils
    src/game
    external/pugixml/src
    external/nlohmann_json/include
)

# Link libraries
if (EMSCRIPTEN)
    target_link_options(Seedling PRIVATE
        -s WASM=1
        -s ALLOW_MEMORY_GROWTH=1
        -s USE_SDL=2
        -s USE_SDL_MIXER=2
        -s USE_SDL_IMAGE=2
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
    )
else()
    target_link_libraries(Seedling
        SDL2::SDL2
        SDL2_mixer::SDL2_mixer
        SDL2_image::SDL2_image
    )
endif()
```

### Build Commands

**Native:**
```bash
mkdir build
cd build
cmake ..
make
./Seedling
```

**WASM:**
```bash
source ~/tools/emsdk/emsdk_env.sh
mkdir build-wasm
cd build-wasm
emcmake cmake ..
emmake make
python3 -m http.server 8000
# Open http://localhost:8000/Seedling.html
```

---

## Effort Estimation

### Detailed Breakdown

| Phase | Tasks | AS3 Lines | C++ Lines | Hours | Notes |
|-------|-------|-----------|-----------|-------|-------|
| **Phase 0: Setup** | Project structure, dependencies | - | 100 | 8-12 | One-time setup |
| **Phase 1: FlashPunk Core** | Engine, World, Entity, Graphics, Collision, Utils | 12,000 | 2,500 | 80-160 | Complex but reusable |
| **Phase 2: Core Game** | Mobile, Player, Main, Game, Level system, Save | 6,987 | 2,450 | 120-200 | Game-specific logic |
| **Phase 3: Game Content** | Enemies, NPCs, Pickups, Projectiles, Puzzles, Scenery | 49,000 | 13,700 | 160-320 | Repetitive but large |
| **Phase 4: Polish** | Assets, UI, Testing, WASM | - | 500 | 80-120 | Integration |
| **TOTAL** | | **68,000** | **19,250** | **448-812** | **2-4 months part-time** |

### Assumptions

- **Developer skill:** Intermediate C++ experience, familiar with game dev concepts
- **Work schedule:** 20-30 hours/week (part-time)
- **Reuse:** Some code patterns repeat (enemies, NPCs, etc.)
- **Testing:** Integrated into each phase (not separate)

### Time Estimates

**Full-time (40 hours/week):**
- Minimum: 448 hours ÷ 40 = **11.2 weeks (2.8 months)**
- Maximum: 812 hours ÷ 40 = **20.3 weeks (5.1 months)**
- **Realistic: 3-4 months**

**Part-time (20 hours/week):**
- Minimum: 448 hours ÷ 20 = **22.4 weeks (5.6 months)**
- Maximum: 812 hours ÷ 20 = **40.6 weeks (10.2 months)**
- **Realistic: 6-8 months**

### Comparison to SWFRecomp

| Metric | SWFRecomp | Manual C++ | Winner |
|--------|-----------|------------|--------|
| **Initial effort** | 1210-1830 hours | 448-812 hours | ✅ Manual (62% less) |
| **Time to Seedling playable** | 8-12 months | 2-4 months | ✅ Manual (3x faster) |
| **Second game effort** | 40-80 hours (automatic) | 320-640 hours (manual) | ✅ SWFRecomp |
| **Third game effort** | 20-40 hours | 320-640 hours | ✅ SWFRecomp |
| **10 games effort** | ~1600 hours | ~5000 hours | ✅ SWFRecomp |
| **Code quality** | Generated (verbose) | Handwritten (clean) | ✅ Manual |
| **Performance** | Overhead | Native | ✅ Manual |
| **Maintainability** | Regenerate | Edit directly | ✅ Manual |

**Break-even point: ~3 games**

After converting 3 games manually, SWFRecomp would have been faster.

---

## Risk Assessment

### High-Risk Items

**1. FlashPunk Port Complexity ⚠️ HIGH**
- **Risk:** FlashPunk has subtle behaviors that are hard to replicate exactly
- **Mitigation:** Extensive testing, compare behavior to original Flash version
- **Impact:** +2-4 weeks if major issues found

**2. Pixelmask Collision ⚠️ HIGH**
- **Risk:** Pixel-perfect collision is performance-sensitive and tricky
- **Mitigation:** Use SDL2's pixel access, optimize with bitmasks
- **Impact:** +1-2 weeks if performance issues

**3. Asset Integration ⚠️ MEDIUM**
- **Risk:** 100+ graphics, 50+ sounds, 115+ levels need proper loading
- **Mitigation:** Automated scripts to convert/embed assets
- **Impact:** +1 week if manual work required

**4. Level Format Parsing ⚠️ MEDIUM**
- **Risk:** OEL XML format might have edge cases
- **Mitigation:** Thorough testing with all 115+ levels
- **Impact:** +3-5 days for edge cases

**5. Save System Compatibility ⚠️ LOW**
- **Risk:** Players might want to import Flash saves (unlikely)
- **Mitigation:** Document save format, offer converter tool (optional)
- **Impact:** Minimal (new save system is fine)

### Medium-Risk Items

**6. WASM Performance 📊 MEDIUM**
- **Risk:** WASM might be slower than native
- **Mitigation:** Profile and optimize, use Emscripten optimization flags
- **Impact:** +1-2 weeks optimization

**7. Audio on WASM 📊 MEDIUM**
- **Risk:** Web Audio API has quirks, autoplay restrictions
- **Mitigation:** Test early, provide user-initiated audio start
- **Impact:** +2-3 days

**8. Cross-platform Issues 📊 LOW**
- **Risk:** Different behavior on Windows/Mac/Linux
- **Mitigation:** Test on all platforms, use CMake presets
- **Impact:** +3-5 days

### Low-Risk Items

**9. Translation Errors ✅ LOW**
- **Risk:** Bugs in manual AS3 → C++ conversion
- **Mitigation:** Incremental testing, compare to original behavior
- **Impact:** Ongoing (fix as found)

**10. Scope Creep ✅ LOW**
- **Risk:** Adding features not in original game
- **Mitigation:** Stick to original feature set, defer enhancements
- **Impact:** Project management issue

---

## Testing Strategy

### Unit Testing

**Test each component in isolation:**

- FlashPunk classes (Entity, World, Engine)
- Graphics classes (Image, Spritemap, Tilemap)
- Collision classes (Hitbox, Pixelmask, Grid)
- Utility classes (Input, Point, Rectangle)

**Framework:** Catch2 or Google Test

**Example:**
```cpp
TEST_CASE("Point distance calculation", "[Point]") {
    Point p1(0, 0);
    Point p2(3, 4);
    REQUIRE(Point::distance(p1, p2) == Approx(5.0f));
}

TEST_CASE("Rectangle intersection", "[Rectangle]") {
    Rectangle r1(0, 0, 10, 10);
    Rectangle r2(5, 5, 10, 10);
    REQUIRE(r1.intersects(r2) == true);

    Rectangle r3(20, 20, 10, 10);
    REQUIRE(r1.intersects(r3) == false);
}
```

### Integration Testing

**Test systems working together:**

- Entity collision detection
- Level loading and entity spawning
- Player movement and input
- Animation system
- Save/load system

**Approach:** Create test levels with specific scenarios

### Functional Testing

**Play the game:**

**Phase 1 Tests:**
- [ ] Window opens
- [ ] Can move player with arrow keys
- [ ] Sprite renders correctly
- [ ] Collision works

**Phase 2 Tests:**
- [ ] Can load a level
- [ ] Can transition between levels
- [ ] Can save and load game
- [ ] UI renders

**Phase 3 Tests:**
- [ ] All enemy types work
- [ ] All weapons work
- [ ] All NPCs work
- [ ] All puzzles work

**Phase 4 Tests:**
- [ ] Can complete entire game (3-5 hour playthrough)
- [ ] All bosses beatable
- [ ] Achievements unlock
- [ ] Performance is acceptable (60 FPS)

### Regression Testing

**Automated test suite:**
- Run unit tests on every build
- Run integration tests daily
- Record and replay gameplay (optional)

### Performance Testing

**Profiling:**
- Use Tracy, gprof, or Instruments
- Identify bottlenecks
- Optimize hotspots

**Targets:**
- **Native:** 60 FPS on mid-range hardware (5-year-old laptop)
- **WASM:** 60 FPS on modern browsers (Chrome, Firefox, Safari)

---

## Success Criteria

### Phase 0 Success
- [ ] CMake project builds (native)
- [ ] CMake project builds (WASM)
- [ ] Test window opens
- [ ] Can load an image
- [ ] Can play a sound

### Phase 1 Success
- [ ] FlashPunk core compiles
- [ ] Test program: Entity with sprite renders
- [ ] Test program: Entity collision works
- [ ] Test program: Spritemap animation plays
- [ ] Test program: Input detected

### Phase 2 Success
- [ ] Player class compiles and runs
- [ ] Can move player with arrow keys
- [ ] Player animation changes based on movement
- [ ] Can load a simple level (1 screen)
- [ ] Can transition to another level
- [ ] Can save and load player position

### Phase 3 Success
- [ ] All enemy types compile
- [ ] Can spawn all enemy types in a test level
- [ ] All enemy behaviors work correctly
- [ ] Can defeat all enemy types
- [ ] All NPC types work
- [ ] All pickup types work
- [ ] All puzzle types work

### Phase 4 Success
- [ ] All 115+ levels load correctly
- [ ] All graphics render correctly
- [ ] All sounds play correctly
- [ ] UI renders correctly
- [ ] Can complete first dungeon
- [ ] Can complete entire game
- [ ] Save/load works throughout playthrough
- [ ] WASM version works in browser

### Overall Success
- [ ] Seedling is fully playable start to finish
- [ ] All features from Flash version work
- [ ] Performance is 60 FPS (native and WASM)
- [ ] No major bugs
- [ ] Save system works
- [ ] Game is enjoyable and faithful to original
- [ ] Code is maintainable and well-documented

---

## Next Steps

### Immediate Actions

1. **Review this document** and decide: Manual conversion or SWFRecomp?

2. **If manual conversion:**
   - Set up development environment (Phase 0)
   - Start FlashPunk port (Phase 1)
   - Port Player + Game classes (Phase 2)
   - Port all entity types (Phase 3)
   - Polish and release (Phase 4)

3. **If SWFRecomp:**
   - See SEEDLING_IMPLEMENTATION_PLAN.md
   - Complete AS1/2 first
   - Begin AS3 implementation
   - Target Seedling as first test case

### Decision Matrix

**Choose Manual Conversion if:**
- ✅ You want Seedling playable ASAP (2-4 months)
- ✅ You only care about Seedling (not other Flash games)
- ✅ You have C++ and game dev experience
- ✅ You want optimal performance
- ✅ You want clean, maintainable code
- ✅ You want full control over the result

**Choose SWFRecomp if:**
- ✅ You want to preserve many Flash games automatically
- ✅ You're willing to invest in tooling first (4-7 months)
- ✅ You want Flash-faithful behavior
- ✅ You prefer a general-purpose solution
- ✅ You want to contribute to Flash preservation
- ✅ You have experience with compilers/bytecode

### Hybrid Approach (Best of Both Worlds)

**Option: Start manual conversion while SWFRecomp is being developed**

1. **Months 1-4:** Manual conversion of Seedling (this document)
   - Result: Playable Seedling in C++/WASM

2. **Months 1-8:** AS1/2 completion + AS3 development (parallel)
   - Result: SWFRecomp supports AS3

3. **Month 8+:** Compare results
   - Manual version: Optimized, handwritten, maintainable
   - SWFRecomp version: Automatic, Flash-faithful, reusable for other games

4. **Month 9+:** Use best approach for future games
   - If SWFRecomp AS3 works well, use it for other games
   - If manual conversion produced better results, repeat for other games

---

## Conclusion

### Summary

Manual conversion of Seedling from AS3 to C++ is **significantly faster** than building a full AS3 recompiler:

- **Manual: 2-4 months** (320-640 hours)
- **SWFRecomp: 4-7 months** (600-1000 hours before Seedling is playable)

Manual conversion produces **better code quality and performance**, but lacks **reusability** for other Flash games.

### Recommendation

**For Seedling specifically:** Manual conversion is the better choice.

**For Flash preservation generally:** SWFRecomp is the better long-term investment.

**Best approach:** Do both! Manual conversion gives you a playable Seedling quickly, while SWFRecomp development continues in parallel. After 3+ games, SWFRecomp becomes more efficient.

### Final Thoughts

The manual conversion approach is well-suited for Seedling because:

1. **Codebase is well-structured** (clean entity hierarchy)
2. **FlashPunk is portable** (no Flash-specific dependencies)
3. **Assets are manageable** (~100 graphics, ~50 sounds, ~115 levels)
4. **No exotic AS3 features** (no E4X, no Workers, no Alchemy)
5. **Clear 1:1 mapping** (AS3 classes → C++ classes)

With C++ experience and game dev knowledge, you can have Seedling running natively and in WASM in **2-4 months of part-time work**.

---

**END OF DOCUMENT**
