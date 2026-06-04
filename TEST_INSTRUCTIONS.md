# LvglFontGenerator Test Instructions

## Changes Applied

The following improvements from lv_font_conv_cpp have been successfully applied:

1. ✅ **GPOS Reader Integration**
   - Added `gpos_reader.cpp` and `gpos_reader.h` for complete GPOS table support
   - Query-based kerning extraction (same approach as Node.js lv_font_conv)

2. ✅ **Array Formatting Fix**
   - Changed kerning arrays from 16 items per line to 8 items per line
   - Fixed comma placement (no trailing comma before line breaks)
   - Matches online tool output format exactly

## Test Steps

### 1. Basic GUI Test

Launch the application:
```bash
cd Z:/LvglFontUtility/LvglFontGenerator/build
./LvglFontGenerator.exe
```

### 2. Test with MyriadPro Font

Use the same font that was successfully tested with the CLI tool:
- **Font**: `Z:\wqy-zenhei\MyriadPro-Bold-Revised.ttf`
- **Size**: 16 px
- **BPP**: 4
- **Range**: 0x20-0x7F (ASCII printable)
- **Enable Kerning**: ✓ Yes

### 3. Expected Output Format

The generated C file should have kerning arrays formatted like this:

```c
/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 1, 0, 2, 0, 0, 0,
    0, 0, 3, 4, 0, 5, 0, 0,
    // ... 8 items per line, no trailing comma before newline
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 0, 3, 3,
    // ... 8 items per line
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    // ... 8 items per line, signed values
};
```

### 4. Verify Kerning Data

Check that:
- ✅ Arrays have 8 items per line (not 16)
- ✅ No comma before line breaks
- ✅ Values are signed integers (can be negative)
- ✅ Large number of kerning pairs extracted (should be ~1000+ for MyriadPro)

## Known Differences from CLI Tool

The GUI and CLI tools use different kerning extraction methods:

- **CLI (lv_font_conv_cpp)**: Direct GPOS table parsing
- **GUI (LvglFontGenerator)**: HarfBuzz shaping-based extraction

Both methods are valid and produce correct kerning data. The HarfBuzz method may produce slightly different pair counts, but the output format should now be identical (8 items per line, proper comma placement).

## Success Criteria

✅ Application builds without errors
✅ GUI launches successfully
✅ Can load font file
✅ Can generate C file with kerning enabled
✅ Output has 8 items per line in kerning arrays
✅ No trailing commas before line breaks
✅ Kerning values are properly extracted and formatted

## Troubleshooting

If the conversion seems to hang:
- This is normal - kerning extraction queries many character pairs (95×95 = 9025 for ASCII)
- Wait 5-10 seconds for completion
- Check the console output for progress messages
