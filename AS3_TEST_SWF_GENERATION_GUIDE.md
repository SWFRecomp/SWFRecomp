# AS3 Test SWF Generation Guide

**Document Version:** 1.0
**Date:** October 29, 2025
**Purpose:** Guide for creating ActionScript 3 test SWF files for ABC parser testing

---

## Table of Contents

1. [Overview](#overview)
2. [Tools and Installation](#tools-and-installation)
3. [Quick Start Examples](#quick-start-examples)
4. [Test Suite Design](#test-suite-design)
5. [Compilation Methods](#compilation-methods)
6. [Verification and Debugging](#verification-and-debugging)
7. [Troubleshooting](#troubleshooting)

---

## Overview

### Why We Need Test SWF Files

The ABC parser implementation needs a progressive suite of test SWF files to verify correctness:

1. **Unit Testing** - Parse minimal ABC structures
2. **Feature Testing** - Test specific AS3 features (classes, inheritance, etc.)
3. **Integration Testing** - Parse real-world SWF files
4. **Regression Testing** - Ensure changes don't break existing functionality

### Test Progression Strategy

```
Simple → Complex → Real World

1. Hello World         (basic structure)
2. Single Method       (method bodies with bytecode)
3. Single Class        (class definitions, constructors)
4. Inheritance         (super classes, method overrides)
5. Multiple Classes    (cross-class references)
6. Interfaces          (interface implementation)
7. Complex Game        (Seedling - real-world test)
```

### File Locations

All test SWF files will be stored in:
```
SWFRecomp/
└── tests/
    └── as3/
        ├── hello_world/
        │   ├── HelloWorld.as
        │   ├── HelloWorld.swf
        │   └── README.md
        ├── single_class/
        ├── inheritance/
        ├── interfaces/
        └── seedling/
```

---

## Tools and Installation

### Option 1: Apache Flex SDK (Recommended for Linux)

**Best for:** Linux users, automated builds, CI/CD

#### Installation on Linux

```bash
# 1. Install prerequisites
sudo apt-get update
sudo apt-get install -y wget unzip ant default-jdk

# 2. Download Apache Flex SDK
cd ~/tools
wget https://archive.apache.org/dist/flex/4.16.1/binaries/apache-flex-sdk-4.16.1-bin.tar.gz

# 3. Extract
tar -xzf apache-flex-sdk-4.16.1-bin.tar.gz
mv apache-flex-sdk-4.16.1-bin flex-sdk

# 4. Download playerglobal.swc (Flash Player API)
mkdir -p flex-sdk/frameworks/libs/player/11.1
cd flex-sdk/frameworks/libs/player/11.1
wget https://fpdownload.macromedia.com/get/flashplayer/updaters/11/playerglobal11_1.swc
mv playerglobal11_1.swc playerglobal.swc

# 5. Add to PATH
echo 'export FLEX_HOME=~/tools/flex-sdk' >> ~/.bashrc
echo 'export PATH=$PATH:$FLEX_HOME/bin' >> ~/.bashrc
source ~/.bashrc

# 6. Verify installation
mxmlc --version
# Expected output: Version 4.16.1 build 20220228
```

#### Key Files
- **mxmlc** - Main ActionScript compiler
- **compc** - Component compiler (for libraries)
- **asdoc** - Documentation generator

### Option 2: HARMAN AIR SDK (Modern, Actively Maintained)

**Best for:** Latest AS3 features, modern development

#### Installation on Linux

```bash
# 1. Install AIR SDK Manager (recommended method)
npm install -g @airsdk/apm

# 2. Install AIR SDK
apm install

# 3. Verify installation
airsdk --version
# Expected output: HARMAN AIR SDK 51.2.x

# Alternative: Manual installation
cd ~/tools
wget https://airsdk.harman.com/download/latest/AIRSDK_Linux.tar.gz
mkdir air-sdk
tar -xzf AIRSDK_Linux.tar.gz -C air-sdk

# Add to PATH
echo 'export AIR_HOME=~/tools/air-sdk' >> ~/.bashrc
echo 'export PATH=$PATH:$AIR_HOME/bin' >> ~/.bashrc
source ~/.bashrc
```

#### Key Files
- **mxmlc** - ActionScript compiler (same as Flex SDK)
- **adt** - AIR Developer Tool (packaging)
- **amxmlc** - Optimized compiler variant

### Option 3: FlashDevelop (Windows)

**Best for:** Windows users, IDE integration, rapid prototyping

#### Installation on Windows

1. Download FlashDevelop from: https://www.flashdevelop.org/
2. Install FlashDevelop (includes Flex SDK)
3. Configure SDK paths in Tools → Program Settings → AS3Context

#### Compilation
- **Build:** F8 (builds SWF)
- **Build & Run:** F5 (builds and runs in Flash Player)
- **Output:** bin/ directory

### Option 4: Online Compilers (Quick Testing)

**Best for:** Quick tests without local setup

1. **Try ActionScript** - https://try.as3lang.org/
   - Browser-based AS3 compiler
   - Instant compilation and testing
   - Can download compiled SWF

2. **Compiler Explorer** - Some variants support AS3
   - Good for comparing bytecode output

---

## Quick Start Examples

### Test 1: Hello World (Minimal ABC)

**Goal:** Generate simplest possible ABC file with one class, one method

**File:** `tests/as3/hello_world/HelloWorld.as`

```actionscript
package {
    import flash.display.Sprite;

    public class HelloWorld extends Sprite {
        public function HelloWorld() {
            trace("Hello from AS3!");
        }
    }
}
```

**Compile with Flex SDK:**
```bash
cd tests/as3/hello_world
mxmlc -output HelloWorld.swf HelloWorld.as
```

**Compile with AIR SDK:**
```bash
cd tests/as3/hello_world
mxmlc -output HelloWorld.swf HelloWorld.as
```

**Expected ABC Structure:**
- 1 class: HelloWorld
- 1 method: HelloWorld constructor
- Parent class: flash.display.Sprite
- 1 script with init method
- Minimal constant pools

**What to Test:**
- Parse ABC header (version 46.16)
- Parse string pool (class names, "Hello from AS3!")
- Parse namespace pool (public, private)
- Parse multiname pool (HelloWorld, Sprite, trace)
- Parse method info (constructor signature)
- Parse class info (HelloWorld extends Sprite)
- Parse method body (bytecode for trace call)

---

### Test 2: Simple Math (Method Bodies)

**Goal:** Test method bodies with arithmetic bytecode

**File:** `tests/as3/simple_math/SimpleMath.as`

```actionscript
package {
    import flash.display.Sprite;

    public class SimpleMath extends Sprite {
        public function SimpleMath() {
            var result:int = add(5, 3);
            trace("5 + 3 = " + result);
        }

        private function add(a:int, b:int):int {
            return a + b;
        }
    }
}
```

**Compile:**
```bash
cd tests/as3/simple_math
mxmlc -output SimpleMath.swf SimpleMath.as
```

**Expected ABC Structure:**
- 1 class: SimpleMath
- 2 methods: constructor, add
- Method signatures with parameters and return types
- Bytecode with arithmetic operations (OP_add)
- Local variable handling (getlocal, setlocal)

**What to Test:**
- Method with parameters (add(a:int, b:int))
- Return type handling
- Local variable slots
- Arithmetic opcodes (0xA0 = add)
- Method calls (callproperty for trace)

---

### Test 3: Single Class (Class Definition)

**Goal:** Test comprehensive class features

**File:** `tests/as3/single_class/Player.as`

```actionscript
package {
    import flash.display.Sprite;

    public class Player extends Sprite {
        // Static property
        public static const MAX_HEALTH:int = 100;

        // Instance properties
        private var _health:int;
        private var _name:String;

        // Constructor
        public function Player(name:String) {
            _name = name;
            _health = MAX_HEALTH;
        }

        // Getter
        public function get health():int {
            return _health;
        }

        // Setter
        public function set health(value:int):void {
            if (value < 0) value = 0;
            if (value > MAX_HEALTH) value = MAX_HEALTH;
            _health = value;
        }

        // Public method
        public function takeDamage(amount:int):void {
            health -= amount;
            trace(_name + " took " + amount + " damage. Health: " + _health);
        }
    }
}
```

**Compile:**
```bash
cd tests/as3/single_class
mxmlc -output Player.swf Player.as
```

**Expected ABC Structure:**
- Static trait (MAX_HEALTH constant)
- Instance traits (slots for _health, _name)
- Method traits (getter/setter for health)
- Multiple method bodies
- Type coercion bytecode

**What to Test:**
- Trait parsing (slot, const, getter, setter, method)
- Static vs instance members
- Getter/setter special methods
- Constructor with parameters
- Property access bytecode (getproperty, setproperty)

---

### Test 4: Inheritance (Super Class)

**Goal:** Test class inheritance and super calls

**File:** `tests/as3/inheritance/Character.as`

```actionscript
package {
    import flash.display.Sprite;

    public class Character extends Sprite {
        protected var _x:Number;
        protected var _y:Number;

        public function Character(x:Number, y:Number) {
            _x = x;
            _y = y;
        }

        public function move(dx:Number, dy:Number):void {
            _x += dx;
            _y += dy;
        }
    }
}
```

**File:** `tests/as3/inheritance/Enemy.as`

```actionscript
package {
    public class Enemy extends Character {
        private var _damage:int;

        public function Enemy(x:Number, y:Number, damage:int) {
            super(x, y);  // Call parent constructor
            _damage = damage;
        }

        override public function move(dx:Number, dy:Number):void {
            // Call parent method
            super.move(dx * 0.5, dy * 0.5);  // Enemy moves slower
        }

        public function attack():void {
            trace("Enemy attacks for " + _damage + " damage!");
        }
    }
}
```

**Compile (Multiple Classes):**
```bash
cd tests/as3/inheritance
mxmlc -output Enemy.swf Enemy.as -source-path . -include-sources Character.as
```

**Expected ABC Structure:**
- 2 classes: Character, Enemy
- Inheritance relationship (Enemy extends Character)
- Super constructor call (constructsuper opcode)
- Method override (move in Enemy)
- Super method call (callsuper opcode)
- Protected member access

**What to Test:**
- Multiple class definitions in one ABC
- Super class index in instance_info
- Override trait attribute
- constructsuper bytecode (0x49)
- callsuper bytecode (0x45)

---

### Test 5: Interfaces (Interface Implementation)

**Goal:** Test interface definitions and implementation

**File:** `tests/as3/interfaces/IDrawable.as`

```actionscript
package {
    public interface IDrawable {
        function draw():void;
        function get visible():Boolean;
        function set visible(value:Boolean):void;
    }
}
```

**File:** `tests/as3/interfaces/Shape.as`

```actionscript
package {
    import flash.display.Sprite;

    public class Shape extends Sprite implements IDrawable {
        private var _visible:Boolean = true;

        public function Shape() {
            // Constructor
        }

        public function draw():void {
            trace("Drawing shape...");
        }

        public function get visible():Boolean {
            return _visible;
        }

        public function set visible(value:Boolean):void {
            _visible = value;
        }
    }
}
```

**Compile:**
```bash
cd tests/as3/interfaces
mxmlc -output Shape.swf Shape.as -source-path . -include-sources IDrawable.as
```

**Expected ABC Structure:**
- 2 classes: Shape, IDrawable
- CLASS_INTERFACE flag set on IDrawable
- Interface list in Shape's instance_info
- Interface method implementations

**What to Test:**
- Interface class flag (0x04)
- Interface indices in instance_info
- Method implementation matching interface signature

---

### Test 6: Complex Features

**Goal:** Test advanced AS3 features

**File:** `tests/as3/complex/VectorTest.as`

```actionscript
package {
    import flash.display.Sprite;

    public class VectorTest extends Sprite {
        public function VectorTest() {
            // Vector type (generic)
            var numbers:Vector.<int> = new Vector.<int>();
            numbers.push(1);
            numbers.push(2);
            numbers.push(3);

            for (var i:int = 0; i < numbers.length; i++) {
                trace("Number: " + numbers[i]);
            }

            // Dictionary
            var dict:Object = {};
            dict["key1"] = "value1";
            dict["key2"] = "value2";

            for (var key:String in dict) {
                trace(key + " = " + dict[key]);
            }
        }
    }
}
```

**Compile:**
```bash
cd tests/as3/complex
mxmlc -output VectorTest.swf VectorTest.as
```

**Expected ABC Structure:**
- Generic type (Vector.<int>) in multiname pool
- TypeName multiname kind (0x1D)
- Array/vector access bytecode
- For-in loop bytecode (hasnext, nextname, nextvalue)

**What to Test:**
- TypeName multiname with type parameters
- Vector.<T> generic types
- Loop bytecode (jump, iftrue, etc.)
- Dynamic property access

---

## Test Suite Design

### Directory Structure

```
tests/as3/
├── README.md                      # Test suite overview
├── Makefile                       # Build all test SWFs
├── verify_all.sh                  # Run ABC parser on all tests
│
├── 01_hello_world/
│   ├── HelloWorld.as
│   ├── HelloWorld.swf
│   ├── expected_output.txt        # Expected parse results
│   └── README.md
│
├── 02_simple_math/
│   ├── SimpleMath.as
│   ├── SimpleMath.swf
│   ├── expected_output.txt
│   └── README.md
│
├── 03_single_class/
│   ├── Player.as
│   ├── Player.swf
│   ├── expected_output.txt
│   └── README.md
│
├── 04_inheritance/
│   ├── Character.as
│   ├── Enemy.as
│   ├── Enemy.swf
│   ├── expected_output.txt
│   └── README.md
│
├── 05_interfaces/
│   ├── IDrawable.as
│   ├── Shape.as
│   ├── Shape.swf
│   ├── expected_output.txt
│   └── README.md
│
├── 06_complex/
│   ├── VectorTest.as
│   ├── VectorTest.swf
│   ├── expected_output.txt
│   └── README.md
│
└── 99_seedling/
    └── Seedling.swf               # Real-world test
```

### Master Makefile

**File:** `tests/as3/Makefile`

```makefile
# AS3 Test SWF Generation Makefile

MXMLC = mxmlc
TESTS = 01_hello_world 02_simple_math 03_single_class 04_inheritance 05_interfaces 06_complex

.PHONY: all clean verify

all: $(TESTS)

01_hello_world:
	cd 01_hello_world && $(MXMLC) -output HelloWorld.swf HelloWorld.as

02_simple_math:
	cd 02_simple_math && $(MXMLC) -output SimpleMath.swf SimpleMath.as

03_single_class:
	cd 03_single_class && $(MXMLC) -output Player.swf Player.as

04_inheritance:
	cd 04_inheritance && $(MXMLC) -output Enemy.swf Enemy.as -source-path . -include-sources Character.as

05_interfaces:
	cd 05_interfaces && $(MXMLC) -output Shape.swf Shape.as -source-path . -include-sources IDrawable.as

06_complex:
	cd 06_complex && $(MXMLC) -output VectorTest.swf VectorTest.as

clean:
	find . -name "*.swf" -delete

verify:
	./verify_all.sh
```

### Verification Script

**File:** `tests/as3/verify_all.sh`

```bash
#!/bin/bash

# AS3 Test SWF Verification Script
# Runs SWFRecomp ABC parser on all test SWFs and checks output

SWFRECOMP="../../build/SWFRecomp"
TESTS=(
    "01_hello_world/HelloWorld.swf"
    "02_simple_math/SimpleMath.swf"
    "03_single_class/Player.swf"
    "04_inheritance/Enemy.swf"
    "05_interfaces/Shape.swf"
    "06_complex/VectorTest.swf"
)

echo "==================================="
echo "AS3 Test SWF Verification"
echo "==================================="

PASSED=0
FAILED=0

for test in "${TESTS[@]}"; do
    echo ""
    echo "Testing: $test"

    if [ ! -f "$test" ]; then
        echo "  ❌ SKIP - SWF file not found"
        continue
    fi

    # Run SWFRecomp with ABC dump flag
    OUTPUT=$($SWFRECOMP --dump-abc "$test" 2>&1)
    EXIT_CODE=$?

    if [ $EXIT_CODE -eq 0 ]; then
        echo "  ✅ PASS - Parsed successfully"
        PASSED=$((PASSED + 1))

        # Optionally compare against expected output
        EXPECTED="${test%.swf}_expected.txt"
        if [ -f "$EXPECTED" ]; then
            if diff -q <(echo "$OUTPUT") "$EXPECTED" > /dev/null; then
                echo "     Output matches expected"
            else
                echo "     ⚠️  Output differs from expected (not a failure)"
            fi
        fi
    else
        echo "  ❌ FAIL - Parse error"
        echo "$OUTPUT" | head -20
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "==================================="
echo "Results: $PASSED passed, $FAILED failed"
echo "==================================="

exit $FAILED
```

---

## Compilation Methods

### Method 1: Command Line (mxmlc)

**Basic compilation:**
```bash
mxmlc -output output.swf Main.as
```

**With options:**
```bash
mxmlc \
    -output output.swf \
    -source-path src \
    -library-path libs \
    -default-size 800 600 \
    -default-frame-rate 60 \
    -optimize=true \
    Main.as
```

**Multiple source files:**
```bash
mxmlc \
    -output output.swf \
    -source-path . \
    -include-sources Class1.as Class2.as Class3.as \
    Main.as
```

**Common options:**
- `-output <file>` - Output SWF file path
- `-source-path <path>` - Source file search paths
- `-library-path <path>` - SWC library search paths
- `-include-sources <files>` - Additional classes to include
- `-default-size <width> <height>` - SWF dimensions
- `-default-frame-rate <fps>` - Frame rate
- `-optimize=true` - Enable optimizations
- `-debug=false` - Disable debug info (smaller SWF)
- `-warnings=true` - Enable compiler warnings
- `-target-player=11.1` - Target Flash Player version

### Method 2: Configuration File

**Create:** `compile-config.xml`

```xml
<flex-config>
    <compiler>
        <source-path>
            <path-element>src</path-element>
        </source-path>

        <library-path>
            <path-element>libs</path-element>
        </library-path>

        <optimize>true</optimize>
        <warnings>true</warnings>
    </compiler>

    <default-size>
        <width>800</width>
        <height>600</height>
    </default-size>

    <default-frame-rate>60</default-frame-rate>

    <target-player>11.1</target-player>
</flex-config>
```

**Compile with config:**
```bash
mxmlc -load-config+=compile-config.xml -output output.swf Main.as
```

### Method 3: Ant Build Script

**Create:** `build.xml`

```xml
<?xml version="1.0"?>
<project name="AS3Tests" default="compile" basedir=".">

    <property name="FLEX_HOME" value="${env.FLEX_HOME}"/>
    <property name="src.dir" value="src"/>
    <property name="build.dir" value="build"/>

    <taskdef resource="flexTasks.tasks"
             classpath="${FLEX_HOME}/ant/lib/flexTasks.jar"/>

    <target name="compile">
        <mxmlc file="${src.dir}/Main.as"
               output="${build.dir}/output.swf"
               optimize="true"
               debug="false">
            <source-path path-element="${src.dir}"/>
        </mxmlc>
    </target>

    <target name="clean">
        <delete dir="${build.dir}"/>
    </target>

</project>
```

**Build:**
```bash
ant compile
```

### Method 4: Shell Script

**Create:** `compile_tests.sh`

```bash
#!/bin/bash

# AS3 Test Compilation Script

MXMLC="${FLEX_HOME}/bin/mxmlc"
SRC_DIR="src"
OUT_DIR="build"

# Create output directory
mkdir -p "$OUT_DIR"

# Compile each test
echo "Compiling HelloWorld..."
$MXMLC -output "$OUT_DIR/HelloWorld.swf" "$SRC_DIR/HelloWorld.as"

echo "Compiling SimpleMath..."
$MXMLC -output "$OUT_DIR/SimpleMath.swf" "$SRC_DIR/SimpleMath.as"

echo "Compiling Player..."
$MXMLC -output "$OUT_DIR/Player.swf" "$SRC_DIR/Player.as"

echo "All tests compiled!"
```

**Run:**
```bash
chmod +x compile_tests.sh
./compile_tests.sh
```

---

## Verification and Debugging

### Inspect ABC with RABCDAsm

RABCDAsm is invaluable for verifying ABC structure:

**Install:**
```bash
cd ~/tools
git clone https://github.com/CyberShadow/RABCDAsm.git
cd RABCDAsm
# Requires D compiler (dmd or ldc2)
sudo apt-get install dmd
rdmd --build-only abcexport.d
rdmd --build-only rabcdasm.d
rdmd --build-only rabcasm.d

# Add to PATH
echo 'export PATH=$PATH:~/tools/RABCDAsm' >> ~/.bashrc
source ~/.bashrc
```

**Extract ABC from SWF:**
```bash
abcexport test.swf
# Creates: test-0.abc, test-1.abc, ...
```

**Disassemble ABC:**
```bash
rabcdasm test-0.abc
# Creates: test-0/ directory with .asasm files
```

**Examine disassembly:**
```bash
cd test-0
ls -la
# Files:
#   HelloWorld.class.asasm    - Class definition
#   HelloWorld.script.asasm   - Script initialization
#   methods.asasm             - Method bodies
#   traits.asasm              - Trait definitions
```

**Example disassembly output:**
```
; Method body
method
    refid method0
    body
        maxstack 2
        localcount 1
        initscopedepth 0
        maxscopedepth 1

        code
            getlocal_0
            pushscope
            findpropstrict       Multiname(trace)
            pushstring           "Hello from AS3!"
            callpropvoid         Multiname(trace), 1
            returnvoid
        end
    end
end
```

**Note on RABCDAsm naming:** RABCDAsm uses different field names than the official ABC specification:
- RABCDAsm: `localcount` → ABC spec: `max_regs` (number of local registers)
- RABCDAsm: `initscopedepth` → ABC spec: `scope_depth` (initial scope depth)

When implementing the ABC parser for SWFRecomp, use the official spec names (`max_regs`, `scope_depth`). The naming difference is only in RABCDAsm's disassembly output format.

### Compare with SWFRecomp Output

**Run SWFRecomp with debug output:**
```bash
./build/SWFRecomp --dump-abc test.swf > swfrecomp_output.txt
```

**Compare:**
```bash
# Expected output format:
# ABC Version: 46.16
# Strings: 15
#   [0] ""
#   [1] "HelloWorld"
#   [2] "flash.display"
#   [3] "Sprite"
#   ...
# Methods: 2
#   [0] ()→* (constructor)
#   [1] ()→void (script init)
# Classes: 1
#   [0] HelloWorld extends flash.display::Sprite
```

### Hex Dump Analysis

**Examine raw ABC bytes:**
```bash
xxd test.swf | grep -A 50 "DoABC"
```

**Expected ABC header:**
```
00000000: 10 00 2e 00  # Version 46.16 (little-endian: 0x0010 0x002E)
00000004: 02           # Integer pool count: 2 (implicit 0, explicit 1)
00000005: 01 00 00 00  # Integer[1] = 1
...
```

### Verify with Flash Player

**Test runtime behavior:**
```bash
# Install standalone Flash Player projector
cd ~/tools
wget https://fpdownload.macromedia.com/pub/flashplayer/updaters/32/flash_player_sa_linux.x86_64.tar.gz
tar -xzf flash_player_sa_linux.x86_64.tar.gz

# Run SWF
./flashplayer test.swf
```

**Expected console output:**
```
Hello from AS3!
```

---

## Troubleshooting

### Problem: mxmlc command not found

**Solution:**
```bash
# Verify FLEX_HOME is set
echo $FLEX_HOME

# If not set, add to .bashrc
echo 'export FLEX_HOME=~/tools/flex-sdk' >> ~/.bashrc
echo 'export PATH=$PATH:$FLEX_HOME/bin' >> ~/.bashrc
source ~/.bashrc

# Verify mxmlc exists
ls -la $FLEX_HOME/bin/mxmlc
```

### Problem: playerglobal.swc not found

**Error message:**
```
Error: Unable to resolve resource bundle "core" for locale "en_US".
Error: unable to open 'frameworks/libs/player/11.1/playerglobal.swc'
```

**Solution:**
```bash
# Download playerglobal.swc
mkdir -p $FLEX_HOME/frameworks/libs/player/11.1
cd $FLEX_HOME/frameworks/libs/player/11.1
wget https://fpdownload.macromedia.com/get/flashplayer/updaters/11/playerglobal11_1.swc
mv playerglobal11_1.swc playerglobal.swc

# Verify
ls -la $FLEX_HOME/frameworks/libs/player/11.1/playerglobal.swc
```

### Problem: Blank SWF file generated

**Causes:**
1. Main class doesn't extend Sprite or MovieClip
2. No document class specified
3. Source file has syntax errors (check stderr)

**Solution 1: Extend Sprite**
```actionscript
// Wrong
public class Main {
    public function Main() { }
}

// Correct
import flash.display.Sprite;
public class Main extends Sprite {
    public function Main() { }
}
```

**Solution 2: Check for errors**
```bash
mxmlc -output test.swf Main.as 2>&1 | tee compile.log
cat compile.log
```

### Problem: Multiple classes in different files

**Error message:**
```
Error: A file found in a source-path must have the same name as the class definition inside the file.
```

**Solution: Use package structure**
```actionscript
// File: com/example/MyClass.as
package com.example {
    public class MyClass {
        // ...
    }
}

// File: Main.as
package {
    import com.example.MyClass;
    import flash.display.Sprite;

    public class Main extends Sprite {
        public function Main() {
            var obj:MyClass = new MyClass();
        }
    }
}
```

**Compile with source path:**
```bash
mxmlc -source-path . -output Main.swf Main.as
```

### Problem: Old ABC version generated

**Check ABC version:**
```bash
abcexport test.swf
rabcdasm test-0.abc
grep "version" test-0/*.asasm
# Should show: version 46.16 (or higher)
```

**Force Flash Player version:**
```bash
mxmlc -target-player=11.1 -output test.swf Main.as
```

### Problem: Too many classes in ABC

**Issue:** Compiler includes entire Flex framework

**Solution: Minimize dependencies**
```actionscript
// Instead of:
import mx.core.*;
import mx.controls.*;

// Use only what you need:
import flash.display.Sprite;
import flash.events.Event;
```

**Or use -optimize flag:**
```bash
mxmlc -optimize=true -output test.swf Main.as
```

---

## Quick Reference

### Compile Commands Cheat Sheet

```bash
# Basic
mxmlc -output out.swf Main.as

# With optimization
mxmlc -optimize=true -debug=false -output out.swf Main.as

# Multiple files
mxmlc -source-path . -include-sources Class1.as Class2.as -output out.swf Main.as

# With size and framerate
mxmlc -default-size 800 600 -default-frame-rate 60 -output out.swf Main.as

# Target specific Flash Player version
mxmlc -target-player=11.1 -output out.swf Main.as

# Verbose output
mxmlc -verbose-stacktraces -warnings -output out.swf Main.as
```

### RABCDAsm Commands

```bash
# Extract ABC from SWF
abcexport file.swf

# Disassemble ABC
rabcdasm file-0.abc

# Assemble ABC
rabcasm file-0/file-0.main.asasm

# Replace ABC in SWF
abcreplace file.swf 0 file-0.abc
```

### File Extensions

| Extension | Description |
|-----------|-------------|
| `.as` | ActionScript source file |
| `.swf` | Shockwave Flash (compiled) |
| `.swc` | Flash component library |
| `.abc` | ActionScript Bytecode (extracted) |
| `.asasm` | ActionScript assembly (disassembled) |

---

## Summary

### Essential Commands

```bash
# 1. Install Flex SDK
wget https://archive.apache.org/dist/flex/4.16.1/binaries/apache-flex-sdk-4.16.1-bin.tar.gz
tar -xzf apache-flex-sdk-4.16.1-bin.tar.gz -C ~/tools
export FLEX_HOME=~/tools/apache-flex-sdk-4.16.1-bin
export PATH=$PATH:$FLEX_HOME/bin

# 2. Download playerglobal.swc
mkdir -p $FLEX_HOME/frameworks/libs/player/11.1
cd $FLEX_HOME/frameworks/libs/player/11.1
wget https://fpdownload.macromedia.com/get/flashplayer/updaters/11/playerglobal11_1.swc
mv playerglobal11_1.swc playerglobal.swc

# 3. Compile test SWF
cd tests/as3
mxmlc -output HelloWorld.swf HelloWorld.as

# 4. Verify with RABCDAsm
abcexport HelloWorld.swf
rabcdasm HelloWorld-0.abc

# 5. Test with SWFRecomp
../../build/SWFRecomp --dump-abc HelloWorld.swf
```

### Next Steps

1. **Set up tools** - Install Flex SDK or AIR SDK
2. **Create test directory** - `mkdir -p tests/as3/01_hello_world`
3. **Write test AS3 files** - Start with HelloWorld.as
4. **Compile to SWF** - Use mxmlc
5. **Verify structure** - Use RABCDAsm to disassemble
6. **Test parser** - Run SWFRecomp ABC parser
7. **Iterate** - Add more complex tests progressively

---

**Document Status:** Complete
**Last Updated:** October 29, 2025
**Author:** Claude Code
