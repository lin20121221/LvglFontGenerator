#ifndef CHARACTERINPUTWIDGET_H
#define CHARACTERINPUTWIDGET_H

#include <QPlainTextEdit>
#include <QMenu>

class CharacterInputWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CharacterInputWidget(QWidget *parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void addDigits();
    void addUppercaseLetters();
    void addLowercaseLetters();
    void addCommonChinese();
    void addBasicPunctuation();
    void addAllASCII();
    void removeDigits();
    void removeUppercaseLetters();
    void removeLowercaseLetters();
    void removeChinese();
    void removeDuplicates();
    void clearAll();

private:
    void addCharacters(const QString &chars);
    void removeCharacters(const QString &chars);
    QString getCommonChineseCharacters();
};

#endif // CHARACTERINPUTWIDGET_H
