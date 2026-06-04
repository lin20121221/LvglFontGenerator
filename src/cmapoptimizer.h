#ifndef CMAPOPTIMIZER_H
#define CMAPOPTIMIZER_H

#include <QVector>
#include <cstdint>

class CmapOptimizer
{
public:
    enum Format {
        FORMAT0_TINY,      // 连续字符，仅头部（16字节）
        FORMAT0,           // 小范围字符，有ID偏移数组
        SPARSE_TINY        // 稀疏字符，有unicode_list
    };

    struct SubTable {
        Format format;
        uint32_t rangeStart;
        uint32_t rangeEnd;
        QVector<uint32_t> codepoints;  // 该子表包含的所有码点
        int glyphIdStart;
    };

    // 动态规划优化入口
    static QVector<SubTable> optimize(const QVector<uint32_t> &allCodepoints);

private:
    // 格式大小估算
    static int estimateFormat0TinySize();
    static int estimateFormat0Size(uint32_t rangeStart, uint32_t rangeEnd);
    static int estimateSparseTinySize(int charCount);

    // 连续性检测
    static bool isContinuous(const QVector<uint32_t> &codepoints, int start, int end);
};

#endif // CMAPOPTIMIZER_H
