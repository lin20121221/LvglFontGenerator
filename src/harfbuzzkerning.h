#ifndef HARFBUZZKERNING_H
#define HARFBUZZKERNING_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QPair>

class HarfBuzzKerning
{
public:
    struct KerningPair {
        int leftIndex;
        int rightIndex;
        int valueLVGL;  // 1/16 pixels, signed 8-bit [-128, 127]
    };

    HarfBuzzKerning();
    ~HarfBuzzKerning();

    // 从字体文件提取 kerning 数据
    // characters: 字符列表（按索引顺序）
    // fontSize: 字号（像素）
    // 返回: 是否成功
    bool extractKerning(const QString &fontPath, int fontSize, const QVector<uint32_t> &characters);

    // 获取提取到的 kerning 对
    QVector<KerningPair> getKerningPairs() const { return m_kerningPairs; }

    QString lastError() const { return m_lastError; }

private:
    QVector<KerningPair> m_kerningPairs;
    QString m_lastError;
};

#endif // HARFBUZZKERNING_H
