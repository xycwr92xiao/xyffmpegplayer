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
#include <QProgressDialog>
#include <QtConcurrent>

#include "globalvars.h"

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
      m_stActAddFolder(this),
      m_stActRemove(this),
      m_stActDeleteFile(this),
      m_stActClearList(this),
      m_stActOpenLocation(this),
      m_stActFileInfo(this),
      m_stActRename(this),       // 新增
      m_stActSeparator1(this),
      m_stActSeparator2(this),
      m_stActSeparator3(this),
      m_stActSeparator4(this),   // 新增
      m_isRenaming(false),
      m_renameIndex(-1)
{
}

MediaList::~MediaList()
{
}

bool MediaList::Init()
{
    // 设置菜单项文本
    m_stActPlayPause.setText("播放/暂停");
    m_stActAdd.setText("添加文件");
    m_stActAddFolder.setText("添加文件夹");
    m_stActRemove.setText("从播放列表移除");
    m_stActDeleteFile.setText("从磁盘删除");
    m_stActClearList.setText("清空列表");
    m_stActOpenLocation.setText("打开文件所在位置");
    m_stActFileInfo.setText("媒体文件信息");
    m_stActGetLyric.setText("获取歌词");
    m_stActRename.setText("重命名");  // 新增
    // 设置分隔线
    m_stActSeparator1.setSeparator(true);
    m_stActSeparator2.setSeparator(true);
    m_stActSeparator3.setSeparator(true);
    m_stActSeparator4.setSeparator(true);  // 新增

    // 按顺序添加菜单项
    m_stMenu.addAction(&m_stActPlayPause);
    m_stMenu.addAction(&m_stActSeparator1);
    m_stMenu.addAction(&m_stActAdd);
    m_stMenu.addAction(&m_stActAddFolder);
    m_stMenu.addAction(&m_stActSeparator2);
    m_stMenu.addAction(&m_stActRemove);
    m_stMenu.addAction(&m_stActDeleteFile);
    m_stMenu.addAction(&m_stActClearList);
    m_stMenu.addAction(&m_stActSeparator3);
    m_stMenu.addAction(&m_stActOpenLocation);
    m_stMenu.addAction(&m_stActFileInfo);
    m_stMenu.addAction(&m_stActGetLyric);
    m_stMenu.addAction(&m_stActSeparator4);  // 新增
    m_stMenu.addAction(&m_stActRename);      // 新增

    // 连接信号槽
    connect(&m_stActPlayPause, &QAction::triggered, this, &MediaList::PlayOrPauseCurrent);
    connect(&m_stActAdd, &QAction::triggered, this, &MediaList::AddFile);
    connect(&m_stActAddFolder, &QAction::triggered, this, &MediaList::AddFolder);
    connect(&m_stActRemove, &QAction::triggered, this, &MediaList::RemoveFileFromList);
    connect(&m_stActDeleteFile, &QAction::triggered, this, &MediaList::DeleteFileFromDisk);
    connect(&m_stActClearList, &QAction::triggered, this, &MediaList::ClearList);
    connect(&m_stActOpenLocation, &QAction::triggered, this, &MediaList::OpenFileLocation);
    connect(&m_stActFileInfo, &QAction::triggered, this, &MediaList::ShowFileInfo);
    connect(&m_stActGetLyric, &QAction::triggered, this, &MediaList::GetLyric);
    connect(&m_stActRename, &QAction::triggered, this, &MediaList::RenameFile);  // 新增
    // 初始化歌词对话框
    m_lyricDialog = nullptr;
    // 设置菜单的字体、颜色和行距
        QFont menuFont("Microsoft YaHei", 10, QFont::Normal);  // 设置字体和大小
        m_stMenu.setFont(menuFont);

        // 设置菜单的样式表，控制文字颜色和行距
        QString menuStyle =
            "QMenu {"
            "   background-color: #2b2b2b;"
            "   border: 1px solid #3c3c3c;"
            "   border-radius: 4px;"
            "   padding: 4px 0px;"
            "}"
            "QMenu::item {"
            "   background-color: transparent;"
            "   color: #aaaaaa;"
            "   padding: 4px 20px 4px 32px;  /* 上右下左，调整行高 */"
            "   margin: 2px 6px;"
            "   border-radius: 3px;"
            "   min-height: 24px;  /* 最小行高 */"
            "   min-width: 120px;"
            "}"
            "QMenu::item:selected {"
            "   background-color: #404040;"
            "   color: #ffffff;"
            "}"
            "QMenu::item:disabled {"
            "   color: #808080;"
            "}"
            "QMenu::separator {"
            "   height: 1px;"
            "   background-color: #3c3c3c;"
            "   margin: 4px 8px;"
            "}"
            "QMenu::icon {"
            "   margin-left: 18px;"
            "   width: 16px;"
            "   height: 16px;"
            "}"
            "QMenu::indicator {"
            "   width: 10px;"
            "   height: 10px;"
            "}";

        m_stMenu.setStyleSheet(menuStyle);

        // 设置各个菜单项的图标（可选，如果需要的话）
        // 你可以为各个菜单项设置图标，比如：
//         m_stActPlayPause.setIcon(QIcon(":/res/icons/play_pause.png"));
//         m_stActAdd.setIcon(QIcon(":/res/icons/add.png"));
//         m_stActRemove.setIcon(QIcon(":/res/icons/remove.png"));
//         m_stActDeleteFile.setIcon(QIcon(":/res/icons/delete.png"));
//         m_stActClearList.setIcon(QIcon(":/res/icons/clear.png"));
//         m_stActOpenLocation.setIcon(QIcon(":/res/icons/folder.png"));
//         m_stActFileInfo.setIcon(QIcon(":/res/icons/info.png"));
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
    m_stActRename.setEnabled(hasSelection);  // 新增
    m_stActClearList.setEnabled(count() > 0);

    m_stMenu.exec(event->globalPos());
}

void MediaList::AddFile()
{
    QString lastDir = GlobalVars::GetLastOpenDir();
    QString filter =GlobalVars::getMediaFileType();

    QStringList listFileName = QFileDialog::getOpenFileNames(this, "打开文件", lastDir, filter);
    if (!listFileName.isEmpty()) {
            // 获取第一个文件的目录并保存
            QFileInfo firstFile(listFileName.first());
            QString selectedDir = firstFile.absolutePath();
            GlobalVars::SaveLastOpenDir(selectedDir);
        }
    for (QString strFileName : listFileName)
    {
        emit SigAddFile(strFileName);
    }
}

