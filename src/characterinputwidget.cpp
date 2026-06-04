#include "characterinputwidget.h"
#include "characterparser.h"
#include <QContextMenuEvent>
#include <QSet>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <algorithm>

CharacterInputWidget::CharacterInputWidget(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_fontSize(16)
{
    // 设置为可编辑的输入框
    setReadOnly(false);

    // 设置占位符文本提示用户输入格式
    setPlaceholderText(tr("输入字符或Unicode编码\n"
                          "支持格式:\n"
                          "  - 直接字符: ABC 123\n"
                          "  - Unicode码点: [U+4E2D] 或 [0x4E2D]\n"
                          "  - Unicode范围: [U+4E00-U+9FFF]\n"
                          "  - 混合使用: ABC [U+4E2D] [0x5B57-0x5B59]"));
}

QString CharacterInputWidget::getAllCharacters() const
{
    // 解析输入文本，返回所有字符
    QString text = toPlainText();
    QSet<QChar> charSet = CharacterParser::parse(text);

    // 按Unicode顺序排序
    QList<QChar> sorted = charSet.values();
    std::sort(sorted.begin(), sorted.end(), [](const QChar &a, const QChar &b) {
        return a.unicode() < b.unicode();
    });

    QString result;
    for (const QChar &ch : sorted) {
        result.append(ch);
    }
    return result;
}

int CharacterInputWidget::getCharacterCount() const
{
    // 解析输入文本，返回字符数量
    QString text = toPlainText();
    QSet<QChar> charSet = CharacterParser::parse(text);
    return charSet.size();
}

void CharacterInputWidget::setFontInfo(const QString &fontPath, int fontSize)
{
    m_fontPath = fontPath;
    m_fontSize = fontSize;
}

void CharacterInputWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    menu->addSeparator();

    // 添加字符子菜单
    QMenu *addMenu = menu->addMenu(tr("添加字符"));
    addMenu->addAction(tr("数字 (0-9)"), this, &CharacterInputWidget::addDigits);
    addMenu->addAction(tr("大写字母 (A-Z)"), this, &CharacterInputWidget::addUppercaseLetters);
    addMenu->addAction(tr("小写字母 (a-z)"), this, &CharacterInputWidget::addLowercaseLetters);
    addMenu->addAction(tr("常用中文"), this, &CharacterInputWidget::addCommonChinese);
    addMenu->addAction(tr("基本标点符号"), this, &CharacterInputWidget::addBasicPunctuation);
    addMenu->addAction(tr("所有ASCII可见字符"), this, &CharacterInputWidget::addAllASCII);
    addMenu->addSeparator();
    addMenu->addAction(tr("按范围添加..."), this, &CharacterInputWidget::addByRange);

    menu->addSeparator();
    menu->addAction(tr("清空全部"), this, &CharacterInputWidget::clearAll);

    menu->exec(event->globalPos());
    delete menu;
}

void CharacterInputWidget::addDigits()
{
    insertPlainText("[0x30-0x39]");  // 0-9
}

void CharacterInputWidget::addUppercaseLetters()
{
    insertPlainText("[0x41-0x5A]");  // A-Z
}

void CharacterInputWidget::addLowercaseLetters()
{
    insertPlainText("[0x61-0x7A]");  // a-z
}

void CharacterInputWidget::addCommonChinese()
{
    // 常用汉字（CJK统一汉字基本区）
    insertPlainText("[0x4E00-0x9FA5]");
}

void CharacterInputWidget::addBasicPunctuation()
{
    insertPlainText(",.!?;:\"'()[]{}@#$%^&*-_=+/\\|`~<>");
}

void CharacterInputWidget::addAllASCII()
{
    insertPlainText("[0x20-0x7E]");  // 所有ASCII可见字符
}

void CharacterInputWidget::addByRange()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("按范围添加字符"));
    dialog.resize(400, 180);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // 说明文本
    QLabel *infoLabel = new QLabel(tr("输入Unicode字符范围（十六进制）"), &dialog);
    mainLayout->addWidget(infoLabel);

    // 起始范围
    QHBoxLayout *startLayout = new QHBoxLayout();
    QLabel *startLabel = new QLabel(tr("起始码点 (0x):"), &dialog);
    QLineEdit *startEdit = new QLineEdit(&dialog);
    startEdit->setPlaceholderText(tr("例如: 4E00"));
    startLayout->addWidget(startLabel);
    startLayout->addWidget(startEdit);
    mainLayout->addLayout(startLayout);

    // 结束范围
    QHBoxLayout *endLayout = new QHBoxLayout();
    QLabel *endLabel = new QLabel(tr("结束码点 (0x):"), &dialog);
    QLineEdit *endEdit = new QLineEdit(&dialog);
    endEdit->setPlaceholderText(tr("例如: 9FA5"));
    endLayout->addWidget(endLabel);
    endLayout->addWidget(endEdit);
    mainLayout->addLayout(endLayout);

    // 示例提示
    QLabel *exampleLabel = new QLabel(
        tr("常用范围示例：\n"
           "• 基本拉丁字母: 0041-005A, 0061-007A\n"
           "• CJK统一汉字: 4E00-9FFF\n"
           "• 数字: 0030-0039"),
        &dialog);
    exampleLabel->setStyleSheet("color: gray; font-size: 9pt;");
    mainLayout->addWidget(exampleLabel);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton(tr("确定"), &dialog);
    QPushButton *cancelButton = new QPushButton(tr("取消"), &dialog);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 解析输入
    QString startStr = startEdit->text().trimmed();
    QString endStr = endEdit->text().trimmed();

    if (startStr.isEmpty() || endStr.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请输入起始和结束码点"));
        return;
    }

    bool startOk, endOk;
    uint startCode = startStr.toUInt(&startOk, 16);
    uint endCode = endStr.toUInt(&endOk, 16);

    if (!startOk || !endOk) {
        QMessageBox::warning(this, tr("错误"), tr("无效的十六进制数值"));
        return;
    }

    if (startCode > endCode) {
        QMessageBox::warning(this, tr("错误"), tr("起始码点不能大于结束码点"));
        return;
    }

    if (endCode - startCode > 10000) {
        QMessageBox::warning(this, tr("警告"), tr("范围过大（超过10000个字符），请缩小范围"));
        return;
    }

    // 插入Unicode范围格式
    QString rangeText = QString("[0x%1-0x%2]")
                            .arg(startCode, 4, 16, QChar('0'))
                            .arg(endCode, 4, 16, QChar('0'))
                            .toUpper();
    insertPlainText(rangeText);
}

void CharacterInputWidget::removeDigits()
{
    // 简化：用户可以直接编辑删除
}

void CharacterInputWidget::removeUppercaseLetters()
{
    // 简化：用户可以直接编辑删除
}

void CharacterInputWidget::removeLowercaseLetters()
{
    // 简化：用户可以直接编辑删除
}

void CharacterInputWidget::removeChinese()
{
    // 简化：用户可以直接编辑删除
}

void CharacterInputWidget::removeDuplicates()
{
    // 不再需要，解析器会自动去重
}

void CharacterInputWidget::clearAll()
{
    clear();  // 清空文本框
}
