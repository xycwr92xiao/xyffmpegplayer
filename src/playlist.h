/*
 * @file 	playlist.h
 * @date 	2018/01/07 11:12
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	播放列表控件
 * @note
 */
#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

#include "globalvars.h"

namespace Ui {
class Playlist;
}
class PlaylistItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PlaylistItemDelegate(QObject *parent = nullptr);
    // 添加设置行高的方法
        void setRowHeight(int height) { m_rowHeight = height; }
        int rowHeight() const { return m_rowHeight; }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
        int m_rowHeight = 24; // 默认行高

};

struct PlaylistInfo {
    int id;
    QString listName;
    QString jsonPath;
    int orderIndex;
};

class Playlist : public QWidget
{
    Q_OBJECT

public:
    explicit Playlist(QWidget *parent = 0);
    ~Playlist();

    bool Init();
    /**
     * @brief	获取播放列表状态
     * @return	true 显示 false 隐藏
     * @note
     */
    bool GetPlaylistStatus();
    int GetCurrentIndex();
public:
    /**
     * @brief	添加文件
     *
     * @param	strFileName 文件完整路径
     * @note
     */
    void OnAddFile(QString strFileName);
    void OnAddFileAndPlay(QString strFileName);
    void appToIndexAndPlay(QStringList filesList,int index);
    //void appendItemTotop(QStringList filesList);
    void appendItemToPosition(QStringList filesList, int index = 0);
    int GetPlayIndex(){return m_nCurrentPlayListIndex;};
    void OnBackwardPlay();
    void OnForwardPlay();
    void ShowCurrentFileInfo();  // 显示当前选中文件的媒体信息
    void ShowFileInfoByIndex(int index);  // 显示指定索引文件的媒体信息
    void ShowFilePathByIndex(int index);
    int GetNextIndex(int currentIndex);  // 根据播放模式获取下一个索引
    int GetPrevIndex(int currentIndex);  // 根据播放模式获取上一个索引
    void SetSelectedIndex(int index);
    // 新增：重命名相关
        void RenameCurrentFile();  // 重命名当前选中文件
        bool RenameFile(int index, const QString &newName);  // 重命名指定文件
    /* 在这里定义dock的初始大小 */
    QSize sizeHint() const
    {
        return QSize(150, 900);
    }
    void setRowHeight(int height);
public slots:
    void PlayByIndex(int nIndex);  // 播放指定索引的文件

protected:
    /**
    * @brief	放下事件
    *
    * @param	event 事件指针
    * @note
    */
    void dropEvent(QDropEvent *event);
    /**
    * @brief	拖动事件
    *
    * @param	event 事件指针
    * @note
    */
    void dragEnterEvent(QDragEnterEvent *event);
signals:
    void SigUpdateUi();	//< 界面排布更新
    void SigPlay(QString strFile); //< 播放文件
    void SigPlayOrPause();         //< 播放/暂停信号（新增）
    void SigPlayModeRestored(int mode);  // 播放模式已恢复信号
    void SigStop();
    // 新增：重命名相关信号
        void SigRenameRequested(int index);  // 请求重命名
        void SigGetPlayPosition();           // 获取播放位置
        void SigSetPlayPosition(qint64 position); // 设置播放位置
        void SigPlaylistChanged();
        void SigSaveSubtitleFile(const QString& lrcPath);
private:
    bool InitUi();
    bool ConnectSignalSlots();
    void updateButtonStates();
    void sortByPath(bool ascending);
    void updateSelectionAfterSort();
    //--------------------------------------------列表时长和图标
    struct PlaylistItemData {
            int orderIndex;  // 保存顺序索引
            QString filePath;
            QString fileName;
            qint64 duration; // 单位：毫秒
            bool durationLoaded;
            PlaylistItemData() : duration(0), durationLoaded(false) {}
        };

        QMap<QString, PlaylistItemData> m_itemData; // 存储项的数据

        // 新添加的私有函数--------------------------------------start
        void updateItemDisplay(int index, bool forceUpdate = false);
        void loadDurationInBackground(int index);
        void saveAllData(); // 保存播放列表数据（包括时长）
        void loadPlaylistData(); // 加载播放列表数据
        QString formatDurationForDisplay(qint64 durationMs);
        // 新增：重命名相关
        void handleFileRenamed(int index, const QString &oldPath, const QString &newPath);
        void updateItemDataPath(const QString &oldPath, const QString &newPath);
        //播放列表管理相关
        QList<PlaylistInfo> m_playlistInfos; // 存储所有播放列表信息
            QString m_currentPlaylistName ="默认列表"; // 当前播放列表名称
            QString m_currentPlaylistJson; // 当前播放列表JSON文件路径
            bool m_isDefaultPlaylist = true; // 是否是默认列表
            // 播放列表菜单
            QMenu *m_playlistMenu;

            void initListMenu(); // 初始化播放列表菜单
            void loadPlaylistIndex(); // 加载播放列表索引文件
            void savePlaylistIndex(); // 保存播放列表索引文件
            void switchToPlaylist(const QString &listName, const QString &jsonPath); // 切换到指定播放列表
            void saveAsNewPlaylist(const QString &newListName); // 另存为新播放列表
            void deleteCurrentPlaylist(); // 删除当前播放列表
            void updateCurrentPlaylistTitle(); // 更新当前播放列表标题
            void loadPlaylistFromFile(const QString &filePath);
private slots:
            // 新增槽函数
            void on_btnListTitle_clicked(); // 点击列表标题
            void on_btnSaveToNewlist_clicked(); // 另存为新列表
            void on_btnDeleteCurrentList_clicked(); // 删除当前列表
            void onPlaylistMenuTriggered(QAction *action); // 播放列表菜单项被触发
        void onDurationLoaded(int index, qint64 duration);
        void onCurrentItemChanged(int currentRow);
        //------------------------------------------------------end
        void on_List_itemDoubleClicked(QListWidgetItem *item);
        //void on_btnAdd_clicked();使用信号，连接到 MediaList 的 AddFile 槽函数
        void on_btnDelete_clicked();
        void on_btnMoveUp_clicked();
        void on_btnMoveDown_clicked();
        void on_btnMoveTop_clicked();
        void on_btnMoveBottom_clicked();
        void on_btnSort_clicked();
        void on_currentRowChanged(int currentRow);
        // 新增：重命名相关槽函数
                //void onRenameFileRequested();  // 重命名按钮点击（如果需要添加按钮）
                void onFileRenamed(int index, QString oldPath, QString newPath);  // 处理文件重命名信号
private:
    Ui::Playlist *ui;

    int m_nCurrentPlayListIndex=-1;
    PlaylistItemDelegate* m_pItemDelegate = nullptr; // 添加委托指针
    // 新增：重命名相关状态
        qint64 m_lastPlayPosition;  // 记录播放位置
        int m_firstIndex = -1;
};


#endif // PLAYLIST_H