void MediaList::AddFolder()
{
    QString lastDir = GlobalVars::GetLastOpenDir();
    QString selectedFolder = QFileDialog::getExistingDirectory(this,
                                                               "选择文件夹",
                                                               lastDir,
                                                               QFileDialog::ShowDirsOnly);

    if (selectedFolder.isEmpty()) {
        return; // 用户取消了选择
    }

    // 保存最后打开的目录
    GlobalVars::SaveLastOpenDir(selectedFolder);

    // 询问是否递归扫描子文件夹
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("扫描选项");
    msgBox.setText("请选择扫描方式：");
    msgBox.setIcon(QMessageBox::Question);

    QPushButton *currentFolderButton = msgBox.addButton("仅当前文件夹", QMessageBox::ActionRole);
    QPushButton *allSubfoldersButton = msgBox.addButton("包括子文件夹", QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox.addButton("取消", QMessageBox::RejectRole);

    msgBox.exec();

    bool recursive = false;
    if (msgBox.clickedButton() == currentFolderButton) {
        recursive = false;
    } else if (msgBox.clickedButton() == allSubfoldersButton) {
        recursive = true;
    } else {
        return; // 用户取消了
    }

    // 支持的媒体文件扩展名
    QStringList mediaExtensions = {
        "mkv", "rmvb", "rm", "mp4", "avi", "flv", "wmv", "3gp",  "mts", "vob", "dat",
        "mp3", "wav", "aac", "flac", "m4a", "wma", "ape", "tta"
    };

    // 收集所有媒体文件
    QStringList mediaFiles;
    QDir dir(selectedFolder);

    // 定义递归扫描函数
    std::function<void(const QDir&)> scanFolder = [&](const QDir& folder) {
        QFileInfoList entries = folder.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Dirs);

        for (const QFileInfo& entry : entries) {
            if (entry.isDir()) {
                if (recursive) {
                    scanFolder(QDir(entry.absoluteFilePath()));
                }
            } else {
                QString extension = entry.suffix().toLower();
                if (mediaExtensions.contains(extension)) {
                    mediaFiles.append(entry.absoluteFilePath());
                }
            }
        }
    };

    // 开始扫描
    QProgressDialog progress("扫描文件夹中...", "取消", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setMaximum(0); // 不确定进度
    progress.setValue(0);

    // 在单独的线程中扫描，避免界面卡顿
    QtConcurrent::run([&]() {
        scanFolder(dir);
    }).waitForFinished(); // 等待扫描完成

    progress.close();

    if (mediaFiles.isEmpty()) {
        QMessageBox::information(this, "提示",
                                QString("所选文件夹%s没有找到支持的媒体文件。\n\n支持的格式：\n视频：mkv, rmvb, rm, mp4, avi, flv, wmv, 3gp\n音频：mp3, wav, aac, flac, m4a, wma, ape, tta")
                                .arg(recursive ? "及其子文件夹" : ""));
        return;
    }

    // 添加到播放列表
    int addedCount = 0;
    QProgressDialog addProgress("添加媒体文件中...", "取消", 0, mediaFiles.size(), this);
    addProgress.setWindowModality(Qt::WindowModal);
    addProgress.setMinimumDuration(500);

    for (int i = 0; i < mediaFiles.size(); i++) {
        if (addProgress.wasCanceled()) {
            break;
        }

        QString fullPath = mediaFiles[i];
        emit SigAddFile(fullPath);
        addedCount++;

        addProgress.setValue(i + 1);
        QCoreApplication::processEvents();
    }

    addProgress.setValue(mediaFiles.size());

    // 显示添加结果
    QString message = QString("成功添加 %1 个媒体文件到播放列表。").arg(addedCount);
    if (recursive) {
        message += QString("\n扫描方式：当前文件夹及其所有子文件夹");
    } else {
        message += QString("\n扫描方式：仅当前文件夹");
    }

    QMessageBox::information(this, "添加完成", message);
}

void MediaList::RemoveFileFromList()
{
    int currentIndex = currentRow();
    if (currentIndex >= 0)
    {
        emit SigRemoveFile(currentIndex);
        //takeItem(currentIndex);
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
        // 判断是否为正在播放的曲目
            if (currentIndex == GlobalVars::currentPlayIndex())
            {
                // 如果是当前正在播放的曲目，先发信号停止播放并从列表移除
                emit SigRemoveFile(currentIndex);
                // 停止后延时100毫秒再删除文件
                QTimer::singleShot(100, this, [this, filePath]() {
                    // 尝试删除文件
                    QFile file(filePath);
                    if (file.remove())
                    {
                        // 列表项已在信号处理中移除，无需再次移除
                        QMessageBox::information(this, "成功", "文件已成功删除。");
                    }
                    else
                    {
                        QMessageBox::warning(this, "失败",
                                            QString("无法删除文件,文件属性可能是只读：\n%1\n\n错误：%2")
                                            .arg(filePath)
                                            .arg(file.errorString()));
                    }
                });
            }
            else
            {
                // 如果不是当前播放的曲目，直接删除文件
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
        GlobalVars::selectedIndex()=-1;
        GlobalVars::currentPlayIndex() = -1;
        emit SigStop();
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
    if (currentIndex < 0) return;
    QListWidgetItem *item = this->item(currentIndex);
    if (!item) return;
    QString filePath = item->toolTip();
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "错误",
            QString("文件不存在或已被删除！\n\n%1").arg(fileInfo.fileName()));
        return;
    }

    // 初始化FFmpeg
    avformat_network_init();
    AVFormatContext *formatContext = nullptr;
    int ret = avformat_open_input(&formatContext, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        QMessageBox::warning(this, "错误", "无法打开媒体文件，可能不是有效的媒体格式。");
        return;
    }
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        QMessageBox::warning(this, "错误", "无法获取媒体文件流信息。");
        return;
    }

    // 数据结构：存储所有媒体信息，使用 shared_ptr 便于在多处共享
    struct FileInfoData {
         QPointer<QTimer> timer;   // 新增：指向动态更新定时器的安全指针
        QString fileName;
        QString filePath;
        double fileSizeMB = 0;
        QDateTime created;
        QDateTime modified;
        QString durationStr;
        // 视频信息
        bool hasVideo = false;
        int videoWidth = 0, videoHeight = 0;
        QString videoCodec;
        double videoBitrate = 0;      // kbps
        double videoFps = 0;
        // 音频信息
        bool hasAudio = false;
        QString audioCodec;
        double audioBitrate = 0;      // kbps
        int audioChannels = 0;
        int audioSampleRate = 0;
        // 帧信息（初始为空）
        bool hasFrameInfo = false;
        int iCount = 0, pCount = 0, bCount = 0;
        double avgGop = 0.0, maxGop = 0.0;
        // 辅助：视频流索引，用于后续重新打开时获取帧信息
        int videoStreamIndex = -1;
        // 文件路径的UTF-8字节数组，确保字符串生命周期
        QByteArray filePathUtf8;

        // 新增：获取帧信息时的状态
        bool isFetching = false;
        bool isStop =false;
        int fetchingDots = 1;          // 用于动态显示点个数
    };

    auto data = std::make_shared<FileInfoData>();
    data->fileName = fileInfo.fileName();
    data->filePath = fileInfo.absoluteFilePath();
    data->fileSizeMB = fileInfo.size() / (1024.0 * 1024.0);
    data->created = fileInfo.birthTime();
    data->modified = fileInfo.lastModified();
    data->filePathUtf8 = filePath.toUtf8();

    // 时长
    int64_t duration = formatContext->duration / AV_TIME_BASE;
    int hours = duration / 3600;
    int minutes = (duration % 3600) / 60;
    int seconds = duration % 60;
    if (hours > 0)
        data->durationStr = QString("%1:%2:%3").arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
    else
        data->durationStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));

    // 遍历流，提取视频/音频基本信息
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVStream *stream = formatContext->streams[i];
        AVCodecParameters *codecParams = stream->codecpar;
        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO && !data->hasVideo) {
            data->hasVideo = true;
            data->videoStreamIndex = i;
            data->videoWidth = codecParams->width;
            data->videoHeight = codecParams->height;
            const AVCodecDescriptor *codecDesc = avcodec_descriptor_get(codecParams->codec_id);
            if (codecDesc) data->videoCodec = QString::fromUtf8(codecDesc->name);
            if (codecParams->bit_rate > 0)
                data->videoBitrate = codecParams->bit_rate / 1000.0;
            if (stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0)
                data->videoFps = av_q2d(stream->avg_frame_rate);
        }
        else if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO && !data->hasAudio) {
            data->hasAudio = true;
            const AVCodecDescriptor *codecDesc = avcodec_descriptor_get(codecParams->codec_id);
            if (codecDesc) data->audioCodec = QString::fromUtf8(codecDesc->name);
            if (codecParams->bit_rate > 0)
                data->audioBitrate = codecParams->bit_rate / 1000.0;
            data->audioChannels = codecParams->channels;
            data->audioSampleRate = codecParams->sample_rate;
        }
    }

    // 关闭formatContext，后续点击按钮时再重新打开只用于遍历帧
    avformat_close_input(&formatContext);

    // ========== 构建HTML的函数 ==========
    auto buildHtml = [](std::shared_ptr<FileInfoData> d) -> QString {
        QString html;
        html += "<html><head><style>";
        html += "body { font-family: 'Microsoft YaHei', Arial, sans-serif; background-color: #2a2a2a; margin: 0px; color: #e0e0e0; }";
        html += "h1 { color: #4CAF50; text-align: center; margin-bottom: 15px; font-size: 18px; text-shadow: 0 0 5px rgba(76, 175, 80, 0.3); }";
        html += "h2 { color: #64B5F6; margin-top: 15px; margin-bottom: 8px; font-size: 14px; border-bottom: 1px solid #3a3a3a; padding-bottom: 5px; }";
        html += ".info-table { width: 100%; border-collapse: collapse; margin-bottom: 1px; }";
        html += ".info-table td { padding: 4px 8px; vertical-align: top; }";
        html += ".label { color: #aaa; width: 35%; font-weight: normal; }";
        html += ".value { color: #fff; }";
        html += ".highlight { color: #4CAF50; font-weight: bold; }";
        html += "a { color: #4CAF50; text-decoration: none; }";
        html += "a:hover { text-decoration: underline; }";
        html += "</style></head><body>";
        html += "<h1>媒体文件信息</h1>";
        html += "<table width='100%' cellpadding='0' cellspacing='10'>";
        html += "  <tr><td width='100%' valign='top'>";
        html += "<h2>基本信息</h2>";
        html += "<table class='info-table'>";
        html += QString("<tr><td class='label'>文件名：</td><td class='value'>%1</td></tr>")
                .arg(d->fileName.toHtmlEscaped());
        html += QString("<tr><td class='label'>路径：</td><td class='value'>%1</td></tr>")
                .arg(d->filePath.toHtmlEscaped());
        html += QString("<tr><td class='label'>大小：</td><td class='value highlight'>%1 MB</td></tr>")
                .arg(QString::number(d->fileSizeMB, 'f', 2));
        html += QString("<tr><td class='label'>创建时间：</td><td class='value'>%1</td></tr>")
                .arg(d->created.toString("yyyy-MM-dd HH:mm:ss").toHtmlEscaped());
        html += QString("<tr><td class='label'>修改时间：</td><td class='value'>%1</td></tr>")
                .arg(d->modified.toString("yyyy-MM-dd HH:mm:ss").toHtmlEscaped());
        html += "</table></td></tr>";
        html += "</table>";

        html += "<table width='100%' cellpadding='0' cellspacing='10'><tr>";

        if (d->hasVideo) {
            html += "<td width='50%' valign='top'>";
            html += "<h2>视频信息</h2>";
            html += "<table class='info-table'>";
            html += QString("<tr><td class='label'>时长：</td><td class='value highlight'>%1</td></tr>")
                    .arg(d->durationStr.toHtmlEscaped());
            html += QString("<tr><td class='label'>分辨率：</td><td class='value highlight'>%1 × %2</td></tr>")
                    .arg(d->videoWidth).arg(d->videoHeight);
            if (!d->videoCodec.isEmpty()) {
                html += QString("<tr><td class='label'>编码：</td><td class='value'>%1</td></tr>")
                        .arg(d->videoCodec.toHtmlEscaped());
            }
            if (d->videoBitrate > 0) {
                html += QString("<tr><td class='label'>码率：</td><td class='value highlight'>%1 kbps</td></tr>")
                        .arg(QString::number(d->videoBitrate, 'f', 1));
            }
            if (d->videoFps > 0) {
                html += QString("<tr><td class='label'>帧率：</td><td class='value highlight'>%1 fps</td></tr>")
                        .arg(QString::number(d->videoFps, 'f', 2));
            }
            // 帧信息行：根据状态显示
                int totalFrames = d->iCount + d->pCount + d->bCount;
            if (d->isFetching) {
                // 正在获取，动态显示点
                QString dots = QString(".").repeated(d->fetchingDots);
                html += QString("<tr><td class='label'>帧信息：</td><td class='value'><a href='stop-getframeinfo'>正在获取%1 %2(I:%3,P:%4,B:%5)</a></td></tr>")
                        .arg(dots).arg(totalFrames).arg(d->iCount).arg(d->pCount).arg(d->bCount);
            } else if (d->hasFrameInfo) {
                html += QString("<tr><td class='label'>总帧数：</td><td class='value'><a href='reget-frame-info'>%1 (I:%2,P:%3,B:%4)</a></td></tr>")
                        .arg(totalFrames).arg(d->iCount).arg(d->pCount).arg(d->bCount);
                if (d->avgGop > 0) {
                    html += QString("<tr><td class='label'>I帧间隔：</td><td class='value'>平均 %1s，最大 %2s</td></tr>")
                            .arg(QString::number(d->avgGop, 'f', 2))
                            .arg(QString::number(d->maxGop, 'f', 2));
                } else {
                    html += QString("<tr><td class='label'>I帧间隔：</td><td class='value'>N/A</td></tr>");
                }
            } else {
                // 未获取，显示可点击链接
                html += QString("<tr><td class='label'>帧信息：</td><td class='value'><a href='get-frame-info' style='color:#4CAF50;'>点击获取</a></td></tr>");
            }
            html += "</table>";
            html += "</td>";
        }

        if (d->hasAudio) {
            html += "<td width='50%' valign='top'>";
            html += "<h2>音频信息</h2>";
            html += "<table class='info-table'>";
            if (!d->audioCodec.isEmpty()) {
                html += QString("<tr><td class='label'>编码：</td><td class='value'>%1</td></tr>")
                        .arg(d->audioCodec.toHtmlEscaped());
            }
            if (d->audioBitrate > 0) {
                html += QString("<tr><td class='label'>码率：</td><td class='value highlight'>%1 kbps</td></tr>")
                        .arg(QString::number(d->audioBitrate, 'f', 1));
            }
            if (d->audioChannels > 0) {
                QString channelStr;
                switch (d->audioChannels) {
                    case 1: channelStr = "单声道"; break;
                    case 2: channelStr = "立体声"; break;
                    case 4: channelStr = "4.0声道"; break;
                    case 6: channelStr = "5.1声道"; break;
                    case 8: channelStr = "7.1声道"; break;
                    default: channelStr = QString("%1声道").arg(d->audioChannels); break;
                }
                html += QString("<tr><td class='label'>声道：</td><td class='value highlight'>%1</td></tr>")
                        .arg(channelStr.toHtmlEscaped());
            }
            if (d->audioSampleRate > 0) {
                html += QString("<tr><td class='label'>采样率：</td><td class='value highlight'>%1 Hz</td></tr>")
                        .arg(QString::number(d->audioSampleRate));
            }
            html += "</table>";
            html += "</td>";
        }

        html += "</tr></table>";
        html += "</body></html>";
        return html;
    };

    // ========== 构建纯文本的函数（用于复制） ==========
    auto buildPlainText = [](std::shared_ptr<FileInfoData> d) -> QString {
        QString text = "媒体文件信息\n";
        text += QString("=").repeated(40) + "\n\n";
        text += "基本信息:\n";
        text += QString("-").repeated(20) + "\n";
        text += QString("文件名: %1\n").arg(d->fileName);
        text += QString("路径: %1\n").arg(d->filePath);
        text += QString("大小: %1 MB\n").arg(d->fileSizeMB, 0, 'f', 2);
        text += QString("创建时间: %1\n").arg(d->created.toString("yyyy-MM-dd HH:mm:ss"));
        text += QString("修改时间: %1\n").arg(d->modified.toString("yyyy-MM-dd HH:mm:ss"));
        text += "\n" + QString("-").repeated(40) + "\n";

        if (d->hasVideo) {
            text += "视频信息:\n";
            text += QString("-").repeated(20) + "\n";
            text += QString("时长: %1\n").arg(d->durationStr);
            text += QString("分辨率: %1 × %2 像素\n").arg(d->videoWidth).arg(d->videoHeight);
            if (!d->videoCodec.isEmpty())
                text += QString("视频编码: %1\n").arg(d->videoCodec);
            if (d->videoBitrate > 0)
                text += QString("视频码率: %1 kbps\n").arg(d->videoBitrate, 0, 'f', 2);
            if (d->videoFps > 0)
                text += QString("帧率: %1 帧/秒\n").arg(d->videoFps, 0, 'f', 2);
            // 帧信息（仅当已获取时才加入文本）
            if (d->hasFrameInfo) {
                int totalFrames = d->iCount + d->pCount + d->bCount;
                text += QString("帧数: %1 (I:%2, P:%3, B:%4)\n")
                        .arg(totalFrames).arg(d->iCount).arg(d->pCount).arg(d->bCount);
                if (d->avgGop > 0)
                    text += QString("I帧间隔: 平均 %1 s, 最大 %2 s\n")
                            .arg(d->avgGop, 0, 'f', 2).arg(d->maxGop, 0, 'f', 2);
                else
                    text += "I帧间隔: N/A\n";
            }
            text += "\n" + QString("-").repeated(40) + "\n";
        }

        if (d->hasAudio) {
            text += "音频信息:\n";
            text += QString("-").repeated(20) + "\n";
            if (!d->audioCodec.isEmpty())
                text += QString("音频编码: %1\n").arg(d->audioCodec);
            if (d->audioBitrate > 0)
                text += QString("音频比特率: %1 kbps\n").arg(d->audioBitrate, 0, 'f', 2);
            if (d->audioChannels > 0) {
                QString channelStr;
                switch (d->audioChannels) {
                    case 1: channelStr = "单声道"; break;
                    case 2: channelStr = "立体声"; break;
                    case 4: channelStr = "4.0声道"; break;
                    case 6: channelStr = "5.1声道"; break;
                    case 8: channelStr = "7.1声道"; break;
                    default: channelStr = QString("%1声道").arg(d->audioChannels); break;
                }
                text += QString("声道: %1\n").arg(channelStr);
            }
            if (d->audioSampleRate > 0)
                text += QString("采样率: %1 Hz\n").arg(d->audioSampleRate);
            text += "\n" + QString("-").repeated(40) + "\n";
        }
        return text;
    };

    // 创建对话框
    QDialog *infoDialog = new QDialog(this->window());
    infoDialog->setWindowTitle("媒体文件信息");
    infoDialog->resize(600, 550);
    infoDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                               Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint |
                               Qt::WindowMinimizeButtonHint);
    infoDialog->setAttribute(Qt::WA_DeleteOnClose);

    infoDialog->setStyleSheet(R"(
        QDialog {
            background-color: #1a1a1a;
            border: 1px solid #333;
        }
        QTextBrowser {
            background-color: #1a1a1a;
            color: #e0e0e0;
            border: none;
            selection-background-color: #4CAF50;
            selection-color: #ffffff;
        }
        QPushButton {
            background-color: #2a2a2a;
            color: #e0e0e0;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 8px 16px 8px 10px;
            font-size: 14px;
            width:60;
            height:25;
        }
        QPushButton:hover {
            background-color: #3a3a3a;
            border-color: #4a4a4a;
        }
        QPushButton:pressed {
            background-color: #4CAF50;
            color: #ffffff;
        }
        QPushButton:focus {
            border-color: #4CAF50;
        }
        /* 修复滚动条设置 */
        QScrollBar:vertical {
            width: 10px;
            background: #202020;
            border: none;
        }
        QScrollBar::handle:vertical {
            min-height: 20px; /* 减小滚动条手柄最小高度 */
            background: #414141;
            margin: 0px 3px; /* 移除滚动条手柄的边距 */
            border-radius: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgb(80, 80, 80);
           margin: 0px 2px;
        }
        QScrollBar::sub-line:vertical {
            height: 0px;
            background: transparent;
            image: url(:/Black/arrowTop);
            subcontrol-position: top;
        }
        QScrollBar::add-line:vertical {
            height: 0px;
            background: transparent;
            image: url(:/Black/arrowBottom);
            subcontrol-position: bottom;
        }
        QScrollBar::sub-line:vertical:hover {
            background: rgb(68, 69, 73);
        }
        QScrollBar::add-line:vertical:hover {
            background: rgb(68, 69, 73);
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
    )");

    if (!this->windowIcon().isNull())
        infoDialog->setWindowIcon(this->windowIcon());

    QTextBrowser *textBrowser = new QTextBrowser(infoDialog);
    textBrowser->setHtml(buildHtml(data));
    textBrowser->setOpenExternalLinks(false);
    textBrowser->document()->setDefaultStyleSheet(R"(
        body {
            font-family: 'Microsoft YaHei', Arial, sans-serif;
            background-color: #1a1a1a;
            color: #e0e0e0;
            margin: 0;
            padding: 10px;
        }
    )");
    textBrowser->setFrameStyle(QFrame::NoFrame);
    textBrowser->viewport()->setStyleSheet("background-color: #1a1a1a;");

    QPushButton *copyButton = new QPushButton("复制信息", infoDialog);
    QPushButton *closeButton = new QPushButton("关闭", infoDialog);
    copyButton->setFocusPolicy(Qt::NoFocus);
    closeButton->setFocusPolicy(Qt::NoFocus);

    // ========== 处理超链接点击（获取帧信息） ==========
    QPointer<QDialog> dialogPtr(infoDialog); // 用于检查对话框是否存活

    QObject::connect(textBrowser, &QTextBrowser::anchorClicked, infoDialog,
        [textBrowser, data, buildHtml, dialogPtr](const QUrl &link) mutable {
            if (link.toString() == "reget-frame-info")
            {
                data->isStop =false; data->isFetching = false;data->iCount=0;data->pCount=0;data->bCount=0;}
            else {
                if (link.toString() == "stop-getframeinfo")data->isStop =true;
                if (link.toString() != "get-frame-info") return;
            }
            // 如果正在获取中，忽略本次点击
            if (data->isFetching) return;
            // 开始获取帧信息
            data->isFetching = true;
            data->fetchingDots = 1;
            textBrowser->setHtml(buildHtml(data));

            // 创建定时器，用于动态更新点
            data->timer = new QTimer();   // 将定时器设为对话框的子对象
            data->timer->setInterval(500);
            QObject::connect(data->timer, &QTimer::timeout, [textBrowser, data, buildHtml]() {
                if (!data->isFetching) return;      // 已结束，不再更新
                data->fetchingDots = (data->fetchingDots % 3) + 1;
                textBrowser->setHtml(buildHtml(data));
            });
            data->timer->start();

            // 在后台线程中执行帧遍历
            QtConcurrent::run([=]() {
                // 后台线程：重新打开文件，遍历视频帧
                AVFormatContext *fmtCtx = nullptr;
                if (avformat_open_input(&fmtCtx, data->filePathUtf8.constData(), nullptr, nullptr) < 0) {
                    QMetaObject::invokeMethod(qApp, [=]() {
                        if (!dialogPtr) return; // 对话框已关闭
                        data->isFetching = false;
                        data->timer->stop();
                        data->timer->deleteLater();
                        textBrowser->setHtml(buildHtml(data));
                    });
                    return;
                }
                if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
                    avformat_close_input(&fmtCtx);
                    QMetaObject::invokeMethod(qApp, [=]() {
                        if (!dialogPtr) return;
                        data->isFetching = false;
                        data->timer->stop();
                        data->timer->deleteLater();
                        textBrowser->setHtml(buildHtml(data));
                    });
                    return;
                }

                // 找到视频流
                AVStream *videoStream = nullptr;
                for (unsigned int i = 0; i < fmtCtx->nb_streams; ++i) {
                    if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                        videoStream = fmtCtx->streams[i];
                        break;
                    }
                }

                if (!videoStream) {
                    avformat_close_input(&fmtCtx);
                    QMetaObject::invokeMethod(qApp, [=]() {
                        if (!dialogPtr) return;
                        data->isFetching = false;
                        data->timer->stop();
                        data->timer->deleteLater();
                        textBrowser->setHtml(buildHtml(data));
                    });
                    return;
                }

                AVCodecParameters *codecParams = videoStream->codecpar;
                AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
                if (!codec) {
                    avformat_close_input(&fmtCtx);
                    QMetaObject::invokeMethod(qApp, [=]() {
                        if (!dialogPtr) return;
                        data->isFetching = false;
                        data->timer->stop();
                        data->timer->deleteLater();
                        textBrowser->setHtml(buildHtml(data));
                    });
                    return;
                }

                AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
                if (!codecCtx) {
                    avformat_close_input(&fmtCtx);
                    QMetaObject::invokeMethod(qApp, [=]() {
                        if (!dialogPtr) return;
                        data->isFetching = false;
                        data->timer->stop();
                        data->timer->deleteLater();
                        textBrowser->setHtml(buildHtml(data));
                    });
                    return;
                }

                avcodec_parameters_to_context(codecCtx, codecParams);
                if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
                    avcodec_free_context(&codecCtx);
                    avformat_close_input(&fmtCtx);
                    QMetaObject::invokeMethod(qApp, [=]() {
                        if (!dialogPtr) return;
                        data->isFetching = false;
                        data->timer->stop();
                        data->timer->deleteLater();
                        textBrowser->setHtml(buildHtml(data));
                    });
                    return;
                }

                AVPacket *pkt = av_packet_alloc();
                AVFrame *frame = av_frame_alloc();
                int i=0;
                QList<double> iFrameTimes;

                while (!data->isStop && av_read_frame(fmtCtx, pkt) >= 0 && (i<2000 || data->iCount <3)) {
                    i++;
                    if (pkt->stream_index == videoStream->index) {
                        avcodec_send_packet(codecCtx, pkt);
                        while (avcodec_receive_frame(codecCtx, frame) == 0) {
                            switch (frame->pict_type) {
                            case AV_PICTURE_TYPE_I: data->iCount++; break;
                            case AV_PICTURE_TYPE_P: data->pCount++; break;
                            case AV_PICTURE_TYPE_B: data->bCount++; break;
                            default: break;
                            }
                            if (frame->pict_type == AV_PICTURE_TYPE_I) {
                                int64_t ts = frame->best_effort_timestamp;
                                if (ts == AV_NOPTS_VALUE) ts = frame->pts;
                                if (ts != AV_NOPTS_VALUE) {
                                    double time_sec = ts * av_q2d(videoStream->time_base);
                                    iFrameTimes.append(time_sec);
                                }
                            }
                            av_frame_unref(frame);
                        }
                    }
                    av_packet_unref(pkt);
                }

                // 刷新解码器
                avcodec_send_packet(codecCtx, nullptr);
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    av_frame_unref(frame);
                }

                // 计算间隔
                double avgGop = 0.0, maxGop = 0.0;
                if (iFrameTimes.size() > 1) {
                    double sumGap = 0.0;
                    maxGop = 0.0;
                    for (int i = 1; i < iFrameTimes.size(); ++i) {
                        double gap = iFrameTimes[i] - iFrameTimes[i-1];
                        sumGap += gap;
                        if (gap > maxGop) maxGop = gap;
                    }
                    avgGop = sumGap / (iFrameTimes.size() - 1);
                }

                av_packet_free(&pkt);
                av_frame_free(&frame);
                avcodec_free_context(&codecCtx);
                avformat_close_input(&fmtCtx);

                // 回到主线程更新UI
                QMetaObject::invokeMethod(qApp, [=]() {
                    if (!dialogPtr) return; // 对话框已关闭，不再更新
                    // 更新数据
//                    data->iCount = iCount;
//                    data->pCount = pCount;
//                    data->bCount = bCount;
                    data->avgGop = avgGop;
                    data->maxGop = maxGop;
                    data->hasFrameInfo = true;
                    data->isFetching = false;

                    // 停止并删除定时器
                    data->timer->stop();
                    data->timer->deleteLater();

                    // 更新显示
                    textBrowser->setHtml(buildHtml(data));
                });
            });
        });
    QObject::connect(infoDialog, &QDialog::finished, infoDialog, [data]() {
        data->isStop = true;                // 通知后台线程退出循环
        if (data->timer) {
            data->timer->stop();             // 停止定时器，不再触发更新
            // 定时器是对话框的子对象，会随对话框自动销毁，无需手动 delete
        }
    });
    // 复制按钮：使用最新的纯文本
    QObject::connect(copyButton, &QPushButton::clicked, infoDialog,
        [data, buildPlainText, infoDialog]() {
            QString plain = buildPlainText(data);
            QApplication::clipboard()->setText(plain);
            QMessageBox msgBox(infoDialog);
            msgBox.setWindowTitle("提示");
            msgBox.setText("媒体文件信息已复制到剪贴板");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(R"(
                QMessageBox {
                    background-color: #1a1a1a;
                    color: white;
                    font-family: 'Microsoft YaHei', sans-serif;
                }
                QMessageBox QLabel {
                    color: white;
                    font-size: 16px;
                    padding: 15px 0;
                    min-height: 40px;
                    min-width: 260px;
                    qproperty-alignment: 'AlignCenter';
                }
                QMessageBox QPushButton {
                    min-height:25;
                    background-color: #2a2a2a;
                    color: white;
                    border: 1px solid #3a3a3a;
                    border-radius: 4px;
                    padding: 6px 12px;
                    min-width: 70px;
                    font-size: 12px;
                    font-family: 'Microsoft YaHei', sans-serif;
                }
                QMessageBox QPushButton:hover {
                    background-color: #3a3a3a;
                }
                QMessageBox QPushButton:pressed {
                    background-color: #4CAF50;
                }
            )");
            msgBox.exec();
        });

    QObject::connect(closeButton, &QPushButton::clicked, infoDialog, &QDialog::close);

    // 布局
    QVBoxLayout *mainLayout = new QVBoxLayout(infoDialog);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(textBrowser);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(copyButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(closeButton);
    buttonLayout->addSpacing(20);
    buttonLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(buttonLayout);

    infoDialog->show();
    infoDialog->raise();
    infoDialog->activateWindow();
}
#include <QRegularExpression>
#include <QInputDialog>

