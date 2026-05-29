#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>

int main() {
    FT_Library library;
    FT_Face face;

    // 初始化 FreeType
    if (FT_Init_FreeType(&library)) {
        printf("Failed to initialize FreeType\n");
        return 1;
    }

    // 加载字体
    const char* fontPath = "Z:/LvglFontUtility/LvglFontGenerator/MyriadPro-Bold-Revised_20250304.ttf";
    if (FT_New_Face(library, fontPath, 0, &face)) {
        printf("Failed to load font\n");
        FT_Done_FreeType(library);
        return 1;
    }

    printf("Font loaded: %s\n", face->family_name);
    printf("FT_HAS_KERNING: %d\n", FT_HAS_KERNING(face));
    printf("face->face_flags: 0x%lx\n", face->face_flags);
    printf("FT_FACE_FLAG_KERNING: 0x%lx\n", (long)FT_FACE_FLAG_KERNING);

    // 设置字体大小
    FT_Set_Pixel_Sizes(face, 0, 48);

    // 测试几个常见的 kerning 对
    const char* pairs[][2] = {
        {"A", "V"},
        {"T", "o"},
        {"W", "a"},
        {"V", "A"},
        {"L", "T"}
    };

    printf("\nTesting kerning pairs:\n");
    for (int i = 0; i < 5; i++) {
        FT_UInt left = FT_Get_Char_Index(face, pairs[i][0][0]);
        FT_UInt right = FT_Get_Char_Index(face, pairs[i][1][0]);

        FT_Vector delta;
        FT_Get_Kerning(face, left, right, FT_KERNING_DEFAULT, &delta);

        printf("  %s-%s: glyph(%u,%u) delta.x=%ld (1/64px) = %ld (px/64)\n",
               pairs[i][0], pairs[i][1], left, right, delta.x, delta.x);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}
