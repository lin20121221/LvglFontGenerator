#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 测试字体渲染
    QFont font("Arial", 16);
    font.setPixelSize(16);

    QFontMetrics metrics(font);
    QChar ch('0');

    QRect boundingRect = metrics.boundingRect(ch);

    qDebug() << "Character: " << ch;
    qDebug() << "BoundingRect:" << boundingRect;
    qDebug() << "Width:" << boundingRect.width();
    qDebug() << "Height:" << boundingRect.height();
    qDebug() << "X:" << boundingRect.x();
    qDebug() << "Y:" << boundingRect.y();
    qDebug() << "Top:" << boundingRect.top();
    qDebug() << "Bottom:" << boundingRect.bottom();

    // 创建图像
    int width = qMax(1, boundingRect.width());
    int height = qMax(1, boundingRect.height());

    QImage image(width, height, QImage::Format_Grayscale8);
    image.fill(0);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255));
    painter.drawText(-boundingRect.x(), -boundingRect.y(), QString(ch));
    painter.end();

    // 保存图像
    image.save("test_glyph.png");

    // 输出前几个像素值
    qDebug() << "\nFirst 20 pixel values:";
    for (int i = 0; i < qMin(20, width * height); i++) {
        int x = i % width;
        int y = i / width;
        int gray = qGray(image.pixel(x, y));
        qDebug() << QString("Pixel[%1,%2] = 0x%3").arg(x).arg(y).arg(gray, 2, 16, QChar('0'));
    }

    return 0;
}
