#!/usr/bin/env python3
"""
Check if a font file contains kerning information using freetype-py
Install: pip install freetype-py
"""

import sys
import freetype

def check_kerning(font_path):
    print(f"Checking font: {font_path}\n")

    # Load the font
    face = freetype.Face(font_path)

    print(f"Font family: {face.family_name.decode('utf-8')}")
    print(f"Style: {face.style_name.decode('utf-8')}")
    print(f"Has kerning flag (FT_HAS_KERNING): {face.has_kerning}")
    print(f"Face flags: 0x{face.face_flags:08x}")
    print(f"FT_FACE_FLAG_KERNING (0x40): {'YES' if face.face_flags & 0x40 else 'NO'}")
    print()
    print("NOTE: FT_HAS_KERNING only checks traditional 'kern' table.")
    print("      Modern OpenType fonts use GPOS table for kerning.")
    print("      FT_Get_Kerning() can extract from both, so we test directly.")
    print()

    # Set font size
    face.set_pixel_sizes(0, 48)

    # Test common kerning pairs
    test_pairs = [
        ('A', 'V'),
        ('T', 'o'),
        ('W', 'a'),
        ('V', 'A'),
        ('L', 'T'),
        ('Y', 'o'),
        ('P', 'A'),
        ('F', 'A'),
    ]

    print("Testing kerning pairs:")
    print("-" * 60)

    kerning_found = False
    for left_char, right_char in test_pairs:
        left_glyph = face.get_char_index(ord(left_char))
        right_glyph = face.get_char_index(ord(right_char))

        if left_glyph == 0 or right_glyph == 0:
            print(f"  {left_char}-{right_char}: glyph not found")
            continue

        # Get kerning (returns FT_Vector with x, y in 1/64 pixels)
        kerning = face.get_kerning(left_glyph, right_glyph, freetype.FT_KERNING_DEFAULT)

        # Convert to pixels
        kern_x_64 = kerning.x  # in 1/64 pixels
        kern_x_px = kern_x_64 / 64.0  # in pixels

        # Convert to LVGL format (1/16 pixels, kern_scale=16)
        kern_lvgl = (kern_x_64 + 2) // 4  # Same formula as in the C++ code

        status = "NON-ZERO" if kern_x_64 != 0 else "zero"
        if kern_x_64 != 0:
            kerning_found = True

        print(f"  {left_char}-{right_char}: glyph({left_glyph:3d},{right_glyph:3d}) "
              f"delta.x={kern_x_64:5d} (1/64px) = {kern_x_px:6.2f}px "
              f"LVGL={kern_lvgl:4d} [{status}]")

    print("-" * 60)
    if kerning_found:
        print(f"\n✓ Font HAS kerning data! ({nonZeroCount} non-zero pairs found)")
        if not face.has_kerning:
            print("  (Note: Kerning is in GPOS table, not traditional kern table)")
    else:
        print("\n✗ Font has NO kerning data (all values are zero)")
        print("  This font does not contain kerning information.")

    return kerning_found

if __name__ == "__main__":
    if len(sys.argv) > 1:
        font_path = sys.argv[1]
    else:
        font_path = "Z:/LvglFontUtility/LvglFontGenerator/MyriadPro-Bold-Revised_20250304.ttf"

    try:
        check_kerning(font_path)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