void MediaList::GetLyric()
{
    int currentIndex = currentRow();
    if (currentIndex < 0) {
        QMessageBox::information(this, "提示", "请先选择一首歌曲");
        return;
    }

    QListWidgetItem *item = this->item(currentIndex);
    if (!item) return;

    QString filePath = item->toolTip();
    m_renameOldPath = item->toolTip();
    qDebug() << "filePath item->toolTip() = " << filePath;
    QFileInfo fileInfo(filePath);
    // 检查文件是否存在
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "错误", "文件不存在或已被删除！");
        return;
    }

    // 解析文件名获取歌手和歌名
    QString fileName = fileInfo.fileName();
    QString baseName = fileInfo.completeBaseName(); // 不带扩展名的文件名

    QString singer, song;

    // 备选方法：基于"-"分割字符串
        QStringList parts = baseName.split("-", Qt::SkipEmptyParts);
        if (parts.size() > 1) {
            // 最后一个部分作为歌名
            song = parts.last().trimmed();

            // 前面所有部分合并作为歌手
            QStringList singerParts;
            for (int i = 0; i < parts.size() - 1; i++) {
                singerParts.append(parts[i].trimmed());
            }
            singer = singerParts.join(" & ");

            // 清理歌手名
            singer = singer.replace("、", " ");
            singer = singer.replace("_", " ");
            singer = singer.replace("-", " ");
            singer = singer.replace(QRegularExpression("\\s+"), " "); // 合并多余空格
        } else if (parts.size() == 1) {
            // 没有"-"的情况
            QString singlePart = parts[0].trimmed();

            // 尝试用空格或下划线分割
            QStringList separators = {"_", "－", " ",".","~", "  "}; // 可以添加更多分隔符
            QStringList spaceParts;
            bool hasMultiParts = false;

            // 尝试每个分隔符
            for (const QString& sep : separators) {
                spaceParts = singlePart.split(sep, Qt::SkipEmptyParts);
                if (spaceParts.size() > 1) {
                    hasMultiParts = true;
                    break;
                }
            }

            if (hasMultiParts) {
                // 最后一个词作为歌名，其他作为歌手
                song = spaceParts.last();
                spaceParts.removeLast();
                singer = spaceParts.join(" ");
            } else {
                // 如果都没有分隔符，整个文件名作为歌名
                singer = "";
                song = singlePart;
            }
        }

    // 获取歌曲时长（需要从播放列表中获取）
    // 这里假设播放列表中有存储时长信息
    qint64 durationMs = 0;
    QVariant durationVar = item->data(Qt::UserRole + 1);
    if (durationVar.isValid()) {
        durationMs = parseDurationString(durationVar.toString());
    }
    // 创建并显示歌词搜索对话框
    if (!m_lyricDialog) {
            m_lyricDialog = new LyricSearchDialog(this->window());
            connect(m_lyricDialog, &LyricSearchDialog::lyricDownloaded,
                    [=](const QString& lrcPath) {
                        // 歌词下载完成后的处理
                        qDebug() << "歌词下载完成:" << lrcPath;
                        QMessageBox::information(this, "成功!",
                       QString("歌词已保存到:\n%1").arg(lrcPath));
                      if (GlobalVars::currentPlayFileName() == m_renameOldPath){
                          emit SigSaveSubtitleFile(lrcPath);}
                    });
        }
    m_lyricDialog->setSearchParams(singer, song, durationMs, filePath);
    m_lyricDialog->show();
    qApp->processEvents();
    QPoint currentPos = m_lyricDialog->pos();
    qDebug() << "currentPos.y():==== " << currentPos.y() ;
    if (currentPos.y() < 0) {
        m_lyricDialog->move(currentPos.x(), 0);
    }
    m_lyricDialog->raise();
    m_lyricDialog->activateWindow();
    // 延迟100毫秒后自动点击搜索按钮
        QTimer::singleShot(100, this, [this]() {
            if (m_lyricDialog && m_lyricDialog->isVisible()) {
                // 假设LyricSearchDialog有一个名为searchButton的按钮
                // 你需要根据实际对话框的按钮名称进行调整
                QPushButton *searchButton = m_lyricDialog->findChild<QPushButton*>("btnSearch");
                if (searchButton) {
                    searchButton->click();
                }
            }
        });
}

