#ifndef LVGLEXPORTER_H
#define LVGLEXPORTER_H

#include <QString>
#include <QList>
#include <QImage>

class LvglExporter
{
public:
    struct GlyphInfo {
        uint32_t unicode;
        QImage bitmap;
        int advWidth;
        int boxW;
        int boxH;
        int ofsX;
        int ofsY;
    };

    LvglExporter();

    void setConfig(int lvglVersion, bool isExternal, int bpp = 8, bool enableKerning = false);
    void setFontName(const QString &name);
    void setFontSize(int size);
    void setFontPath(const QString &fontPath, int fontSize);  // 用于提取kerning数据
    void addGlyph(const GlyphInfo &glyph);

    bool exportCFile(const QString &filePath);
    bool exportBinFile(const QString &filePath);

private:
    QString generateCFileContent();

    QString generateBitmapArray();
    QString generateGlyphDescArray();
    QString generateCmapTables();
    QString generateKernTables();  // 新增：生成Kerning表
    QString generateFontStruct();

    QByteArray generateBinaryData();

    int m_lvglVersion;
    bool m_isExternal;
    int m_bpp;
    bool m_enableKerning;  // 新增：是否启用Kerning
    QString m_fontName;
    int m_fontSize;
    QString m_fontPath;    // 字体文件路径，用于提取kerning
    QList<GlyphInfo> m_glyphs;
};

#endif // LVGLEXPORTER_H
