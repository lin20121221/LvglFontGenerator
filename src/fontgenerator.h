#ifndef FONTGENERATOR_H
#define FONTGENERATOR_H

#include <QObject>
#include <QString>
#include <QFont>
#include <QImage>

class FontGenerator : public QObject
{
    Q_OBJECT

public:
    struct Config {
        QString fontPath;
        int fontSize;
        QString characters;
        QString outputName;
        int lvglVersion;
        bool isExternal;
        QString outputDir;
        int bpp;
        bool enableKerning;  // 是否启用Kerning输出，默认false
    };

    explicit FontGenerator(QObject *parent = nullptr);
    ~FontGenerator();

    bool generate(const Config &config);
    QString lastError() const { return m_lastError; }

signals:
    void progressChanged(int value);

private:
    struct GlyphData {
        QChar character;
        QImage bitmap;
        int advanceWidth;
        int bearingX;
        int bearingY;
        int width;
        int height;
    };

    bool loadFont(const QString &fontPath, int fontSize);
    bool renderGlyphs(const QString &characters, const QString &fontPath, int fontSize);
    bool exportToC(const Config &config, const QString &fontPath);
    bool exportToBin(const Config &config);

    QString generateCHeader(const Config &config);
    QString generateCSource(const Config &config);
    QString generateLvgl8Format(const Config &config);
    QString generateLvgl9Format(const Config &config);

    QByteArray generateBitmapData();
    QByteArray generateGlyphDescriptors();

    void removeDuplicateCharacters(QString &characters);

    QFont m_font;
    QList<GlyphData> m_glyphs;
    QString m_lastError;
};

#endif // FONTGENERATOR_H
