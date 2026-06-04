#include "charactergridwidget.h"
#include "freetyperenderer.h"
#include "glyphdetaildialog.h"
#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QToolTip>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

CharacterGridWidget::CharacterGridWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
    , m_fontSize(16)
    , m_bpp(4)
    , m_cellSize(80)
    , m_columns(10)
    , m_totalRows(0)
    , m_hoveredIndex(-1)
    , m_isRendering(false)
{
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    viewport()->setBackgroundRole(QPalette::Base);
    viewport()->setAutoFillBackground(true);

    // 创建异步渲染监视器
    m_renderWatcher = new QFutureWatcher<QVector<CharacterGridItem>>(this);
    connect(m_renderWatcher, &QFutureWatcher<QVector<CharacterGridItem>>::finished,
            this, &CharacterGridWidget::onRenderFinished);
}

CharacterGridWidget::~CharacterGridWidget()
{
    if (m_renderWatcher->isRunning()) {
        m_renderWatcher->cancel();
        m_renderWatcher->waitForFinished();
    }
}

void CharacterGridWidget::setCharacters(const QString &characters)
{
    // 取消正在进行的渲染，优先响应UI事件
    if (m_renderWatcher->isRunning()) {
        qDebug() << "Canceling rendering due to character change";
        m_renderWatcher->cancel();
        // 不阻塞等待，让渲染线程自己检查并退出
    }

    m_items.clear();
    m_hoveredIndex = -1;
    m_isRendering = false;

    // 为每个字符创建项
    for (const QChar &ch : characters) {
        CharacterGridItem item;
        item.character = ch;
        item.codePoint = ch.unicode();
        item.existsInFont = true;
        item.isRendered = false;
        m_items.append(item);
    }

    calculateLayout();
    viewport()->update();
}

void CharacterGridWidget::setFontInfo(const QString &fontPath, int fontSize, int bpp)
{
    // 检查是否真的改变了，避免不必要的刷新
    bool changed = (m_fontPath != fontPath || m_fontSize != fontSize || m_bpp != bpp);

    m_fontPath = fontPath;
    m_fontSize = fontSize;
    m_bpp = bpp;

    // 如果参数改变了，取消正在进行的渲染
    if (changed && m_renderWatcher->isRunning()) {
        qDebug() << "Canceling rendering due to font info change";
        m_renderWatcher->cancel();
        // 不阻塞等待
    }
}

void CharacterGridWidget::refreshDisplay()
{
    // 立即刷新，不使用延迟
    // 如果正在渲染，会被取消并重新开始
    if (m_fontPath.isEmpty() || m_items.isEmpty()) {
        return;
    }

    // 如果正在渲染，立即取消
    if (m_renderWatcher->isRunning()) {
        qDebug() << "Canceling current rendering task";
        m_renderWatcher->cancel();
    }

    // 标记所有项为未渲染
    for (auto &item : m_items) {
        item.isRendered = false;
    }

    qDebug() << "Starting rendering for" << m_items.size() << "characters";

    // 启动异步渲染
    renderThumbnailsAsync();
}

void CharacterGridWidget::setCellSize(int size)
{
    m_cellSize = qBound(48, size, 128);
    calculateLayout();
    viewport()->update();
}

void CharacterGridWidget::calculateLayout()
{
    if (viewport()->width() <= 0) {
        m_columns = 10;
    } else {
        m_columns = qMax(1, viewport()->width() / m_cellSize);
    }

    m_totalRows = m_items.isEmpty() ? 0 : (m_items.size() + m_columns - 1) / m_columns;

    updateScrollBars();
}

void CharacterGridWidget::updateScrollBars()
{
    int contentHeight = m_totalRows * m_cellSize;
    int viewportHeight = viewport()->height();

    verticalScrollBar()->setPageStep(viewportHeight);
    verticalScrollBar()->setRange(0, qMax(0, contentHeight - viewportHeight));
    verticalScrollBar()->setSingleStep(m_cellSize);
}

