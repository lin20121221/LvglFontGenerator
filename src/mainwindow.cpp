#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDatabase>
#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QSettings>
#include <QApplication>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fontGenerator(new FontGenerator(this))
    , characterGridWidget(new CharacterGridWidget(this))
    , currentFontId(-1)
    , isEnglish(false)
{
    ui->setupUi(this);

    // 加载设置
    QSettings settings("LvglTools", "LvglFontGenerator");
    isEnglish = settings.value("language", "zh").toString() == "en";
    lastFontDirectory = settings.value("lastFontDirectory", QApplication::applicationDirPath()).toString();

    setupUi();

    connect(ui->btnSelectFont, &QPushButton::clicked, this, &MainWindow::onSelectFontFile);
    connect(ui->btnSystemFont, &QPushButton::clicked, this, &MainWindow::onSelectSystemFont);
    connect(ui->btnGenerate, &QPushButton::clicked, this, &MainWindow::onGenerate);
    connect(ui->comboFontType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFontTypeChanged);
    connect(ui->textCharacters, &QPlainTextEdit::textChanged,
            this, &MainWindow::onCharactersChanged);
    connect(ui->lineOutputName, &QLineEdit::textChanged,
            this, [this]() { ui->btnGenerate->setEnabled(validateInputs()); });
    connect(ui->spinFontSize, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFontSizeChanged);
    connect(ui->comboBpp, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onBppChanged);
    connect(ui->actionUsage, &QAction::triggered,
            this, &MainWindow::onShowUsage);
    connect(ui->actionAbout, &QAction::triggered,
            this, &MainWindow::onShowAbout);
    connect(ui->actionSwitchLanguage, &QAction::triggered,
            this, &MainWindow::onSwitchLanguage);
    connect(ui->actionLoadConfig, &QAction::triggered,
            this, &MainWindow::onLoadConfig);
    connect(ui->actionSaveConfig, &QAction::triggered,
            this, &MainWindow::onSaveConfig);
    connect(characterGridWidget, &CharacterGridWidget::characterDoubleClicked,
            this, &MainWindow::onCharacterDoubleClicked);
    connect(characterGridWidget, &CharacterGridWidget::renderProgress,
            this, [this](int current, int total) {
                if (total > 0) {
                    int percent = (current * 100) / total;
                    ui->progressBar->setValue(percent);
                    if (current == total) {
                        // 渲染完成后清空进度条
                        QTimer::singleShot(500, this, [this]() {
                            ui->progressBar->setValue(0);
                        });
                    }
                }
            });

    retranslateUi();
}

MainWindow::~MainWindow()
{
    if (currentFontId != -1) {
        QFontDatabase::removeApplicationFont(currentFontId);
    }
    delete ui;
}

void MainWindow::setupUi()
{
    setWindowTitle("LVGL字体生成工具");
    resize(1200, 700);

    ui->comboFontType->addItem("内部字体");
    ui->comboFontType->addItem("外部字体");

    ui->comboBpp->addItem("1位 (无抗锯齿 - 纯黑白)");
    ui->comboBpp->addItem("2位 (4级灰度)");
    ui->comboBpp->addItem("4位 (16级灰度)");
    ui->comboBpp->addItem("8位 (256级灰度)");
    ui->comboBpp->setCurrentIndex(2);

    ui->spinFontSize->setRange(8, 128);
    ui->spinFontSize->setValue(16);

    ui->textCharacters->setPlaceholderText("请输入需要生成的字符，例如：\n"
                                           "0123456789\n"
                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
                                           "abcdefghijklmnopqrstuvwxyz\n"
                                           "你好世界");

    ui->lineOutputName->setPlaceholderText("my_font");

    // 直接添加字符网格到预览布局，不使用选项卡
    ui->previewLayout->addWidget(characterGridWidget);

    ui->btnGenerate->setEnabled(false);
}

