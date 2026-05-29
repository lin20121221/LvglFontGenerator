#ifndef FREETYPERENDERER_H
#define FREETYPERENDERER_H

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMap>
#include <ft2build.h>
#include FT_FREETYPE_H

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

    // 获取两个字符之间的kerning值（以1/64像素为单位）
    int getKerning(uint32_t leftChar, uint32_t rightChar);

    // 检查字体是否包含kerning信息
    bool hasKerning() const;

    void cleanup();

    QString lastError() const { return m_lastError; }

private:
    FT_Library m_library;
    FT_Face m_face;
    QByteArray m_fontData;  // 保持字体数据在内存中
    QString m_lastError;
    bool m_initialized;
};

#endif // FREETYPERENDERER_H