void CharacterGridWidget::renderThumbnailsAsync()
{
    if (m_isRendering) {
        return;
    }

    m_isRendering = true;
    emit renderProgress(0, m_items.size());

    // 复制必要的数据到 lambda
    QString fontPath = m_fontPath;
    int fontSize = m_fontSize;
    int bpp = m_bpp;  // 添加 BPP
    QVector<CharacterGridItem> items = m_items;

    // 在后台线程渲染
    QFuture<QVector<CharacterGridItem>> future = QtConcurrent::run([this, fontPath, fontSize, bpp, items]() {
        QVector<CharacterGridItem> renderedItems = items;

        FreeTypeRenderer renderer;
        if (!renderer.loadFont(fontPath, fontSize)) {
            qWarning() << "Failed to load font for thumbnail rendering:" << fontPath;
            return renderedItems;
        }

        // 根据 BPP 设置抗锯齿：BPP 1 不使用抗锯齿，其他使用
        renderer.setAntialiasing(bpp > 1);

        // 渲染每个字符
        for (int i = 0; i < renderedItems.size(); ++i) {
            // 定期检查是否被取消，快速响应中断请求
            if (i % 10 == 0 && m_renderWatcher->isCanceled()) {
                qDebug() << "Rendering canceled at item" << i << "of" << renderedItems.size();
                return renderedItems;  // 立即返回，停止渲染
            }

            CharacterGridItem &item = renderedItems[i];
            FreeTypeRenderer::GlyphData glyphData;

            if (!renderer.renderGlyph(item.codePoint, glyphData)) {
                item.existsInFont = false;
                item.thumbnail = QPixmap();
                item.isRendered = true;
                continue;
            }

            item.existsInFont = true;

            // 创建 QImage - 使用更快的格式
            if (glyphData.width > 0 && glyphData.height > 0 && !glyphData.pixels.isEmpty()) {
                // 限制缩略图最大尺寸，提高渲染速度
                int maxSize = 64;
                int targetWidth = glyphData.width;
                int targetHeight = glyphData.height;

                if (targetWidth > maxSize || targetHeight > maxSize) {
                    float scale = qMin(float(maxSize) / targetWidth, float(maxSize) / targetHeight);
                    targetWidth = int(targetWidth * scale);
                    targetHeight = int(targetHeight * scale);
                }

                QImage image(targetWidth, targetHeight, QImage::Format_Alpha8);
                image.fill(Qt::transparent);

                // 使用缩放采样绘制位图
                float xScale = float(glyphData.width) / targetWidth;
                float yScale = float(glyphData.height) / targetHeight;

                for (int y = 0; y < targetHeight; y++) {
                    for (int x = 0; x < targetWidth; x++) {
                        int srcX = int(x * xScale);
                        int srcY = int(y * yScale);

                        if (srcY < glyphData.pixels.size() && srcX < glyphData.pixels[srcY].size()) {
                            uint8_t alpha = glyphData.pixels[srcY][srcX];
                            image.setPixel(x, y, qRgba(0, 0, 0, alpha));
                        }
                    }
                }

                item.thumbnail = QPixmap::fromImage(image);
            } else {
                item.thumbnail = QPixmap();
            }

            item.isRendered = true;
        }

        return renderedItems;
    });

    m_renderWatcher->setFuture(future);
}

void CharacterGridWidget::onRenderFinished()
{
    if (m_renderWatcher->isCanceled()) {
        m_isRendering = false;
        return;
    }

    // 获取渲染结果
    m_items = m_renderWatcher->result();
    m_isRendering = false;

    emit renderProgress(m_items.size(), m_items.size());
    emit renderFinished();

    // 刷新显示
    viewport()->update();
}

void CharacterGridWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    // 填充背景
    painter.fillRect(viewport()->rect(), Qt::white);

    if (m_items.isEmpty()) {
        painter.setPen(Qt::gray);
        painter.drawText(viewport()->rect(), Qt::AlignCenter, tr("无字符可显示"));
        return;
    }

    renderVisibleItems(painter);
}