void MainWindow::onSelectFontFile()
{
    // 使用上次选择的目录，如果没有则使用应用程序目录
    QString initialDir = lastFontDirectory;

    QFileDialog dialog(this, isEnglish ? "Select Font File" : "选择字体文件", initialDir);
    dialog.setNameFilter(isEnglish ? "Font Files (*.ttf *.otf *.ttc);;All Files (*.*)"
                                   : "字体文件 (*.ttf *.otf *.ttc);;所有文件 (*.*)");
    dialog.setFileMode(QFileDialog::ExistingFile);

    // 添加常用字体目录到侧边栏
#ifdef Q_OS_WIN
    QList<QUrl> urls = dialog.sidebarUrls();

    QString windowsFontsDir = "C:/Windows/Fonts";
    QString userFontsDir = QDir::homePath() + "/AppData/Local/Microsoft/Windows/Fonts";

    // 添加系统字体目录
    if (QDir(windowsFontsDir).exists()) {
        urls.append(QUrl::fromLocalFile(windowsFontsDir));
    }

    // 添加用户字体目录
    if (QDir(userFontsDir).exists()) {
        urls.append(QUrl::fromLocalFile(userFontsDir));
    }

    // 添加常用位置
    urls.append(QUrl::fromLocalFile(QDir::homePath()));
    urls.append(QUrl::fromLocalFile(QDir::homePath() + "/Desktop"));

    dialog.setSidebarUrls(urls);
#endif

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QStringList files = dialog.selectedFiles();
    if (files.isEmpty()) {
        return;
    }

    QString fileName = files.first();
    currentFontPath = fileName;
    ui->lineFontPath->setText(fileName);

    // 保存选择的目录
    QFileInfo fileInfo(fileName);
    lastFontDirectory = fileInfo.absolutePath();
    QSettings settings("LvglTools", "LvglFontGenerator");
    settings.setValue("lastFontDirectory", lastFontDirectory);

    // 移除之前加载的字体（如果有）
    if (currentFontId != -1) {
        QFontDatabase::removeApplicationFont(currentFontId);
    }

    currentFontId = QFontDatabase::addApplicationFont(fileName);
    if (currentFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(currentFontId);
        if (!fontFamilies.isEmpty()) {
            currentFont = QFont(fontFamilies.first(), ui->spinFontSize->value());
            currentFont.setPixelSize(ui->spinFontSize->value());
            updateFontInfo();
            updateCharacterGrid();
        } else {
            QMessageBox::warning(this, isEnglish ? "Error" : "错误",
                               isEnglish ? "Unable to read font information from file"
                                         : "无法从字体文件中读取字体信息");
        }
    } else {
        QMessageBox::warning(this, isEnglish ? "Error" : "错误",
                           isEnglish ? "Unable to load font file"
                                     : "无法加载字体文件");
    }

    if (validateInputs()) {
        ui->btnGenerate->setEnabled(true);
    }
}

void MainWindow::onSelectSystemFont()
{
    // 创建系统字体选择对话框
    QDialog dialog(this);
    dialog.setWindowTitle(isEnglish ? "Select System Font" : "选择系统字体");
    dialog.resize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 搜索框
    QLineEdit *searchBox = new QLineEdit(&dialog);
    searchBox->setPlaceholderText(isEnglish ? "Search fonts..." : "搜索字体...");
    layout->addWidget(searchBox);

    // 字体列表
    QListWidget *fontList = new QListWidget(&dialog);
    layout->addWidget(fontList);

    // 获取系统所有字体
    QStringList fontFamilies = QFontDatabase::families();

    // 填充字体列表
    for (const QString &family : fontFamilies) {
        fontList->addItem(family);
    }

    // 搜索功能
    connect(searchBox, &QLineEdit::textChanged, [fontList, fontFamilies](const QString &text) {
        fontList->clear();
        if (text.isEmpty()) {
            fontList->addItems(fontFamilies);
        } else {
            for (const QString &family : fontFamilies) {
                if (family.contains(text, Qt::CaseInsensitive)) {
                    fontList->addItem(family);
                }
            }
        }
    });

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton(isEnglish ? "OK" : "确定", &dialog);
    QPushButton *cancelButton = new QPushButton(isEnglish ? "Cancel" : "取消", &dialog);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(fontList, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted || !fontList->currentItem()) {
        return;
    }

    QString selectedFamily = fontList->currentItem()->text();

    // 查找字体文件路径
    QString fontPath;
    QStringList fontDirs;

#ifdef Q_OS_WIN
    fontDirs << "C:/Windows/Fonts"
             << QDir::homePath() + "/AppData/Local/Microsoft/Windows/Fonts";
#endif

    // 尝试通过QFontDatabase获取字体样式
    QStringList styles = QFontDatabase::styles(selectedFamily);
    QString style = styles.isEmpty() ? "Regular" : styles.first();

    // 搜索字体文件
    for (const QString &dir : fontDirs) {
        QDir fontDir(dir);
        if (!fontDir.exists()) continue;

        QStringList filters;
        filters << "*.ttf" << "*.otf" << "*.ttc";
        QFileInfoList files = fontDir.entryInfoList(filters, QDir::Files);

        for (const QFileInfo &fileInfo : files) {
            int id = QFontDatabase::addApplicationFont(fileInfo.absoluteFilePath());
            if (id != -1) {
                QStringList families = QFontDatabase::applicationFontFamilies(id);
                if (families.contains(selectedFamily)) {
                    fontPath = fileInfo.absoluteFilePath();
                    QFontDatabase::removeApplicationFont(id);
                    break;
                }
                QFontDatabase::removeApplicationFont(id);
            }
        }

        if (!fontPath.isEmpty()) break;
    }

    if (fontPath.isEmpty()) {
        QMessageBox::warning(this, isEnglish ? "Error" : "错误",
                           isEnglish ? QString("Cannot find font file for: %1").arg(selectedFamily)
                                     : QString("无法找到字体文件: %1").arg(selectedFamily));
        return;
    }

    // 加载字体
    currentFontPath = fontPath;
    ui->lineFontPath->setText(fontPath);

    // 移除之前加载的字体
    if (currentFontId != -1) {
        QFontDatabase::removeApplicationFont(currentFontId);
    }

    currentFontId = QFontDatabase::addApplicationFont(fontPath);
    if (currentFontId != -1) {
        currentFont = QFont(selectedFamily, ui->spinFontSize->value());
        currentFont.setPixelSize(ui->spinFontSize->value());
        updateFontInfo();
        updateCharacterGrid();
    } else {
        QMessageBox::warning(this, isEnglish ? "Error" : "错误",
                           isEnglish ? "Unable to load font file"
                                     : "无法加载字体文件");
    }

    if (validateInputs()) {
        ui->btnGenerate->setEnabled(true);
    }
}

