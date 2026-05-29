#ifndef KERNINGOPTIMIZER_H
#define KERNINGOPTIMIZER_H

#include <QMap>
#include <QList>
#include <QPair>
#include <QVector>

class KerningOptimizer
{
public:
    struct OptimizedKerning {
        QVector<int> leftClassMapping;   // glyph_id -> left_class
        QVector<int> rightClassMapping;  // glyph_id -> right_class
        QVector<int> classValues;        // [left_class * right_cnt + right_class]
        int leftClassCount;
        int rightClassCount;
    };

    // 输入：字形列表和 kerning 对 (left_idx, right_idx) -> value
    // glyphs: 按 unicode 排序的字形列表，用于获取 unicode 值
    // 输出：优化后的类映射和值数组
    static OptimizedKerning optimize(
        int glyphCount,
        const QMap<QPair<int, int>, int> &kernPairs,
        const QList<uint32_t> &unicodeList  // 新增：unicode 列表
    );

private:
    // 构建左侧类：将具有相同右侧 kerning 模式的字符分组
    static QList<QList<int>> buildLeftClasses(
        int glyphCount,
        const QMap<QPair<int, int>, int> &kernPairs,
        const QList<uint32_t> &unicodeList
    );

    // 构建右侧类：将具有相同左侧 kerning 模式的字符分组
    static QList<QList<int>> buildRightClasses(
        int glyphCount,
        const QMap<QPair<int, int>, int> &kernPairs,
        const QList<uint32_t> &unicodeList
    );

    // 将类列表转换为映射表
    static QVector<int> classesToMapping(
        int glyphCount,
        const QList<QList<int>> &classes
    );

    // 生成类值数组
    static QVector<int> generateClassValues(
        const QList<QList<int>> &leftClasses,
        const QList<QList<int>> &rightClasses,
        const QMap<QPair<int, int>, int> &kernPairs
    );
};

#endif // KERNINGOPTIMIZER_H
