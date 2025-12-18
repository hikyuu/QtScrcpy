# calcPerspectiveSkillDistance Implementation

## Overview

This document describes the implementation of the `calcPerspectiveSkillDistance` function for the QtScrcpy project, specifically for MOBA game input conversion with perspective correction.

## Problem Statement

The original code had duplicated perspective calculation logic in three different locations within `inputconvertgame.cpp`. This implementation extracts and consolidates this logic into a reusable static function.

## Implementation Details

### Function Signature

```cpp
static QPointF calcPerspectiveSkillDistance(const QPointF &rawPos, const QPointF &centerPos, double ratio);
```

### Parameters

- `rawPos`: The raw position (normalized coordinates, 0.0-1.0) of the mouse/touch input
- `centerPos`: The center position of the skill wheel or MOBA control center
- `ratio`: The speed/skill ratio for scaling the distance

### Return Value

Returns a `QPointF` representing the perspective-corrected distance that should be applied to the skill or wheel position.

## Algorithm Explanation

The function implements a perspective transformation that simulates depth perception for MOBA-style games:

### 1. Calculate Raw Distance
```cpp
const QPointF rawDistance{rawPos - centerPos};
```
This calculates the vector from the center position to the current position.

### 2. Apply Perspective Scale
```cpp
const double perspectiveScale = PERSPECTIVE_COEFFICIENT * rawDistance.y() + 1;
```
The perspective scale increases linearly based on vertical position (y-coordinate):
- When `rawDistance.y()` is 0 (at center): scale = 1.0
- When `rawDistance.y()` is positive (moving down): scale > 1.0 (objects appear closer/larger)
- When `rawDistance.y()` is negative (moving up): scale < 1.0 (objects appear farther/smaller)

The `PERSPECTIVE_COEFFICIENT` (0.6) is a named constant that controls the strength of the perspective effect.

### 3. Calculate Y Position Factor
```cpp
const double yPosFactor = perspectiveScale * rawPos.y();
```
This combines the perspective scale with the absolute y position to create a non-linear transformation that becomes more pronounced at different vertical positions.

### 4. Calculate Perspective-Corrected Distance
```cpp
const double yDivisor = ratio / 2 + yPosFactor;
const QPointF distance{
    rawDistance.x() / qMax(ratio * perspectiveScale, 0.0001),
    rawDistance.y() / qMax(yDivisor, 0.0001)
};
```

**Horizontal (X) Correction:**
- Divided by `ratio * perspectiveScale` for combined scaling and perspective correction
- Uses `qMax` with 0.0001 minimum to prevent division by zero
- This ensures horizontal movement feels natural regardless of vertical position

**Vertical (Y) Correction:**
- Divided by a more complex formula: `(ratio / 2 + yPosFactor)`
- Using `ratio / 2` creates asymmetric scaling (vertical movement is more sensitive)
- Adding `yPosFactor` creates non-linear scaling that varies with position
- Uses `qMax` with 0.0001 minimum to prevent division by zero
- This simulates the visual compression that occurs in perspective views

## Usage Locations

The function is used in three places:

1. **processMobaMouseMove - Skill Handling** (Line ~2117):
   Used when dragging a skill indicator while a skill button is pressed.

2. **processMobaMouseMove - Wheel Handling** (Line ~2129):
   Used when controlling the MOBA movement wheel with mouse/touch input.

3. **processMobaSkill** (Line ~2179):
   Used when initially positioning a skill target indicator.

## Benefits

1. **Code Reusability**: Eliminates code duplication across three locations
2. **Maintainability**: Changes to the perspective algorithm only need to be made in one place
3. **Clarity**: Well-documented function with clear purpose and algorithm explanation
4. **Testability**: Static function can be easily unit tested
5. **Safety**: Includes division-by-zero protection
6. **Configurability**: Uses named constant for perspective effect strength

## Mathematical Foundation

The perspective transformation is based on the principle that in a 3D perspective view projected onto a 2D screen:
- Objects farther from the viewer (higher on screen in top-down games) appear smaller
- Objects closer to the viewer (lower on screen) appear larger
- Movement of objects should be scaled proportionally to maintain consistent control feel

## Files Modified

- `QtScrcpy/QtScrcpyCore/src/device/controller/inputconvert/inputconvertgame.h`: Added function declaration
- `QtScrcpy/QtScrcpyCore/src/device/controller/inputconvert/inputconvertgame.cpp`: Added function implementation and replaced three instances of duplicated code

## Patch File

The complete changes are available in: `0001-Implement-calcPerspectiveSkillDistance-function.patch`

To apply the patch to the QtScrcpyCore submodule:
```bash
cd QtScrcpy/QtScrcpyCore
git apply ../../0001-Implement-calcPerspectiveSkillDistance-function.patch
```
