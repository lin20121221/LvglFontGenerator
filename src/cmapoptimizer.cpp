#include "cmapoptimizer.h"
#include <algorithm>
#include <climits>
#include <QDebug>

// 格式大小估算函数
int CmapOptimizer::estimateFormat0TinySize()
{
    return 16;  // 仅头部
}

int CmapOptimizer::estimateFormat0Size(uint32_t rangeStart, uint32_t rangeEnd)
{
    return 16 + (rangeEnd - rangeStart + 1);  // 头部 + 每个位置1字节
}

int CmapOptimizer::estimateSparseTinySize(int charCount)
{
    return 16 + charCount * 2;  // 头部 + 每个字符2字节（uint16偏移）
}

// 连续性检测
bool CmapOptimizer::isContinuous(const QVector<uint32_t> &codepoints, int start, int end)
{
    // 检测码点之间没有间隙
    if (start >= end) return true;

    uint32_t expected = codepoints[start];
    for (int i = start; i <= end; i++) {
        if (codepoints[i] != expected) {
            return false;
        }
        expected++;
    }
    return true;
}

// 动态规划优化算法
QVector<CmapOptimizer::SubTable> CmapOptimizer::optimize(const QVector<uint32_t> &allCodepoints)
{
    // 确保排序
    QVector<uint32_t> sorted = allCodepoints;
    std::sort(sorted.begin(), sorted.end());

    int n = sorted.size();

    if (n == 0) {
        return QVector<SubTable>();
    }

    // min_paths[i] 存储到第 i 个码点的最优方案
    struct PathNode {
        int totalSize;      // 到此的总大小
        int startIdx;       // 当前子表的起始索引
        int endIdx;         // 当前子表的结束索引
        Format format;      // 当前子表的格式
    };
    QVector<PathNode> minPaths(n);

    // 动态规划：正向计算
    for (int i = 0; i < n; i++) {
        PathNode min;
        min.totalSize = INT_MAX;
        min.startIdx = 0;
        min.endIdx = i;
        min.format = SPARSE_TINY;  // 默认格式

        // 尝试所有可能的起始位置 j
        for (int j = 0; j <= i; j++) {
            int prevSize = (j > 0) ? minPaths[j-1].totalSize : 0;
            uint32_t rangeSpan = sorted[i] - sorted[j];
            int charCount = i - j + 1;

            // 尝试 FORMAT0 (范围 < 256)
            if (rangeSpan < 256) {
                int size = estimateFormat0Size(sorted[j], sorted[i]);
                if (prevSize + size < min.totalSize) {
                    min = {prevSize + size, j, i, FORMAT0};
                }
            }

            // 尝试 FORMAT0_TINY (范围 < 256 且完全连续)
            if (rangeSpan < 256 && isContinuous(sorted, j, i)) {
                int size = estimateFormat0TinySize();
                if (prevSize + size < min.totalSize) {
                    min = {prevSize + size, j, i, FORMAT0_TINY};
                }
            }

            // 尝试 SPARSE_TINY (范围 < 65536)
            if (rangeSpan < 65536) {
                int size = estimateSparseTinySize(charCount);
                if (prevSize + size < min.totalSize) {
                    min = {prevSize + size, j, i, SPARSE_TINY};
                }
            }
        }

        minPaths[i] = min;
    }

    // 反向回溯：构建最优分割方案
    QVector<SubTable> result;

    // 第一遍：反向回溯构建子表结构（不分配glyph ID）
    for (int i = n - 1; i >= 0; ) {
        PathNode path = minPaths[i];

        SubTable sub;
        sub.format = path.format;
        sub.rangeStart = sorted[path.startIdx];
        sub.rangeEnd = sorted[path.endIdx];
        sub.glyphIdStart = 0;  // 暂时置0

        // 提取该子表的所有码点
        for (int k = path.startIdx; k <= path.endIdx; k++) {
            sub.codepoints.append(sorted[k]);
        }

        result.prepend(sub);  // 前插（因为是反向回溯）
        i = path.startIdx - 1;
    }

    // 第二遍：正向分配glyph ID（从1开始，0预留）
    int currentGlyphId = 1;
    for (int i = 0; i < result.size(); i++) {
        result[i].glyphIdStart = currentGlyphId;
        currentGlyphId += result[i].codepoints.size();
    }

    // 输出调试信息
    qDebug() << "Cmap optimization result:";
    qDebug() << "  Total subtables:" << result.size();
    for (int i = 0; i < result.size(); i++) {
        const SubTable &sub = result[i];
        QString formatName;
        switch (sub.format) {
            case FORMAT0_TINY: formatName = "FORMAT0_TINY"; break;
            case FORMAT0: formatName = "FORMAT0"; break;
            case SPARSE_TINY: formatName = "SPARSE_TINY"; break;
        }
        qDebug() << "  SubTable" << i << ":" << formatName
                 << "| Range: 0x" << QString::number(sub.rangeStart, 16).toUpper()
                 << "- 0x" << QString::number(sub.rangeEnd, 16).toUpper()
                 << "| Chars:" << sub.codepoints.size();
    }

    return result;
}
