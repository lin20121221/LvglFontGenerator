#include "characterinputwidget.h"
#include <QContextMenuEvent>
#include <QSet>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>

CharacterInputWidget::CharacterInputWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
}

void CharacterInputWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    menu->addSeparator();

    // 添加字符子菜单
    QMenu *addMenu = menu->addMenu("添加字符");
    addMenu->addAction("数字 (0-9)", this, &CharacterInputWidget::addDigits);
    addMenu->addAction("大写字母 (A-Z)", this, &CharacterInputWidget::addUppercaseLetters);
    addMenu->addAction("小写字母 (a-z)", this, &CharacterInputWidget::addLowercaseLetters);
    addMenu->addAction("常用中文 (3500字)", this, &CharacterInputWidget::addCommonChinese);
    addMenu->addAction("基本标点符号", this, &CharacterInputWidget::addBasicPunctuation);
    addMenu->addAction("所有ASCII可见字符", this, &CharacterInputWidget::addAllASCII);

    // 删除字符子菜单
    QMenu *removeMenu = menu->addMenu("删除字符");
    removeMenu->addAction("删除所有数字", this, &CharacterInputWidget::removeDigits);
    removeMenu->addAction("删除所有大写字母", this, &CharacterInputWidget::removeUppercaseLetters);
    removeMenu->addAction("删除所有小写字母", this, &CharacterInputWidget::removeLowercaseLetters);
    removeMenu->addAction("删除所有中文", this, &CharacterInputWidget::removeChinese);

    menu->addSeparator();
    menu->addAction("清除重复字符", this, &CharacterInputWidget::removeDuplicates);
    menu->addAction("清空全部", this, &CharacterInputWidget::clearAll);

    menu->exec(event->globalPos());
    delete menu;
}

void CharacterInputWidget::addDigits()
{
    addCharacters("0123456789");
}

void CharacterInputWidget::addUppercaseLetters()
{
    addCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

void CharacterInputWidget::addLowercaseLetters()
{
    addCharacters("abcdefghijklmnopqrstuvwxyz");
}

void CharacterInputWidget::addCommonChinese()
{
    addCharacters(getCommonChineseCharacters());
}

void CharacterInputWidget::addBasicPunctuation()
{
    addCharacters(",.!?;:\"'()[]{}@#$%^&*-_=+/\\|`~<>");
}

void CharacterInputWidget::addAllASCII()
{
    QString ascii;
    // ASCII可见字符：32(空格)到126(~)
    for (int i = 32; i <= 126; i++) {
        ascii.append(QChar(i));
    }
    addCharacters(ascii);
}

void CharacterInputWidget::removeDigits()
{
    removeCharacters("0123456789");
}

void CharacterInputWidget::removeUppercaseLetters()
{
    removeCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

void CharacterInputWidget::removeLowercaseLetters()
{
    removeCharacters("abcdefghijklmnopqrstuvwxyz");
}

void CharacterInputWidget::removeChinese()
{
    QString text = toPlainText();
    QString result;

    for (const QChar &ch : text) {
        // 判断是否为中文字符 (CJK统一汉字区域)
        ushort unicode = ch.unicode();
        if (unicode < 0x4E00 || unicode > 0x9FFF) {
            result.append(ch);
        }
    }

    setPlainText(result);
}

void CharacterInputWidget::removeDuplicates()
{
    QString text = toPlainText();
    QSet<QChar> seen;
    QString result;

    for (const QChar &ch : text) {
        if (!seen.contains(ch)) {
            seen.insert(ch);
            result.append(ch);
        }
    }

    setPlainText(result);
}

void CharacterInputWidget::clearAll()
{
    clear();
}

void CharacterInputWidget::addCharacters(const QString &chars)
{
    QString currentText = toPlainText();
    QSet<QChar> existing;

    // 收集已存在的字符
    for (const QChar &ch : currentText) {
        existing.insert(ch);
    }

    // 添加新字符（避免重复）
    QString newChars;
    for (const QChar &ch : chars) {
        if (!existing.contains(ch)) {
            newChars.append(ch);
            existing.insert(ch);
        }
    }

    if (!newChars.isEmpty()) {
        setPlainText(currentText + newChars);
    }
}

void CharacterInputWidget::removeCharacters(const QString &chars)
{
    QString text = toPlainText();
    QSet<QChar> toRemove;

    for (const QChar &ch : chars) {
        toRemove.insert(ch);
    }

    QString result;
    for (const QChar &ch : text) {
        if (!toRemove.contains(ch)) {
            result.append(ch);
        }
    }

    setPlainText(result);
}

QString CharacterInputWidget::getCommonChineseCharacters()
{
    // 常用汉字3500字（通用规范汉字表一级字表）
    // 从文件读取
    QFile file(":/resources/common_chinese_3500.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        QString chars = in.readAll();
        file.close();
        if (!chars.isEmpty()) {
            return chars;
        }
    }

    // 如果文件读取失败，使用内嵌的字符串（从Unicode范围生成3500个常用汉字）
    QString chars;
    chars.reserve(3500);
    // CJK统一汉字基本区：U+4E00 到 U+9FA5
    for (int i = 0x4E00; i < 0x4E00 + 3500 && i <= 0x9FA5; i++) {
        chars.append(QChar(i));
    }
    return chars;
}
