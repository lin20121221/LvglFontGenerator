#include "fontgenerator.h"
#include "lvglexporter.h"
#include "freetyperenderer.h"
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QDebug>

FontGenerator::FontGenerator(QObject *parent)
    : QObject(parent)
{
}

FontGenerator::~FontGenerator()
{
}

bool FontGenerator::generate(const Config &config)
{
    m_glyphs.clear();
    m_lastError.clear();

    if (!loadFont(config.fontPath, config.fontSize)) {
        return false;
    }

    QString chars = config.characters;
    removeDuplicateCharacters(chars);

    // 使用 FreeType 渲染字形
    if (!renderGlyphs(chars, config.fontPath, config.fontSize)) {
        return false;
    }

    if (!exportToC(config, config.fontPath)) {
        return false;
    }

    if (config.isExternal) {
        if (!exportToBin(config)) {
            return false;
        }
    }

    return true;
}

bool FontGenerator::loadFont(const QString &fontPath, int fontSize)
{
    // 不再使用 Qt 的字体加载，直接保存路径供 FreeType 使用
    // 但仍然加载到 Qt 用于预览
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId == -1) {
        m_lastError = "无法加载字体文件";
        return false;
    }

    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    if (fontFamilies.isEmpty()) {
        m_lastError = "字体文件中没有可用的字体";
        return false;
    }

    m_font = QFont(fontFamilies.first(), fontSize);
    return true;
}

bool FontGenerator::renderGlyphs(const QString &characters, const QString &fontPath, int fontSize)
{
    // 使用 FreeType 渲染器
    FreeTypeRenderer renderer;

    if (!renderer.loadFont(fontPath, fontSize)) {
        m_lastError = "FreeType 加载字体失败: " + renderer.lastError();
        return false;
    }

    for (const QChar &ch : characters) {
        if (ch == '\n' || ch == '\r') {
            continue;
        }

        GlyphData glyph;
        glyph.character = ch;

        // 使用 FreeType 渲染字形
        FreeTypeRenderer::GlyphData ftGlyph;
        if (!renderer.renderGlyph(ch.unicode(), ftGlyph)) {
            qWarning() << "Failed to render glyph for character" << ch << ":" << renderer.lastError();
            continue;
        }

        // 调试输出第一个字符的信息
        if (m_glyphs.isEmpty()) {
            qDebug() << "\nFirst character '" << ch << "' (FreeType):";
            qDebug() << "  width:" << ftGlyph.width;
            qDebug() << "  height:" << ftGlyph.height;
            qDebug() << "  bitmap_left:" << ftGlyph.bitmap_left;
            qDebug() << "  bitmap_top:" << ftGlyph.bitmap_top;
            qDebug() << "  advance_x:" << ftGlyph.advance_x;
            qDebug() << "  Calculated ofs_y (bitmap_top - height):" << (ftGlyph.bitmap_top - ftGlyph.height);
        }

        // 使用 FreeType 的数据（完全参照官方工具）
        glyph.width = ftGlyph.width;
        glyph.height = ftGlyph.height;

        // LVGL使用1/16像素作为adv_w的单位，所以先乘以16再四舍五入
        // 这样可以保留小数精度，避免截断误差
        glyph.advanceWidth = qRound(ftGlyph.advance_x * 16);

        // 参照官方工具 collect_font_data.js 第 111-124 行
        glyph.bearingX = ftGlyph.bitmap_left;
        glyph.bearingY = ftGlyph.bitmap_top - ftGlyph.height;  // 关键公式！

        // 转换位图数据到 QImage
        if (glyph.width > 0 && glyph.height > 0) {
            QImage image(glyph.width, glyph.height, QImage::Format_Grayscale8);

            for (int y = 0; y < glyph.height; y++) {
                for (int x = 0; x < glyph.width; x++) {
                    unsigned char value = ftGlyph.pixels[y][x];
                    image.setPixel(x, y, qRgb(value, value, value));
                }
            }

            glyph.bitmap = image;
        } else {
            // 对于空字形（如空格），创建空图像，不生成位图数据
            glyph.bitmap = QImage();
        }

        m_glyphs.append(glyph);

        emit progressChanged(m_glyphs.size() * 50 / characters.size());
    }

    return true;
}

bool FontGenerator::exportToC(const Config &config, const QString &fontPath)
{
    LvglExporter exporter;
    exporter.setConfig(config.lvglVersion, config.isExternal, config.bpp, config.enableKerning);
    exporter.setFontName(config.outputName);
    exporter.setFontSize(m_font.pointSize());

    // 如果启用了kerning，设置字体路径和字符列表
    if (config.enableKerning) {
        exporter.setFontPath(fontPath, config.fontSize);
    }

    for (const GlyphData &glyph : m_glyphs) {
        LvglExporter::GlyphInfo info;
        info.unicode = glyph.character.unicode();
        info.bitmap = glyph.bitmap;
        info.advWidth = glyph.advanceWidth;
        info.boxW = glyph.width;
        info.boxH = glyph.height;
        info.ofsX = glyph.bearingX;
        info.ofsY = glyph.bearingY;

        exporter.addGlyph(info);
    }

    QString cFilePath = config.outputDir + "/" + config.outputName + ".c";
    if (!exporter.exportCFile(cFilePath)) {
        m_lastError = "无法写入C文件";
        return false;
    }

    emit progressChanged(75);
    return true;
}

bool FontGenerator::exportToBin(const Config &config)
{
    LvglExporter exporter;
    exporter.setConfig(config.lvglVersion, config.isExternal, config.bpp, config.enableKerning);
    exporter.setFontName(config.outputName);
    exporter.setFontSize(m_font.pointSize());

    for (const GlyphData &glyph : m_glyphs) {
        LvglExporter::GlyphInfo info;
        info.unicode = glyph.character.unicode();
        info.bitmap = glyph.bitmap;
        info.advWidth = glyph.advanceWidth;
        info.boxW = glyph.width;
        info.boxH = glyph.height;
        info.ofsX = glyph.bearingX;
        info.ofsY = glyph.bearingY;

        exporter.addGlyph(info);
    }

    QString binFilePath = config.outputDir + "/" + config.outputName + ".bin";
    if (!exporter.exportBinFile(binFilePath)) {
        m_lastError = "无法写入BIN文件";
        return false;
    }

    emit progressChanged(90);
    return true;
}

void FontGenerator::removeDuplicateCharacters(QString &characters)
{
    QSet<QChar> uniqueChars;
    QString result;

    for (const QChar &ch : characters) {
        if (!uniqueChars.contains(ch)) {
            uniqueChars.insert(ch);
            result.append(ch);
        }
    }

    characters = result;
}