qint64 MediaList::parseDurationString(const QString& durationStr) {
    if (durationStr.isEmpty() || durationStr == "--:--") {
        return 0;
    }

    QString cleanStr = durationStr.trimmed();
    QStringList parts = cleanStr.split(':');

    if (parts.size() == 2) {
        bool ok1, ok2;
        int minutes = parts[0].toInt(&ok1);
        int seconds = parts[1].toInt(&ok2);

        if (ok1 && ok2 && minutes >= 0 && seconds >= 0 && seconds < 60) {
            return (minutes * 60 + seconds) * 1000;
        }
    } else if (parts.size() == 3) {
        bool ok1, ok2, ok3;
        int hours = parts[0].toInt(&ok1);
        int minutes = parts[1].toInt(&ok2);
        int seconds = parts[2].toInt(&ok3);

        if (ok1 && ok2 && ok3 && hours >= 0 && minutes >= 0 && minutes < 60 && seconds >= 0 && seconds < 60) {
            return (hours * 3600 + minutes * 60 + seconds) * 1000;
        }
    }

    return 0; // 解析失败
}

#include <QLabel>

// 新增：开始重命名当前项
void MediaList::startRenameCurrentItem()
{
    int index = currentRow();
    if (index >= 0 && index < count()) {
        // 先尝试直接编辑（如果支持）
        if (editTriggers() & QAbstractItemView::SelectedClicked) {
            edit(currentIndex());
        } else {
            // 否则弹出对话框
            showRenameDialog(index);
        }
    }
}

