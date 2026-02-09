#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QProcess>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QTextBrowser>
#include <qDebug>

#include "medialist.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}
#pragma execution_character_set("utf-8")

MediaList::MediaList(QWidget *parent)
    : QListWidget(parent),
      m_stMenu(this),
      m_stActPlayPause(this),
      m_stActAdd(this),
      m_stActRemove(this),
      m_stActDeleteFile(this),
      m_stActClearList(this),
      m_stActOpenLocation(this),
      m_stActFileInfo(this),
      m_stActSeparator1(this),
      m_stActSeparator2(this),
      m_stActSeparator3(this)
{
}

MediaList::~MediaList()
{
}

bool MediaList::Init()
{
    // 设置菜单项文本
    m_stActPlayPause.setText("播放/暂停");
    m_stActAdd.setText("添加");
    m_stActRemove.setText("从播放列表移除");
    m_stActDeleteFile.setText("从磁盘删除");
    m_stActClearList.setText("清空列表");
    m_stActOpenLocation.setText("打开文件所在位置");
    m_stActFileInfo.setText("媒体文件信息");

    // 设置分隔线
    m_stActSeparator1.setSeparator(true);
    m_stActSeparator2.setSeparator(true);
    m_stActSeparator3.setSeparator(true);

    // 按顺序添加菜单项
    m_stMenu.addAction(&m_stActPlayPause);
    m_stMenu.addAction(&m_stActSeparator1);
    m_stMenu.addAction(&m_stActAdd);
    m_stMenu.addAction(&m_stActRemove);
    m_stMenu.addAction(&m_stActDeleteFile);
    m_stMenu.addAction(&m_stActSeparator2);
    m_stMenu.addAction(&m_stActClearList);
    m_stMenu.addAction(&m_stActSeparator3);
    m_stMenu.addAction(&m_stActOpenLocation);
    m_stMenu.addAction(&m_stActFileInfo);

    // 连接信号槽
    connect(&m_stActPlayPause, &QAction::triggered, this, &MediaList::PlayOrPauseCurrent);
    connect(&m_stActAdd, &QAction::triggered, this, &MediaList::AddFile);
    connect(&m_stActRemove, &QAction::triggered, this, &MediaList::RemoveFileFromList);
    connect(&m_stActDeleteFile, &QAction::triggered, this, &MediaList::DeleteFileFromDisk);
    connect(&m_stActClearList, &QAction::triggered, this, &MediaList::ClearList);
    connect(&m_stActOpenLocation, &QAction::triggered, this, &MediaList::OpenFileLocation);
    connect(&m_stActFileInfo, &QAction::triggered, this, &MediaList::ShowFileInfo);

    return true;
}

void MediaList::contextMenuEvent(QContextMenuEvent* event)
{
    // 根据是否有选中项来启用/禁用相关菜单项
    bool hasSelection = (currentRow() >= 0);
    m_stActPlayPause.setEnabled(hasSelection);
    m_stActRemove.setEnabled(hasSelection);
    m_stActDeleteFile.setEnabled(hasSelection);
    m_stActOpenLocation.setEnabled(hasSelection);
    m_stActFileInfo.setEnabled(hasSelection);
    m_stActClearList.setEnabled(count() > 0);

    m_stMenu.exec(event->globalPos());
}

void MediaList::AddFile()
{
    QString filter =
    "所有媒体文件(*.mkv *.rmvb *.rm *.mp4 *.avi *.flv *.wmv *.3gp *.mp3 *.wav *.aac *.flac *.m4a *.wma *.ape *.tta);;"
            "视频文件(*.mkv *.rm *.rmvb *.mp4 *.avi *.flv *.wmv *.3gp);;"
            "音频文件(*.mp3 *.wav *.aac *.flac *.m4a *.wma *.ape *.tta)";

    QStringList listFileName = QFileDialog::getOpenFileNames(this, "打开文件", QDir::homePath(), filter);

    for (QString strFileName : listFileName)
    {
        emit SigAddFile(strFileName);
    }
}

void MediaList::RemoveFileFromList()
{
    int currentIndex = currentRow();
    if (currentIndex >= 0)
    {
        emit SigRemoveFile(currentIndex);
        takeItem(currentIndex);
    }
}

void MediaList::PlayOrPauseCurrent()
{
    int currentIndex = currentRow();
    if (currentIndex >= 0)
    {
        emit SigPlayOrPause(currentIndex);
    }
}

