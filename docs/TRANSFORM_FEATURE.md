# Transform Feature Documentation

## Overview

The transform feature allows each ESP device to apply rotation and flip operations to the extracted image region before mapping it to the LED strip. This enables flexible physical LED strip orientation without changing the image data.

## Transform Byte (Byte 47)

Located at position 47 in the binary discovery response packet.

### Bit Layout

```
 7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ R │ R │ R │ Z │ Y │ X │ Rot1│Rot0│
└───┴───┴───┴───┴───┴───┴───┴───┘
     │       │   │   │   │
     │       │   │   │   └──── Flip X (horizontal)
     │       │   │   └──────── Flip Y (vertical)
     │       │   └──────────── Flip Z (diagonal/transpose)
     │       └──────────────── Rotation (bits 0-1)
     └──────────────────────── Reserved (bits 5-7)
```

### Values

**Rotation (Bits 0-1):**
- `0` = 0° (no rotation)
- `1` = 90° clockwise
- `2` = 180°
- `3` = 270° clockwise (90° counter-clockwise)

**Flip X (Bit 2):**
- `0` = No horizontal flip
- `1` = Mirror across vertical axis (left ↔ right)

**Flip Y (Bit 3):**
- `0` = No vertical flip
- `1` = Mirror across horizontal axis (top ↔ bottom)

**Flip Z (Bit 4):**
- `0` = No diagonal flip
- `1` = Transpose (swap X and Y coordinates)

## Transform Operations

### Order of Application

1. **Rotation** - Applied first
2. **Flip X** - Horizontal flip
3. **Flip Y** - Vertical flip
4. **Flip Z** - Diagonal flip (transpose)

### Visual Examples

#### Original Image (4×4)
```
A B C D
E F G H
I J K L
M N O P
```

#### Rotation 90° (0x01)
```
M I E A
N J F B
O K G C
P L H D
```

#### Rotation 180° (0x02)
```
P O N M
L K J I
H G F E
D C B A
```

#### Rotation 270° (0x03)
```
D H L P
C G K O
B F J N
A E I M
```

#### Flip X (0x04)
```
D C B A
H G F E
L K J I
P O N M
```

#### Flip Y (0x08)
```
M N O P
I J K L
E F G H
A B C D
```

#### Flip Z / Transpose (0x10)
```
A E I M
B F J N
C G K O
D H L P
```

## Use Cases

### Physical LED Strip Mounting

1. **LED strip mounted upside down**
   - Use Flip Y (0x08) to correct orientation

2. **LED strip mounted left-to-right instead of top-to-bottom**
   - Use Rotation 90° + Flip X to correct

3. **LED matrix with reversed wiring**
   - Use Flip X or Flip Y depending on direction

4. **Diagonal LED arrangement**
   - Use Flip Z to transpose coordinates

### Combined Transforms

**Example 1: 180° rotation via mirrors**
```
FlipX + FlipY = 0x0C
Equivalent to Rotation 180° but using mirrors
```

**Example 2: Transpose and rotate**
```
Rotation 90° + FlipZ = 0x11
Complex orientation correction
```

## Implementation

### ESP Side (C++)

```cpp
struct MappingConfig {
  // ... other fields ...
  uint8_t rotation = 0;    // 0-3
  bool flipX = false;
  bool flipY = false;
  bool flipZ = false;
};

// Pack into transform byte
uint8_t transform = (mappingConfig.rotation & 0x03) |
                    (mappingConfig.flipX ? 0x04 : 0) |
                    (mappingConfig.flipY ? 0x08 : 0) |
                    (mappingConfig.flipZ ? 0x10 : 0);
response[47] = transform;
```

### Server Side (JavaScript)

```javascript
// Parse transform byte
const transformByte = buffer.readUInt8(47);
const rotation = transformByte & 0x03;
const flipX = (transformByte & 0x04) !== 0;
const flipY = (transformByte & 0x08) !== 0;
const flipZ = (transformByte & 0x10) !== 0;

device.mapping = {
  // ... other fields ...
  rotation,
  flipX,
  flipY,
  flipZ
};
```

### Applying Transforms (Pseudocode)

```cpp
RgbColor getTransformedPixel(int ledX, int ledY) {
  int x = ledX;
  int y = ledY;

  // 1. Apply rotation
  switch (rotation) {
    case 1:  // 90° clockwise
      { int tmp = x; x = y; y = width - 1 - tmp; }
      break;
    case 2:  // 180°
      x = width - 1 - x;
      y = height - 1 - y;
      break;
    case 3:  // 270° clockwise
      { int tmp = x; x = height - 1 - y; y = tmp; }
      break;
  }

  // 2. Apply Flip X (horizontal)
  if (flipX) {
    x = width - 1 - x;
  }

  // 3. Apply Flip Y (vertical)
  if (flipY) {
    y = height - 1 - y;
  }

  // 4. Apply Flip Z (transpose)
  if (flipZ) {
    int tmp = x;
    x = y;
    y = tmp;
  }

  return imagePixelAt(rectX + x, rectY + y);
}
```

## Configuration

### Via Web Interface (ESP)

POST to `/config` endpoint:
```json
{
  "rotation": 1,
  "flipX": false,
  "flipY": true,
  "flipZ": false
}
```

### Discovery Response

Transforms are automatically included in binary discovery responses and displayed in the server web interface:

```
Device: ESP-1234AB
  Rotation: 90°
  Flip X: No
  Flip Y: Yes
  Flip Z (Diagonal): No
```

## Performance

- **No overhead** - Transform applied during pixel lookup
- **Integer operations** - Very fast on ESP8266
- **No memory cost** - No additional buffers needed

## Testing

### Test Pattern Recommendations

1. **Gradient pattern** - Easy to see rotation
2. **Asymmetric pattern** - Shows flip direction
3. **Text/numbers** - Clear orientation indicator
4. **Checkerboard** - Verifies alignment

### Example Test Sequence

```bash
# 1. Send gradient frame
curl -X POST http://localhost:3001/api/frame \
  -d '{"pattern":"gradient","width":16,"height":16}'

# 2. Configure device with 90° rotation
curl -X POST http://192.168.1.100/config \
  -d '{"rotation":1}'

# 3. Observe rotated output on LEDs

# 4. Try different flip combinations
curl -X POST http://192.168.1.100/config \
  -d '{"rotation":0,"flipX":true,"flipY":false}'
```

## Limitations

1. **Rectangle mode only** - Transforms apply to rectangle extraction
   - Row/column modes may need different transform logic
2. **No sub-pixel transforms** - Integer coordinate transforms only
3. **No shear/skew** - Only rotation by 90° multiples

## Future Enhancements

Possible additions:
- [ ] Arbitrary angle rotation (using interpolation)
- [ ] Scale transforms (zoom in/out)
- [ ] Offset transforms (pan)
- [ ] Perspective correction
- [ ] Custom transformation matrices

## Summary

The transform feature provides flexible image orientation control with:
- ✅ 4 rotation angles (0°, 90°, 180°, 270°)
- ✅ 3 flip axes (X, Y, Z)
- ✅ Combined transforms for complex orientations
- ✅ Zero performance overhead
- ✅ Simple bit-packed encoding (1 byte)

Perfect for correcting physical LED strip mounting without changing image generation code!