// 新增：重命名菜单项触发
void MediaList::RenameFile()
{
    int index = currentRow();
    if (index >= 0 && index < count()) {
        showRenameDialog(index);
    }
}
//#include "SimpleMessageBox.h"
// 新增：显示重命名对话框
void MediaList::showRenameDialog(int index)
{
    if (index < 0 || index >= count()) {
        return;
    }

    QListWidgetItem *item = this->item(index);
    if (!item) {
        return;
    }

    QString oldPath = item->toolTip();
    QFileInfo fileInfo(oldPath);

    // 检查文件是否存在
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "错误",
                            QString("文件不存在或已被删除！\n\n%1").arg(fileInfo.fileName()));
        return;
    }

    // 记录重命名信息
    m_isRenaming = true;
    m_renameIndex = index;
    m_renameOldPath = oldPath;

    // 如果是正在播放的文件，获取当前播放位置
    qint64 playPosition = 0;
    bool isPlaying = (index == GlobalVars::currentPlayIndex());

    // 创建重命名对话框
    QDialog renameDialog(this->window());
    renameDialog.setWindowTitle("重命名文件");
    renameDialog.setFixedSize(460, 400);  // 增加高度以容纳错误提示
    renameDialog.setWindowFlags(renameDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 设置样式
    renameDialog.setStyleSheet(R"(
        QDialog {
            background-color: #2a2a2a;
            color: #e0e0e0;
            font-family: 'Microsoft YaHei', Arial, sans-serif;
        }
        QLabel {
            color: #e0e0e0;
            font-size: 14px;
            padding: 5px;
        }
        QLineEdit {
            background-color: #1a1a1a;
            color: #ffffff;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 8px;
            font-size: 14px;
            selection-background-color: #4CAF50;
        }
        QLineEdit:focus {
            border-color: #4CAF50;
        }
        QLineEdit.error {
            border-color: #ff6b6b;
            border-width: 2px;
        }
        QPushButton {
            background-color: #3a3a3a;
            color: #e0e0e0;
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 14px;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
            border-color: #5a5a5a;
        }
        QPushButton:pressed {
            background-color: #4CAF50;
            color: #ffffff;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #808080;
            border-color: #3a3a3a;
        }
    )");

    // 创建界面元素
    QVBoxLayout *mainLayout = new QVBoxLayout(&renameDialog);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 原文件名
    QLabel *oldNameLabel = new QLabel("原文件名:", &renameDialog);
    QLineEdit *oldNameEdit = new QLineEdit(fileInfo.fileName(), &renameDialog);
    oldNameEdit->setReadOnly(true);
    oldNameEdit->setStyleSheet("background-color: #333333;");

    // 新文件名
    QLabel *newNameLabel = new QLabel("新文件名 (不带扩展名):", &renameDialog);
    QLineEdit *newNameEdit = new QLineEdit(fileInfo.completeBaseName(), &renameDialog); // 去掉扩展名
    newNameEdit->setFocus();
    newNameEdit->selectAll();

    // 文件信息提示
    QString fileType = fileInfo.suffix().toUpper();
    QString location = fileInfo.absolutePath();
    QLabel *infoLabel = new QLabel(
        QString("文件类型: %1\n文件位置: %2").arg(fileType).arg(location),
        &renameDialog
    );
    infoLabel->setStyleSheet("color: #aaaaaa; font-size: 12px;");
    infoLabel->setWordWrap(true);

    // 错误提示标签
    QLabel *errorLabel = new QLabel("", &renameDialog);
    errorLabel->setStyleSheet("color: #ff6b6b; font-size: 12px; padding: 5px;");
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *cancelButton = new QPushButton("取消", &renameDialog);
    QPushButton *renameButton = new QPushButton("重命名", &renameDialog);
    renameButton->setDefault(true);

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(renameButton);

    // 添加到布局
    mainLayout->addWidget(oldNameLabel);
    mainLayout->addWidget(oldNameEdit);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(newNameLabel);
    mainLayout->addWidget(newNameEdit);
    mainLayout->addWidget(errorLabel);  // 添加错误提示标签
    mainLayout->addSpacing(8);
    mainLayout->addWidget(infoLabel);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(cancelButton, &QPushButton::clicked, &renameDialog, &QDialog::reject);

    // 重命名按钮的自定义处理
    connect(renameButton, &QPushButton::clicked, [&]() {
        QString newName = newNameEdit->text().trimmed();

        // 清空错误提示
        errorLabel->clear();
        errorLabel->setVisible(false);
        newNameEdit->setStyleSheet("");  // 清除错误样式

        // 检查新文件名是否为空或与原名相同
        if (newName.isEmpty()) {
            errorLabel->setText("文件名不能为空");
            errorLabel->setVisible(true);
            newNameEdit->setFocus();
            newNameEdit->selectAll();
            return;
        }

        if (newName == fileInfo.completeBaseName()) {
            errorLabel->setText("新文件名与原文件名相同");
            errorLabel->setVisible(true);
            newNameEdit->setFocus();
            newNameEdit->selectAll();
            return;
        }

        // 添加扩展名
        QString fullNewName = newName;
        if (!fileInfo.suffix().isEmpty()) {
            fullNewName = newName + "." + fileInfo.suffix();
        }

        // 验证文件名
        QString errorMsg;
        if (!validateNewFileName(oldPath, fullNewName, errorMsg)) {
            errorLabel->setText(errorMsg);
            errorLabel->setVisible(true);
            newNameEdit->setStyleSheet("border-color: #ff6b6b; border-width: 2px;");  // 设置错误样式
            newNameEdit->setFocus();
            newNameEdit->selectAll();
            return;
        }

        // 所有验证通过，执行重命名
        if (isPlaying) {
            // 如果是正在播放的文件，先获取播放位置
            emit SigRequestPlayPosition();  // 发送信号请求播放位置
            // 等待一小段时间让播放器完全停止
            QEventLoop loop;
            QTimer::singleShot(100, &loop, &QEventLoop::quit);
            loop.exec();
        }

        if (performRename(index, fullNewName)) {
            // 重命名成功
            QString successMessage = QString("文件已重命名为:\n%1").arg(fullNewName);

            // 检查歌词文件是否也被重命名了
            QString oldBaseName = fileInfo.completeBaseName();
            QString oldLyricPath = fileInfo.absoluteDir().absoluteFilePath(oldBaseName + ".lrc");
            QString newLyricPath = fileInfo.absoluteDir().absoluteFilePath(newName + ".lrc");

            if (QFile::exists(oldLyricPath)) {
                if (QFile::exists(newLyricPath)) {
                    successMessage += QString("\n\n对应的歌词文件也已重命名为:\n%1.lrc").arg(newName);
                }
            }

            // 关闭对话框
            renameDialog.accept();

            // 如果是正在播放的文件，先恢复播放
            if (isPlaying) {
                // 获取新路径
                QString newPath = QFileInfo(oldPath).absoluteDir().absoluteFilePath(fullNewName);
                // 更新播放索引
                GlobalVars::currentPlayIndex() = index;
                // 发送恢复播放信号
                emit SigResumePlay(index, playPosition);
            }
           // SimpleMessageBox::information(this, "成功", successMessage,450,140);
//            // 延迟显示成功消息，让恢复播放先开始
//            QTimer::singleShot(100, this, [this, successMessage]() {
//                QMessageBox msgBox(this);
//                msgBox.setWindowTitle("成功");
//                msgBox.setText(successMessage);
//                msgBox.setIcon(QMessageBox::Information);
//                msgBox.setStandardButtons(QMessageBox::Ok);
//                msgBox.setStyleSheet(R"(
//                    QMessageBox {
//                        background-color: #2a2a2a;
//                        color: #e0e0e0;
//                        font-family: 'Microsoft YaHei', Arial, sans-serif;
//                    }
//                    QMessageBox QLabel {
//                        color: #e0e0e0;
//                        font-size: 14px;
//                        min-width: 300px;
//                        min-height: 80px;
//                    }
//                    QMessageBox QPushButton {
//                        background-color: #3a3a3a;
//                        color: #e0e0e0;
//                        border: 1px solid #4a4a4a;
//                        border-radius: 4px;
//                        padding: 8px 16px;
//                        font-size: 14px;
//                        min-width: 80px;
//                    }
//                )");
//                msgBox.exec();
//            });

        } else {
            // 重命名失败
            errorLabel->setText("重命名失败，请检查文件权限或文件是否被占用。");
            errorLabel->setVisible(true);
            newNameEdit->setStyleSheet("border-color: #ff6b6b; border-width: 2px;");
            newNameEdit->setFocus();
            newNameEdit->selectAll();

            // 如果是正在播放的文件，恢复播放
            if (isPlaying) {
                emit SigResumePlay(index, playPosition);
            }
        }
    });

    // 输入框变化时清空错误提示
    connect(newNameEdit, &QLineEdit::textChanged, [=](const QString &text) {
        QString trimmedText = text.trimmed();
        renameButton->setEnabled(!trimmedText.isEmpty() && trimmedText != fileInfo.completeBaseName());

        // 清空错误提示
        errorLabel->clear();
        errorLabel->setVisible(false);
        newNameEdit->setStyleSheet("");
    });


    // 自定义事件过滤器处理回车键
    QObject::connect(&renameDialog, &QDialog::destroyed, &renameDialog, [&renameDialog]() {
        renameDialog.removeEventFilter(&renameDialog);
    });

    // 显示对话框
    renameDialog.exec();

    m_isRenaming = false;
}