void MediaList::DeleteFileFromDisk()
{
    int currentIndex = currentRow();
    if (currentIndex < 0)
    {
        return;
    }

    QListWidgetItem *item = this->item(currentIndex);
    if (!item)
    {
        return;
    }

    QString filePath = item->toolTip();
    QFileInfo fileInfo(filePath);

    // 确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  QString("确定要从磁盘删除文件吗？\n\n%1\n\n此操作不可恢复！")
                                  .arg(fileInfo.fileName()),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        // 尝试删除文件
        QFile file(filePath);
        if (file.remove())
        {
            // 从列表中移除
            emit SigRemoveFile(currentIndex);
            takeItem(currentIndex);
            QMessageBox::information(this, "成功", "文件已成功删除。");
        }
        else
        {
            QMessageBox::warning(this, "失败",
                                QString("无法删除文件：\n%1\n\n错误：%2")
                                .arg(filePath)
                                .arg(file.errorString()));
        }
    }
}

void MediaList::ClearList()
{
    if (count() == 0)
    {
        return;
    }

    // 确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认清空",
                                  QString("确定要清空播放列表吗？\n\n共 %1 个项目")
                                  .arg(count()),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        clear();
    }
}

void MediaList::OpenFileLocation()
{
    int currentIndex = currentRow();
    if (currentIndex < 0)
    {
        return;
    }

    QListWidgetItem *item = this->item(currentIndex);
    if (!item)
    {
        return;
    }

    QString filePath = item->toolTip();
    QFileInfo fileInfo(filePath);

    // 检查文件是否存在
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "错误",
                            QString("文件不存在或已被删除！\n\n%1")
                            .arg(fileInfo.fileName()));
        return;
    }

    bool success = false;

    // Windows 系统：使用 explorer 命令选中文件
#ifdef Q_OS_WIN
    // 使用 /select 参数可以选中文件
    QString command = QString("explorer.exe /select,\"%1\"")
                      .arg(QDir::toNativeSeparators(fileInfo.absoluteFilePath()));

    // 准备进程
    QProcess process;
    process.setProgram("explorer.exe");

    // 注意：路径中的空格需要用引号括起来，并且需要转义引号
    QString nativePath = QDir::toNativeSeparators(fileInfo.absoluteFilePath());
    QStringList arguments;
    arguments << "/select," << nativePath;

    process.setArguments(arguments);

    // 执行命令
    success = process.startDetached();

    if (!success)
    {
        // 如果上述方法失败，尝试备用方法
        QString backupCommand = QString("cmd.exe /c start explorer /select,\"%1\"")
                                .arg(QDir::toNativeSeparators(fileInfo.absoluteFilePath()));
        success = QProcess::startDetached(backupCommand);
    }

// macOS 系统
#elif defined(Q_OS_MAC)
    // macOS 使用 AppleScript 选中文件
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                  .arg(fileInfo.absoluteFilePath());
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);

    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
    success = true; // 假设成功

// Linux 系统
#elif defined(Q_OS_LINUX)
    // 尝试使用 xdg-open 打开文件夹
    QUrl url = QUrl::fromLocalFile(fileInfo.absolutePath());
    success = QDesktopServices::openUrl(url);

    // Linux 上很难实现选中文件，这里只是打开文件夹
    // 可以尝试使用一些特定的文件管理器
    QStringList linuxManagers = {
        "nautilus",  // GNOME
        "dolphin",   // KDE
        "thunar",    // XFCE
        "pcmanfm",   // LXDE
        "nemo",      // Cinnamon
    };

    for (const QString &manager : linuxManagers)
    {
        QString command = QString("%1 \"%2\"").arg(manager).arg(fileInfo.absolutePath());
        if (QProcess::startDetached(command))
        {
            success = true;
            break;
        }
    }
#endif

    if (!success)
    {
        // 如果所有方法都失败了，至少打开文件夹
        QUrl url = QUrl::fromLocalFile(fileInfo.absolutePath());
        QDesktopServices::openUrl(url);
    }
}

