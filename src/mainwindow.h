#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTranslator>
#include "fontgenerator.h"
#include "charactergridwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSelectFontFile();
    void onSelectSystemFont();
    void onGenerate();
    void onFontTypeChanged(int index);
    void onCharactersChanged();
    void onFontSizeChanged(int size);
    void onBppChanged(int index);
    void onShowUsage();
    void onShowAbout();
    void onSwitchLanguage();
    void onCharacterDoubleClicked(QChar ch);
    void onLoadConfig();
    void onSaveConfig();

private:
    void setupUi();
    void updateCharacterGrid();
    bool validateInputs();
    void updateFontInfo();
    void retranslateUi();
    void loadConfigFromFile(const QString &filePath);
    void saveConfigToFile(const QString &filePath);
    QString getUsageTextChinese();
    QString getUsageTextEnglish();
    QString getAboutTextChinese();
    QString getAboutTextEnglish();

    Ui::MainWindow *ui;
    FontGenerator *fontGenerator;
    CharacterGridWidget *characterGridWidget;
    QString currentFontPath;
    QString lastFontDirectory;
    QFont currentFont;
    int currentFontId;
    bool isEnglish;
};

#endif // MAINWINDOW_H
