#include "fontpreviewwidget.h"
#include "freetyperenderer.h"
#include <QPainter>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>

FontPreviewWidget::FontPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_fontSize(16)
    , m_enableKerning(false)
    , m_bpp(8)
    , m_showGrid(true)
    , m_zoomLevel(4)
    , m_offset(0, 0)
    , m_isEnglish(false)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);
}

void FontPreviewWidget::setPreviewText(const QString &text)
{
    m_previewText = parseUnicodeText(text);
    renderPreview();
    update();
}

void FontPreviewWidget::setFont(const QFont &font)
{
    m_font = font;
    m_fontSize = font.pointSize();
    renderPreview();
    update();
}

void FontPreviewWidget::setFontPath(const QString &fontPath, int fontSize)
{
    m_fontPath = fontPath;
    m_fontSize = fontSize;
    renderPreview();
    update();
}

void FontPreviewWidget::setEnableKerning(bool enable)
{
    m_enableKerning = enable;
    renderPreview();
    update();
}

void FontPreviewWidget::setAntialiasing(int bpp)
{
    m_bpp = bpp;
    renderPreview();
    update();
}

void FontPreviewWidget::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

void FontPreviewWidget::setZoomLevel(int level)
{
    m_zoomLevel = qBound(1, level, 32);
    emit zoomChanged(m_zoomLevel);
    update();
}

void FontPreviewWidget::setLanguage(bool isEnglish)
{
    m_isEnglish = isEnglish;
    update();
}

void FontPreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (m_previewImage.isNull()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter,
                        m_isEnglish ? "Enter preview text" : "请输入预览文本");
        return;
    }

    // 绘制预览图像（使用最近邻插值保持像素清晰）
    painter.save();
    painter.translate(m_offset);

    // 对于像素字体预览，使用最近邻插值（FastTransformation）
    // 这样可以保持像素边界清晰，不会模糊
    QImage scaledImage = m_previewImage.scaled(
        m_previewImage.width() * m_zoomLevel,
        m_previewImage.height() * m_zoomLevel,
        Qt::IgnoreAspectRatio,  // 使用精确的整数倍缩放
        Qt::FastTransformation  // 最近邻插值，保持像素清晰
    );

    painter.drawImage(0, 0, scaledImage);

    // 绘制栅格
    if (m_showGrid && m_zoomLevel >= 4) {
        painter.setPen(QPen(QColor(200, 200, 200), 1));

        // 垂直线
        for (int x = 0; x <= m_previewImage.width(); x++) {
            int px = x * m_zoomLevel;
            painter.drawLine(px, 0, px, scaledImage.height());
        }

        // 水平线
        for (int y = 0; y <= m_previewImage.height(); y++) {
            int py = y * m_zoomLevel;
            painter.drawLine(0, py, scaledImage.width(), py);
        }
    }

    painter.restore();

    // 绘制缩放信息
    painter.setPen(Qt::black);
    if (m_isEnglish) {
        painter.drawText(10, 20, QString("Zoom: %1x | BPP: %2 | Grid: %3")
                         .arg(m_zoomLevel)
                         .arg(m_bpp)
                         .arg(m_showGrid ? "On" : "Off"));
    } else {
        painter.drawText(10, 20, QString("缩放: %1x | BPP: %2 | 栅格: %3")
                         .arg(m_zoomLevel)
                         .arg(m_bpp)
                         .arg(m_showGrid ? "开" : "关"));
    }
}

void FontPreviewWidget::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y() / 120;
    int newZoom = m_zoomLevel + delta;
    setZoomLevel(newZoom);
}

void FontPreviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
    }
}

void FontPreviewWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        update();
    }
}

void FontPreviewWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // 右键直接居中显示
    centerPreview();
}

void FontPreviewWidget::centerPreview()
{
    if (m_previewImage.isNull()) {
        return;
    }

    int scaledWidth = m_previewImage.width() * m_zoomLevel;
    int scaledHeight = m_previewImage.height() * m_zoomLevel;

    m_offset.setX((width() - scaledWidth) / 2);
    m_offset.setY((height() - scaledHeight) / 2);

    update();
}

void FontPreviewWidget::resetPosition()
{
    m_offset = QPoint(0, 0);
    update();
}

