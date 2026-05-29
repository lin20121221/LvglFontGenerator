#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
完整的Kerning验证脚本
用于检查生成的字体文件中的kerning数据
"""

import re
import sys
import io

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def analyze_kerning(file_path):
    """分析字体文件中的kerning数据"""

    print(f"Analyzing: {file_path}")
    print("=" * 70)

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. 检查是否有kerning表
    has_kern = 'kern_left_class_mapping' in content
    print(f"\n1. Has kerning tables: {has_kern}")

    if not has_kern:
        print("   [WARNING] No kerning tables found!")
        print("   Make sure you checked 'Enable Kerning' when generating.")
        return False

    # 2. 提取字符列表
    pattern = r'/\* U\+([0-9A-F]+) \"(.*)\" \*/'
    chars = re.findall(pattern, content)
    print(f"\n2. Total characters: {len(chars)}")

    # 3. 提取映射表
    pattern = r'static const uint8_t kern_left_class_mapping\[\] =\s*\{([^}]+)\}'
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        print("   [ERROR] kern_left_class_mapping not found!")
        return False
    left_mapping = [int(x.strip()) for x in match.group(1).split(',') if x.strip() and x.strip().isdigit()]

    pattern = r'static const uint8_t kern_right_class_mapping\[\] =\s*\{([^}]+)\}'
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        print("   [ERROR] kern_right_class_mapping not found!")
        return False
    right_mapping = [int(x.strip()) for x in match.group(1).split(',') if x.strip() and x.strip().isdigit()]

    print(f"   Left mapping entries: {len(left_mapping)}")
    print(f"   Right mapping entries: {len(right_mapping)}")

    # 4. 提取类数量
    left_cnt_match = re.search(r'\.left_class_cnt\s*=\s*(\d+)', content)
    right_cnt_match = re.search(r'\.right_class_cnt\s*=\s*(\d+)', content)

    left_cnt = int(left_cnt_match.group(1))
    right_cnt = int(right_cnt_match.group(1))

    print(f"\n3. Class counts:")
    print(f"   left_class_cnt: {left_cnt}")
    print(f"   right_class_cnt: {right_cnt}")
    print(f"   Expected kern_values size: {left_cnt} x {right_cnt} = {left_cnt * right_cnt}")

    # 5. 提取kern_class_values
    pattern = r'static const int8_t kern_class_values\[\] =\s*\{([^}]+)\}'
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        print("   [ERROR] kern_class_values not found!")
        return False

    kern_values_str = match.group(1)
    kern_values = [int(x.strip()) for x in kern_values_str.split(',')
                   if x.strip() and x.strip().lstrip('-').isdigit()]

    print(f"\n4. Kern values array:")
    print(f"   Total values: {len(kern_values)}")

    non_zero = [v for v in kern_values if v != 0]
    print(f"   Non-zero values: {len(non_zero)}")

    if non_zero:
        print(f"   Min: {min(kern_values)}, Max: {max(kern_values)}")
    else:
        print("   [WARNING] All kern values are ZERO!")
        print("   This means no kerning data was extracted from the font.")

    # 6. 测试特定字符对
    print(f"\n5. Testing specific character pairs:")

    test_pairs = [
        ('A', 'V'),
        ('T', 'o'),
        ('L', 'T'),
        ('Y', 'o'),
        ('W', 'a'),
        ('V', 'A'),
    ]

    char_index = {}
    for i, (code, char) in enumerate(chars):
        char_index[char] = i

    for left_char, right_char in test_pairs:
        if left_char not in char_index or right_char not in char_index:
            print(f"   {left_char}-{right_char}: Character not found")
            continue

        left_idx = char_index[left_char]
        right_idx = char_index[right_char]

        # 注意：glyph_index = char_index + 1 (因为0是保留的)
        left_glyph = left_idx + 1
        right_glyph = right_idx + 1

        # 检测映射方式：
        # 如果 left_class_cnt 接近字符数量，说明每个字符一个类（我们的实现）
        # 否则是类优化（官方工具）
        if left_cnt >= len(chars):
            # 每个字符一个类的方式（我们的实现）
            # 映射表按 glyph_id 索引: mapping[glyph_id] = class_id
            if left_glyph < len(left_mapping):
                left_class = left_mapping[left_glyph]
            else:
                left_class = 0
            if right_glyph < len(right_mapping):
                right_class = right_mapping[right_glyph]
            else:
                right_class = 0
        else:
            # 类优化方式（官方工具）
            # 映射表按 glyph_id 索引，但多个 glyph 可能映射到同一个类
            if left_glyph < len(left_mapping):
                left_class = left_mapping[left_glyph]
            else:
                left_class = 0
            if right_glyph < len(right_mapping):
                right_class = right_mapping[right_glyph]
            else:
                right_class = 0

        # kern_class_values[left_class * right_cnt + right_class]
        kern_index = left_class * right_cnt + right_class

        if kern_index < len(kern_values):
            kern_value = kern_values[kern_index]
            status = "OK" if kern_value != 0 else "ZERO"
            print(f"   {left_char}-{right_char}: "
                  f"glyph({left_glyph},{right_glyph}) -> "
                  f"class({left_class},{right_class}) -> "
                  f"index[{kern_index}] = {kern_value:3d} [{status}]")
        else:
            print(f"   {left_char}-{right_char}: INDEX OUT OF BOUNDS!")

    print("\n" + "=" * 70)

    if len(non_zero) > 0:
        print("SUCCESS: Kerning data found and extracted!")
        return True
    else:
        print("FAILED: No kerning data in the generated file!")
        print("\nPossible reasons:")
        print("1. Font file doesn't contain kerning information")
        print("2. FreeType failed to extract kerning data")
        print("3. Check console output for error messages")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python verify_kerning.py <font_file.c>")
        print("\nExample:")
        print("  python verify_kerning.py My_Font_with_kerning.c")
        sys.exit(1)

    file_path = sys.argv[1]
    success = analyze_kerning(file_path)
    sys.exit(0 if success else 1)
