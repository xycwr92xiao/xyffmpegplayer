/*
 * @file 	mainwid.h
 * @date 	2018/01/07 11:12
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	主界面
 * @note
 */
#ifndef MainWid_H
#define MainWid_H

#include <QWidget>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QMenu>
#include <QAction>
#include <QPropertyAnimation>
#include <QTimer>
#include <QMainWindow>
#include <QFrame>
#include <QSplitter>
#include <QFontDatabase>

#include "playlist.h"
#include "title.h"
#include "settingwid.h"
#include "equalizerdialog.h"

namespace Ui {
class MainWid;
}

class MainWid : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWid(QMainWindow *parent = 0);
    ~MainWid();

    //初始化
    bool Init();
    bool isSubtitleVisible() const { return m_bShowSubtitle; }
public slots:
    void OnAutoPlayNext();
    void openFileFromCommandLine(const QString &filePath);
protected:
    //绘制
    void paintEvent(QPaintEvent *event) override;

//     //窗口大小变化事件
//     void resizeEvent(QResizeEvent *event);
//     //窗口移动事件
//     void moveEvent(QMoveEvent *event);

    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    //void dragEnterEvent(QDragEnterEvent *event);
    //void dragMoveEvent(QDragMoveEvent *event);
    //void dropEvent(QDropEvent *event);

    //按键事件
    void keyReleaseEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override ;
    void mouseMoveEvent(QMouseEvent *event) override;

    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
private:
    void processCommandLineFile();
    QStringList m_commandLineFiles; // 存储命令行文件
    bool m_bCommandLineFileProcessed =false;  // 新增标志位

    //连接信号槽
    bool ConnectSignalSlots();

    //关闭、最小化、最大化按钮响应
    void OnCloseBtnClicked();
    void OnMinBtnClicked();
    void OnMaxBtnClicked();
    void OnAlwaysOnTopBtnClicked();
    //显示、隐藏播放列表
    void OnShowOrHidePlaylist();

    void CreateDefaultMenu();

    /**
    * @brief	全屏播放
    */
    void OnFullScreenPlay();
    void setExtend();
    void onShowSubtitle();
    void OnCtrlBarAnimationTimeOut();
    //void OnFullscreenMouseDetectTimeOut();

    void OnCtrlBarHideTimeOut();
    void OnShowMenu();
    void OnShowAbout();
    void OpenFile();
    void OnShowSettingWid();


    //添加菜单
    void InitMenu();

    // 边框调整大小相关函数
    void UpdateCursorForBorder(const QPoint &pos);
    void StartResize(const QPoint &pos);
    void DoResize(const QPoint &pos);
    void StopResize();
    Qt::CursorShape GetCursorForBorderPos(const QPoint &pos) const;
    Qt::Edges GetEdgesForBorderPos(const QPoint &pos) const;

    QFrame *m_borderFrame;
    bool m_bPlaylistVisible; // 记录播放列表是否可见
    QSize m_originalWindowSize; // 原始窗口大小，用于恢复
    // 均衡器相关
    EqualizerDialog* m_equalizerDialog;
    QList<int> m_eqGains;  // 10段均衡器增益
    int m_balance;         // 左右声道平衡 (-100到100)
    bool m_eqEnabled;      // 均衡器是否启用
    void applyEqualizerToAudio();
    void saveEqualizerSettings();
    void loadEqualizerSettings();
    // 添加视频尺寸相关变量
    int m_currentVideoWidth = 0;
    int m_currentVideoHeight = 0;
    void adjustWindowToVideoSize();  // 调整窗口到视频实际大小
    bool isVideoPlaying() const;     // 检查是否正在播放视频
    void onVideoSizeChanged(int width, int height);
private slots:
    void onEqualizerSettings();  // 打开均衡器设置
    void onEqualizerChanged(const QList<int>& gains, int balance, bool enabled);
    void onShowMediaInfo();      // 显示媒体信息（新增）
    void onShowMediaPath();
    void onActualSize();  // 实际大小菜单项响应函数
    void onUpdateActualSizeMenuState();  // 更新菜单项状态
    void onSubtitleSettingsChanged(const QString& fontFamily, int fontSize);
signals:
    void SigStepFrameForward();   // 步进一帧信号
        void SigStepFrameBackward();  // 后退一帧信号
        void SigToggleFrameStepMode(bool enable); // 切换逐帧模式信号
    //最大化信号
    void SigShowMax(bool bIfMax);
    void SigSeekForward();
    void SigSeekBack();
    void SigSeekForward10s();
    void SigSeekBack10s();
    void SigAddVolume();
    void SigSubVolume();
    void SigPlayOrPause();
    void SigOpenFile(QString strFilename);
    void SigAppendItemTotop(QStringList filesList,int index);
    void SigFadeVolmeClose(int optionType);
