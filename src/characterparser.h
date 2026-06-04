#ifndef CHARACTERPARSER_H
#define CHARACTERPARSER_H

#include <QString>
#include <QSet>
#include <QChar>

class CharacterParser
{
public:
    // 解析输入字符串，返回字符集合
    // 支持格式：
    //   - 直接字符: ABC 123
    //   - Unicode码点: [U+4E2D] 或 [0x4E2D]
    //   - Unicode范围: [U+4E00-U+9FFF] 或 [0x4E00-0x9FFF]
    //   - 混合: ABC [U+4E2D] [0x5B57] [U+6587-U+6599]
    static QSet<QChar> parse(const QString &input);

private:
    // 解析单个Unicode码点 (例如: "U+4E2D" 或 "0x4E2D")
    static bool parseCodePoint(const QString &str, uint32_t &codePoint);

    // 解析Unicode范围 (例如: "U+4E00-U+9FFF" 或 "0x4E00-0x9FFF")
    static bool parseRange(const QString &str, uint32_t &start, uint32_t &end);
};

#endif // CHARACTERPARSER_H