void MainWindow::updateFontInfo()
{
    if (currentFont.family().isEmpty()) {
        ui->labelFontInfo->setText(isEnglish ? "Font Info: Not Selected" : "字体信息: 未选择");
        return;
    }

    QFontInfo info(currentFont);
    QString styleName = info.styleName();
    if (styleName.isEmpty()) {
        styleName = "Regular";
    }

    ui->labelFontInfo->setText(
        isEnglish ? QString("Font: %1 | Size: %2px | Style: %3")
                        .arg(info.family())
                        .arg(currentFont.pixelSize())
                        .arg(styleName)
                  : QString("字体: %1 | 大小: %2px | 样式: %3")
                        .arg(info.family())
                        .arg(currentFont.pixelSize())
                        .arg(styleName));
}

void MainWindow::onGenerate()
{
    if (!validateInputs()) {
        return;
    }

    QString outputDir = QFileDialog::getExistingDirectory(
        this,
        "选择输出目录",
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (outputDir.isEmpty()) {
        return;
    }

    FontGenerator::Config config;
    config.fontPath = currentFontPath;
    config.fontSize = ui->spinFontSize->value();
    config.characters = ui->textCharacters->getAllCharacters();  // 使用新方法获取所有字符
    config.outputName = ui->lineOutputName->text();
    config.lvglVersion = 9;  // 生成兼容LVGL 8.x和9.x的代码
    config.isExternal = ui->comboFontType->currentIndex() == 1;
    config.outputDir = outputDir;
    config.enableKerning = ui->checkEnableKerning->isChecked();

    int bppIndex = ui->comboBpp->currentIndex();
    config.bpp = (bppIndex == 0) ? 1 : (bppIndex == 1) ? 2 : (bppIndex == 2) ? 4 : 8;

    ui->btnGenerate->setEnabled(false);
    ui->progressBar->setValue(0);

    bool success = fontGenerator->generate(config);

    ui->progressBar->setValue(100);
    ui->btnGenerate->setEnabled(true);

    if (success) {
        QString cFilePath = outputDir + "/" + config.outputName + ".c";
        QString message;
        if (config.isExternal) {
            QString binFilePath = outputDir + "/" + config.outputName + ".bin";
            message = isEnglish
                ? QString("Font generated successfully!\nGenerated files:\n%1\n%2").arg(cFilePath, binFilePath)
                : QString("字体生成成功！\n生成文件:\n%1\n%2").arg(cFilePath, binFilePath);
        } else {
            message = isEnglish
                ? QString("Font generated successfully!\nGenerated file:\n%1").arg(cFilePath)
                : QString("字体生成成功！\n生成文件:\n%1").arg(cFilePath);
        }
        QMessageBox::information(this, isEnglish ? "Success" : "成功", message);
    } else {
        QMessageBox::critical(this, isEnglish ? "Error" : "错误",
            isEnglish ? QString("Font generation failed: %1").arg(fontGenerator->lastError())
                      : QString("字体生成失败: %1").arg(fontGenerator->lastError()));
    }
}

void MainWindow::onFontTypeChanged(int index)
{
    bool isExternal = (index == 1);
    QString info = isEnglish
        ? (isExternal ? "External Font: Generate .c and .bin files, requires external FLASH"
                      : "Internal Font: Generate .c file, compile directly into program")
        : (isExternal ? "外部字体: 生成.c和.bin文件，需要外部FLASH支持"
                      : "内部字体: 生成.c文件，直接编译到程序中");
    ui->labelFontTypeInfo->setText(info);
}

void MainWindow::onCharactersChanged()
{
    int charCount = ui->textCharacters->getCharacterCount();

    ui->labelCharCount->setText(
        isEnglish ? QString("Character Count: %1").arg(charCount)
                  : QString("字符数: %1").arg(charCount));

    // 更新字符网格
    updateCharacterGrid();

    // 更新按钮状态
    ui->btnGenerate->setEnabled(validateInputs());
}

void MainWindow::onFontSizeChanged(int size)
{
    if (!currentFont.family().isEmpty()) {
        currentFont.setPixelSize(size);
        updateFontInfo();
        updateCharacterGrid();
    }
}

void MainWindow::onBppChanged(int index)
{
    Q_UNUSED(index);
    updateCharacterGrid();
}

void MainWindow::onShowUsage()
{
    QString usageText = isEnglish ? getUsageTextEnglish() : getUsageTextChinese();

    // 创建自定义对话框，使用QTextBrowser支持滚动
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(isEnglish ? "Usage Guide" : "使用说明");
    dialog->resize(700, 600);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QTextBrowser *textBrowser = new QTextBrowser(dialog);
    textBrowser->setHtml(usageText);
    textBrowser->setOpenExternalLinks(true);
    layout->addWidget(textBrowser);

    QPushButton *closeButton = new QPushButton(isEnglish ? "Close" : "关闭", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);

    dialog->exec();
    delete dialog;
}

QString MainWindow::getUsageTextChinese()
{
    return
        "<h2>LVGL字体生成工具 - 使用说明</h2>"

        "<h3>1. 基本流程</h3>"
        "<ol>"
        "<li><b>选择字体文件</b>：点击\"选择...\"按钮，选择TTF/OTF/TTC字体文件</li>"
        "<li><b>设置字体大小</b>：调整字体大小（8-128像素）</li>"
        "<li><b>输入字符</b>：在字符设置框中输入需要生成的字符</li>"
        "<li><b>配置选项</b>：选择LVGL版本、字体类型、抗锯齿级别</li>"
        "<li><b>生成字体</b>：点击\"生成字体\"按钮，选择输出目录</li>"
        "</ol>"

        "<h3>2. 字符输入技巧</h3>"
        "<p><b>右键菜单快速添加：</b></p>"
        "<ul>"
        "<li>数字 (0-9)</li>"
        "<li>大写字母 (A-Z)</li>"
        "<li>小写字母 (a-z)</li>"
        "<li>常用中文 (3500字)</li>"
        "<li>基本标点符号</li>"
        "<li>所有ASCII可见字符</li>"
        "</ul>"
        "<p><b>右键菜单快速删除：</b></p>"
        "<ul>"
        "<li>删除所有数字/大写字母/小写字母/中文</li>"
        "<li>清除重复字符</li>"
        "<li>清空全部</li>"
        "</ul>"

        "<h3>3. 抗锯齿选项</h3>"
        "<table border='1' cellpadding='5' cellspacing='0'>"
        "<tr><th>选项</th><th>说明</th><th>适用场景</th></tr>"
        "<tr><td>1位</td><td>无抗锯齿，纯黑白</td><td>单色OLED、墨水屏</td></tr>"
        "<tr><td>2位</td><td>4级灰度</td><td>低灰度屏幕</td></tr>"
        "<tr><td>4位</td><td>16级灰度</td><td>灰度屏幕</td></tr>"
        "<tr><td>8位</td><td>256级灰度</td><td>彩色TFT屏（推荐）</td></tr>"
        "</table>"

        "<h3>4. 字体预览</h3>"
        "<ul>"
        "<li><b>输入预览文本</b>：在\"预览文本\"框中输入要预览的字符</li>"
        "<li><b>缩放</b>：使用+/-按钮或鼠标滚轮缩放预览</li>"
        "<li><b>平移</b>：鼠标左键拖拽移动预览</li>"
        "<li><b>栅格</b>：勾选\"显示栅格\"查看像素细节</li>"
        "<li><b>右键菜单</b>：居中显示或重置位置</li>"
        "</ul>"

        "<h3>5. 字体类型</h3>"
        "<p><b>内部字体：</b>生成.c文件，直接编译到程序中，无需外部存储</p>"
        "<p><b>外部字体：</b>生成.c和.bin文件，.bin需烧录到外部FLASH，节省内部存储空间</p>"

        "<h3>6. 外部字体集成</h3>"
        "<p>使用外部字体时，需要实现<code>user_font_get_data</code>函数：</p>"
        "<pre>"
        "const uint8_t *user_font_get_data(uint32_t offset, uint32_t size) {\n"
        "    static uint8_t buffer[256];\n"
        "    external_flash_read(FONT_BASE_ADDR + offset, buffer, size);\n"
        "    return buffer;\n"
        "}"
        "</pre>"

        "<h3>7. 在LVGL中使用</h3>"
        "<pre>"
        "// 声明字体\n"
        "LV_FONT_DECLARE(my_font);\n\n"
        "// 使用字体\n"
        "lv_obj_t *label = lv_label_create(lv_scr_act());\n"
        "lv_obj_set_style_text_font(label, &amp;my_font, 0);\n"
        "lv_label_set_text(label, \"Hello LVGL!\");"
        "</pre>"

        "<h3>8. Windows字体目录问题</h3>"
        "<p>如果无法访问<code>C:\\Windows\\Fonts</code>目录：</p>"
        "<ul>"
        "<li>使用文件对话框侧边栏的快捷方式</li>"
        "<li>将字体文件复制到其他目录</li>"
        "<li>从网络下载开源字体（如Google Fonts）</li>"
        "</ul>"

        "<p style='margin-top:20px;'><b>提示：</b>更多详细信息请查看项目目录中的README.md和USAGE_GUIDE.md文件。</p>";
}

QString MainWindow::getUsageTextEnglish()
{
    return
        "<h2>LVGL Font Generator - Usage Guide</h2>"

        "<h3>1. Basic Workflow</h3>"
        "<ol>"
        "<li><b>Select Font File</b>: Click \"Browse...\" button to select TTF/OTF/TTC font file</li>"
        "<li><b>Set Font Size</b>: Adjust font size (8-128 pixels)</li>"
        "<li><b>Input Characters</b>: Enter characters to generate in the character settings box</li>"
        "<li><b>Configure Options</b>: Select LVGL version, font type, and antialiasing level</li>"
        "<li><b>Generate Font</b>: Click \"Generate Font\" button and select output directory</li>"
        "</ol>"

        "<h3>2. Character Input Tips</h3>"
        "<p><b>Right-click menu quick add:</b></p>"
        "<ul>"
        "<li>Digits (0-9)</li>"
        "<li>Uppercase letters (A-Z)</li>"
        "<li>Lowercase letters (a-z)</li>"
        "<li>Common Chinese characters (3500 chars)</li>"
        "<li>Basic punctuation</li>"
        "<li>All ASCII printable characters</li>"
        "</ul>"
        "<p><b>Right-click menu quick remove:</b></p>"
        "<ul>"
        "<li>Remove all digits/uppercase/lowercase/Chinese</li>"
        "<li>Remove duplicates</li>"
        "<li>Clear all</li>"
        "</ul>"

        "<h3>3. Antialiasing Options</h3>"
        "<table border='1' cellpadding='5' cellspacing='0'>"
        "<tr><th>Option</th><th>Description</th><th>Use Case</th></tr>"
        "<tr><td>1-bit</td><td>No antialiasing, pure B&amp;W</td><td>Monochrome OLED, E-ink</td></tr>"
        "<tr><td>2-bit</td><td>4 gray levels</td><td>Low grayscale displays</td></tr>"
        "<tr><td>4-bit</td><td>16 gray levels</td><td>Grayscale displays</td></tr>"
        "<tr><td>8-bit</td><td>256 gray levels</td><td>Color TFT (Recommended)</td></tr>"
        "</table>"

        "<h3>4. Font Preview</h3>"
        "<ul>"
        "<li><b>Input preview text</b>: Enter characters in \"Preview Text\" box</li>"
        "<li><b>Zoom</b>: Use +/- buttons or mouse wheel to zoom</li>"
        "<li><b>Pan</b>: Left-click and drag to move preview</li>"
        "<li><b>Grid</b>: Check \"Show Grid\" to view pixel details</li>"
        "<li><b>Right-click menu</b>: Center view or reset position</li>"
        "</ul>"

        "<h3>5. Font Types</h3>"
        "<p><b>Internal Font:</b> Generates .c file, compiled directly into program, no external storage needed</p>"
        "<p><b>External Font:</b> Generates .c and .bin files, .bin needs to be flashed to external FLASH, saves internal storage</p>"

        "<h3>6. External Font Integration</h3>"
        "<p>When using external fonts, implement <code>user_font_get_data</code> function:</p>"
        "<pre>"
        "const uint8_t *user_font_get_data(uint32_t offset, uint32_t size) {\n"
        "    static uint8_t buffer[256];\n"
        "    external_flash_read(FONT_BASE_ADDR + offset, buffer, size);\n"
        "    return buffer;\n"
        "}"
        "</pre>"

        "<h3>7. Using in LVGL</h3>"
        "<pre>"
        "// Declare font\n"
        "LV_FONT_DECLARE(my_font);\n\n"
        "// Use font\n"
        "lv_obj_t *label = lv_label_create(lv_scr_act());\n"
        "lv_obj_set_style_text_font(label, &amp;my_font, 0);\n"
        "lv_label_set_text(label, \"Hello LVGL!\");"
        "</pre>"

        "<h3>8. Windows Fonts Directory Issue</h3>"
        "<p>If unable to access <code>C:\\Windows\\Fonts</code> directory:</p>"
        "<ul>"
        "<li>Use shortcuts in file dialog sidebar</li>"
        "<li>Copy font files to another directory</li>"
        "<li>Download open-source fonts from web (e.g., Google Fonts)</li>"
        "</ul>"

        "<p style='margin-top:20px;'><b>Tip:</b> For more details, see README.md and USAGE_GUIDE.md in project directory.</p>";
}

void MainWindow::onShowAbout()
{
    QString aboutText =
        "<h2>LVGL字体生成工具</h2>"
        "<p><b>版本：</b>1.0.0</p>"
        "<p><b>描述：</b>基于Qt6的LVGL字体生成工具，生成的字体文件兼容LVGL 8.x和9.x版本</p>"
        "<h3>主要功能</h3>"
        "<ul>"
        "<li>支持TTF、OTF、TTC等常见字体格式</li>"
        "<li>支持内部字体和外部字体两种模式</li>"
        "<li>4种抗锯齿级别（1/2/4/8位）</li>"
        "<li>实时字体预览，支持缩放和栅格显示</li>"
        "<li>右键菜单快速添加/删除字符集</li>"
        "<li>自动去除重复字符</li>"
        "</ul>"
        "<h3>技术栈</h3>"
        "<ul>"
        "<li>Qt 6.11.0</li>"
        "<li>C++17</li>"
        "<li>CMake 3.16+</li>"
        "</ul>"
        "<p style='margin-top:20px;'><b>许可证：</b>MIT License</p>"
        "<p><b>参考：</b><a href='https://docs.lvgl.io/'>LVGL官方文档</a></p>";

    QMessageBox::about(this, isEnglish ? "About" : "关于", aboutText);
}

QString MainWindow::getAboutTextChinese()
{
    return
        "<h2>LVGL字体生成工具</h2>"
        "<p><b>版本：</b>1.0.0</p>"
        "<p><b>描述：</b>基于Qt6的LVGL字体生成工具，生成的字体文件兼容LVGL 8.x和9.x版本</p>"
        "<h3>主要功能</h3>"
        "<ul>"
        "<li>支持TTF、OTF、TTC等常见字体格式</li>"
        "<li>支持内部字体和外部字体两种模式</li>"
        "<li>4种抗锯齿级别（1/2/4/8位）</li>"
        "<li>实时字体预览，支持缩放和栅格显示</li>"
        "<li>右键菜单快速添加/删除字符集</li>"
        "<li>自动去除重复字符</li>"
        "<li>中英文双语界面</li>"
        "</ul>"
        "<h3>技术栈</h3>"
        "<ul>"
        "<li>Qt 6.11.0</li>"
        "<li>C++17</li>"
        "<li>CMake 3.16+</li>"
        "</ul>"
        "<p style='margin-top:20px;'><b>许可证：</b>MIT License</p>"
        "<p><b>参考：</b><a href='https://docs.lvgl.io/'>LVGL官方文档</a></p>";
}

QString MainWindow::getAboutTextEnglish()
{
    return
        "<h2>LVGL Font Generator</h2>"
        "<p><b>Version:</b> 1.0.0</p>"
        "<p><b>Description:</b> Qt6-based LVGL font generator, generates font files compatible with LVGL 8.x and 9.x</p>"
        "<h3>Main Features</h3>"
        "<ul>"
        "<li>Supports TTF, OTF, TTC and other common font formats</li>"
        "<li>Supports internal and external font modes</li>"
        "<li>4 antialiasing levels (1/2/4/8-bit)</li>"
        "<li>Real-time font preview with zoom and grid display</li>"
        "<li>Right-click menu for quick character set add/remove</li>"
        "<li>Automatic duplicate character removal</li>"
        "<li>Bilingual interface (Chinese/English)</li>"
        "</ul>"
        "<h3>Technology Stack</h3>"
        "<ul>"
        "<li>Qt 6.11.0</li>"
        "<li>C++17</li>"
        "<li>CMake 3.16+</li>"
        "</ul>"
        "<p style='margin-top:20px;'><b>License:</b> MIT License</p>"
        "<p><b>Reference:</b> <a href='https://docs.lvgl.io/'>LVGL Official Documentation</a></p>";
}

void MainWindow::onSwitchLanguage()
{
    isEnglish = !isEnglish;

    // 保存语言设置
    QSettings settings("LvglTools", "LvglFontGenerator");
    settings.setValue("language", isEnglish ? "en" : "zh");

    // 更新界面
    retranslateUi();
}

void MainWindow::onLoadConfig()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        isEnglish ? "Load Configuration" : "载入配置",
        lastFontDirectory,
        isEnglish ? "Config Files (*.json);;All Files (*.*)" : "配置文件 (*.json);;所有文件 (*.*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    loadConfigFromFile(fileName);
}

