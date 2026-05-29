#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test all BPP format conversions
"""

def test_1bpp():
    """Test 1bpp conversion: 8 pixels per byte"""
    print("=" * 60)
    print("Testing 1bpp (1 bit per pixel, 8 pixels per byte)")
    print("=" * 60)

    # Test data: 8 grayscale values
    test_values = [0, 50, 100, 128, 150, 200, 255, 128]

    # Convert to 1-bit (threshold at 127)
    pixels_1bit = [1 if v > 127 else 0 for v in test_values]

    # Pack into byte (MSB first)
    packed = 0
    for i, bit in enumerate(pixels_1bit):
        packed |= (bit << (7 - i))

    print(f"Input (8 pixels):  {test_values}")
    print(f"1-bit values:      {pixels_1bit}")
    print(f"Packed byte:       {packed} (0x{packed:02x})")
    print(f"Binary:            {bin(packed)}")
    print(f"Expected pattern:  0b00001101 (0x0d)")
    print()

def test_2bpp():
    """Test 2bpp conversion: 4 pixels per byte"""
    print("=" * 60)
    print("Testing 2bpp (2 bits per pixel, 4 pixels per byte)")
    print("=" * 60)

    # Test data: 4 grayscale values
    test_values = [0, 85, 170, 255]

    # Convert to 2-bit (0-3)
    pixels_2bit = [(v * 3 + 127) // 255 for v in test_values]

    # Pack into byte (MSB first)
    packed = 0
    for i, pixel in enumerate(pixels_2bit):
        packed |= (pixel << (6 - i * 2))

    print(f"Input (4 pixels):  {test_values}")
    print(f"2-bit values:      {pixels_2bit}")
    print(f"Packed byte:       {packed} (0x{packed:02x})")
    print(f"Binary:            {bin(packed)}")
    print(f"Expected:          0=00, 85=01, 170=10, 255=11")
    print(f"Expected pattern:  0b00011011 (0x1b)")
    print()

def test_4bpp():
    """Test 4bpp conversion: 2 pixels per byte"""
    print("=" * 60)
    print("Testing 4bpp (4 bits per pixel, 2 pixels per byte)")
    print("=" * 60)

    # Test data: 2 grayscale values
    test_values = [26, 255]

    # Convert to 4-bit (0-15)
    pixels_4bit = [(v * 15 + 127) // 255 for v in test_values]

    # Pack into byte (high nibble first)
    packed = (pixels_4bit[0] << 4) | pixels_4bit[1]

    print(f"Input (2 pixels):  {test_values}")
    print(f"4-bit values:      {pixels_4bit}")
    print(f"Packed byte:       {packed} (0x{packed:02x})")
    print(f"Binary:            {bin(packed)}")
    print(f"Expected:          26->1, 255->15")
    print(f"Expected pattern:  0x1f")
    print()

def test_8bpp():
    """Test 8bpp: 1 pixel per byte"""
    print("=" * 60)
    print("Testing 8bpp (8 bits per pixel, 1 pixel per byte)")
    print("=" * 60)

    # Test data: 1 grayscale value
    test_values = [128]

    print(f"Input (1 pixel):   {test_values}")
    print(f"Output byte:       {test_values[0]} (0x{test_values[0]:02x})")
    print(f"No conversion needed for 8bpp")
    print()

def calculate_bitmap_size(width, height, bpp):
    """Calculate bitmap size in bytes for given dimensions and bpp"""
    total_pixels = width * height

    if bpp == 1:
        return (total_pixels + 7) // 8
    elif bpp == 2:
        return (total_pixels + 3) // 4
    elif bpp == 4:
        return (total_pixels + 1) // 2
    else:  # 8bpp
        return total_pixels

def test_bitmap_sizes():
    """Test bitmap size calculations"""
    print("=" * 60)
    print("Testing bitmap size calculations")
    print("=" * 60)

    test_cases = [
        (10, 10, 1),   # 100 pixels
        (10, 10, 2),   # 100 pixels
        (10, 10, 4),   # 100 pixels
        (10, 10, 8),   # 100 pixels
        (11, 11, 1),   # 121 pixels (odd)
        (11, 11, 2),   # 121 pixels (odd)
        (11, 11, 4),   # 121 pixels (odd)
    ]

    for width, height, bpp in test_cases:
        size = calculate_bitmap_size(width, height, bpp)
        pixels = width * height
        print(f"{width}x{height} @ {bpp}bpp: {pixels} pixels -> {size} bytes")
    print()

if __name__ == "__main__":
    print("\n")
    print("*" * 60)
    print("BPP Format Conversion Tests")
    print("*" * 60)
    print()

    test_1bpp()
    test_2bpp()
    test_4bpp()
    test_8bpp()
    test_bitmap_sizes()

    print("*" * 60)
    print("All tests completed!")
    print("*" * 60)
