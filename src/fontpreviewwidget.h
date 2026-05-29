#ifndef FONTPREVIEWWIDGET_H
#define FONTPREVIEWWIDGET_H

#include <QWidget>
#include <QImage>
#include <QFont>

class FontPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FontPreviewWidget(QWidget *parent = nullptr);

    void setPreviewText(const QString &text);
    void setFont(const QFont &font);
    void setFontPath(const QString &fontPath, int fontSize);  // 新增：设置字体路径用于FreeType渲染
    void setAntialiasing(int bpp);
    void setShowGrid(bool show);
    void setZoomLevel(int level);
    void setLanguage(bool isEnglish);
    void setEnableKerning(bool enable);  // 新增：设置是否启用kerning

    int zoomLevel() const { return m_zoomLevel; }
    bool showGrid() const { return m_showGrid; }

signals:
    void zoomChanged(int level);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void renderPreview();
    void renderPreviewWithQt();         // Qt渲染（已废弃，保留以防需要）
    void renderPreviewWithFreeType();   // 使用FreeType渲染（支持kerning开关）
    QImage applyAntialiasing(const QImage &source);
    void centerPreview();
    void resetPosition();

    QString m_previewText;
    QFont m_font;
    QString m_fontPath;      // 新增：字体文件路径
    int m_fontSize;          // 新增：字体大小
    bool m_enableKerning;    // 新增：是否启用kerning
    int m_bpp;
    bool m_showGrid;
    int m_zoomLevel;
    QImage m_previewImage;
    QPoint m_lastMousePos;
    QPoint m_offset;
    bool m_isEnglish;
};

#endif // FONTPREVIEWWIDGET_H
