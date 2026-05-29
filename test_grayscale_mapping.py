#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
检查灰度等级映射是否一致
"""

def test_grayscale_mapping():
    """测试不同灰度值的映射"""

    print("=" * 70)
    print("Grayscale Mapping Test")
    print("=" * 70)

    # 测试8位到4位的转换
    print("\n8-bit to 4-bit conversion:")
    print("Input (8-bit) | Output (4-bit) | Formula: (gray * 15 + 127) / 255")
    print("-" * 70)

    test_values = [0, 16, 32, 64, 85, 128, 170, 192, 224, 240, 255]

    for gray8 in test_values:
        # 当前工具的转换公式
        gray4_current = (gray8 * 15 + 127) // 255

        # 简单线性映射（可能的替代方案）
        gray4_simple = gray8 // 16

        # 四舍五入映射
        gray4_round = round(gray8 * 15 / 255)

        print(f"  {gray8:3d}         |      {gray4_current:2d}       | Simple: {gray4_simple:2d}, Round: {gray4_round:2d}")

    print("\n" + "=" * 70)
    print("Analysis:")
    print("=" * 70)

    # 检查边界值
    print("\nBoundary values:")
    print(f"  0   -> {(0 * 15 + 127) // 255} (should be 0)")
    print(f"  255 -> {(255 * 15 + 127) // 255} (should be 15)")

    # 检查中间值
    print("\nMid-point values:")
    for i in range(16):
        # 理想的8位值（均匀分布）
        ideal_8bit = i * 255 // 15
        # 转换回4位
        back_to_4bit = (ideal_8bit * 15 + 127) // 255
        print(f"  4-bit {i:2d} -> ideal 8-bit {ideal_8bit:3d} -> back to 4-bit {back_to_4bit:2d}")

    print("\n" + "=" * 70)
    print("Checking official tool's approach:")
    print("=" * 70)

    # 官方工具可能的映射方式
    print("\nOfficial tool likely uses direct 8-bit grayscale from FreeType")
    print("Then converts to 4-bit when packing:")
    print("  - FreeType outputs 0-255 grayscale")
    print("  - Pack to 4bpp: value >> 4 (simple shift)")
    print("  OR")
    print("  - Pack to 4bpp: (value * 15 + 127) / 255 (proportional)")

    print("\nComparing methods:")
    print("Input | Shift>>4 | Proportional | Difference")
    print("-" * 50)

    for gray8 in [0, 32, 64, 96, 128, 160, 192, 224, 255]:
        shift = gray8 >> 4
        prop = (gray8 * 15 + 127) // 255
        diff = abs(shift - prop)
        print(f" {gray8:3d}  |    {shift:2d}    |      {prop:2d}      |     {diff:2d}")

if __name__ == "__main__":
    test_grayscale_mapping()
