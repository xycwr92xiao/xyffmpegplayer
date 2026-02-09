#include "lyricsearch.h"
#include "ui_lyricsearch.h"  // 注意：这个头文件是uic工具从UI文件生成的
#include "simplemessagebox.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QInputDialog>
#include <QProgressDialog>
#include <QDesktopServices>

LyricSearchDialog::LyricSearchDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LyricSearch)
    , m_hasExistingLyric(false)
{
    ui->setupUi(this);

    // 设置窗口属性
    setWindowTitle("歌词搜索");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 应用深色主题样式
    applyDarkTheme();

    // 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);

    // 初始化定时器
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(10000); // 10秒超时

    m_downloadTimer = new QTimer(this);
    m_downloadTimer->setSingleShot(true);
    m_downloadTimer->setInterval(10000); // 10秒超时

    // 隐藏ID列（第5列，索引4）
    ui->tableResults->hideColumn(5);

    // 设置表格列宽
    ui->tableResults->setColumnWidth(0, 150); // 歌手
    ui->tableResults->setColumnWidth(1, 200); // 歌名
    ui->tableResults->setColumnWidth(2, 80);  // 时长
    ui->tableResults->setColumnWidth(3, 60);  // 热度
    // 默认隐藏右侧歌词对比面板
    updateLyricCompareDisplay(false);
    // 连接信号槽
    connect(ui->btnSearch, &QPushButton::clicked, this, &LyricSearchDialog::onSearchClicked);
    connect(ui->btnClear, &QPushButton::clicked, this, &LyricSearchDialog::onClearClicked);
    connect(ui->btnAddToPlaylist, &QPushButton::clicked, this, &LyricSearchDialog::saveShowLyricToFile);
    connect(ui->btnClose, &QPushButton::clicked, this, &LyricSearchDialog::onCloseClicked);

    // 表格点击事件
    connect(ui->tableResults, &QTableWidget::itemClicked, this, &LyricSearchDialog::onTableItemClicked);

    // 连接网络信号
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &LyricSearchDialog::onSearchReplyFinished);
    connect(m_searchTimer, &QTimer::timeout, this, &LyricSearchDialog::onSearchTimeout);
    connect(m_downloadTimer, &QTimer::timeout, this, &LyricSearchDialog::onDownloadTimeout);

    // 设置默认按钮
    ui->btnAddToPlaylist->setEnabled(false);
}

LyricSearchDialog::~LyricSearchDialog()
{
    delete ui;
}

void LyricSearchDialog::setSearchParams(const QString& singer, const QString& song,
                                       int durationMs, const QString& songFilePath)
{
    // 将搜索参数设置到搜索框
    if (!singer.isEmpty() && !song.isEmpty()) {
        ui->songerSearch->setText(QString("%1").arg(singer));
        ui->lineSearch->setText(QString("%2").arg(song));
    } else if (!song.isEmpty()) {
        ui->songerSearch->setText("");
        ui->lineSearch->setText(song);
    }
    m_keyWord = QString("%1 - %2").arg(singer).arg(song);
    ui->durationLabel->setText(formatDuration(durationMs));
    m_durationMs = durationMs;
    m_songFilePath = songFilePath;
    // 如果有歌曲文件路径，将"下载歌词"按钮改为"保存歌词"
    if (!songFilePath.isEmpty()) {
        ui->btnAddToPlaylist->setText("保存歌词");
        QFileInfo songFileInfo(m_songFilePath);
        // 使用歌曲文件的完整路径，只改变扩展名
        m_targetLrcPath = songFileInfo.absolutePath() + "/" +
                             songFileInfo.completeBaseName() + ".lrc";
        checkAndLoadExistingLyric();
    }
}

void LyricSearchDialog::startSearch()
{
    onSearchClicked();
}

