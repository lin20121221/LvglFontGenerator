#include "characterparser.h"
#include <QRegularExpression>
#include <QDebug>

QSet<QChar> CharacterParser::parse(const QString &input)
{
    QSet<QChar> result;

    int i = 0;
    while (i < input.length()) {
        if (input[i] == '[') {
            // 找到匹配的 ]
            int closePos = input.indexOf(']', i);
            if (closePos == -1) {
                // 没有找到闭合的 ]，当作普通字符处理
                result.insert(input[i]);
                i++;
                continue;
            }

            // 提取 [] 内的内容
            QString encoded = input.mid(i + 1, closePos - i - 1).trimmed();

            // 尝试解析为范围
            uint32_t start, end;
            if (parseRange(encoded, start, end)) {
                // 添加范围内的所有字符
                for (uint32_t code = start; code <= end; code++) {
                    if (code <= 0xFFFF) {
                        result.insert(QChar(code));
                    } else {
                        // 处理超过BMP的字符（需要代理对）
                        // 简化处理：暂时只支持BMP字符
                        qWarning() << "Code point beyond BMP not supported:" << QString::number(code, 16);
                    }
                }
                i = closePos + 1;
                continue;
            }

            // 尝试解析为单个码点
            uint32_t codePoint;
            if (parseCodePoint(encoded, codePoint)) {
                if (codePoint <= 0xFFFF) {
                    result.insert(QChar(codePoint));
                } else {
                    qWarning() << "Code point beyond BMP not supported:" << QString::number(codePoint, 16);
                }
                i = closePos + 1;
                continue;
            }

            // 解析失败，当作普通字符处理
            result.insert(input[i]);
            i++;
        } else {
            // 普通字符，直接添加
            result.insert(input[i]);
            i++;
        }
    }

    return result;
}

bool CharacterParser::parseCodePoint(const QString &str, uint32_t &codePoint)
{
    QString s = str.trimmed().toUpper();

    // 支持 U+XXXX 或 0xXXXX 格式
    QRegularExpression re("^(?:U\\+|0X)([0-9A-F]+)$");
    QRegularExpressionMatch match = re.match(s);

    if (match.hasMatch()) {
        bool ok;
        codePoint = match.captured(1).toUInt(&ok, 16);
        return ok;
    }

    return false;
}

bool CharacterParser::parseRange(const QString &str, uint32_t &start, uint32_t &end)
{
    QString s = str.trimmed().toUpper();

    // 支持 U+XXXX-U+YYYY 或 0xXXXX-0xYYYY 格式
    QRegularExpression re("^(?:U\\+|0X)([0-9A-F]+)\\s*-\\s*(?:U\\+|0X)([0-9A-F]+)$");
    QRegularExpressionMatch match = re.match(s);

    if (match.hasMatch()) {
        bool ok1, ok2;
        start = match.captured(1).toUInt(&ok1, 16);
        end = match.captured(2).toUInt(&ok2, 16);

        if (ok1 && ok2 && start <= end) {
            return true;
        }
    }

    return false;
}