// 新增：验证新文件名
bool MediaList::validateNewFileName(const QString &oldPath, const QString &newName, QString &errorMsg)
{
    QFileInfo oldInfo(oldPath);
    QDir dir = oldInfo.absoluteDir();

    // 检查新文件名是否为空
    if (newName.isEmpty()) {
        errorMsg = "文件名不能为空";
        return false;
    }

    // 检查是否包含非法字符（根据操作系统）
#ifdef Q_OS_WIN
    QString invalidChars = "<>:\"/\\|?*";
    for (QChar ch : invalidChars) {
        if (newName.contains(ch)) {
            errorMsg = QString("文件名不能包含字符: %1").arg(ch);
            return false;
        }
    }

    // Windows 保留文件名
    QStringList reservedNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    QString nameWithoutExt = newName;
    int dotIndex = nameWithoutExt.lastIndexOf('.');
    if (dotIndex != -1) {
        nameWithoutExt = nameWithoutExt.left(dotIndex);
    }

    if (reservedNames.contains(nameWithoutExt.toUpper())) {
        errorMsg = "文件名是系统保留名称，请使用其他名称";
        return false;
    }
#endif

    // 检查文件名长度限制
    if (newName.length() > 255) {
        errorMsg = "文件名过长（最大255个字符）";
        return false;
    }

    // 检查新文件是否已存在
    QString newPath = dir.absoluteFilePath(newName);
    if (QFile::exists(newPath)) {
        errorMsg = "该文件名已存在，请使用其他名称";
        return false;
    }

    return true;
}

