#pragma once

#include <QListWidget>
#include <QMenu>
#include <QAction>

#include "lyricsearch.h"

class MediaList : public QListWidget
{
    Q_OBJECT

public:
    MediaList(QWidget *parent = 0);
    ~MediaList();
    bool Init();
    void AddFile(); //添加文件
    void AddFolder(); //添加文件夹
    void ShowFileInfo(); // 显示文件信息
    void OpenFileLocation(); // 打开文件所在位置
    // 重命名相关方法
        bool renameFileInList(int index, const QString &newName);
        void startRenameCurrentItem();
protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
private:
    void RemoveFileFromList(); // 从播放列表移除
    void DeleteFileFromDisk(); // 从磁盘删除文件
    void ClearList(); // 清空列表
    void PlayOrPauseCurrent(); // 播放/暂停当前项
    void RenameFile(); // 重命名文件
    void showRenameDialog(int index);
    bool validateNewFileName(const QString &oldPath, const QString &newName, QString &errorMsg);
    bool performRename(int index, const QString &newName);
    void updateListAfterRename(int index, const QString &oldPath, const QString &newPath);
signals:
    void SigAddFile(QString strFileName);   //添加文件信号
    void SigStop();
    void SigRemoveFile(int index);          //移除文件信号
    void SigPlayOrPause(int index);         //播放/暂停信号
    void SigPlayFile(int index);            //播放指定文件信号
    void SigRenameFile(int index, QString oldPath, QString newPath); // 重命名文件信号
    void SigRequestPlayPosition();          // 请求播放位置（用于重命名时记录）
    void SigResumePlay(int index, qint64 position); // 恢复播放
    void SigSaveSubtitleFile(const QString& lrcPath);
private:
    QMenu m_stMenu;
    QAction m_stActPlayPause;    // 播放/暂停
    QAction m_stActAdd;          // 添加文件
    QAction m_stActAddFolder;    // 添加文件夹
    QAction m_stActRemove;       // 从播放列表移除
    QAction m_stActDeleteFile;   // 从磁盘删除文件
    QAction m_stActClearList;    // 清空列表
    QAction m_stActOpenLocation; // 打开文件所在位置
    QAction m_stActFileInfo;     // 文件信息
    QAction m_stActGetLyric;     // 获取歌词
    QAction m_stActSeparator1;   // 分隔线1
    QAction m_stActSeparator2;   // 分隔线2
    QAction m_stActSeparator3;   // 分隔线3
    QAction m_stActSeparator4;   // 分隔线4（新增）
    QAction m_stActRename;       // 重命名（新增）
    LyricSearchDialog* m_lyricDialog;  // 歌词搜索对话框
    qint64 parseDurationString(const QString& durationStr);
    // 重命名相关状态
        bool m_isRenaming;
        int m_renameIndex;
        QString m_renameOldPath;

private slots:
    void GetLyric();  // 获取歌词
};
