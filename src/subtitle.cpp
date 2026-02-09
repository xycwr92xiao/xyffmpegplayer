#include "subtitle.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTextCodec>
#include <QRegularExpression>

Subtitle::Subtitle(QObject *parent) : QObject(parent)
{
}

bool Subtitle::loadSubtitle(const QString& mediaFilePath)
{
    clear();

    QFileInfo mediaFileInfo(mediaFilePath);
    QString baseName = mediaFileInfo.completeBaseName();
    QString dirPath = mediaFileInfo.absolutePath();

    QStringList subtitleExtensions = {".srt", ".lrc", ".txt"};

    for (const QString& ext : subtitleExtensions) {
        QString subtitlePath = dirPath + "/" + baseName + ext;

        if (QFile::exists(subtitlePath)) {
            qDebug() << "找到字幕文件:" << subtitlePath;

            if (ext == ".srt") {
                return loadSrtFile(subtitlePath);
            } else if (ext == ".lrc" || ext == ".txt") {
                return loadLrcFile(subtitlePath);
            } else if (ext == ".txt") {
                return loadTxtFile(subtitlePath);
            }
        }
    }

    return false;
}
bool Subtitle::loadSubtitleFile(const QString& subtitleFilePath)
{
    clear();

    QFileInfo fileInfo(subtitleFilePath);
    QString suffix = fileInfo.suffix().toLower();

    if (suffix == "srt") {
        return loadSrtFile(subtitleFilePath);
    } else if (suffix == "lrc") {
        return loadLrcFile(subtitleFilePath);
    } else if (suffix == "txt") {
        return loadTxtFile(subtitleFilePath);
    }

    return false;
}
bool Subtitle::loadSrtFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    QVector<SubtitleItem> subtitles;
    QString line;
    bool readingText = false;
    SubtitleItem currentItem;
    QString currentText;

    while (!in.atEnd()) {
        line = in.readLine();

        if (line.trimmed().isEmpty()) {
            if (!currentText.isEmpty()) {
                currentItem.text = currentText.trimmed();
                subtitles.append(currentItem);
                currentText.clear();
                readingText = false;
            }
            continue;
        }

        if (!readingText) {
            // 尝试解析时间轴行
            QRegularExpression timeRegex(R"((\d{2}):(\d{2}):(\d{2}),(\d{3})\s*-->\s*(\d{2}):(\d{2}):(\d{2}),(\d{3}))");
            QRegularExpressionMatch match = timeRegex.match(line);

            if (match.hasMatch()) {
                QTime startTime = QTime(
                    match.captured(1).toInt(),
                    match.captured(2).toInt(),
                    match.captured(3).toInt(),
                    match.captured(4).toInt()
                );

                QTime endTime = QTime(
                    match.captured(5).toInt(),
                    match.captured(6).toInt(),
                    match.captured(7).toInt(),
                    match.captured(8).toInt()
                );

                currentItem.startTime = startTime;
                currentItem.endTime = endTime;
                readingText = true;
            }
        } else {
            if (!currentText.isEmpty()) {
                currentText += "\n";
            }
            currentText += line;
        }
    }

    // 添加最后一个字幕项
    if (!currentText.isEmpty()) {
        currentItem.text = currentText.trimmed();
        subtitles.append(currentItem);
    }

    file.close();

    if (!subtitles.isEmpty()) {
        m_subtitles = subtitles;
        qDebug() << "加载SRT字幕成功，共" << m_subtitles.size() << "条";
        return true;
    }

    return false;
}

