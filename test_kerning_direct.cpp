#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H

int main() {
    FT_Library library;
    FT_Face face;

    // 初始化FreeType
    if (FT_Init_FreeType(&library)) {
        std::cerr << "Failed to init FreeType" << std::endl;
        return 1;
    }

    // 加载字体（请根据实际路径修改）
    const char* fontPath = "Z:/LvglFontUtility/lv_font_conv/MyriadPro-Bold-Revised_20250304.ttf";
    if (FT_New_Face(library, fontPath, 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        FT_Done_FreeType(library);
        return 1;
    }

    // 设置字体大小（与官方工具一致：18px）
    FT_Set_Char_Size(face, 0, 18 * 64, 300, 300);
    FT_Set_Pixel_Sizes(face, 0, 18);

    // 检查是否有kerning
    std::cout << "Font has kerning: " << (FT_HAS_KERNING(face) ? "YES" : "NO") << std::endl;

    if (FT_HAS_KERNING(face)) {
        // 测试A-V
        FT_UInt glyph_A = FT_Get_Char_Index(face, 'A');
        FT_UInt glyph_V = FT_Get_Char_Index(face, 'V');

        std::cout << "\nGlyph indices:" << std::endl;
        std::cout << "  A: " << glyph_A << std::endl;
        std::cout << "  V: " << glyph_V << std::endl;

        FT_Vector delta;
        FT_Error error = FT_Get_Kerning(face, glyph_A, glyph_V, FT_KERNING_DEFAULT, &delta);

        if (error) {
            std::cout << "\nFT_Get_Kerning failed with error: " << error << std::endl;
        } else {
            std::cout << "\nA-V kerning:" << std::endl;
            std::cout << "  delta.x (1/64 pixels): " << delta.x << std::endl;
            std::cout << "  delta.y (1/64 pixels): " << delta.y << std::endl;
            std::cout << "  delta.x (pixels): " << (delta.x / 64.0) << std::endl;
            std::cout << "  Converted to LVGL (1/16 pixels): " << ((delta.x + 2) / 4) << std::endl;
        }

        // 测试V-A（反向）
        FT_Get_Kerning(face, glyph_V, glyph_A, FT_KERNING_DEFAULT, &delta);
        std::cout << "\nV-A kerning:" << std::endl;
        std::cout << "  delta.x (1/64 pixels): " << delta.x << std::endl;
        std::cout << "  Converted to LVGL (1/16 pixels): " << ((delta.x + 2) / 4) << std::endl;

        // 测试其他字符对
        std::cout << "\n\nOther character pairs:" << std::endl;

        struct TestPair {
            char left;
            char right;
        };

        TestPair pairs[] = {
            {'T', 'o'},
            {'L', 'T'},
            {'Y', 'o'},
            {'W', 'a'}
        };

        for (const auto& pair : pairs) {
            FT_UInt left_glyph = FT_Get_Char_Index(face, pair.left);
            FT_UInt right_glyph = FT_Get_Char_Index(face, pair.right);

            FT_Get_Kerning(face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &delta);

            std::cout << pair.left << "-" << pair.right << ": "
                      << "delta.x=" << delta.x
                      << " (1/64px), LVGL=" << ((delta.x + 2) / 4)
                      << " (1/16px)" << std::endl;
        }
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}
