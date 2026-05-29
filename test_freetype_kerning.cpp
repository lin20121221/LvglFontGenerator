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

    // 加载字体
    const char* fontPath = "Z:/LvglFontUtility/lv_font_conv/MyriadPro-Bold-Revised_20250304.ttf";
    if (FT_New_Face(library, fontPath, 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        return 1;
    }

    // 设置字体大小
    FT_Set_Char_Size(face, 0, 18 * 64, 300, 300);
    FT_Set_Pixel_Sizes(face, 0, 18);

    // 检查是否有kerning
    std::cout << "Font has kerning: " << (FT_HAS_KERNING(face) ? "YES" : "NO") << std::endl;

    if (FT_HAS_KERNING(face)) {
        // 测试A-V
        FT_UInt glyph_A = FT_Get_Char_Index(face, 'A');
        FT_UInt glyph_V = FT_Get_Char_Index(face, 'V');

        FT_Vector delta;
        FT_Get_Kerning(face, glyph_A, glyph_V, FT_KERNING_DEFAULT, &delta);

        std::cout << "\nA-V kerning:" << std::endl;
        std::cout << "  delta.x (1/64 pixels): " << delta.x << std::endl;
        std::cout << "  delta.x (pixels): " << (delta.x / 64.0) << std::endl;
        std::cout << "  Rounded: " << ((delta.x + 32) / 64) << std::endl;

        // 测试T-o
        FT_UInt glyph_T = FT_Get_Char_Index(face, 'T');
        FT_UInt glyph_o = FT_Get_Char_Index(face, 'o');
        FT_Get_Kerning(face, glyph_T, glyph_o, FT_KERNING_DEFAULT, &delta);

        std::cout << "\nT-o kerning:" << std::endl;
        std::cout << "  delta.x (1/64 pixels): " << delta.x << std::endl;
        std::cout << "  delta.x (pixels): " << (delta.x / 64.0) << std::endl;
        std::cout << "  Rounded: " << ((delta.x + 32) / 64) << std::endl;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}
