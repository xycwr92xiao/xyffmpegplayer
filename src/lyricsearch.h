#ifndef LYRICSEARCHDIALOG_H
#define LYRICSEARCHDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QTableWidgetItem>

// 前向声明UI类
namespace Ui {
class LyricSearch;
}

// 歌词候选结构
struct LyricCandidate {
    QString id;
    QString accesskey;
    QString singer;
    QString song;
    int duration;
    int score;
    QString language;
};

class LyricSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LyricSearchDialog(QWidget *parent = nullptr);
    ~LyricSearchDialog();

    // 设置初始搜索参数并开始搜索
    void setSearchParams(const QString& singer, const QString& song,
                         int durationMs, const QString& songFilePath = "");

    // 开始搜索（根据UI中的搜索框内容）
    void startSearch();

public slots:
    void onSearchClicked();
    void onClearClicked();
    void onAddToPlaylistClicked();
    void onCloseClicked();
    void onTableItemClicked(QTableWidgetItem *item);

signals:
    // 歌词下载完成信号
    void lyricDownloaded(const QString& lrcPath);

private slots:
    // 网络响应槽函数
    void onSearchReplyFinished(QNetworkReply* reply);
    void onDownloadReplyFinished(QNetworkReply* reply);
    // 处理网络超时
    void onSearchTimeout();
    void onDownloadTimeout();

private:
    Ui::LyricSearch *ui;

    QNetworkAccessManager* m_networkManager;
    QTimer* m_searchTimer;
    QTimer* m_downloadTimer;
    bool m_downloadFinished = false;
    // 当前歌词列表
    QList<LyricCandidate> m_candidates;

    // 当前下载的信息
    QString m_currentDownloadId;
    QString m_currentAccesskey;
    QString m_targetLrcPath;
    QString m_songFilePath; // 原始歌曲文件路径
    QString m_keyWord;
    bool m_openFilePath = false;
    int m_durationMs = 0;
    // 已存在歌词文件路径
        QString m_existingLrcPath;
        bool m_hasExistingLyric = false;
    // 解析JSON响应
    bool parseSearchResponse(const QByteArray& jsonData);
    bool parseDownloadResponse(const QByteArray& jsonData, QString& lyricContent);

    // 保存歌词到文件
    bool saveLyricToFile(const QString& content, const QString& filePath);
    void saveShowLyricToFile();
    // 检查并加载已存在的歌词文件
        void checkAndLoadExistingLyric();
    // 格式化时长显示
    QString formatDuration(int ms) const;

    // 清理文件名中的非法字符
    QString cleanFileName(const QString& fileName) const;

    // 更新表格显示
    void updateTable();

    // 下载并显示选中的歌词
    void downloadAndShowLyric(int row);

    // 下载歌词并保存
    void downloadAndSaveLyric(int row);
    void applyDarkTheme();
    // 更新歌词对比显示
    void updateLyricCompareDisplay(bool showRightPanel);
};

#endif // LYRICSEARCHDIALOG_H