void LyricSearchDialog::onSearchClicked()
{
    QString singer =ui->songerSearch->text().trimmed();
    QString song =ui->lineSearch->text().trimmed();
    if (song.isEmpty()) {
        QMessageBox::information(this, "提示", "请输入歌名");
        return;
    }
    QString keyword;
    if (singer.isEmpty()) keyword =ui->lineSearch->text().trimmed();
    else keyword = singer +" - "+ui->lineSearch->text().trimmed();
    if (keyword != m_keyWord){
        QFileInfo songFileInfo(m_songFilePath);
        QString fileName = cleanFileName(keyword);
                // 如果清理后的文件名是空的，使用歌曲文件的基本名
                if (fileName.isEmpty()) {
                    fileName = songFileInfo.completeBaseName();
                } else {
                    // 确保文件名不包含扩展名（如果有的话）
                    QFileInfo textFileInfo(fileName);
                    fileName = textFileInfo.completeBaseName();
                }
        m_targetLrcPath = songFileInfo.absolutePath() + "/" + fileName + ".lrc";
        qDebug() << "更新目标LRC路径:" << m_targetLrcPath;
        m_keyWord = keyword;
        m_openFilePath =true;
    }else m_openFilePath = false;
    // 清空表格
    ui->tableResults->setRowCount(0);
    m_candidates.clear();
    ui->btnAddToPlaylist->setEnabled(false);
    ui->textLyric->clear();
    ui->textLyric->setPlaceholderText("正在搜索歌词...");

    // 解析搜索关键词，尝试提取歌手和歌名
//    QString singer, song;
//    QRegularExpression regex("^(.+?)\\s*-\\s*(.+)$");
//    QRegularExpressionMatch match = regex.match(keyword);

//    if (match.hasMatch()) {
//        singer = match.captured(1).trimmed();
//        song = match.captured(2).trimmed();
//    } else {
//        // 如果没有"-"，整个作为歌名
//        song = keyword;
//    }

    // 构建搜索URL
    QString encodedKeyword = QUrl::toPercentEncoding(keyword);

    // 这里需要一个时长参数，如果没有提供，可以使用默认值或让用户输入
    // 在实际使用中，应该从歌曲文件获取时长或让用户输入
    int durationMs = m_durationMs; // 默认3分钟

    QString urlStr = QString("http://lyrics.kugou.com/search?ver=1&man=yes&client=pc&keyword=%1&duration=%2&hash=")
                     .arg(encodedKeyword)
                     .arg(durationMs);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    // 发送请求
    m_networkManager->get(request);
    m_searchTimer->start();
}

void LyricSearchDialog::onClearClicked()
{
    ui->lineSearch->clear();
    ui->tableResults->setRowCount(0);
    ui->textLyric->clear();
    ui->btnAddToPlaylist->setEnabled(false);
    m_candidates.clear();
}

void LyricSearchDialog::onAddToPlaylistClicked()
{
    int currentRow = ui->tableResults->currentRow();
    if (currentRow < 0 || currentRow >= m_candidates.size()) {
        QMessageBox::warning(this, "提示", "请先选择一首歌曲");
        return;
    }

    downloadAndSaveLyric(currentRow);
}

void LyricSearchDialog::onCloseClicked()
{
    accept();
}

void LyricSearchDialog::onTableItemClicked(QTableWidgetItem *item)
{
    if (!item) return;

    int row = item->row();
    if (row >= 0 && row < m_candidates.size()) {
        downloadAndShowLyric(row);
    }
}

