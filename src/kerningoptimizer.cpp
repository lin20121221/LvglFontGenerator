#include "kerningoptimizer.h"
#include <QDebug>
#include <QSet>

KerningOptimizer::OptimizedKerning KerningOptimizer::optimize(
    int glyphCount,
    const QMap<QPair<int, int>, int> &kernPairs,
    const QList<uint32_t> &unicodeList)
{
    qDebug() << "Starting kerning optimization...";
    qDebug() << "  Glyph count:" << glyphCount;
    qDebug() << "  Total kerning pairs:" << kernPairs.size();

    // 构建左侧和右侧类
    QList<QList<int>> leftClasses = buildLeftClasses(glyphCount, kernPairs, unicodeList);
    QList<QList<int>> rightClasses = buildRightClasses(glyphCount, kernPairs, unicodeList);

    qDebug() << "  Left classes:" << leftClasses.size();
    qDebug() << "  Right classes:" << rightClasses.size();

    // 生成映射表
    QVector<int> leftMapping = classesToMapping(glyphCount, leftClasses);
    QVector<int> rightMapping = classesToMapping(glyphCount, rightClasses);

    // 生成类值数组
    QVector<int> classValues = generateClassValues(leftClasses, rightClasses, kernPairs);

    qDebug() << "  Class values size:" << classValues.size();
    qDebug() << "  Non-zero values:" << std::count_if(classValues.begin(), classValues.end(),
                                                       [](int v) { return v != 0; });

    OptimizedKerning result;
    result.leftClassMapping = leftMapping;
    result.rightClassMapping = rightMapping;
    result.classValues = classValues;
    result.leftClassCount = leftClasses.size();
    result.rightClassCount = rightClasses.size();

    return result;
}

QList<QList<int>> KerningOptimizer::buildLeftClasses(
    int glyphCount,
    const QMap<QPair<int, int>, int> &kernPairs,
    const QList<uint32_t> &unicodeList)
{
    // 为每个字符构建其右侧 kerning 模式
    // 使用 Unicode code point 作为键（与官方工具一致）
    QMap<uint32_t, QMap<uint32_t, int>> leftKernings; // unicode -> {unicode -> kern_value}

    for (auto it = kernPairs.constBegin(); it != kernPairs.constEnd(); ++it) {
        int leftIdx = it.key().first;
        int rightIdx = it.key().second;
        int value = it.value();

        // 将 glyph index 转换为 unicode
        if (leftIdx > 0 && leftIdx <= unicodeList.size() &&
            rightIdx > 0 && rightIdx <= unicodeList.size()) {
            uint32_t leftUnicode = unicodeList[leftIdx - 1];
            uint32_t rightUnicode = unicodeList[rightIdx - 1];
            leftKernings[leftUnicode][rightUnicode] = value;
        }
    }

    // 将具有相同 kerning 模式的字符分组
    // 关键：使用 QMap 而不是 QHash 来保持插入顺序的一致性
    // 并且按照 glyph_id 顺序遍历（与官方工具一致）
    QMap<QString, QList<int>> patternToGlyphs;
    QList<QString> patternOrder; // 记录模式出现的顺序

    // 按 glyph_id 顺序遍历（1, 2, 3, ...）
    for (int i = 1; i <= glyphCount; i++) {
        if (i > unicodeList.size()) continue;

        uint32_t unicode = unicodeList[i - 1];

        if (!leftKernings.contains(unicode)) {
            // 没有 kerning 的字符都归到类 0
            continue;
        }

        // 构建 JSON 风格的模式字符串: {"unicode1":value1,"unicode2":value2,...}
        QString pattern = "{";
        QMap<uint32_t, int> kerning = leftKernings[unicode];
        QList<uint32_t> keys = kerning.keys();
        std::sort(keys.begin(), keys.end());

        for (int j = 0; j < keys.size(); j++) {
            if (j > 0) pattern += ",";
            pattern += QString("\"%1\":%2").arg(keys[j]).arg(kerning[keys[j]]);
        }
        pattern += "}";

        // 如果是新模式，记录顺序
        if (!patternToGlyphs.contains(pattern)) {
            patternOrder.append(pattern);
        }
        patternToGlyphs[pattern].append(i);
    }

    // 按照模式出现的顺序构建类列表（与官方工具一致）
    QList<QList<int>> classes;
    for (const QString &pattern : patternOrder) {
        classes.append(patternToGlyphs[pattern]);
    }

    return classes;
}

