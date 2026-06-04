#include "glyphdetaildialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>

// ============================================================================
// GlyphDetailWidget
// ============================================================================

GlyphDetailWidget::GlyphDetailWidget(QWidget *parent)
    : QWidget(parent)
    , m_showGrid(true)
    , m_zoomLevel(8)
    , m_offset(0, 0)
    , m_isDragging(false)
{
    setMinimumSize(400, 400);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void GlyphDetailWidget::setGlyphImage(const QPixmap &pixmap)
{
    m_glyphImage = pixmap;
    m_offset = QPoint(0, 0);
    update();
}

void GlyphDetailWidget::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

void GlyphDetailWidget::setZoomLevel(int level)
{
    m_zoomLevel = qBound(1, level, 32);
    update();
}

void GlyphDetailWidget::fitToView()
{
    if (m_glyphImage.isNull()) {
        return;
    }

    int imageWidth = m_glyphImage.width();
    int imageHeight = m_glyphImage.height();

    if (imageWidth == 0 || imageHeight == 0) {
        return;
    }

    // 计算合适的缩放级别
    int maxZoomW = (width() - 40) / imageWidth;
    int maxZoomH = (height() - 40) / imageHeight;
    m_zoomLevel = qMax(1, qMin(maxZoomW, maxZoomH));

    // 居中显示
    int scaledWidth = imageWidth * m_zoomLevel;
    int scaledHeight = imageHeight * m_zoomLevel;
    m_offset.setX((width() - scaledWidth) / 2);
    m_offset.setY((height() - scaledHeight) / 2);

    update();
}

void GlyphDetailWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(240, 240, 240));

    if (m_glyphImage.isNull()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No glyph loaded"));
        return;
    }

    int imageWidth = m_glyphImage.width();
    int imageHeight = m_glyphImage.height();
    int pixelSize = m_zoomLevel;

    // 绘制白色背景
    QRect glyphRect(m_offset.x(), m_offset.y(),
                    imageWidth * pixelSize, imageHeight * pixelSize);
    painter.fillRect(glyphRect, Qt::white);

    // 绘制字形像素
    QImage image = m_glyphImage.toImage();
    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0) {
                QRect pixelRect(m_offset.x() + x * pixelSize,
                               m_offset.y() + y * pixelSize,
                               pixelSize, pixelSize);
                painter.fillRect(pixelRect, color);
            }
        }
    }

    // 绘制网格
    if (m_showGrid && pixelSize >= 4) {
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        for (int x = 0; x <= imageWidth; ++x) {
            int xPos = m_offset.x() + x * pixelSize;
            painter.drawLine(xPos, m_offset.y(),
                           xPos, m_offset.y() + imageHeight * pixelSize);
        }
        for (int y = 0; y <= imageHeight; ++y) {
            int yPos = m_offset.y() + y * pixelSize;
            painter.drawLine(m_offset.x(), yPos,
                           m_offset.x() + imageWidth * pixelSize, yPos);
        }
    }

    // 绘制边框
    painter.setPen(QPen(Qt::black, 2));
    painter.drawRect(glyphRect);
}

void GlyphDetailWidget::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    if (delta > 0) {
        setZoomLevel(m_zoomLevel + 1);
    } else if (delta < 0) {
        setZoomLevel(m_zoomLevel - 1);
    }
    event->accept();
}

void GlyphDetailWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void GlyphDetailWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        update();
    }
}

void GlyphDetailWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void GlyphDetailWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    QWidget::resizeEvent(event);
}

// ============================================================================
// GlyphDetailDialog
// ============================================================================

GlyphDetailDialog::GlyphDetailDialog(QWidget *parent)
    : QDialog(parent)
    , m_character(0)
    , m_codePoint(0)
{
    setWindowTitle(tr("Glyph Detail"));
    resize(600, 650);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 信息标签
    m_infoLabel = new QLabel(this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    QFont font = m_infoLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    m_infoLabel->setFont(font);
    mainLayout->addWidget(m_infoLabel);

    // 字形显示控件
    m_glyphWidget = new GlyphDetailWidget(this);
    mainLayout->addWidget(m_glyphWidget, 1);

    // 控制按钮
    QHBoxLayout *controlLayout = new QHBoxLayout();

    m_zoomOutBtn = new QPushButton("-", this);
    m_zoomOutBtn->setFixedWidth(40);
    controlLayout->addWidget(m_zoomOutBtn);

    m_zoomInBtn = new QPushButton("+", this);
    m_zoomInBtn->setFixedWidth(40);
    controlLayout->addWidget(m_zoomInBtn);

    m_fitBtn = new QPushButton(tr("Fit"), this);
    controlLayout->addWidget(m_fitBtn);

    m_gridCheckBox = new QCheckBox(tr("Show Grid"), this);
    m_gridCheckBox->setChecked(true);
    controlLayout->addWidget(m_gridCheckBox);

    controlLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(tr("Close"), this);
    controlLayout->addWidget(closeBtn);

    mainLayout->addLayout(controlLayout);

    // 连接信号
    connect(m_zoomInBtn, &QPushButton::clicked, this, &GlyphDetailDialog::onZoomIn);
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &GlyphDetailDialog::onZoomOut);
    connect(m_fitBtn, &QPushButton::clicked, this, &GlyphDetailDialog::onFit);
    connect(m_gridCheckBox, &QCheckBox::toggled, this, &GlyphDetailDialog::onToggleGrid);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void GlyphDetailDialog::setGlyphInfo(QChar character, uint32_t codePoint, const QPixmap &pixmap)
{
    m_character = character;
    m_codePoint = codePoint;

    QString info = QString("Character: %1  |  Unicode: U+%2  |  Size: %3x%4")
                       .arg(character)
                       .arg(codePoint, 4, 16, QChar('0'))
                       .arg(pixmap.width())
                       .arg(pixmap.height());
    m_infoLabel->setText(info);

    m_glyphWidget->setGlyphImage(pixmap);
    m_glyphWidget->fitToView();
}

void GlyphDetailDialog::onZoomIn()
{
    m_glyphWidget->setZoomLevel(m_glyphWidget->zoomLevel() + 1);
}

void GlyphDetailDialog::onZoomOut()
{
    m_glyphWidget->setZoomLevel(m_glyphWidget->zoomLevel() - 1);
}

void GlyphDetailDialog::onFit()
{
    m_glyphWidget->fitToView();
}

void GlyphDetailDialog::onToggleGrid(bool checked)
{
    m_glyphWidget->setShowGrid(checked);
}