void CharacterGridWidget::renderVisibleItems(QPainter &painter)
{
    int scrollY = verticalScrollBar()->value();
    int viewportHeight = viewport()->height();

    // 计算可见的行范围
    int startRow = scrollY / m_cellSize;
    int endRow = qMin((scrollY + viewportHeight) / m_cellSize + 1, m_totalRows);

    // 渲染可见的字符
    for (int row = startRow; row < endRow; ++row) {
        for (int col = 0; col < m_columns; ++col) {
            int index = row * m_columns + col;
            if (index >= m_items.size()) break;

            QRect cellRect = getItemRect(index);
            cellRect.translate(0, -scrollY);

            bool hovered = (index == m_hoveredIndex);
            renderItem(painter, m_items[index], cellRect, hovered);
        }
    }
}

void CharacterGridWidget::renderItem(QPainter &painter, const CharacterGridItem &item,
                                     const QRect &rect, bool hovered)
{
    // 绘制边框
    painter.setPen(hovered ? QPen(QColor(74, 144, 226), 2) : QPen(QColor(200, 200, 200)));
    painter.setBrush(Qt::white);
    painter.drawRect(rect.adjusted(2, 2, -2, -2));

    if (!item.isRendered) {
        // 正在渲染中
        painter.setPen(Qt::gray);
        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);
        painter.drawText(rect.adjusted(0, 0, 0, -20), Qt::AlignCenter, "...");
    } else if (!item.thumbnail.isNull()) {
        // 绘制缩略图
        QRect thumbnailRect = rect.adjusted(10, 10, -10, -25);
        QPixmap scaled = item.thumbnail.scaled(thumbnailRect.size(),
                                               Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);

        int x = thumbnailRect.x() + (thumbnailRect.width() - scaled.width()) / 2;
        int y = thumbnailRect.y() + (thumbnailRect.height() - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
    } else if (!item.existsInFont) {
        // 字符不存在，显示问号
        painter.setPen(Qt::red);
        QFont font = painter.font();
        font.setPointSize(20);
        painter.setFont(font);
        painter.drawText(rect.adjusted(0, 0, 0, -20), Qt::AlignCenter, "?");
    }

    // 绘制 Unicode 代码
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    QString codeText = QString("U+%1").arg(item.codePoint, 4, 16, QChar('0')).toUpper();
    painter.drawText(rect.adjusted(0, rect.height() - 18, 0, 0), Qt::AlignCenter, codeText);
}

QRect CharacterGridWidget::getItemRect(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QRect();
    }

    int row = index / m_columns;
    int col = index % m_columns;

    return QRect(col * m_cellSize, row * m_cellSize, m_cellSize, m_cellSize);
}

int CharacterGridWidget::getItemIndex(const QPoint &pos) const
{
    int scrollY = verticalScrollBar()->value();
    QPoint adjustedPos = pos + QPoint(0, scrollY);

    int col = adjustedPos.x() / m_cellSize;
    int row = adjustedPos.y() / m_cellSize;

    if (col < 0 || col >= m_columns || row < 0 || row >= m_totalRows) {
        return -1;
    }

    int index = row * m_columns + col;
    return (index < m_items.size()) ? index : -1;
}

void CharacterGridWidget::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    calculateLayout();
    viewport()->update();
}

void CharacterGridWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    int index = getItemIndex(event->pos());
    if (index >= 0 && index < m_items.size()) {
        const CharacterGridItem &item = m_items[index];

        // 发出信号
        emit characterDoubleClicked(item.character);

        // 显示详细信息对话框
        if (!item.thumbnail.isNull()) {
            GlyphDetailDialog dialog(this);
            dialog.setGlyphInfo(item.character, item.codePoint, item.thumbnail);
            dialog.exec();
        }
    }
}

void CharacterGridWidget::mouseMoveEvent(QMouseEvent *event)
{
    int oldHovered = m_hoveredIndex;
    m_hoveredIndex = getItemIndex(event->pos());

    if (oldHovered != m_hoveredIndex) {
        viewport()->update();

        // 显示工具提示
        if (m_hoveredIndex >= 0 && m_hoveredIndex < m_items.size()) {
            const CharacterGridItem &item = m_items[m_hoveredIndex];
            QString tooltip = QString("U+%1\n字符: %2")
                                  .arg(item.codePoint, 4, 16, QChar('0'))
                                  .arg(item.character);
            if (!item.existsInFont) {
                tooltip += "\n(字体中不存在)";
            }
            QToolTip::showText(event->globalPosition().toPoint(), tooltip, this);
        }
    }
}
