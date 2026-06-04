#ifndef CHARACTERGRIDWIDGET_H
#define CHARACTERGRIDWIDGET_H

#include <QAbstractScrollArea>
#include <QVector>
#include <QPixmap>
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>

// 单个字符网格项
struct CharacterGridItem {
    QChar character;
    uint32_t codePoint;
    QPixmap thumbnail;  // 缩略图
    bool existsInFont;  // 字符是否在字体中存在
    bool isRendered;    // 是否已渲染
};

// 字符网格显示组件 - 可滚动的网格视图
class CharacterGridWidget : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit CharacterGridWidget(QWidget *parent = nullptr);
    ~CharacterGridWidget();

    // 设置要显示的字符列表
    void setCharacters(const QString &characters);

    // 设置字体信息用于渲染
    void setFontInfo(const QString &fontPath, int fontSize, int bpp);

    // 刷新显示
    void refreshDisplay();

    // 设置单元格大小
    void setCellSize(int size);
    int cellSize() const { return m_cellSize; }

signals:
    // 字符被双击时发出信号
    void characterDoubleClicked(QChar ch);

    // 渲染进度信号
    void renderProgress(int current, int total);
    void renderFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onRenderFinished();

private:
    void renderThumbnails();     // 渲染所有字符的缩略图
    void renderThumbnailsAsync(); // 异步渲染缩略图
    void calculateLayout();      // 计算布局
    void updateScrollBars();     // 更新滚动条
    void renderVisibleItems(QPainter &painter);  // 渲染可见的字符
    void renderItem(QPainter &painter, const CharacterGridItem &item, const QRect &rect, bool hovered);

    QRect getItemRect(int index) const;  // 获取指定索引的字符矩形
    int getItemIndex(const QPoint &pos) const;  // 获取指定位置的字符索引

    QString m_fontPath;
    int m_fontSize;
    int m_bpp;

    QVector<CharacterGridItem> m_items;  // 所有字符项

    // 布局参数
    int m_cellSize;          // 单元格大小
    int m_columns;           // 列数
    int m_totalRows;         // 总行数
    int m_hoveredIndex;      // 鼠标悬停的字符索引

    // 异步渲染
    QFutureWatcher<QVector<CharacterGridItem>> *m_renderWatcher;
    bool m_isRendering;
};

#endif // CHARACTERGRIDWIDGET_H