void MainWindow::onSaveConfig()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        isEnglish ? "Save Configuration" : "保存配置",
        lastFontDirectory + "/font_config.json",
        isEnglish ? "Config Files (*.json);;All Files (*.*)" : "配置文件 (*.json);;所有文件 (*.*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    // 确保文件扩展名
    if (!fileName.endsWith(".json", Qt::CaseInsensitive)) {
        fileName += ".json";
    }

    saveConfigToFile(fileName);
}

void MainWindow::loadConfigFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, isEnglish ? "Error" : "错误",
                           isEnglish ? "Cannot open config file" : "无法打开配置文件");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, isEnglish ? "Error" : "错误",
                           isEnglish ? QString("Invalid config file: %1").arg(parseError.errorString())
                                     : QString("无效的配置文件: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject config = doc.object();

    // 载入字体路径
    if (config.contains("fontPath")) {
        QString fontPath = config["fontPath"].toString();
        if (QFile::exists(fontPath)) {
            currentFontPath = fontPath;
            ui->lineFontPath->setText(fontPath);

            // 移除之前加载的字体
            if (currentFontId != -1) {
                QFontDatabase::removeApplicationFont(currentFontId);
            }

            currentFontId = QFontDatabase::addApplicationFont(fontPath);
            if (currentFontId != -1) {
                QStringList fontFamilies = QFontDatabase::applicationFontFamilies(currentFontId);
                if (!fontFamilies.isEmpty()) {
                    int fontSize = config.contains("fontSize") ? config["fontSize"].toInt() : 16;
                    currentFont = QFont(fontFamilies.first(), fontSize);
                    currentFont.setPixelSize(fontSize);
                }
            }
        } else {
            QMessageBox::warning(this, isEnglish ? "Warning" : "警告",
                               isEnglish ? QString("Font file not found: %1").arg(fontPath)
                                         : QString("字体文件不存在: %1").arg(fontPath));
        }
    }

    // 载入字体大小
    if (config.contains("fontSize")) {
        ui->spinFontSize->setValue(config["fontSize"].toInt());
    }

    // 载入字体类型
    if (config.contains("fontType")) {
        ui->comboFontType->setCurrentIndex(config["fontType"].toInt());
    }

    // 载入BPP
    if (config.contains("bpp")) {
        int bppValue = config["bpp"].toInt();
        int index = 0;
        switch (bppValue) {
            case 1: index = 0; break;
            case 2: index = 1; break;
            case 4: index = 2; break;
            case 8: index = 3; break;
            default: index = 2; break;
        }
        ui->comboBpp->setCurrentIndex(index);
    }

    // 载入输出名称
    if (config.contains("outputName")) {
        ui->lineOutputName->setText(config["outputName"].toString());
    }

    // 载入Kerning设置
    if (config.contains("enableKerning")) {
        ui->checkEnableKerning->setChecked(config["enableKerning"].toBool());
    }

    // 载入字符集
    if (config.contains("characters")) {
        ui->textCharacters->setPlainText(config["characters"].toString());
    }

    // 更新UI
    updateFontInfo();
    updateCharacterGrid();

    QMessageBox::information(this, isEnglish ? "Success" : "成功",
                           isEnglish ? "Configuration loaded successfully" : "配置载入成功");
}

