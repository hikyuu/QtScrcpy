# calcPerspectiveSkillDistance - Visual Implementation Guide

## Function Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                calcPerspectiveSkillDistance                      │
│                                                                  │
│  Input:  rawPos (QPointF)    - Mouse/touch position (0.0-1.0)  │
│          centerPos (QPointF) - Control center position           │
│          ratio (double)      - Speed/skill scaling ratio        │
│                                                                  │
│  Output: distance (QPointF)  - Perspective-corrected distance   │
└─────────────────────────────────────────────────────────────────┘
```

## Algorithm Flow

```
┌────────────────────┐
│   Input: rawPos    │
│   centerPos, ratio │
└─────────┬──────────┘
          │
          ▼
┌─────────────────────────────────────────────────────┐
│ Step 1: Calculate Raw Distance                      │
│   rawDistance = rawPos - centerPos                  │
└─────────┬───────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────┐
│ Step 2: Apply Perspective Scale                     │
│   perspectiveScale = 0.6 * rawDistance.y + 1       │
│                                                      │
│   Effect: Objects lower on screen appear larger     │
│   - At center (y=0): scale = 1.0 (neutral)         │
│   - Moving down (+y): scale > 1.0 (closer)         │
│   - Moving up (-y): scale < 1.0 (farther)          │
└─────────┬───────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────┐
│ Step 3: Calculate Y Position Factor                 │
│   yPosFactor = perspectiveScale * rawPos.y         │
│                                                      │
│   Effect: Non-linear transformation based on        │
│   absolute position combined with perspective       │
└─────────┬───────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────┐
│ Step 4: Calculate Corrected Distance                │
│                                                      │
│   X (Horizontal):                                    │
│   distance.x = rawDistance.x                        │
│              / qMax(ratio * perspectiveScale, ε)    │
│                                                      │
│   Y (Vertical):                                      │
│   distance.y = rawDistance.y                        │
│              / qMax(ratio/2 + yPosFactor, ε)        │
│                                                      │
│   where ε = 0.0001 (division-by-zero protection)   │
└─────────┬───────────────────────────────────────────┘
          │
          ▼
┌────────────────────┐
│  Return: distance  │
└────────────────────┘
```

## Perspective Transformation Visualization

```
Screen Coordinates (Top-Down MOBA View):
                   
     (0,0) ─────────────────────────── (1,0)
       │                                  │
       │         ↑ farther (scale < 1)   │
       │         │                        │
       │         │                        │
       │    center (0.5, 0.5)            │
       │         │  (scale = 1.0)        │
       │         │                        │
       │         ↓ closer (scale > 1)    │
       │                                  │
     (0,1) ─────────────────────────── (1,1)
```

## Usage Pattern

### Before (Duplicated Code):
```cpp
// In processMobaMouseMove (3 places)
const QPointF rawDistance{rawPos - centerPos};
const double perspectiveScale = 0.6 * rawDistance.y() + 1;
const double yPosFactor = perspectiveScale * rawPos.y();
const QPointF distance{
    rawDistance.x() / ratio / perspectiveScale,
    rawDistance.y() / (ratio / 2 + yPosFactor)
};
```

### After (Using Function):
```cpp
// Consolidated into single function call
const QPointF distance = calcPerspectiveSkillDistance(
    rawPos, centerPos, ratio
);
```

## Three Usage Locations

```
┌───────────────────────────────────────────┐
│     processMobaMouseMove (Skill)          │
│  - Handles skill indicator dragging       │
│  - Updates as mouse moves with skill      │
│    button pressed                         │
└───────────────┬───────────────────────────┘
                │
                ├─> calcPerspectiveSkillDistance()
                │
┌───────────────┴───────────────────────────┐
│     processMobaMouseMove (Wheel)          │
│  - Handles movement wheel control         │
│  - Updates continuously while wheel       │
│    button is pressed                      │
└───────────────┬───────────────────────────┘
                │
                ├─> calcPerspectiveSkillDistance()
                │
┌───────────────┴───────────────────────────┐
│     processMobaSkill                      │
│  - Initial skill target positioning       │
│  - Called when skill key is first         │
│    pressed (if not quick cast)            │
└───────────────┬───────────────────────────┘
                │
                └─> calcPerspectiveSkillDistance()
```

## Safety Features

```
Division-by-zero Protection:
─────────────────────────────
  
  Before: distance.x = rawDistance.x() / (ratio * perspectiveScale)
  After:  distance.x = rawDistance.x() / qMax(ratio * perspectiveScale, 0.0001)
  
  Effect: Minimum divisor of 0.0001 prevents crashes
          Returns very large value if inputs are extreme
```

## Configuration

```
┌────────────────────────────────────────────┐
│  PERSPECTIVE_COEFFICIENT = 0.6             │
│  ─────────────────────────────             │
│  Controls strength of perspective effect   │
│                                             │
│  Lower value (e.g., 0.3):                  │
│    - Subtle perspective effect             │
│    - More uniform feeling across screen    │
│                                             │
│  Higher value (e.g., 0.9):                 │
│    - Strong perspective effect             │
│    - More dramatic scaling difference      │
│      between top and bottom of screen      │
└────────────────────────────────────────────┘
```

## Benefits Summary

```
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│  Code Reduction  │    │  Maintainability │    │     Safety       │
│  ─────────────   │    │  ──────────────  │    │    ──────        │
│  3 duplicates    │    │  Single source   │    │  Zero-division   │
│  → 1 function    │    │  of truth        │    │  protection      │
│  -27 lines       │    │  Easy updates    │    │  Robust code     │
└──────────────────┘    └──────────────────┘    └──────────────────┘

┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│  Documentation   │    │   Testability    │    │ Configurability  │
│  ─────────────   │    │   ───────────    │    │ ──────────────   │
│  JSDoc comments  │    │  Static function │    │  Named constant  │
│  Inline docs     │    │  No side effects │    │  Easy to tune    │
│  Full guide      │    │  Unit testable   │    │  PERSPECTIVE_*   │
└──────────────────┘    └──────────────────┘    └──────────────────┘
```

## Testing Scenarios

```
Test Case 1: Center Position
  Input:  rawPos = (0.5, 0.5), centerPos = (0.5, 0.5), ratio = 3
  Expect: Neutral perspective (scale = 1.0)
  
Test Case 2: Bottom Position  
  Input:  rawPos = (0.5, 0.9), centerPos = (0.5, 0.5), ratio = 3
  Expect: Closer perspective (scale > 1.0), larger apparent distance
  
Test Case 3: Top Position
  Input:  rawPos = (0.5, 0.1), centerPos = (0.5, 0.5), ratio = 3
  Expect: Farther perspective (scale < 1.0), smaller apparent distance

Test Case 4: Edge Case - Zero Ratio
  Input:  rawPos = (0.5, 0.5), centerPos = (0.5, 0.5), ratio = 0
  Expect: No crash, returns safe value due to qMax protection
```
