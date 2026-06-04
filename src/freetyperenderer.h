#ifndef FREETYPERENDERER_H
#define FREETYPERENDERER_H

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMap>
#include <ft2build.h>
#include FT_FREETYPE_H

// 前向声明 HarfBuzz 类型
struct hb_font_t;
struct hb_buffer_t;

class FreeTypeRenderer
{
public:
    struct GlyphData {
        int width;
        int height;
        int bitmap_left;
        int bitmap_top;
        double advance_x;
        double advance_y;
        QVector<QVector<unsigned char>> pixels;  // 位图数据 [行][列]
    };

    FreeTypeRenderer();
    ~FreeTypeRenderer();

    bool loadFont(const QString &fontPath, int fontSize);
    bool renderGlyph(uint32_t charCode, GlyphData &outGlyph);

    // 设置渲染模式：是否使用抗锯齿
    void setAntialiasing(bool enabled) { m_antialiasing = enabled; }

    // 获取两个字符之间的kerning值（以1/16像素为单位）
    // 优先使用 HarfBuzz，失败时回退到 FreeType
    int getKerning(uint32_t leftChar, uint32_t rightChar);

    // 检查字体是否包含kerning信息
    bool hasKerning() const;

    // 获取字体的 underline 信息（以字体单位为单位）
    int getUnderlinePosition() const;
    int getUnderlineThickness() const;
    int getUnitsPerEM() const;

    // 获取字体的高度信息
    int getAscender() const;
    int getDescender() const;
    int getHeight() const;

    // 获取缩放后的度量值（已转换为像素，从 size->metrics）
    int getScaledAscender() const;
    int getScaledDescender() const;
    int getScaledHeight() const;

    void cleanup();

    QString lastError() const { return m_lastError; }

private:
    // 使用 HarfBuzz 获取 kerning（支持 GPOS）
    int getKerningHarfBuzz(uint32_t leftChar, uint32_t rightChar);

    // 使用 FreeType 获取 kerning（仅支持 kern 表）
    int getKerningFreeType(uint32_t leftChar, uint32_t rightChar);

    FT_Library m_library;
    FT_Face m_face;
    hb_font_t *m_hb_font;  // HarfBuzz font
    QByteArray m_fontData;  // 保持字体数据在内存中
    QString m_lastError;
    bool m_initialized;
    bool m_antialiasing;    // 是否使用抗锯齿渲染

    // Kerning 缓存
    QMap<QPair<uint32_t, uint32_t>, int> m_kerningCache;
};

#endif // FREETYPERENDERER_H