bool Subtitle::loadLrcFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开LRC文件:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        qDebug() << "LRC文件为空:" << filePath;
        return false;
    }

    QString text = detectAndConvertEncoding(data);

    QTextStream in(&text);
    QVector<SubtitleItem> subtitles;
    QString line;

    while (!in.atEnd()) {
            line = in.readLine().trimmed();

            if (line.isEmpty()) {
                continue;
            }
            // 跳过元数据行（不以时间标签开头）
            if (!line.startsWith('[') || (line.contains('[') && line.contains(':'))) {
                // 检查是否是元数据，如 [ti:...], [ar:...] 等
                QRegularExpression metaRegex(R"(^\[(ar|ti|al|by|offset|re|ve):.*\])");
                if (metaRegex.match(line).hasMatch()) {
                    qDebug() << "跳过元数据行:" << line;
                    continue;
                }
            }

            // 特殊情况：一行中有多个时间标签和歌词，如 [mm:ss.xx][mm:ss.xx]歌词
            // 首先提取所有时间标签
            QVector<QTime> timeTags;
            QString lyricsText;

            // 用于匹配时间标签的正则表达式
            QRegularExpression timeRegex(R"(\[(\d{1,2}):(\d{1,2})(?:\.(\d{1,3}))?\])");
            QRegularExpressionMatchIterator timeIterator = timeRegex.globalMatch(line);

            // 收集所有时间标签
            while (timeIterator.hasNext()) {
                QRegularExpressionMatch match = timeIterator.next();
                int minutes = match.captured(1).toInt();
                int seconds = match.captured(2).toInt();
                int milliseconds = 0;

                if (!match.captured(3).isEmpty()) {
                    QString msStr = match.captured(3);
                    if (msStr.length() == 2) {
                        milliseconds = msStr.toInt() * 10; // xx -> 毫秒
                    } else if (msStr.length() == 3) {
                        milliseconds = msStr.toInt(); // xxx -> 毫秒
                    } else {
                        milliseconds = msStr.toInt();
                    }
                }

                QTime time(0, minutes, seconds, milliseconds);
                timeTags.append(time);
                //qDebug() << "找到时间标签:" << time.toString("mm:ss.zzz");
            }

            // 如果找到时间标签，提取歌词文本
            if (!timeTags.isEmpty()) {
                // 找到最后一个时间标签结束的位置
                int lastTimeEndPos = 0;
                QRegularExpressionMatch lastMatch = timeRegex.match(line, 0);
                while (lastMatch.hasMatch()) {
                    lastTimeEndPos = lastMatch.capturedEnd();
                    lastMatch = timeRegex.match(line, lastTimeEndPos);
                }

                // 从最后一个时间标签后开始提取歌词
                if (lastTimeEndPos < line.length()) {
                    lyricsText = line.mid(lastTimeEndPos).trimmed();
                }

                // 为每个时间标签创建字幕项
                if (!lyricsText.isEmpty()) {
                    for (const QTime& time : timeTags) {
                        SubtitleItem item;
                        item.startTime = time;
                        item.endTime = time.addMSecs(3000); // 默认显示3秒
                        item.text = lyricsText;
                        subtitles.append(item);

//                        qDebug() << "添加字幕项:" << time.toString("mm:ss.zzz")
//                                 << "->" << item.endTime.toString("mm:ss.zzz")
//                                 << ":" << lyricsText;
                    }
                } else {
                    qDebug() << "跳过无歌词的时间标签行";
                }
            } else {
                // 如果没有时间标签，可能是其他格式或者注释
                qDebug() << "跳过无时间标签的行:" << line;
            }
        }

        if (!subtitles.isEmpty()) {
            m_subtitles = subtitles;
            std::sort(m_subtitles.begin(), m_subtitles.end());
            qDebug() << "加载LRC字幕成功，共" << m_subtitles.size() << "条";
            return true;
        }

    qDebug() << "LRC文件解析失败，无有效字幕";
    return false;
}
bool Subtitle::loadTxtFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    QVector<SubtitleItem> subtitles;
    QString line;
    int lineNumber = 0;

    while (!in.atEnd()) {
        line = in.readLine().trimmed();

        if (line.isEmpty()) {
            continue;
        }

        SubtitleItem item;
        // 简单按行显示，每行显示3秒
        item.startTime = QTime(0, 0, 0).addMSecs(lineNumber * 3000);
        item.endTime = item.startTime.addMSecs(3000);
        item.text = line;
        subtitles.append(item);

        lineNumber++;
    }

    file.close();

    if (!subtitles.isEmpty()) {
        m_subtitles = subtitles;
        qDebug() << "加载TXT字幕成功，共" << m_subtitles.size() << "条";
        return true;
    }

    return false;
}

QString Subtitle::getCurrentSubtitle(const QTime& currentTime)
{
    for (const SubtitleItem& item : m_subtitles) {
        if (currentTime >= item.startTime && currentTime <= item.endTime) {
            if (m_currentSubtitle != item.text) {
                m_currentSubtitle = item.text;
                return item.text;
            }
            break;
        }
    }

    // 如果没有匹配的字幕，清空当前字幕
    if (!m_currentSubtitle.isEmpty()) {
        m_currentSubtitle.clear();
        return "";
    }

    return "";
}

