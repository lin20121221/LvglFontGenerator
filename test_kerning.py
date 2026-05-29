#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试Kerning功能的脚本
检查生成的字体文件是否包含正确的kerning结构
"""

import re
import sys
import io

# 设置标准输出为UTF-8编码
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def check_kerning_structure(file_path):
    """检查文件中的kerning结构"""
    print(f"Checking file: {file_path}")

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 检查是否包含kerning相关的结构
    has_kern_left_mapping = 'kern_left_class_mapping' in content
    has_kern_right_mapping = 'kern_right_class_mapping' in content
    has_kern_values = 'kern_class_values' in content
    has_kern_classes_struct = 'lv_font_fmt_txt_kern_classes_t kern_classes' in content

    print(f"  kern_left_class_mapping: {'OK' if has_kern_left_mapping else 'NO'}")
    print(f"  kern_right_class_mapping: {'OK' if has_kern_right_mapping else 'NO'}")
    print(f"  kern_class_values: {'OK' if has_kern_values else 'NO'}")
    print(f"  kern_classes struct: {'OK' if has_kern_classes_struct else 'NO'}")

    # 检查font_dsc中的kern配置
    kern_dsc_match = re.search(r'\.kern_dsc\s*=\s*([^,]+)', content)
    kern_scale_match = re.search(r'\.kern_scale\s*=\s*(\d+)', content)
    kern_classes_match = re.search(r'\.kern_classes\s*=\s*(\d+)', content)

    if kern_dsc_match:
        kern_dsc_value = kern_dsc_match.group(1).strip()
        print(f"  .kern_dsc = {kern_dsc_value}")

    if kern_scale_match:
        kern_scale_value = kern_scale_match.group(1)
        print(f"  .kern_scale = {kern_scale_value}")

    if kern_classes_match:
        kern_classes_value = kern_classes_match.group(1)
        print(f"  .kern_classes = {kern_classes_value}")

    # 判断是否启用了kerning
    has_kerning = (has_kern_left_mapping and has_kern_right_mapping and
                   has_kern_values and has_kern_classes_struct and
                   kern_dsc_match and kern_dsc_match.group(1).strip() != 'NULL')

    print(f"\nResult: {'Kerning ENABLED' if has_kerning else 'Kerning DISABLED (or empty)'}")

    return has_kerning

def compare_files(file_with_kerning, file_without_kerning):
    """比较两个文件，验证kerning开关的效果"""
    print("\n" + "="*60)
    print("Comparison Test")
    print("="*60)

    print("\n1. Check file with Kerning enabled:")
    has_kerning_1 = check_kerning_structure(file_with_kerning)

    print("\n2. Check file with Kerning disabled:")
    has_kerning_2 = check_kerning_structure(file_without_kerning)

    print("\n" + "="*60)
    if has_kerning_1 and not has_kerning_2:
        print("PASS: Kerning switch works correctly")
        return True
    else:
        print("FAIL: Kerning switch may have issues")
        return False

if __name__ == "__main__":
    if len(sys.argv) == 2:
        # Single file check mode
        check_kerning_structure(sys.argv[1])
    elif len(sys.argv) == 3:
        # Comparison mode
        compare_files(sys.argv[1], sys.argv[2])
    else:
        print("Usage:")
        print("  Single file: python test_kerning.py <font_file.c>")
        print("  Compare mode: python test_kerning.py <with_kerning.c> <without_kerning.c>")
        sys.exit(1)
