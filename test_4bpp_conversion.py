#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test 4bpp conversion logic
"""

def convert_8bit_to_4bit(gray8):
    """Convert 8-bit grayscale (0-255) to 4-bit (0-15)"""
    return (gray8 * 15 + 127) // 255

def pack_4bpp(pixel1, pixel2):
    """Pack two 4-bit pixels into one byte"""
    return (pixel1 << 4) | pixel2

# Test with some sample values
test_values = [26, 255, 255, 97, 13, 255, 255, 85, 1, 254, 255, 73]

print("Testing 8bpp to 4bpp conversion:")
print("=" * 60)

# Convert to 4-bit
pixels_4bit = [convert_8bit_to_4bit(v) for v in test_values]
print(f"8-bit values: {test_values}")
print(f"4-bit values: {pixels_4bit}")

# Pack into bytes
packed = []
for i in range(0, len(pixels_4bit), 2):
    p1 = pixels_4bit[i]
    p2 = pixels_4bit[i+1] if i+1 < len(pixels_4bit) else 0
    packed_byte = pack_4bpp(p1, p2)
    packed.append(packed_byte)

print(f"Packed bytes: {packed}")
print(f"Packed hex:   {[hex(b) for b in packed]}")

print("\nExpected from official tool (first 6 bytes):")
print("Hex: [0x1f, 0xf6, 0x0f, 0xf5, 0x0f, 0xf4]")
print("Dec: [31, 246, 15, 245, 15, 244]")

print("\nComparison:")
official = [31, 246, 15, 245, 15, 244]
for i in range(min(len(packed), len(official))):
    match = "OK" if packed[i] == official[i] else "DIFF"
    print(f"  Byte {i}: Generated={packed[i]:3d} (0x{packed[i]:02x}), Official={official[i]:3d} (0x{official[i]:02x}) {match}")