void FontPreviewWidget::renderPreview()
{
    if (m_previewText.isEmpty()) {
        m_previewImage = QImage();
        return;
    }

    // 如果没有字体路径，不显示预览
    if (m_fontPath.isEmpty()) {
        m_previewImage = QImage();
        return;
    }

    // 始终使用 FreeType 渲染（不进行量化，显示原始渲染质量）
    renderPreviewWithFreeType();
}

void FontPreviewWidget::renderPreviewWithQt()
{
    // 使用实际字体大小渲染每个字符
    QFontMetrics metrics(m_font);

    // 计算总宽度和基线信息
    int totalWidth = 0;
    int maxAscent = metrics.ascent();
    int maxDescent = metrics.descent();

    for (const QChar &ch : m_previewText) {
        totalWidth += metrics.horizontalAdvance(ch) + 2; // 字符间距
    }

    totalWidth += 20;
    int maxHeight = maxAscent + maxDescent + 20;

    // 创建高分辨率图像用于渲染
    QImage tempImage(totalWidth, maxHeight, QImage::Format_ARGB32);
    tempImage.fill(Qt::white);

    QPainter painter(&tempImage);
    painter.setRenderHint(QPainter::Antialiasing, m_bpp > 1);
    painter.setRenderHint(QPainter::TextAntialiasing, m_bpp > 1);
    painter.setFont(m_font);
    painter.setPen(Qt::black);

    // 计算基线位置（与 kerning 渲染保持一致）
    int baseline = 10 + maxAscent;

    // 逐字符渲染，使用统一的基线
    int xPos = 10;
    for (const QChar &ch : m_previewText) {
        // 使用基线对齐绘制
        painter.drawText(xPos, baseline, QString(ch));
        xPos += metrics.horizontalAdvance(ch) + 2;
    }

    painter.end();

    // 不应用量化处理 - 直接显示原始渲染质量
    m_previewImage = tempImage.convertToFormat(QImage::Format_Grayscale8);
}