// 新增：执行重命名操作
bool MediaList::performRename(int index, const QString &newName)
{
    if (index < 0 || index >= count()) {
        return false;
    }

    QListWidgetItem *item = this->item(index);
    if (!item) {
        return false;
    }

    QString oldPath = item->toolTip();
    QFileInfo oldInfo(oldPath);
    QDir dir = oldInfo.absoluteDir();

    QString newPath = dir.absoluteFilePath(newName);

    // 执行重命名
    QFile file(oldPath);
    if (file.rename(newPath)) {
        // 检查并重命名对应的 .lrc 歌词文件
                QString oldBaseName = oldInfo.completeBaseName();
                QString oldLyricPath = dir.absoluteFilePath(oldBaseName + ".lrc");
                QFile oldLyricFile(oldLyricPath);

                if (oldLyricFile.exists()) {
                    // 构建新的歌词文件名
                    QString newBaseName = newName;
                    int lastDotIndex = newName.lastIndexOf('.');
                    if (lastDotIndex != -1) {
                        newBaseName = newName.left(lastDotIndex);
                    }

                    QString newLyricPath = dir.absoluteFilePath(newBaseName + ".lrc");

                    // 检查新歌词文件是否已存在
                    if (!QFile::exists(newLyricPath)) {
                        if (oldLyricFile.rename(newLyricPath)) {
                            qDebug() << "歌词文件重命名成功:" << oldLyricPath << "->" << newLyricPath;
                        } else {
                            qDebug() << "歌词文件重命名失败:" << oldLyricFile.errorString();
                        }
                    } else {
                        qDebug() << "歌词文件已存在，跳过重命名:" << newLyricPath;
                    }
                }
        // 更新列表项
        updateListAfterRename(index, oldPath, newPath);
        return true;
    } else {
        qDebug() << "重命名失败:" << file.errorString();
        return false;
    }
}

// 新增：更新列表显示
void MediaList::updateListAfterRename(int index, const QString &oldPath, const QString &newPath)
{
    QListWidgetItem *item = this->item(index);
    if (!item) {
        return;
    }

    QFileInfo newInfo(newPath);

    // 更新工具提示（完整路径）
    item->setToolTip(newPath);

    // 更新显示文本（文件名）
    item->setText(newInfo.fileName());

    // 更新用户数据
    item->setData(Qt::UserRole, QVariant(newPath));

    // 发出重命名信号，让Playlist更新其内部数据结构
    emit SigRenameFile(index, oldPath, newPath);

    // 如果当前是选中项，可能需要更新其他UI
    if (index == currentRow()) {
        setCurrentRow(index);
    }
}

// 新增：从外部重命名文件（供Playlist调用）
bool MediaList::renameFileInList(int index, const QString &newName)
{
    return performRename(index, newName);
}