void Subtitle::clear()
{
    m_subtitles.clear();
    m_currentSubtitle.clear();
}
// 添加编码检测和转换函数
QString Subtitle::detectAndConvertEncoding(const QByteArray &data)
{
    // 常见编码列表
    const char *encodings[] = {
        "UTF-8",
        "UTF-8-BOM",  // 带BOM的UTF-8
        "GB2312",
        "GBK",
        "GB18030",    // 中国国家标准
        "Big5",       // 繁体中文
        "Shift_JIS",  // 日文
        "EUC-KR",     // 韩文
        "ISO-8859-1", // 拉丁文
        "Windows-1252", // 西欧
        nullptr
    };
    // 首先检查BOM
    if (data.size() >= 3 &&
        (unsigned char)data[0] == 0xEF &&
        (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF) {
        // UTF-8 with BOM
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        if (codec) {
            return codec->toUnicode(data.mid(3)); // 跳过BOM
        }
    }
    // 尝试各种编码
    for (int i = 0; encodings[i] != nullptr; i++) {
        QString result = tryDecode(data, encodings[i]);
        if (!result.isEmpty() && !result.contains(QChar::ReplacementCharacter)) {
            // 检查是否包含常见中文字符（可选）
            static QRegularExpression chineseRegex("[\u4e00-\u9fa5]");
            if (result.contains(chineseRegex)) {
                qDebug() << "检测到编码:" << encodings[i];
                return result;
            }
            // 如果没有中文字符，也返回第一个成功的解码
            qDebug() << "使用编码:" << encodings[i];
            return result;
        }
    }
    // 最后尝试本地编码
    QTextCodec *codec = QTextCodec::codecForLocale();
    if (codec) {
        return codec->toUnicode(data);
    }
    // 如果都不行，使用UTF-8并忽略错误
    return QString::fromUtf8(data);
}

QString Subtitle::tryDecode(const QByteArray &data, const char *codecName)
{
    QTextCodec *codec = QTextCodec::codecForName(codecName);
    if (!codec) {
        return QString();
    }
    QTextCodec::ConverterState state;
    QString text = codec->toUnicode(data.constData(), data.size(), &state);
    // 如果有太多无效字符，认为失败
    if (state.invalidChars > data.size() * 0.1) { // 超过10%无效字符
        return QString();
    }
    return text;
}
void Subtitle::processComplexLrcLine(const QString& line, QVector<SubtitleItem>& subtitles)
{
    // 处理复杂的LRC行，如多个时间标签紧挨着
    // 例如：[00:10.00][00:12.00]歌词文本

    // 首先找到所有时间标签的位置
    QRegularExpression timeTagRegex(R"(\[(\d{1,2}):(\d{1,2})(?:\.(\d{1,3}))?\])");
    QRegularExpressionMatchIterator tagIterator = timeTagRegex.globalMatch(line);

    QList<QPair<int, QTime>> timeTags; // 位置，时间
    while (tagIterator.hasNext()) {
        QRegularExpressionMatch match = tagIterator.next();

        int minutes = match.captured(1).toInt();
        int seconds = match.captured(2).toInt();
        int milliseconds = 0;

        if (!match.captured(3).isEmpty()) {
            QString msStr = match.captured(3);
            if (msStr.length() == 2) {
                milliseconds = msStr.toInt() * 10;
            } else if (msStr.length() == 3) {
                milliseconds = msStr.toInt();
            }
        }

        QTime time(0, minutes, seconds, milliseconds);
        timeTags.append(qMakePair(match.capturedStart(), time));
    }

    // 为每个时间标签创建字幕项
    for (int i = 0; i < timeTags.size(); i++) {
        int startPos = timeTags[i].first;
        QTime startTime = timeTags[i].second;

        // 确定歌词文本：从这个时间标签结束到下一个时间标签开始
        int textStart = startPos + line.indexOf(']', startPos) - startPos + 1;
        int textEnd = (i + 1 < timeTags.size()) ? timeTags[i + 1].first : line.length();

        QString text = line.mid(textStart, textEnd - textStart).trimmed();

        // 跳过空文本
        if (text.isEmpty()) {
            continue;
        }

        // 跳过元数据
        if (text.startsWith("ar:") || text.startsWith("ti:") ||
            text.startsWith("al:") || text.startsWith("by:")) {
            continue;
        }

        SubtitleItem item;
        item.startTime = startTime;
        item.endTime = startTime.addMSecs(3000); // 默认显示3秒
        item.text = text;
        subtitles.append(item);

        qDebug() << "处理复杂行添加字幕:" << startTime.toString("mm:ss.zzz") << ":" << text;
    }
}