QList<QList<int>> KerningOptimizer::buildRightClasses(
    int glyphCount,
    const QMap<QPair<int, int>, int> &kernPairs,
    const QList<uint32_t> &unicodeList)
{
    // 为每个字符构建其左侧 kerning 模式
    QMap<uint32_t, QMap<uint32_t, int>> rightKernings; // unicode -> {unicode -> kern_value}

    for (auto it = kernPairs.constBegin(); it != kernPairs.constEnd(); ++it) {
        int leftIdx = it.key().first;
        int rightIdx = it.key().second;
        int value = it.value();

        if (leftIdx > 0 && leftIdx <= unicodeList.size() &&
            rightIdx > 0 && rightIdx <= unicodeList.size()) {
            uint32_t leftUnicode = unicodeList[leftIdx - 1];
            uint32_t rightUnicode = unicodeList[rightIdx - 1];
            rightKernings[rightUnicode][leftUnicode] = value;
        }
    }

    // 将具有相同 kerning 模式的字符分组
    QMap<QString, QList<int>> patternToGlyphs;
    QList<QString> patternOrder; // 记录模式出现的顺序

    // 按 glyph_id 顺序遍历
    for (int i = 1; i <= glyphCount; i++) {
        if (i > unicodeList.size()) continue;

        uint32_t unicode = unicodeList[i - 1];

        if (!rightKernings.contains(unicode)) {
            continue;
        }

        QString pattern = "{";
        QMap<uint32_t, int> kerning = rightKernings[unicode];
        QList<uint32_t> keys = kerning.keys();
        std::sort(keys.begin(), keys.end());

        for (int j = 0; j < keys.size(); j++) {
            if (j > 0) pattern += ",";
            pattern += QString("\"%1\":%2").arg(keys[j]).arg(kerning[keys[j]]);
        }
        pattern += "}";

        // 如果是新模式，记录顺序
        if (!patternToGlyphs.contains(pattern)) {
            patternOrder.append(pattern);
        }
        patternToGlyphs[pattern].append(i);
    }

    // 按照模式出现的顺序构建类列表
    QList<QList<int>> classes;
    for (const QString &pattern : patternOrder) {
        classes.append(patternToGlyphs[pattern]);
    }

    return classes;
}

QVector<int> KerningOptimizer::classesToMapping(
    int glyphCount,
    const QList<QList<int>> &classes)
{
    // 创建映射表：mapping[glyph_id] = class_id
    // glyphCount 包括 glyph 0（保留），所以映射表大小就是 glyphCount
    // mapping[0] = 0 (glyph 0 保留)
    // mapping[1] = class_id (第一个字符)
    // ...
    // mapping[glyphCount-1] = class_id (最后一个字符)
    QVector<int> mapping(glyphCount, 0);

    for (int classIdx = 0; classIdx < classes.size(); classIdx++) {
        for (int glyphIdx : classes[classIdx]) {
            // glyphIdx 是 1-based (1 到 glyphCount-1)
            // 直接用作映射表索引
            if (glyphIdx < mapping.size()) {
                mapping[glyphIdx] = classIdx + 1; // 类从 1 开始
            }
        }
    }

    return mapping;
}

QVector<int> KerningOptimizer::generateClassValues(
    const QList<QList<int>> &leftClasses,
    const QList<QList<int>> &rightClasses,
    const QMap<QPair<int, int>, int> &kernPairs)
{
    int leftCount = leftClasses.size();
    int rightCount = rightClasses.size();

    // 创建值数组：[left_class * right_count + right_class]
    QVector<int> values(leftCount * rightCount, 0);

    // 对于每个左类和右类的组合，取该类中第一个字符的 kerning 值
    for (int leftClassIdx = 0; leftClassIdx < leftCount; leftClassIdx++) {
        for (int rightClassIdx = 0; rightClassIdx < rightCount; rightClassIdx++) {
            // 取每个类的第一个字符作为代表
            int leftChar = leftClasses[leftClassIdx][0];
            int rightChar = rightClasses[rightClassIdx][0];

            // 查找 kerning 值
            auto key = qMakePair(leftChar, rightChar);
            if (kernPairs.contains(key)) {
                int value = kernPairs[key];
                values[leftClassIdx * rightCount + rightClassIdx] = value;
            }
        }
    }

    return values;
}
