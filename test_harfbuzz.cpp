#include <QCoreApplication>
#include <QDebug>
#include "harfbuzzkerning.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== HarfBuzz Kerning 提取测试 ===\n";

    // 测试字体路径
    QString fontPath = "Z:/wqy-zenhei/arialbd.ttf";
    int fontSize = 16;

    // 测试字符
    QVector<uint32_t> characters = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        ' ', '.', ',', '!', '?', '\'', '"'
    };

    qDebug() << "字体文件:" << fontPath;
    qDebug() << "字号:" << fontSize << "px";
    qDebug() << "字符数量:" << characters.size();
    qDebug() << "";

    HarfBuzzKerning extractor;
    if (extractor.extractKerning(fontPath, fontSize, characters)) {
        qDebug() << "\n✓ HarfBuzz kerning 提取成功！";

        QVector<HarfBuzzKerning::KerningPair> pairs = extractor.getKerningPairs();
        qDebug() << "提取到" << pairs.size() << "个 kerning 对";

        if (pairs.size() > 0) {
            qDebug() << "\n前 20 个 kerning 对:";
            for (int i = 0; i < qMin(20, pairs.size()); i++) {
                const auto &pair = pairs[i];
                qDebug() << "  " << QChar(characters[pair.leftIndex]) << "-"
                         << QChar(characters[pair.rightIndex]) << ":"
                         << pair.valueLVGL << "(1/16 px)";
            }

            return 0; // 成功
        } else {
            qDebug() << "\n⚠ 警告：没有提取到 kerning 数据";
            return 1;
        }
    } else {
        qDebug() << "\n✗ HarfBuzz kerning 提取失败！";
        qDebug() << "错误:" << extractor.lastError();
        return 1;
    }
}