private:
    bool m_bFrameStepMode = false;       // 逐帧播放模式标志
    Ui::MainWid *ui;
    bool m_bPlaying; ///< 正在播放

    const int m_nShadowWidth; ///< 阴影宽度

    bool m_bFullScreenPlay; ///< 全屏播放标志

    QPropertyAnimation *m_stCtrlbarAnimationShow; //全屏时控制面板浮动显示
    QPropertyAnimation *m_stCtrlbarAnimationHide; //全屏时控制面板浮动显示
    QRect m_stCtrlBarAnimationShow;//控制面板显示区域
    QRect m_stCtrlBarAnimationHide;//控制面板隐藏区域
    QPropertyAnimation *m_stTitlebarAnimationShow; //全屏时标题面板浮动显示
    QPropertyAnimation *m_stTitlebarAnimationHide; //全屏时标题面板浮动显示
    QRect m_stTitleBarAnimationShow;//控标题显示区域
    QRect m_stTitleBarAnimationHide;//控标题隐藏区域
    QPropertyAnimation *m_stPlaylistAnimationShow; //全屏时播放列表浮动显示
    QPropertyAnimation *m_stPlaylistAnimationHide; //全屏时播放列表浮动显示
    QRect m_stPlayListAnimationShow;//控播放列表显示区域
    QRect m_stPlayListAnimationHide;//控播放列表隐藏区域

    //QTimer m_stCtrlBarAnimationTimer;
    QTimer m_stFullscreenMouseDetectTimer;//全屏时鼠标位置监测时钟
    bool m_bFullscreenCtrlBarShow;
    QTimer stCtrlBarHideTimer;

    Playlist m_stPlaylist;
    Title m_stTitle;

    bool m_bMoveDrag;//移动窗口标志
    QPoint m_DragPosition;

    About m_stAboutWidget;
    SettingWid m_stSettingWid;

    QMenu m_stMenu;
    QAction m_stActFullscreen;
    QAction m_stActPlaylist;
    QAction m_stActShowSubtitle;
    bool m_bShowSubtitle = true;
    QAction m_stActExtend;
    QMap<QString, void(MainWid::*)()> map_act_;
    // 边框调整大小相关成员变量
        bool m_bResizeDrag;            ///< 是否正在调整大小
        QPoint m_ResizeStartPos;       ///< 调整大小起始位置
        QRect m_ResizeStartGeometry;   ///< 调整大小前窗口几何位置
        Qt::Edges m_ResizeEdges;       ///< 调整的边
        const int m_nBorderWidth = 5;  ///< 边框调整区域宽度（像素）
        QSplitter *m_splitter;  // 添加分割器成员
        QAction m_stActAlwaysOnTop;  // 新增：置顶菜单动作
        bool m_bAlwaysOnTop;         // 新增：置顶状态标志
        void ShowAllFontAwesomeIcons();//显示图标字库所有图标－－－暂时
        void SetMenuStyle(QMenu* menu);
        QIcon createMenuIcon(const QString& svgPath, const QColor& normalColor, const QColor& hoverColor);
        QPixmap createColoredSvg(const QString& svgPath, const QColor& color, QSize size);
        QColor m_menuNormalColor;      // 正常状态颜色
        QColor m_menuHoverColor;       // 悬停/选中状态颜色
        // 不同状态下的播放列表显示/隐藏
            void showPlaylistNormal(bool show);
            void showPlaylistMaximized(bool show);
            void showFullscreenPlaylist(bool show);
            void updatePlaylistButtonIcon(bool visible);
            // 播放列表相关状态
            // 播放列表相关
                int playlistWidth = 300;
                int mainwin_width = 640;
                // 全屏状态相关
                int m_preFullScreenVideoWidth=0;
                bool m_bIsFullScreen = false;
                QTimer *m_playlistHideTimer = nullptr;
                bool m_bClickVideoToHidePlaylist = false;
                QPoint m_lastMousePos = QPoint(-1, -1);
                bool m_bPlaylistBlockingControls = false;

                // 保存全屏前状态
                QSize m_preFullScreenSize;
                QPoint m_preFullScreenPos;
                QSize m_preFullScreenVideoSize;
                void updatePlaylistButtonState();

                qint64 m_lastMouseActivityTime; ///< 最后鼠标活动时间戳（毫秒）
                bool m_bMouseCursorHidden;     ///< 鼠标指针是否已隐藏
                // 鼠标活动检测函数
                void UpdateMouseActivity();
                void changeCtrlTileShow();
                void CheckMouseActivity();
                // 隐藏/显示鼠标指针
                void HideMouseCursor();
                void ShowMouseCursor();
                void ShowFullscreenControls();
                void HideFullscreenControls();
                // 定时器槽函数
                    void OnMouseActivityCheckTimeOut();
                    bool isShowPlaylistOnWin =false;//窗口横式下是否显示了列表
                    QByteArray m_preFullScreenSplitterState;
                    int m_windowPlaylistFontSize = 9; // 窗口模式下播放列表字体大小
};

#endif // MainWid_H