void FontPreviewWidget::renderPreviewWithFreeType()
{
    // 检查字体路径
    if (m_fontPath.isEmpty()) {
        m_previewImage = QImage();
        return;
    }

    FreeTypeRenderer renderer;
    if (!renderer.loadFont(m_fontPath, m_fontSize)) {
        qWarning() << "Failed to load font:" << renderer.lastError();
        m_previewImage = QImage();
        return;
    }

    // 根据 BPP 设置抗锯齿：BPP 1 不使用抗锯齿，其他使用
    renderer.setAntialiasing(m_bpp > 1);

    // 计算总宽度和最大高度
    int totalWidth = 10;
    int maxHeight = 0;
    int maxAscent = 0;
    int maxDescent = 0;

    struct GlyphInfo {
        FreeTypeRenderer::GlyphData data;
        bool valid;
    };
    QList<GlyphInfo> glyphs;

    for (int i = 0; i < m_previewText.length(); i++) {
        uint32_t charCode = m_previewText[i].unicode();

        GlyphInfo glyphInfo;
        glyphInfo.valid = renderer.renderGlyph(charCode, glyphInfo.data);

        if (!glyphInfo.valid) {
            // 字符不存在，使用空格的宽度
            qDebug() << "Character not in font:" << m_previewText[i] << QString("(U+%1)").arg(charCode, 4, 16, QChar('0'));

            // 尝试获取空格的宽度作为默认宽度
            FreeTypeRenderer::GlyphData spaceGlyph;
            if (renderer.renderGlyph(' ', spaceGlyph)) {
                glyphInfo.data.advance_x = spaceGlyph.advance_x;
            } else {
                // 如果连空格都没有，使用字体大小的一半作为默认宽度
                glyphInfo.data.advance_x = m_fontSize / 2.0;
            }
            glyphInfo.data.width = 0;
            glyphInfo.data.height = 0;
            glyphInfo.data.bitmap_left = 0;
            glyphInfo.data.bitmap_top = 0;
        }

        glyphs.append(glyphInfo);

        totalWidth += glyphInfo.data.advance_x;

        // 应用 kerning（如果启用）
        if (m_enableKerning && i < m_previewText.length() - 1) {
            uint32_t nextCharCode = m_previewText[i + 1].unicode();
            int kernValue = renderer.getKerning(charCode, nextCharCode);
            // kernValue是以1/16像素为单位，转换为像素
            totalWidth += kernValue / 16.0;
        }

        if (glyphInfo.valid) {
            maxAscent = qMax(maxAscent, glyphInfo.data.bitmap_top);
            maxDescent = qMax(maxDescent, glyphInfo.data.height - glyphInfo.data.bitmap_top);
        }
    }

    // 如果所有字符都无效，使用默认高度
    if (maxAscent == 0 && maxDescent == 0) {
        maxAscent = m_fontSize * 0.8;
        maxDescent = m_fontSize * 0.2;
    }

    maxHeight = maxAscent + maxDescent;
    totalWidth += 10;
    maxHeight += 20;

    // 确保尺寸有效
    if (totalWidth <= 0 || maxHeight <= 0) {
        qWarning() << "Invalid image size:" << totalWidth << "x" << maxHeight;
        m_previewImage = QImage();
        return;
    }

    // 创建图像（使用ARGB32格式）
    QImage tempImage(totalWidth, maxHeight, QImage::Format_ARGB32);
    tempImage.fill(Qt::white); // 白色背景

    // 渲染字形
    double xPos = 10;
    int baseline = 10 + maxAscent;

    for (int i = 0; i < glyphs.size(); i++) {
        const GlyphInfo &glyphInfo = glyphs[i];

        // 只渲染有效的字形
        if (glyphInfo.valid && glyphInfo.data.width > 0 && glyphInfo.data.height > 0) {
            const FreeTypeRenderer::GlyphData &glyph = glyphInfo.data;

            // FreeType 坐标系统：
            // - bitmap_left: 字形左边缘相对于原点的水平偏移
            // - bitmap_top: 字形顶部相对于基线的垂直距离（向上为正）
            //
            // 屏幕坐标系统（Y轴向下）：
            // - glyphX = xPos + bitmap_left
            // - glyphY = baseline - bitmap_top（因为 bitmap_top 是向上的距离）
            int glyphX = qRound(xPos) + glyph.bitmap_left;
            int glyphY = baseline - glyph.bitmap_top;

            // 绘制字形位图
            for (int y = 0; y < glyph.height; y++) {
                for (int x = 0; x < glyph.width; x++) {
                    int imgX = glyphX + x;
                    int imgY = glyphY + y;

                    if (imgX >= 0 && imgX < tempImage.width() &&
                        imgY >= 0 && imgY < tempImage.height()) {

                        unsigned char pixelValue = glyph.pixels[y][x];
                        // FreeType: 0=透明, 255=不透明
                        // 只绘制非透明像素，避免覆盖已有内容
                        if (pixelValue > 0) {
                            // 创建灰度颜色：pixelValue越大，颜色越深（越黑）
                            int grayValue = 255 - pixelValue;

                            // 获取当前像素值，进行alpha混合
                            QRgb existingColor = tempImage.pixel(imgX, imgY);
                            int existingGray = qRed(existingColor); // RGB相同，取任意通道

                            // Alpha混合：新颜色根据pixelValue的不透明度与现有颜色混合
                            // alpha = pixelValue / 255.0
                            int blendedGray = (grayValue * pixelValue + existingGray * (255 - pixelValue)) / 255;
                            QRgb color = qRgb(blendedGray, blendedGray, blendedGray);
                            tempImage.setPixel(imgX, imgY, color);
                        }
                    }
                }
            }
        }

        xPos += glyphInfo.data.advance_x;

        // 应用 kerning（如果启用）
        if (m_enableKerning && i < glyphs.size() - 1) {
            uint32_t charCode = m_previewText[i].unicode();
            uint32_t nextCharCode = m_previewText[i + 1].unicode();
            int kernValue = renderer.getKerning(charCode, nextCharCode);
            xPos += kernValue / 16.0;
        }
    }

    // 不应用量化处理 - 直接显示 FreeType 的原始渲染质量
    // BPP 量化是在导出时进行的，预览应该显示高质量的渲染结果
    m_previewImage = tempImage.convertToFormat(QImage::Format_Grayscale8);
}

