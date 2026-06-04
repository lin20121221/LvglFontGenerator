#ifndef OPENTYPEKERNING_H
#define OPENTYPEKERNING_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QPair>

class OpenTypeKerning
{
public:
    struct KerningPair {
        int leftIndex;
        int rightIndex;
        int valueLVGL;  // 1/16 pixels, signed 8-bit [-128, 127]
    };

    OpenTypeKerning();
    ~OpenTypeKerning();

    // 从字体文件提取 kerning 数据
    // characters: 字符列表（按索引顺序）
    // fontSize: 字号（像素）
    // 返回: kerning 对列表
    bool extractKerning(const QString &fontPath, int fontSize, const QVector<uint32_t> &characters);

    // 获取提取到的 kerning 对
    QVector<KerningPair> getKerningPairs() const { return m_kerningPairs; }

    QString lastError() const { return m_lastError; }

private:
    QVector<KerningPair> m_kerningPairs;
    QString m_lastError;

    // 使用纯 C++ 解析 OpenType GPOS 表
    bool parseGPOSTable(const QByteArray &fontData, int fontSize, int unitsPerEM,
                        const QVector<uint32_t> &characters, const QMap<uint32_t, int> &charToIndex);
};

#endif // OPENTYPEKERNING_H
