#ifndef CHARACTERINPUTWIDGET_H
#define CHARACTERINPUTWIDGET_H

#include <QPlainTextEdit>
#include <QMenu>

class CharacterInputWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CharacterInputWidget(QWidget *parent = nullptr);

    // 获取解析后的字符集
    QString getAllCharacters() const;
    int getCharacterCount() const;

    // 设置字体信息（保留以便将来扩展）
    void setFontInfo(const QString &fontPath, int fontSize);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void addDigits();
    void addUppercaseLetters();
    void addLowercaseLetters();
    void addCommonChinese();
    void addBasicPunctuation();
    void addAllASCII();
    void addByRange();
    void removeDigits();
    void removeUppercaseLetters();
    void removeLowercaseLetters();
    void removeChinese();
    void removeDuplicates();
    void clearAll();

private:
    // 字体信息（保留以便将来扩展字符预览功能）
    QString m_fontPath;
    int m_fontSize;
};

#endif // CHARACTERINPUTWIDGET_H