void MainWindow::saveConfigToFile(const QString &filePath)
{
    QJsonObject config;

    // 保存字体路径
    config["fontPath"] = currentFontPath;

    // 保存字体大小
    config["fontSize"] = ui->spinFontSize->value();

    // 保存字体类型
    config["fontType"] = ui->comboFontType->currentIndex();

    // 保存BPP
    int bppIndex = ui->comboBpp->currentIndex();
    int bpp = (bppIndex == 0) ? 1 : (bppIndex == 1) ? 2 : (bppIndex == 2) ? 4 : 8;
    config["bpp"] = bpp;

    // 保存输出名称
    config["outputName"] = ui->lineOutputName->text();

    // 保存Kerning设置
    config["enableKerning"] = ui->checkEnableKerning->isChecked();

    // 保存字符集
    config["characters"] = ui->textCharacters->toPlainText();

    // 写入文件
    QJsonDocument doc(config);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, isEnglish ? "Error" : "错误",
                           isEnglish ? "Cannot save config file" : "无法保存配置文件");
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, isEnglish ? "Success" : "成功",
                           isEnglish ? "Configuration saved successfully" : "配置保存成功");
}

void MainWindow::retranslateUi()
{
    // 更新窗口标题
    setWindowTitle(isEnglish ? "LVGL Font Generator" : "LVGL字体生成工具");

    // 更新菜单
    ui->menuFile->setTitle(isEnglish ? "File" : "文件");
    ui->menuHelp->setTitle(isEnglish ? "Help" : "帮助");
    ui->menuLanguage->setTitle(isEnglish ? "语言" : "Language");
    ui->actionExit->setText(isEnglish ? "Exit" : "退出");
    ui->actionLoadConfig->setText(isEnglish ? "Load Configuration..." : "载入配置...");
    ui->actionSaveConfig->setText(isEnglish ? "Save Configuration..." : "保存配置...");
    ui->actionUsage->setText(isEnglish ? "Usage Guide" : "使用说明");
    ui->actionAbout->setText(isEnglish ? "About" : "关于");
    ui->actionSwitchLanguage->setText(isEnglish ? "中文" : "English");

    // 更新分组框标题
    ui->groupFont->setTitle(isEnglish ? "Font Settings" : "字体设置");
    ui->groupCharacters->setTitle(isEnglish ? "Character Settings" : "字符设置");
    ui->groupConfig->setTitle(isEnglish ? "Generation Config" : "生成配置");
    ui->groupPreview->setTitle(isEnglish ? "Character Preview" : "字符预览");

    // 更新标签
    ui->labelFontFile->setText(isEnglish ? "Font File:" : "字体文件:");
    ui->labelFontSize->setText(isEnglish ? "Font Size:" : "字体大小:");
    ui->labelFontType->setText(isEnglish ? "Font Type:" : "字体类型:");
    ui->labelBpp->setText(isEnglish ? "Antialiasing:" : "抗锯齿:");
    ui->labelOutputName->setText(isEnglish ? "Output Name:" : "输出名称:");
    ui->labelKerning->setText(isEnglish ? "Kerning:" : "字距调整:");

    // 更新按钮
    ui->btnSelectFont->setText(isEnglish ? "Browse..." : "浏览...");
    ui->btnSystemFont->setText(isEnglish ? "System Fonts..." : "系统字体...");
    ui->btnGenerate->setText(isEnglish ? "Generate Font" : "生成字体");

    // 更新复选框
    ui->checkEnableKerning->setText(isEnglish ? "Enable Kerning" : "启用Kerning");
    ui->checkEnableKerning->setToolTip(isEnglish
        ? "Enable kerning to optimize spacing between specific character pairs for better appearance (disabled by default)"
        : "启用字距调整可以优化特定字符对之间的间距，使文本更美观（默认关闭）");

    // 更新组合框
    ui->comboFontType->clear();
    ui->comboFontType->addItem(isEnglish ? "Internal Font" : "内部字体");
    ui->comboFontType->addItem(isEnglish ? "External Font" : "外部字体");

    ui->comboBpp->clear();
    ui->comboBpp->addItem(isEnglish ? "1-bit (No Antialiasing - Pure B&W)" : "1位 (无抗锯齿 - 纯黑白)");
    ui->comboBpp->addItem(isEnglish ? "2-bit (4 Gray Levels)" : "2位 (4级灰度)");
    ui->comboBpp->addItem(isEnglish ? "4-bit (16 Gray Levels)" : "4位 (16级灰度)");
    ui->comboBpp->addItem(isEnglish ? "8-bit (256 Gray Levels)" : "8位 (256级灰度)");
    ui->comboBpp->setCurrentIndex(2);

    // 更新字符网格语言（如果需要的话）
    // characterGridWidget->setLanguage(isEnglish);

    // 更新字体信息
    updateFontInfo();

    // 更新字符数
    onCharactersChanged();

    // 更新字体类型信息
    onFontTypeChanged(ui->comboFontType->currentIndex());
}

void MainWindow::updateCharacterGrid()
{
    if (currentFontPath.isEmpty()) {
        return;
    }

    // 获取当前输入的所有字符
    QString characters = ui->textCharacters->getAllCharacters();

    // 设置字符列表
    characterGridWidget->setCharacters(characters);

    // 获取当前的BPP设置
    int bppIndex = ui->comboBpp->currentIndex();
    int bpp = (bppIndex == 0) ? 1 : (bppIndex == 1) ? 2 : (bppIndex == 2) ? 4 : 8;

    // 设置字体信息
    characterGridWidget->setFontInfo(currentFontPath, ui->spinFontSize->value(), bpp);

    // 刷新显示
    characterGridWidget->refreshDisplay();
}

void MainWindow::onCharacterDoubleClicked(QChar ch)
{
    // 当字符网格中的字符被双击时，可以添加其他操作
    // 例如：滚动到该字符在预览中的位置
    Q_UNUSED(ch);
}

bool MainWindow::validateInputs()
{
    if (currentFontPath.isEmpty()) {
        return false;
    }

    if (ui->textCharacters->getCharacterCount() == 0) {
        return false;
    }

    if (ui->lineOutputName->text().isEmpty()) {
        return false;
    }

    return true;
}
