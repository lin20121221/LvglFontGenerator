#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTranslator>
#include "fontgenerator.h"
#include "fontpreviewwidget.h"

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
    void onPreviewTextChanged();
    void onShowGridToggled(bool checked);
    void onKerningToggled(bool checked);
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();
    void onShowUsage();
    void onShowAbout();
    void onSwitchLanguage();

private:
    void setupUi();
    void updatePreview();
    bool validateInputs();
    void updateFontInfo();
    void retranslateUi();
    QString getUsageTextChinese();
    QString getUsageTextEnglish();
    QString getAboutTextChinese();
    QString getAboutTextEnglish();

    Ui::MainWindow *ui;
    FontGenerator *fontGenerator;
    FontPreviewWidget *previewWidget;
    QString currentFontPath;
    QString lastFontDirectory;
    QFont currentFont;
    int currentFontId;
    bool isEnglish;
};

#endif // MAINWINDOW_H