void LyricSearchDialog::onSearchReplyFinished(QNetworkReply* reply)
{
    m_searchTimer->stop();

    if (reply->error() != QNetworkReply::NoError) {
        ui->textLyric->setPlaceholderText("网络错误: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    if (!parseSearchResponse(responseData)) {
        ui->textLyric->setPlaceholderText("解析响应数据失败");
        return;
    }

    if (m_candidates.isEmpty()) {
        ui->textLyric->setPlaceholderText("未找到匹配的歌词，你可以添加上歌手信息试试。");
        return;
    }

    // 更新表格显示
    updateTable();

    ui->textLyric->setPlaceholderText("选择一首歌曲查看歌词");
    ui->btnAddToPlaylist->setEnabled(true);
}

bool LyricSearchDialog::parseSearchResponse(const QByteArray& jsonData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    int status = root.value("status").toInt();

    if (status != 200) {
        qDebug() << "API返回错误:" << root.value("info").toString();
        return false;
    }

    QJsonArray candidates = root.value("candidates").toArray();
    m_candidates.clear();

    for (const QJsonValue& value : candidates) {
        QJsonObject candidate = value.toObject();

        LyricCandidate lyric;
        lyric.id = candidate.value("id").toString();
        lyric.accesskey = candidate.value("accesskey").toString();
        lyric.singer = candidate.value("singer").toString();
        lyric.song = candidate.value("song").toString();
        lyric.duration = candidate.value("duration").toInt();
        lyric.score = candidate.value("score").toInt();

        if (candidate.contains("language")) {
            lyric.language = candidate.value("language").toString();
        } else {
            lyric.language = "未知";
        }

        m_candidates.append(lyric);
    }

    return true;
}

void LyricSearchDialog::updateTable()
{
    ui->tableResults->setRowCount(m_candidates.size());

    for (int i = 0; i < m_candidates.size(); ++i) {
        const LyricCandidate& candidate = m_candidates[i];
        // 序号
        QTableWidgetItem* numItem = new QTableWidgetItem(QString::number(i+1));
        numItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  // 水平垂直都居中
        ui->tableResults->setItem(i, 0, numItem);

        // 歌手
        QTableWidgetItem* singerItem = new QTableWidgetItem(candidate.singer);
        ui->tableResults->setItem(i, 1, singerItem);

        // 歌名
        QTableWidgetItem* songItem = new QTableWidgetItem(candidate.song);
        ui->tableResults->setItem(i, 2, songItem);

        // 时长
        QTableWidgetItem* durationItem = new QTableWidgetItem(formatDuration(candidate.duration));
        durationItem->setTextAlignment(Qt::AlignCenter);
        ui->tableResults->setItem(i, 3, durationItem);

        // 热度（评分）
        QTableWidgetItem* scoreItem = new QTableWidgetItem(QString::number(candidate.score));
        scoreItem->setTextAlignment(Qt::AlignCenter);
        ui->tableResults->setItem(i, 4, scoreItem);

        // ID（隐藏）
        QTableWidgetItem* idItem = new QTableWidgetItem(candidate.id);
        ui->tableResults->setItem(i, 5, idItem);
    }

    // 自动调整列宽
    ui->tableResults->resizeColumnsToContents();
}

void LyricSearchDialog::downloadAndShowLyric(int row)
{
    if (row < 0 || row >= m_candidates.size()) return;

    const LyricCandidate& candidate = m_candidates[row];

    // 下载歌词
    QString urlStr = QString("http://lyrics.kugou.com/download?ver=1&client=pc&id=%1&accesskey=%2&fmt=lrc&charset=utf8")
                     .arg(candidate.id)
                     .arg(candidate.accesskey);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    // 临时存储当前选中的行
    m_currentDownloadId = candidate.id;
    m_currentAccesskey = candidate.accesskey;

    ui->textLyric->setPlaceholderText("正在下载歌词...");
    m_downloadFinished = false;
    // 重新连接信号，只处理下载响应
    disconnect(m_networkManager, &QNetworkAccessManager::finished,
               this, &LyricSearchDialog::onSearchReplyFinished);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &LyricSearchDialog::onDownloadReplyFinished);

    m_networkManager->get(request);
    m_downloadTimer->start();
}
void LyricSearchDialog::saveShowLyricToFile()
{
    QString lyricContent = ui->textLyric->toPlainText();
    qDebug() << "m_targetLrcPath = ----------------------------------------------:" <<   m_targetLrcPath;
    if (m_openFilePath){
               // 弹出文件选择对话框
               QString fileName = QFileDialog::getSaveFileName(this, "保存歌词文件",
                                                              m_targetLrcPath,
                                                              "LRC歌词文件 (*.lrc)");
               if (fileName.isEmpty()) return;
               // 确保扩展名为.lrc
               if (!fileName.endsWith(".lrc", Qt::CaseInsensitive)) {
                   fileName += ".lrc";
               }
               m_targetLrcPath = fileName;
    }
    if (!m_targetLrcPath.isEmpty() && !lyricContent.isEmpty())
    {
        // 保存到文件
        if (saveLyricToFile(lyricContent, m_targetLrcPath)) {
            // 发射下载完成信号
            emit lyricDownloaded(m_targetLrcPath);

        } else {
            QMessageBox::warning(this, "错误", "无法保存歌词文件");
        }
    }
}
void LyricSearchDialog::downloadAndSaveLyric(int row)
{
    if (row < 0 || row >= m_candidates.size()) return;

    const LyricCandidate& candidate = m_candidates[row];

    // 确定保存路径
    QString lrcFilePath;

    // 如果提供了歌曲文件路径，则保存到同目录
    if (!m_songFilePath.isEmpty()) {
                // 检查文件是否已存在
        if (QFile::exists(m_targetLrcPath)) {
            int reply = SimpleMessageBox::question(this, "文件已存在",
                                              "歌词文件已存在，是否覆盖？",
                                              400,140);
            if (reply != QMessageBox::Ok) {
                return;
            }
        }
    }
    if (!m_downloadFinished){
    m_currentDownloadId = candidate.id;
    m_currentAccesskey = candidate.accesskey;

    // 下载歌词
    QString urlStr = QString("http://lyrics.kugou.com/download?ver=1&client=pc&id=%1&accesskey=%2&fmt=lrc&charset=utf8")
                     .arg(candidate.id)
                     .arg(candidate.accesskey);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

       // 显示进度提示
       QProgressDialog progress("正在下载歌词...", "取消", 0, 0, this);
       progress.setWindowModality(Qt::WindowModal);
       progress.setMinimumDuration(0);

       // 重新连接信号，只处理下载响应
       disconnect(m_networkManager, &QNetworkAccessManager::finished,
                  this, &LyricSearchDialog::onSearchReplyFinished);
       connect(m_networkManager, &QNetworkAccessManager::finished,
               this, &LyricSearchDialog::onDownloadReplyFinished);

       m_networkManager->get(request);
       m_downloadTimer->start();

       // 等待下载完成
       QEventLoop loop;
       connect(this, &LyricSearchDialog::lyricDownloaded, &loop, &QEventLoop::quit);
       connect(m_downloadTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
       loop.exec();
   }
}

void LyricSearchDialog::onDownloadReplyFinished(QNetworkReply* reply)
{
    m_downloadTimer->stop();
        m_downloadFinished = true;
    // 恢复信号连接
    disconnect(m_networkManager, &QNetworkAccessManager::finished,
               this, &LyricSearchDialog::onDownloadReplyFinished);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &LyricSearchDialog::onSearchReplyFinished);

    if (reply->error() != QNetworkReply::NoError) {
        ui->textLyric->setPlaceholderText("下载失败: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QString lyricContent;
    if (!parseDownloadResponse(responseData, lyricContent)) {
        ui->textLyric->setPlaceholderText("解析歌词内容失败");
        return;
    }

    // 如果只是预览（没有设置保存路径），只显示歌词

        ui->textLyric->setPlainText(lyricContent);
        ui->labelLyricTitle->setText(QString("歌词预览：%1 - %2")
                                    .arg(m_candidates[ui->tableResults->currentRow()].singer)
                                    .arg(m_candidates[ui->tableResults->currentRow()].song));
qDebug() << "m_targetLrcPath = " << m_targetLrcPath << m_targetLrcPath.isEmpty();

}

bool LyricSearchDialog::parseDownloadResponse(const QByteArray& jsonData, QString& lyricContent)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    int status = root.value("status").toInt();

    if (status != 200) {
        qDebug() << "API返回错误:" << root.value("info").toString();
        return false;
    }

    QString base64Content = root.value("content").toString();
    if (base64Content.isEmpty()) {
        return false;
    }

    // Base64解码
    QByteArray decoded = QByteArray::fromBase64(base64Content.toUtf8());
    lyricContent = QString::fromUtf8(decoded);

    return true;
}

bool LyricSearchDialog::saveLyricToFile(const QString& content, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << content;
    file.close();

    return true;
}

QString LyricSearchDialog::formatDuration(int ms) const
{
    int totalSeconds = ms / 1000;
    int hours = totalSeconds / 3600;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    if (hours > 0)
    return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0'))
                          .arg(seconds, 2, 10, QChar('0'));
    else  return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
}

QString LyricSearchDialog::cleanFileName(const QString& fileName) const
{
    QString cleaned = fileName;

    // 移除Windows文件名中的非法字符
    QRegularExpression invalidChars("[\\\\/:*?\"<>|]");
    cleaned.replace(invalidChars, "_");

    // 限制长度
    if (cleaned.length() > 255) {
        cleaned = cleaned.left(255);
    }

    return cleaned;
}

void LyricSearchDialog::onSearchTimeout()
{
    ui->textLyric->setPlaceholderText("搜索超时，请重试");
    m_networkManager->disconnect();
}

void LyricSearchDialog::onDownloadTimeout()
{
    ui->textLyric->setPlaceholderText("下载超时，请重试");
    m_networkManager->disconnect();
}
#include <QScrollBar>
void LyricSearchDialog::applyDarkTheme()
{
    // 深色主题样式表
    QString styleSheet = R"(
                         QDialog {
                         background-color: #2b2b2b;
                         color: #e0e0e0;
                         }

                         QLineEdit {
                         background-color: #3c3c3c;
                         color: #e0e0e0;
                         border: 1px solid #555555;
                         border-radius: 3px;
                         padding: 4px;
                         }

                         QLineEdit:focus {
                         border: 1px solid #4a90d9;
                         }

                         QPushButton {
                         background-color: #3c3c3c;
                         color: #e0e0e0;
                         border: 1px solid #555555;
                         border-radius: 3px;
                         padding: 6px 12px;
                         }

                         QPushButton:hover {
                         background-color: #4c4c4c;
                         border: 1px solid #666666;
                         }

                         QPushButton:pressed {
                         background-color: #2c2c2c;
                         }

                         QPushButton:disabled {
                         background-color: #2c2c2c;
                         color: #888888;
                         border: 1px solid #444444;
                         }

                         QTableWidget {
                         background-color: #2b2b2b;
                         color: #e0e0e0;
                         border: 1px solid #555555;
                         gridline-color: #444444;
                         alternate-background-color: #333333;
                         margin-left: 2px;
                         margin-right: 2px;
                         }

                         QTableWidget QHeaderView::section {
                         background-color: #3c3c3c;
                         color: #e0e0e0;
                         border: 1px solid #555555;
                         padding: 4px;
                         }

                         QTableWidget::item {
                         padding: 4px;
                         }

                         QTableWidget::item:selected {
                         background-color: #4a90d9;
                         color: white;
                         }

                         QTextEdit {
                         background-color: #2b2b2b;
                         color: #e0e0e0;
                         border: 1px solid #555555;
                         font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;
                         font-size: 11pt;
                         }
                         QFrame#frameButtons {
                         border: none;
                         margin-bottom:4px;
                         margin-right:1px;
                         background-color: transparent; /* 如果需要透明背景 */
                         }
                         QFrame#frameSearch {
                         border: none;
                         margin-top:2px;
                         margin-right:2px;
                         background-color: transparent; /* 如果需要透明背景 */
                         }
                         QFrame#frameLyric {
                         border: none;
                         background-color: transparent; /* 如果需要透明背景 */
                         }

                         QLabel {
                         color: #e0e0e0;
                         }
                         QLabel#labelSeparator {
                         color: #e0e0e0;
                         border: none;
                         padding: 0px 4px;
                         font-weight: bold;
                         }
                         QProgressDialog {
                         background-color: #2b2b2b;
                         color: #e0e0e0;
                         }
                         /* 底部按钮区域 */
                         QFrame {
                         border: 1px solid #555555;
                         background-color: #3b3b3b;
                         }
                         )";

    setStyleSheet(styleSheet);

    // 设置表格的交替行颜色
    QPalette palette = ui->tableResults->palette();
    palette.setColor(QPalette::Base, QColor(43, 43, 43));
    palette.setColor(QPalette::AlternateBase, QColor(51, 51, 51));
    palette.setColor(QPalette::Text, QColor(224, 224, 224));
    palette.setColor(QPalette::Highlight, QColor(74, 144, 217));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    ui->tableResults->setPalette(palette);

    // 设置表格表头的样式
        ui->tableResults->horizontalHeader()->setStyleSheet(R"(
            QHeaderView::section {
                background-color: #3c3c3c;
                color: #e0e0e0;
                border: 1px solid #555555;
                padding: 4px;
                font-weight: bold;
            }
        )");

        // 设置垂直表头（行号）的样式
        ui->tableResults->verticalHeader()->setStyleSheet(R"(
            QHeaderView::section {
                background-color: #3c3c3c;
                color: #e0e0e0;
                border: 1px solid #555555;
                padding: 4px;
            }
        )");

    // 设置表格的网格线
    ui->tableResults->setShowGrid(true);
    ui->tableResults->setGridStyle(Qt::SolidLine);
    // 隐藏垂直表头（行号）如果不需要显示
    ui->tableResults->verticalHeader()->setVisible(false);
    // 设置文本编辑器的字体
    QFont font = ui->textLyric->font();
    font.setFamily("Microsoft YaHei");
    font.setPointSize(11);
    ui->textLyric->setFont(font);

    // 设置占位符文本颜色
    ui->textLyric->setStyleSheet(ui->textLyric->styleSheet() +
        "QTextEdit { color: #e0e0e0; }");

    // 设置按钮的悬停效果
    ui->btnSearch->setCursor(Qt::PointingHandCursor);
    ui->btnClear->setCursor(Qt::PointingHandCursor);
    ui->btnAddToPlaylist->setCursor(Qt::PointingHandCursor);
    ui->btnClose->setCursor(Qt::PointingHandCursor);
    // 查找对话框中的所有滚动条并设置样式
        QList<QScrollBar*> scrollbars = this->findChildren<QScrollBar*>();
        for (QScrollBar* scrollbar : scrollbars) {
            QString styleSheet = R"(
                                 QScrollBar:vertical {
                                             width: 14px;
                                             background: #202020;
                                             border: none;
                                         }
                                         QScrollBar::handle:vertical {
                                             min-height: 20px;
                                             background: #3a3a3a;
                                             margin: 0px 4px;
                                             border-radius: 4px;
                                         }
                                         QScrollBar::handle:vertical:hover {
                                             background: rgb(80, 80, 80);
                                             margin: 0px 3px;  /* 悬停时减小边距，使滑块变宽 */
                                         }
                                         QScrollBar::sub-line:vertical {
                                             height: 0px;
                                             background: transparent;
                                             subcontrol-position: top;
                                         }
                                         QScrollBar::add-line:vertical {
                                             height: 0px;
                                             background: transparent;
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
                                         )";
            scrollbar->setStyleSheet(
                styleSheet
            );
        }
}

