#include "characterinputwidget.h"
#include <QContextMenuEvent>
#include <QSet>

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
    // 常用汉字3500字（国家语委现代汉语常用字表）
    // 这里提供一级常用字（2500字）的一部分作为示例
    return "的一是在不了有和人这中大为上个国我以要他时来用们生到作地于出就分对成会可主发年动同工也能下过子说产种面而方后多定行学法所民得经十三之进着等部度家电力里如水化高自二理起小物现实加量都两体制机当使点从业本去把性好应开它合还因由其些然前外天政四日那社义事平形相全表间样与关各重新线内数正心反你明看原又么利比或但质气第向道命此变条只没结解问意建月公无系军很情者最立代想已通并提直题党程展五果料象员革位入常文总次品式活设及管特件长求老头基资边流路级少图山统接知较将组见计别她手角期根论运农指几九区强放决西被干做必战先回则任取据处队南给色光门即保治北造百规热领七海口东导器压志世金增争济阶油思术极交受联什认六共权收证改清己美再采转更单风切打白教速花带安场身车例真务具万每目至达走积示议声报斗完类八离华名确才科张信马节话米整空元况今集温传土许步群广石记需段研界拉林律叫且究观越织装影算低持音众书布复容儿须际商非验连断深难近矿千周委素技备半办青省列习响约支般史感劳便团往酸历市克何除消构府称太准精值号率族维划选标写存候毛亲快效斯院查江型眼王按格养易置派层片始却专状育厂京识适属圆包火住调满县局照参红细引听该铁价严";
}