QImage FontPreviewWidget::applyAntialiasing(const QImage &source)
{
    QImage result = source.convertToFormat(QImage::Format_Grayscale8);

    if (m_bpp == 1) {
        // 1位：纯黑白（二值化）
        for (int y = 0; y < result.height(); y++) {
            uchar *line = result.scanLine(y);
            for (int x = 0; x < result.width(); x++) {
                line[x] = (line[x] > 127) ? 255 : 0;
            }
        }
    } else if (m_bpp == 2) {
        // 2位：4级灰度 (0, 85, 170, 255)
        for (int y = 0; y < result.height(); y++) {
            uchar *line = result.scanLine(y);
            for (int x = 0; x < result.width(); x++) {
                // 将 0-255 映射到 0-3，然后映射回 0-255
                int level = (line[x] * 3 + 128) / 255;  // 四舍五入
                line[x] = (level * 255) / 3;  // 精确映射到 0, 85, 170, 255
            }
        }
    } else if (m_bpp == 4) {
        // 4位：16级灰度 (0, 17, 34, ..., 255)
        for (int y = 0; y < result.height(); y++) {
            uchar *line = result.scanLine(y);
            for (int x = 0; x < result.width(); x++) {
                // 将 0-255 映射到 0-15，然后映射回 0-255
                int level = (line[x] * 15 + 128) / 255;  // 四舍五入
                line[x] = (level * 255) / 15;  // 精确映射到 0, 17, 34, ..., 255
            }
        }
    }
    // 8位保持原样（256级灰度）

    return result;
}

QString FontPreviewWidget::parseUnicodeText(const QString &text)
{
    QString result;
    int i = 0;

    while (i < text.length()) {
        // 检查是否是 \u 格式 (如 一)
        if (i + 5 < text.length() && text[i] == '\\' && text[i + 1] == 'u') {
            QString hexStr = text.mid(i + 2, 4);
            bool ok;
            uint codePoint = hexStr.toUInt(&ok, 16);
            if (ok) {
                result.append(QChar(codePoint));
                i += 6;
                continue;
            }
        }

        // 检查是否是 \U 格式 (如 \U0001F600 - 8位十六进制)
        if (i + 9 < text.length() && text[i] == '\\' && text[i + 1] == 'U') {
            QString hexStr = text.mid(i + 2, 8);
            bool ok;
            uint codePoint = hexStr.toUInt(&ok, 16);
            if (ok && codePoint <= 0x10FFFF) {
                result.append(QString::fromUcs4(&codePoint, 1));
                i += 10;
                continue;
            }
        }

        // 检查是否是 U+ 格式 (如 U+4E00)
        if (i + 2 < text.length() && text[i] == 'U' && text[i + 1] == '+') {
            int hexStart = i + 2;
            int hexEnd = hexStart;
            // 查找十六进制数字的结束位置（最多8位）
            while (hexEnd < text.length() && hexEnd < hexStart + 8) {
                QChar ch = text[hexEnd];
                if ((ch >= '0' && ch <= '9') ||
                    (ch >= 'A' && ch <= 'F') ||
                    (ch >= 'a' && ch <= 'f')) {
                    hexEnd++;
                } else {
                    break;
                }
            }

            if (hexEnd > hexStart) {
                QString hexStr = text.mid(hexStart, hexEnd - hexStart);
                bool ok;
                uint codePoint = hexStr.toUInt(&ok, 16);
                if (ok && codePoint <= 0x10FFFF) {
                    if (codePoint <= 0xFFFF) {
                        result.append(QChar(codePoint));
                    } else {
                        result.append(QString::fromUcs4(&codePoint, 1));
                    }
                    i = hexEnd;
                    continue;
                }
            }
        }

        // 检查是否是 0x 格式 (如 0x4E00)
        if (i + 2 < text.length() && text[i] == '0' && (text[i + 1] == 'x' || text[i + 1] == 'X')) {
            int hexStart = i + 2;
            int hexEnd = hexStart;
            // 查找十六进制数字的结束位置（最多8位）
            while (hexEnd < text.length() && hexEnd < hexStart + 8) {
                QChar ch = text[hexEnd];
                if ((ch >= '0' && ch <= '9') ||
                    (ch >= 'A' && ch <= 'F') ||
                    (ch >= 'a' && ch <= 'f')) {
                    hexEnd++;
                } else {
                    break;
                }
            }

            if (hexEnd > hexStart) {
                QString hexStr = text.mid(hexStart, hexEnd - hexStart);
                bool ok;
                uint codePoint = hexStr.toUInt(&ok, 16);
                if (ok && codePoint <= 0x10FFFF) {
                    if (codePoint <= 0xFFFF) {
                        result.append(QChar(codePoint));
                    } else {
                        result.append(QString::fromUcs4(&codePoint, 1));
                    }
                    i = hexEnd;
                    continue;
                }
            }
        }

        // 普通字符，直接添加
        result.append(text[i]);
        i++;
    }

    return result;
}
