#ifndef GLYPHDETAILDIALOG_H
#define GLYPHDETAILDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QPoint>

class QLabel;
class QPushButton;
class QCheckBox;

class GlyphDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GlyphDetailWidget(QWidget *parent = nullptr);

    void setGlyphImage(const QPixmap &pixmap);
    void setShowGrid(bool show);
    void setZoomLevel(int level);
    void fitToView();

    int zoomLevel() const { return m_zoomLevel; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPixmap m_glyphImage;
    bool m_showGrid;
    int m_zoomLevel;
    QPoint m_offset;
    QPoint m_lastMousePos;
    bool m_isDragging;
};

class GlyphDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GlyphDetailDialog(QWidget *parent = nullptr);

    void setGlyphInfo(QChar character, uint32_t codePoint, const QPixmap &pixmap);

private slots:
    void onZoomIn();
    void onZoomOut();
    void onFit();
    void onToggleGrid(bool checked);

private:
    GlyphDetailWidget *m_glyphWidget;
    QLabel *m_infoLabel;
    QPushButton *m_zoomInBtn;
    QPushButton *m_zoomOutBtn;
    QPushButton *m_fitBtn;
    QCheckBox *m_gridCheckBox;

    QChar m_character;
    uint32_t m_codePoint;
};

#endif // GLYPHDETAILDIALOG_H