void MediaList::ShowFileInfo()
{
    int currentIndex = currentRow();
    if (currentIndex < 0)
    {
        return;
    }

    QListWidgetItem *item = this->item(currentIndex);
    if (!item)
    {
        return;
    }

    QString filePath = item->toolTip();
    QFileInfo fileInfo(filePath);

    // 检查文件是否存在
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "错误",
                            QString("文件不存在或已被删除！\n\n%1")
                            .arg(fileInfo.fileName()));
        return;
    }

    // 初始化FFmpeg
    avformat_network_init();

    AVFormatContext *formatContext = nullptr;

    // 打开媒体文件
    int ret = avformat_open_input(&formatContext, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0)
    {
        QMessageBox::warning(this, "错误", "无法打开媒体文件，可能不是有效的媒体格式。");
        return;
    }

    // 获取流信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        avformat_close_input(&formatContext);
        QMessageBox::warning(this, "错误", "无法获取媒体文件流信息。");
        return;
    }

    // 开始构建HTML信息字符串
    QString htmlText;
    htmlText += "<!DOCTYPE html>";
    htmlText += "<html>";
    htmlText += "<head>";
    htmlText += "<meta charset=\"UTF-8\">";
    htmlText += "<style>";
    htmlText += "body { font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; background-color: #1e1e1e; color: #ffffff; margin: 0; padding: 15px; }";
    htmlText += "h1 { text-align: center; color: #4CAF50; margin-bottom: 20px; font-size: 22px; font-weight: bold; }";
    htmlText += "h2 { color: #FF9800; margin-top: 20px; margin-bottom: 10px; font-size: 16px; border-bottom: 1px solid #444; padding-bottom: 8px; }";
    htmlText += ".section { margin-bottom: 15px; }";
    htmlText += ".info-line { margin-bottom: 8px; line-height: 1.5; }";
    htmlText += ".label { color: #cccccc; font-weight: normal; min-width: 80px; display: inline-block; }";
    htmlText += ".value { color: #ffffff; }";
    htmlText += ".two-columns { width: 100%; }";
    htmlText += ".two-columns > div { float: left; width: 48%; }";
    htmlText += ".column-left { margin-right: 2%; }";
    htmlText += ".column-right { border-left: 1px solid #444; padding-left: 20px; }";
    htmlText += ".clearfix { clear: both; }";
    htmlText += ".button-container { text-align: right; padding: 10px 15px; margin-top: 15px; border-top: 1px solid #444; background-color: #1a1a1a; }";
    htmlText += ".copy-button { background-color: #4CAF50; color: white; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; font-size: 12px; font-weight: normal; }";
    htmlText += ".close-button { background-color: #f44336; color: white; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; font-size: 12px; font-weight: normal; margin-left: 10px; }";
    htmlText += ".copy-button:hover { background-color: #45a049; }";
    htmlText += ".close-button:hover { background-color: #d32f2f; }";
    htmlText += ".file-name { color: #ffffff; font-weight: bold; }";
    htmlText += ".numeric-value { color: #4CAF50; }";
    htmlText += "</style>";
    htmlText += "</head>";
    htmlText += "<body>";

    // 标题 - 只保留主标题
    htmlText += "<h1>媒体文件信息</h1>";

    // 基本信息部分
    htmlText += "<div class='section'>";
    htmlText += "<h2>基本信息</h2>";
    htmlText += QString("<div class='info-line'><span class='label'>文件名:</span><span class='value file-name'> %1</span></div>")
                .arg(fileInfo.fileName().toHtmlEscaped());
    htmlText += QString("<div class='info-line'><span class='label'>路径:</span><span class='value'> %1</span></div>")
                .arg(fileInfo.absoluteFilePath().toHtmlEscaped());

    // 文件大小
    qint64 fileSize = fileInfo.size();
    double fileSizeMB = fileSize / (1024.0 * 1024.0);
    htmlText += QString("<div class='info-line'><span class='label'>大小:</span><span class='value numeric-value'>%1 MB</span></div>")
                .arg(QString::number(fileSizeMB, 'f', 2));

    htmlText += QString("<div class='info-line'><span class='label'>创建时间:</span><span class='value'> %1</span></div>")
                .arg(fileInfo.created().toString("yyyy-MM-dd HH:mm:ss").toHtmlEscaped());
    htmlText += QString("<div class='info-line'><span class='label'>修改时间:</span><span class='value'> %1</span></div>")
                .arg(fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss").toHtmlEscaped());
    htmlText += "</div>";

    // 获取媒体信息
    int64_t duration = formatContext->duration / AV_TIME_BASE; // 转换为秒
    int hours = duration / 3600;
    int minutes = (duration % 3600) / 60;
    int seconds = duration % 60;

    QString durationStr;
    if (hours > 0)
        durationStr = QString("%1:%2:%3").arg(hours, 2, 10, QChar('0'))
                                        .arg(minutes, 2, 10, QChar('0'))
                                        .arg(seconds, 2, 10, QChar('0'));
    else
        durationStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                                     .arg(seconds, 2, 10, QChar('0'));

    // 遍历所有流，寻找视频和音频流
    int videoStreamIndex = -1;
    int audioStreamIndex = -1;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVCodecParameters *videoCodecParams = nullptr;
    AVCodecParameters *audioCodecParams = nullptr;

    for (unsigned int i = 0; i < formatContext->nb_streams; i++)
    {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (videoStreamIndex == -1) // 只取第一个视频流
            {
                videoStreamIndex = i;
                videoStream = formatContext->streams[i];
                videoCodecParams = videoStream->codecpar;
            }
        }
        else if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if (audioStreamIndex == -1) // 只取第一个音频流
            {
                audioStreamIndex = i;
                audioStream = formatContext->streams[i];
                audioCodecParams = audioStream->codecpar;
            }
        }
    }

    // 创建两栏容器 - 使用简单的浮动布局
    htmlText += "<div class='two-columns'>";

    // 视频信息栏 - 左栏
    htmlText += "<div class='column-left'>";
    htmlText += "<h2>视频信息</h2>";

    if (videoStreamIndex != -1 && videoCodecParams)
    {
        htmlText += QString("<div class='info-line'><span class='label'>时长:</span><span class='value numeric-value'>%1</span></div>")
                    .arg(durationStr.toHtmlEscaped());
        htmlText += QString("<div class='info-line'><span class='label'>分辨率:</span><span class='value numeric-value'>%1 × %2</span></div>")
                    .arg(videoCodecParams->width).arg(videoCodecParams->height);

        // 帧率
        if (videoStream->avg_frame_rate.num > 0 && videoStream->avg_frame_rate.den > 0)
        {
            double fps = av_q2d(videoStream->avg_frame_rate);
            htmlText += QString("<div class='info-line'><span class='label'>帧率:</span><span class='value numeric-value'>%1 fps</span></div>")
                        .arg(QString::number(fps, 'f', 2));
        }

        // 视频编码器
        const AVCodecDescriptor *codecDesc = avcodec_descriptor_get(videoCodecParams->codec_id);
        if (codecDesc)
        {
            htmlText += QString("<div class='info-line'><span class='label'>编码:</span><span class='value'>%1</span></div>")
                        .arg(codecDesc->name);
        }

        // 视频码率
        if (videoCodecParams->bit_rate > 0)
        {
            double videoBitrateKBps = videoCodecParams->bit_rate / 1000.0;
            htmlText += QString("<div class='info-line'><span class='label'>码率:</span><span class='value numeric-value'>%1 kbps</span></div>")
                        .arg(QString::number(videoBitrateKBps, 'f', 1));
        }
    }
    else
    {
        htmlText += "<div class='info-line'><span class='value'>无视频流</span></div>";
    }

    htmlText += "</div>";

    // 音频信息栏 - 右栏
    htmlText += "<div class='column-right'>";
    htmlText += "<h2>音频信息</h2>";

    if (audioStreamIndex != -1 && audioCodecParams)
    {
        // 音频编码器
        const AVCodecDescriptor *audioCodecDesc = avcodec_descriptor_get(audioCodecParams->codec_id);
        if (audioCodecDesc)
        {
            htmlText += QString("<div class='info-line'><span class='label'>编码:</span><span class='value'>%1</span></div>")
                        .arg(audioCodecDesc->name);
        }

        // 音频比特率
        if (audioCodecParams->bit_rate > 0)
        {
            double audioBitrateKBps = audioCodecParams->bit_rate / 1000.0;
            htmlText += QString("<div class='info-line'><span class='label'>码率:</span><span class='value numeric-value'>%1 kbps</span></div>")
                        .arg(QString::number(audioBitrateKBps, 'f', 1));
        }

        // 声道数
        if (audioCodecParams->channels > 0)
        {
            int channels = audioCodecParams->channels;
            QString channelStr;

            // 根据声道数显示常见布局
            switch (channels) {
                case 1: channelStr = "单声道"; break;
                case 2: channelStr = "立体声"; break;
                case 4: channelStr = "4.0声道"; break;
                case 6: channelStr = "5.1声道"; break;
                case 8: channelStr = "7.1声道"; break;
                default: channelStr = QString("%1声道").arg(channels); break;
            }

            htmlText += QString("<div class='info-line'><span class='label'>声道:</span><span class='value numeric-value'>%1</span></div>")
                        .arg(channelStr.toHtmlEscaped());
        }

        // 采样率
        if (audioCodecParams->sample_rate > 0)
        {
            htmlText += QString("<div class='info-line'><span class='label'>采样率:</span><span class='value numeric-value'>%1 Hz</span></div>")
                        .arg(QString::number(audioCodecParams->sample_rate));
        }
    }
    else
    {
        htmlText += "<div class='info-line'><span class='value'>无音频流</span></div>";
    }

    htmlText += "</div>";
    htmlText += "<div class='clearfix'></div>";
    htmlText += "</div>";

    // 结束HTML
    htmlText += "</body>";
    htmlText += "</html>";

    // 创建纯文本版本用于复制
    QString plainText;
    plainText += "媒体文件信息\n";
    plainText += QString("=").repeated(40) + "\n\n";

    plainText += "基本信息:\n";
    plainText += QString("-").repeated(20) + "\n";
    plainText += QString("文件名: %1\n").arg(fileInfo.fileName());
    plainText += QString("路径: %1\n").arg(fileInfo.absoluteFilePath());
    plainText += QString("大小: %.2f MB\n").arg(fileSizeMB);
    plainText += QString("创建时间: %1\n").arg(fileInfo.created().toString("yyyy-MM-dd HH:mm:ss"));
    plainText += QString("修改时间: %1\n").arg(fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss"));
    plainText += "\n" + QString("-").repeated(40) + "\n";

    // 视频信息
    if (videoStreamIndex != -1 && videoCodecParams)
    {
        plainText += "视频信息:\n";
        plainText += QString("-").repeated(20) + "\n";
        plainText += QString("时长: %1\n").arg(durationStr);
        plainText += QString("分辨率: %1 × %2 像素\n").arg(videoCodecParams->width).arg(videoCodecParams->height);

        // 视频编码器
        const AVCodecDescriptor *codecDesc = avcodec_descriptor_get(videoCodecParams->codec_id);
        if (codecDesc)
        {
            plainText += QString("视频编码: %1\n").arg(codecDesc->name);
        }

        // 视频码率
        if (videoCodecParams->bit_rate > 0)
        {
            double videoBitrateKBps = videoCodecParams->bit_rate / 1000.0;
            plainText += QString("视频码率: %.1f kbps\n").arg(videoBitrateKBps);
        }

        // 帧率
        if (videoStream->avg_frame_rate.num > 0 && videoStream->avg_frame_rate.den > 0)
        {
            double fps = av_q2d(videoStream->avg_frame_rate);
            plainText += QString("帧率: %.2f 帧/秒\n").arg(fps);
        }

        plainText += "\n" + QString("-").repeated(40) + "\n";
    }

    // 音频信息
    if (audioStreamIndex != -1 && audioCodecParams)
    {
        plainText += "音频信息:\n";
        plainText += QString("-").repeated(20) + "\n";

        // 音频编码器
        const AVCodecDescriptor *audioCodecDesc = avcodec_descriptor_get(audioCodecParams->codec_id);
        if (audioCodecDesc)
        {
            plainText += QString("音频编码: %1\n").arg(audioCodecDesc->name);
        }

        // 音频比特率
        if (audioCodecParams->bit_rate > 0)
        {
            double audioBitrateKBps = audioCodecParams->bit_rate / 1000.0;
            plainText += QString("音频比特率: %.1f kbps\n").arg(audioBitrateKBps);
        }

        // 声道数
        if (audioCodecParams->channels > 0)
        {
            int channels = audioCodecParams->channels;
            QString channelStr;

            switch (channels) {
                case 1: channelStr = "单声道"; break;
                case 2: channelStr = "立体声"; break;
                case 4: channelStr = "4.0声道"; break;
                case 6: channelStr = "5.1声道"; break;
                case 8: channelStr = "7.1声道"; break;
                default: channelStr = QString("%1声道").arg(channels); break;
            }

            plainText += QString("声道: %1\n").arg(channelStr);
        }

        // 采样率
        if (audioCodecParams->sample_rate > 0)
        {
            plainText += QString("采样率: %1 Hz\n").arg(audioCodecParams->sample_rate);
        }

        plainText += "\n" + QString("-").repeated(40) + "\n";
    }

    // 关闭FFmpeg资源
    avformat_close_input(&formatContext);

    // 创建对话框
    QDialog *infoDialog = new QDialog(nullptr);
    infoDialog->setWindowTitle(QString("媒体文件信息 - %1").arg(fileInfo.fileName()));
    infoDialog->resize(400, 500);

    // 设置为独立窗口
    infoDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    infoDialog->setAttribute(Qt::WA_DeleteOnClose);

    if (!this->windowIcon().isNull()) {
        infoDialog->setWindowIcon(this->windowIcon());
    }

    // 设置对话框样式
    infoDialog->setStyleSheet(R"(
        QDialog {
            background-color: #121212;
            border: 1px solid #444;
        }
        QPushButton {
            background-color: #333333;
            color: white;
            border: 1px solid #555555;
            padding: 6px 12px;
            border-radius: 3px;
            font-size: 11px;
            font-weight: normal;
        }
        QPushButton:hover {
            background-color: #444444;
            border: 1px solid #666666;
        }
        QPushButton:pressed {
            background-color: #222222;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(infoDialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 使用QWebEngineView代替QTextEdit，以获得更好的HTML/CSS支持
    #ifdef QT_WEBENGINEWIDGETS_LIB
    #include <QWebEngineView>
    QWebEngineView *webView = new QWebEngineView(infoDialog);
    webView->setHtml(htmlText);
    webView->setStyleSheet("background-color: #1e1e1e; border: none;");
    layout->addWidget(webView);
    #else
    // 如果Qt版本不支持WebEngine，回退到QTextEdit
    QTextEdit *textEdit = new QTextEdit(infoDialog);
    textEdit->setReadOnly(true);
    textEdit->setHtml(htmlText);
    textEdit->document()->setDocumentMargin(10);

    // 设置文本编辑器的样式
    QString scrollBarStyle = R"(
        QTextEdit {
            background-color: #1e1e1e;
            border: none;
            color: white;
        }
        QScrollBar:vertical {
            background-color: #2a2a2a;
            width: 14px;
            border-radius: 7px;
        }
        QScrollBar::handle:vertical {
            background-color: #555555;
            border-radius: 7px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #666666;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            background: none;
        }
        QScrollBar:horizontal {
            height: 14px;
            background-color: #2a2a2a;
            border-radius: 7px;
        }
        QScrollBar::handle:horizontal {
            background-color: #2d2d2d;
            border-radius: 7px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #333333;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
            background: none;
        }
    )";

    textEdit->setStyleSheet(scrollBarStyle);
    layout->addWidget(textEdit);
    #endif

    // 添加按钮容器
    QWidget *buttonContainer = new QWidget(infoDialog);
    buttonContainer->setStyleSheet("background-color: #1a1a1a; border-top: 1px solid #444;");
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(15, 10, 15, 10);
    buttonLayout->addStretch();

    QPushButton *copyButton = new QPushButton("复制信息", buttonContainer);
    QPushButton *closeButton = new QPushButton("关闭", buttonContainer);

    // 设置按钮样式
    copyButton->setStyleSheet(R"(
        QPushButton {
            background-color: #393a3c;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-size: 12px;
            font-weight: normal;
        }
        QPushButton:hover {
            background-color: #4CAF50;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
    )");

    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #393a3c;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-size: 12px;
            font-weight: normal;
            margin-left: 10px;
        }
        QPushButton:hover {
            background-color: #f44336;
        }
        QPushButton:pressed {
            background-color: #d32f2f;
        }
    )");

    // 连接按钮
    connect(copyButton, &QPushButton::clicked, [plainText, infoDialog]() {
        QApplication::clipboard()->setText(plainText);

        QMessageBox msgBox(infoDialog);
        msgBox.setWindowTitle("提示");
        msgBox.setText("媒体文件信息已复制到剪贴板");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(R"(
            QMessageBox {
                background-color: #121212;
                color: #e0e0e0;
            }
            QMessageBox QLabel {
                color: white;
                font-size: 11pt;
            }
            QMessageBox QPushButton {
                background-color: #333333;
                color: white;
                border: 1px solid #555555;
                padding: 6px 12px;
                border-radius: 3px;
                min-width: 70px;
                font-size: 11px;
                font-weight: normal;
            }
            QMessageBox QPushButton:hover {
                background-color: #444444;
            }
        )");
        msgBox.exec();
    });

    connect(closeButton, &QPushButton::clicked, infoDialog, &QDialog::close);

    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(closeButton);

    layout->addWidget(buttonContainer);

    // 改为非模态显示
    infoDialog->show();

    // 确保窗口获得焦点
    infoDialog->raise();
    infoDialog->activateWindow();
}
