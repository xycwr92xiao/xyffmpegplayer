#ifndef SUBTITLE_H
#define SUBTITLE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QTime>
#include <QFile>
#include <QTextStream>

struct SubtitleItem {
    QTime startTime;
    QTime endTime;
    QString text;
    bool operator<(const SubtitleItem& other) const {
        return startTime < other.startTime;
    }
};

class Subtitle : public QObject
{
    Q_OBJECT
public:
    explicit Subtitle(QObject *parent = nullptr);

    bool loadSubtitle(const QString& mediaFilePath);
    QString getCurrentSubtitle(const QTime& currentTime);
    void clear();

    bool hasSubtitles() const { return !m_subtitles.isEmpty(); }
    bool loadSubtitleFile(const QString& subtitleFilePath);
private:
    bool loadSrtFile(const QString& filePath);
    bool loadLrcFile(const QString& filePath);
    bool loadTxtFile(const QString& filePath);
    QTime parseTimeString(const QString& timeStr, const QString& format);
    // 添加编码检测函数
    QString detectAndConvertEncoding(const QByteArray &data);
    // 尝试多种编码
    QString tryDecode(const QByteArray &data, const char *codecName);
    QVector<SubtitleItem> m_subtitles;
    QString m_currentSubtitle;
    void processComplexLrcLine(const QString& line, QVector<SubtitleItem>& subtitles);
};

#endif // SUBTITLE_H