// 检查并加载已存在的歌词文件
void LyricSearchDialog::checkAndLoadExistingLyric()
{
    m_hasExistingLyric = false;
    m_existingLrcPath = "";
qDebug() << "准备查找：－－－－－－－－－－－－－－－－－－－－－" << m_songFilePath;
    if (m_songFilePath.isEmpty()) {
        updateLyricCompareDisplay(false);
        return;
    }

    QFileInfo songFileInfo(m_songFilePath);
    QString baseName = songFileInfo.completeBaseName();

    // 检查可能的歌词文件名
    QStringList possibleExtensions = {".lrc", ".txt", ".krc", ".trc"};
qDebug() << "准备查找：－－－－－－－－－－－－－－－－－－－－－" << baseName;
    foreach (const QString& ext, possibleExtensions) {
        QString possibleLrcPath = songFileInfo.absolutePath() + "/" + baseName + ext;
        QFile lrcFile(possibleLrcPath);
        qDebug() << "正在查找：－－－－－－－－－－－－－－－－－－－－－" << lrcFile;
        if (lrcFile.exists()) {
            m_existingLrcPath = possibleLrcPath;
            break;
        }
    }

    // 如果没有找到精确匹配的文件名，尝试搜索目录下的.lrc文件
    if (m_existingLrcPath.isEmpty()) {
        QDir songDir(songFileInfo.absolutePath());
        QStringList lrcFiles = songDir.entryList(QStringList() << "*.lrc", QDir::Files);

        foreach (const QString& lrcFile, lrcFiles) {
            QString lrcPath = songDir.absolutePath() + "/" + lrcFile;
            if (lrcPath.contains(baseName, Qt::CaseInsensitive)) {
                m_existingLrcPath = lrcPath;
                break;
            }
        }
    }

    // 加载已存在的歌词内容
    if (!m_existingLrcPath.isEmpty()) {
        QFile lrcFile(m_existingLrcPath);
        if (lrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&lrcFile);
            in.setCodec("UTF-8");
            QString existingLyric = in.readAll();
            lrcFile.close();

            // 显示右侧对比面板
            updateLyricCompareDisplay(true);

            // 更新标签显示文件名
            QFileInfo lrcFileInfo(m_existingLrcPath);
            ui->labelExistingLyricTitle->setText(QString("已存在的歌词 (%1)：").arg(lrcFileInfo.fileName()));

            // 设置歌词内容
            ui->textExistingLyric->setPlainText(existingLyric);
            ui->textExistingLyric->setPlaceholderText("");

            m_hasExistingLyric = true;
        } else {
            updateLyricCompareDisplay(false);
        }
    } else {
        // 没有找到已存在的歌词文件
        updateLyricCompareDisplay(false);
    }
}

// 更新歌词对比显示
void LyricSearchDialog::updateLyricCompareDisplay(bool showRightPanel)
{
    if (showRightPanel) {
        // 显示右侧对比面板
        ui->labelExistingLyricTitle->setVisible(true);
        ui->textExistingLyric->setVisible(true);

        // 设置左右各占50%的宽度
        QList<int> sizes;
        sizes << width() / 2 << width() / 2;
        ui->horizontalLayout_lyricContent->setStretch(0, 1);
        ui->horizontalLayout_lyricContent->setStretch(1, 1);

        // 更新左侧标签文本
        ui->labelLyricTitle->setText("搜索到的歌词：");
    } else {
        // 隐藏右侧对比面板
        ui->labelExistingLyricTitle->setVisible(false);
        ui->textExistingLyric->setVisible(false);

        // 设置左侧占100%宽度
        ui->horizontalLayout_lyricContent->setStretch(0, 1);
        ui->horizontalLayout_lyricContent->setStretch(1, 0);

        // 更新左侧标签文本
        ui->labelLyricTitle->setText("歌词预览：");

        // 清空右侧内容
        ui->textExistingLyric->clear();
        ui->textExistingLyric->setPlaceholderText("未找到已存在的歌词文件");
    }
}
