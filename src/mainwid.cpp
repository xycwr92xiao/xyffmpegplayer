/*
 * @file 	mainwid.cpp
 * @date 	2018/03/10 22:26
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	主窗口
 * @note
 */
#include <QFile>
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <QAbstractItemView>
#include <QMimeData>
#include <QSizeGrip>
#include <QWindow>
#include <QDesktopWidget>
#include <QScreen>
#include <QRect>
#include <QFileDialog>
#include <QSvgRenderer>
#include <QGraphicsOpacityEffect>
#include "mainwid.h"
#include "ui_mainwid.h"
#include "globalhelper.h"
#include "videoctl.h"

MainWid::MainWid(QMainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWid),
    m_nShadowWidth(0),
    m_stMenu(this),
    m_stPlaylist(this),
    m_stTitle(this),
    m_bMoveDrag(false),
    m_stActFullscreen(this),
    m_bResizeDrag(false),
    m_bPlaylistVisible(false),
    m_ResizeEdges(Qt::Edges()),
    m_equalizerDialog(nullptr),
        m_eqEnabled(false),
        m_balance(0),
        m_bAlwaysOnTop(false),
            m_bIsFullScreen(false),
            m_playlistHideTimer(nullptr),
            m_bClickVideoToHidePlaylist(true),
            m_bPlaylistBlockingControls(false),
            m_bMouseCursorHidden(false),  // 新增：初始化鼠标指针状态
            m_lastMouseActivityTime(0),    // 新增：初始化最后活动时间
            m_bFullscreenCtrlBarShow(false),   // 新增：初始化控制栏显示状态
            m_bCommandLineFileProcessed(false)  // 新增标志位

{
    ui->setupUi(this);
    // 添加窗口位置恢复代码
        QSettings settings("XyPlayer", "PlayerSettings");
        settings.beginGroup("Window");
        // 读取保存的窗口位置和大小
        QRect savedGeometry = settings.value("Geometry", QRect(100, 100, 600, 500)).toRect();
        m_bPlaylistVisible = settings.value("PlaylistVisible", false).toBool();
        playlistWidth = settings.value("playlistWidth", 300).toInt();
        settings.endGroup();

        // 检查保存的位置是否在屏幕内
        QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
        if (screenGeometry.contains(savedGeometry.topLeft())) {
            this->setGeometry(savedGeometry);
        } else {
            // 如果不在屏幕内，则居中显示
            this->resize(600, 500);
            this->move(screenGeometry.center() - rect().center());
        }
    //setMinimumSize(350, 300); // 设置合适的最小尺寸
    this->setMinimumSize(200, 400);
    // 初始化均衡器增益为0
    m_eqGains = QList<int>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ui->statusbar->hide();
    // 1. 创建边框框架
        m_borderFrame = new QFrame(this);
        m_borderFrame->setObjectName("borderFrame");

        // 2. 设置边框属性
        m_borderFrame->setFrameShape(QFrame::Box);
        m_borderFrame->setFrameShadow(QFrame::Plain);
        m_borderFrame->setLineWidth(1);
        m_borderFrame->setStyleSheet("QFrame { border: 1px solid #333333; }");

        // 3. 设置为主窗口的centralWidget
        setCentralWidget(m_borderFrame);

        // 4. 创建主布局（垂直布局）
        QVBoxLayout* mainLayout = new QVBoxLayout(m_borderFrame);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 5. 将TitleWid添加到主布局（占据整个宽度）
        mainLayout->addWidget(ui->TitleWid);

        // 6. 将m_stTitle添加到TitleWid的布局中
        ui->TitleWid->layout()->addWidget(&m_stTitle);

        // 7. 创建水平分割器（替换原来的水平布局）
            m_splitter = new QSplitter(Qt::Horizontal);
            m_splitter->setObjectName("mainSplitter");
            m_splitter->setHandleWidth(1); // 分割条宽度设为1像素
            // 设置分割器样式（可拖动的竖线）
            m_splitter->setStyleSheet(
                "QSplitter::handle {"
                "   background-color: #2a2a2a;"
                "   width: 1px;"
                "}"
                "QSplitter::handle:pressed {"
                    "    background-color: #4f4f4f;"
                    "}"
            );

            // 8. 将ShowCtrlBarPlaylistBgWidget添加到分割器左侧
            m_splitter->addWidget(ui->ShowCtrlBarPlaylistBgWidget);

            // 9. 将PlaylistWid添加到分割器右侧
            m_splitter->addWidget(ui->PlaylistWid);

            // 10. 设置初始比例（7:3，即视频占70%，播放列表占30%）
            QList<int> sizes;
            int totalWidth = this->width(); // 初始窗口宽度
            if (playlistWidth <= 0 || playlistWidth >= totalWidth) {
                   // 如果保存的宽度无效，使用默认比例
                   playlistWidth = totalWidth * 0.3;
               }
            // 根据保存的播放列表可见性设置分割器大小
                if (m_bPlaylistVisible) {
                    // 显示播放列表，设置分割器大小
                    m_splitter->setSizes({totalWidth - playlistWidth, playlistWidth});
                    ui->PlaylistWid->setVisible(true);
                    m_splitter->widget(1)->show();

                } else {
                    // 隐藏播放列表，左侧占据全部空间
                    m_splitter->setSizes({totalWidth, 0});
                    ui->PlaylistWid->setVisible(false);
                    m_splitter->widget(1)->hide();

                }

            // 11. 将分割器添加到主布局
            mainLayout->addWidget(m_splitter, 1); // 占据剩余空间

            // 12. 将m_stPlaylist添加到PlaylistWid的布局中
            ui->PlaylistWid->layout()->addWidget(&m_stPlaylist);

            // 13. 设置播放列表初始为隐藏状态
            //ui->PlaylistWid->setVisible(m_bPlaylistVisible);
            // 同时需要隐藏分割器中的播放列表部件
            //if(!m_bPlaylistVisible) m_splitter->widget(1)->hide();

            // 14. 其他初始化...
            m_borderFrame->setAttribute(Qt::WA_Hover, true);
            m_borderFrame->installEventFilter(this);
            m_borderFrame->setMouseTracking(true);

            // 15. 设置窗口标志
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
            this->setWindowIcon(QIcon("://res/xyplayer.ico"));

            // 加载样式
            QString qss = GlobalHelper::GetQssStr("://res/qss/mainwid.css");
            setStyleSheet(qss);

            // 16. 追踪鼠标
            this->setMouseTracking(true);

            // 17. 初始化其他成员变量
            m_bPlaying = false;
            m_bFullScreenPlay = false;
            //m_stCtrlBarAnimationTimer.setInterval(2000);
            //m_stFullscreenMouseDetectTimer.setInterval(FULLSCREEN_MOUSE_DETECT_TIME);
            m_stFullscreenMouseDetectTimer.setInterval(1000); // 改为1秒检测一次

            // 18. 安装事件过滤器
            ui->CtrlBarWid->installEventFilter(this);
            ui->ShowWid->installEventFilter(this);
            ui->PlaylistWid->installEventFilter(this);
            ui->TitleWid->installEventFilter(this);

            qDebug() << "MainWid构造函数完成";
            m_splitter->setChildrenCollapsible(true);
            ui->ShowWid->setMinimumWidth(0);
            qDebug() << "窗口初始大小：" << this->size();
            qDebug() << "窗口最小大小：" << minimumSize();
            qDebug() << "窗口最大大小：" << maximumSize();
            // 创建自动隐藏定时器
            // 创建播放列表自动隐藏定时器
                m_playlistHideTimer = new QTimer(this);
                m_playlistHideTimer->setSingleShot(true);
                connect(m_playlistHideTimer, &QTimer::timeout, this, [this]() {
                    if (m_bFullScreenPlay && m_bPlaylistVisible) {
                        OnShowOrHidePlaylist(); // 自动隐藏播放列表
                    }
                });
          bool showSubtitle = true; // 默认显示
         // 如果需要从配置加载：showSubtitle = GlobalHelper::LoadShowSubtitleSetting();
          m_stActShowSubtitle.setChecked(showSubtitle);
          mainwin_width = this->width();
          QTimer::singleShot(100, this, [this]() {
                  // 尝试将焦点设置到控制面板的播放/暂停按钮
                  QPushButton* playBtn = ui->CtrlBarWid->findChild<QPushButton*>("PlayBtn");
                  if (playBtn) {
                      playBtn->setFocus();
                  } else {
                      // 如果找不到按钮，设置焦点到控制面板本身
                      ui->CtrlBarWid->setFocus();
                  }

                  // 同时确保播放列表不获取焦点
                  m_stPlaylist.setFocusPolicy(Qt::NoFocus);
              });
}

MainWid::~MainWid()
{
    delete ui;
}


bool MainWid::Init()
{
    QWidget *em = new QWidget(this);
    ui->PlaylistWid->setTitleBarWidget(em);
    ui->PlaylistWid->setWidget(&m_stPlaylist);
    //ui->PlaylistWid->setFixedWidth(100);

    QWidget *emTitle = new QWidget(this);
    ui->TitleWid->setTitleBarWidget(emTitle);
    ui->TitleWid->setWidget(&m_stTitle);


    //     FramelessHelper *pHelper = new FramelessHelper(this); //无边框管理
    //     pHelper->activateOn(this);  //激活当前窗体
    //     pHelper->setTitleHeight(ui->TitleWid->height());  //设置窗体的标题栏高度
    //     pHelper->setWidgetMovable(true);  //设置窗体可移动
    //     pHelper->setWidgetResizable(true);  //设置窗体可缩放
    //     pHelper->setRubberBandOnMove(true);  //设置橡皮筋效果-可移动
    //     pHelper->setRubberBandOnResize(true);  //设置橡皮筋效果-可缩放

    //连接自定义信号与槽
    if (ConnectSignalSlots() == false)
    {
        return false;
    }

    if (ui->CtrlBarWid->Init() == false ||
            m_stPlaylist.Init() == false ||
            ui->ShowWid->Init() == false ||
            m_stTitle.Init() == false)
    {
        return false;
    }


    m_stCtrlbarAnimationShow = new QPropertyAnimation(ui->CtrlBarWid, "geometry");
    m_stCtrlbarAnimationHide = new QPropertyAnimation(ui->CtrlBarWid, "geometry");

    if (m_stAboutWidget.Init() == false)
    {
        return false;
    }


    return true;
}

void MainWid::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
}


void MainWid::enterEvent(QEvent *event)
{
    Q_UNUSED(event);

}

void MainWid::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

}

bool MainWid::ConnectSignalSlots()
{
    //连接信号与槽
    connect(&m_stTitle, &Title::SigCloseBtnClicked, this, &MainWid::OnCloseBtnClicked);
    connect(&m_stTitle, &Title::SigMaxBtnClicked, this, &MainWid::OnMaxBtnClicked);
    connect(&m_stTitle, &Title::SigMinBtnClicked, this, &MainWid::OnMinBtnClicked);
    connect(&m_stTitle, &Title::SigDoubleClicked, this, &MainWid::OnFullScreenPlay);
    connect(&m_stTitle, &Title::SigAlwaysOnTopBtnClicked, this, &MainWid::OnAlwaysOnTopBtnClicked);
    connect(&m_stTitle, &Title::SigEqualizerClicked, this, &MainWid::OnShowSettingWid);
    connect(&m_stTitle, &Title::SigOpenFile, &m_stPlaylist, &Playlist::OnAddFileAndPlay);
    connect(&m_stTitle, &Title::SigShowMenu, this, &MainWid::OnShowMenu);


    connect(&m_stPlaylist, &Playlist::SigPlay, ui->ShowWid, &Show::SigPlay);
    connect(&m_stPlaylist, &Playlist::SigSaveSubtitleFile, ui->ShowWid, &Show::OnLyricDownloaded);
    connect(&m_stPlaylist, &Playlist::SigStop, VideoCtl::GetInstance(), &VideoCtl::OnStop);

    // 连接信号
    //connect(ui->ShowWid, &Show::SigOpenFile, &m_stPlaylist, &Playlist::OnAddFileAndPlay);
    connect(ui->ShowWid, &Show::SigAddFilesAndPlay, &m_stPlaylist, &Playlist::appToIndexAndPlay);
    connect(ui->ShowWid, &Show::SigFullScreen, this, &MainWid::OnFullScreenPlay);
    connect(ui->ShowWid, &Show::MinBtnClicked, this, &MainWid::OnMinBtnClicked);
    connect(ui->ShowWid, &Show::SigPlayOrPause, VideoCtl::GetInstance(), &VideoCtl::OnPause);
    connect(ui->ShowWid, &Show::SigStop, VideoCtl::GetInstance(), &VideoCtl::OnStop);
    connect(ui->ShowWid, &Show::SigShowMenu, this, &MainWid::OnShowMenu);
//    connect(ui->ShowWid, &Show::SigSeekForward, ui->CtrlBarWid, &CtrlBar::on_Forward5Btn_clicked);
//    connect(ui->ShowWid, &Show::SigSeekBack, ui->CtrlBarWid, &CtrlBar::on_Backward5Btn_clicked);

    connect(ui->ShowWid, &Show::SigAddVolume, VideoCtl::GetInstance(), &VideoCtl::OnAddVolume);
    connect(ui->ShowWid, &Show::SigSubVolume, VideoCtl::GetInstance(), &VideoCtl::OnSubVolume);

    connect(ui->CtrlBarWid, &CtrlBar::SigSpeed, VideoCtl::GetInstance(), &VideoCtl::OnSpeed);
    connect(ui->CtrlBarWid, &CtrlBar::SigShowOrHidePlaylist, this, &MainWid::OnShowOrHidePlaylist);
    connect(ui->CtrlBarWid, &CtrlBar::SigPlaySeek, VideoCtl::GetInstance(), &VideoCtl::OnPlaySeek);
    connect(ui->CtrlBarWid, &CtrlBar::SigPlayVolume, VideoCtl::GetInstance(), &VideoCtl::OnPlayVolume);
    connect(ui->CtrlBarWid, &CtrlBar::SigPlayOrPause, VideoCtl::GetInstance(), &VideoCtl::OnPause);
//    connect(ui->CtrlBarWid, &CtrlBar::SigStop, VideoCtl::GetInstance(), &VideoCtl::OnStop);

    connect(ui->CtrlBarWid, &CtrlBar::SigFullScreenBtn, this, &MainWid::OnFullScreenPlay);
    connect(ui->CtrlBarWid, &CtrlBar::SigBackwardPlay, &m_stPlaylist, &Playlist::OnBackwardPlay);
    connect(ui->CtrlBarWid, &CtrlBar::SigForwardPlay, &m_stPlaylist, &Playlist::OnForwardPlay);
    connect(ui->CtrlBarWid, &CtrlBar::SigShowMenu, this, &MainWid::OnShowMenu);
    connect(ui->CtrlBarWid, &CtrlBar::SigShowSetting, this, &MainWid::OnShowSettingWid);
    // 连接CtrlBar的SigPlaySelected信号到Playlist的PlayByIndex槽
    connect(ui->CtrlBarWid, &CtrlBar::SigPlaySelected, &m_stPlaylist, &Playlist::PlayByIndex);

    connect(this, &MainWid::SigShowMax, &m_stTitle, &Title::OnChangeMaxBtnStyle);
    connect(this, &MainWid::SigSeekForward, ui->CtrlBarWid, &CtrlBar::on_Forward5Btn_clicked);
    connect(this, &MainWid::SigSeekBack, ui->CtrlBarWid, &CtrlBar::on_Backward5Btn_clicked);
    connect(this, &MainWid::SigSeekForward10s, ui->CtrlBarWid, &CtrlBar::on_Forward5Btn_rightClicked);
    connect(this, &MainWid::SigSeekBack10s, ui->CtrlBarWid, &CtrlBar::on_Backward5Btn_rightClicked);
    connect(this, &MainWid::SigAddVolume, VideoCtl::GetInstance(), &VideoCtl::OnAddVolume);
    connect(this, &MainWid::SigSubVolume, VideoCtl::GetInstance(), &VideoCtl::OnSubVolume);
    connect(this, &MainWid::SigOpenFile, &m_stPlaylist, &Playlist::OnAddFileAndPlay);
    connect(this, &MainWid::SigPlayOrPause, VideoCtl::GetInstance(), &VideoCtl::OnPause);
    connect(this, &MainWid::SigAppendItemTotop, &m_stPlaylist, &Playlist::appToIndexAndPlay);
    connect(this, &MainWid::SigFadeVolmeClose, VideoCtl::GetInstance(), &VideoCtl::runFadeProc);
    connect(this, &MainWid::SigStepFrameForward, VideoCtl::GetInstance(), &VideoCtl::StepFrameForward);
    connect(this, &MainWid::SigStepFrameBackward, VideoCtl::GetInstance(), &VideoCtl::StepFrameBackward);
    connect(this, &MainWid::SigToggleFrameStepMode, VideoCtl::GetInstance(), &VideoCtl::SetFrameStepMode);

    connect(VideoCtl::GetInstance(), &VideoCtl::SigSpeed, ui->CtrlBarWid, &CtrlBar::OnSpeed);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigCloseWin,this, &MainWid::close);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigVideoTotalSeconds, ui->CtrlBarWid, &CtrlBar::OnVideoTotalSeconds);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigVideoPlaySeconds, ui->CtrlBarWid, &CtrlBar::OnVideoPlaySeconds);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigVideoVolume, ui->CtrlBarWid, &CtrlBar::OnVideopVolume);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigPauseStat, ui->CtrlBarWid, &CtrlBar::OnPauseStat, Qt::QueuedConnection);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigStopFinished, ui->CtrlBarWid, &CtrlBar::OnStopFinished, Qt::QueuedConnection);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigStopFinished, ui->ShowWid, &Show::OnStopFinished, Qt::QueuedConnection);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigFrameDimensionsChanged, ui->ShowWid, &Show::OnFrameDimensionsChanged, Qt::QueuedConnection);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigStopFinished, &m_stTitle, &Title::OnStopFinished, Qt::DirectConnection);
    connect(VideoCtl::GetInstance(), &VideoCtl::SigStartPlay, &m_stTitle, &Title::OnPlay, Qt::DirectConnection);

    connect(&m_stSettingWid, &SettingWid::subtitleSettingsChanged,this, &MainWid::onSubtitleSettingsChanged);
    connect(&m_stSettingWid, &SettingWid::subtitleColorSettingsChanged,ui->ShowWid, &Show::OnSubtitleColorSettingsChanged);
    //connect(&m_stCtrlBarAnimationTimer, &QTimer::timeout, this, &MainWid::OnCtrlBarAnimationTimeOut);

    connect(&m_stFullscreenMouseDetectTimer, &QTimer::timeout,
                    this, &MainWid::OnMouseActivityCheckTimeOut);

    connect(VideoCtl::GetInstance(), &VideoCtl::SigAudioSpectrumData,
                    ui->ShowWid, &Show::OnAudioSpectrumData);
    // 添加播放模式恢复信号连接
        connect(&m_stPlaylist, &Playlist::SigPlayModeRestored,
                ui->CtrlBarWid, &CtrlBar::OnPlayModeChanged);
        connect(VideoCtl::GetInstance(), &VideoCtl::SigStop, this, &MainWid::OnAutoPlayNext);
        connect(&m_stPlaylist, &Playlist::SigPlayOrPause, VideoCtl::GetInstance(), &VideoCtl::OnPause);
        //重命令名时，要停止播放。要保存和恢复播放的时间点。
        connect(&m_stPlaylist, &Playlist::SigGetPlayPosition, VideoCtl::GetInstance(), &VideoCtl::onGetPlayPosition);
        connect(&m_stPlaylist, &Playlist::SigSetPlayPosition, VideoCtl::GetInstance(), &VideoCtl::onSetPlayPosition);
    //连接菜单信号
        connect(&m_stActFullscreen, &QAction::triggered, this, &MainWid::OnFullScreenPlay);
        connect(&m_stActPlaylist, &QAction::triggered, this, &MainWid::OnShowOrHidePlaylist);
        connect(&m_stActExtend, &QAction::triggered, this, &MainWid::setExtend);
        connect(&m_stActShowSubtitle, &QAction::triggered,this, &MainWid::onShowSubtitle);
        connect(&m_stActAlwaysOnTop, &QAction::triggered, this, &MainWid::OnAlwaysOnTopBtnClicked);
    // 连接播放状态变化信号来更新菜单状态
        // 连接视频尺寸变化信号
        connect(ui->ShowWid, &Show::SigVideoSizeChanged,
                    this, &MainWid::onVideoSizeChanged);
        connect(VideoCtl::GetInstance(), &VideoCtl::SigStartPlay,
                this, &MainWid::onUpdateActualSizeMenuState);
        connect(VideoCtl::GetInstance(), &VideoCtl::SigStopFinished,
                this, &MainWid::onUpdateActualSizeMenuState);
        // 添加延迟处理命令行文件的连接
            QTimer::singleShot(100, this, [this]() {
                processCommandLineFile();
            });
    return true;
}

// 处理命令行文件的槽函数
void MainWid::openFileFromCommandLine(const QString &filePath)
{
    m_commandLineFiles.append(filePath);
}



// 处理单个命令行文件
void MainWid::processCommandLineFile()
{
    if (m_commandLineFiles.size()<=0)return;
    GlobalHelper::SaveAutoPlay(true);
    // 或者带索引号输出
    int index = 1;
    for (const QString &file : m_commandLineFiles) {
        qDebug() << index++ << ":" << file;
    }
    GlobalHelper::haveCommandLine()=0;
    emit SigAppendItemTotop(m_commandLineFiles,0);
}


void MainWid::keyReleaseEvent(QKeyEvent *event)
{
    // 	    // 是否按下Ctrl键      特殊按键
    //     if(event->modifiers() == Qt::ControlModifier){
    //         // 是否按下M键    普通按键  类似
    //         if(event->key() == Qt::Key_M)
    //             ···
    //     }
    if (m_bFullScreenPlay) {
        if (!(event->key() == Qt::Key_Left || event->key() == Qt::Key_Right || event->key() == Qt::Key_Up || event->key() == Qt::Key_Down))
            UpdateMouseActivity();
        }
    if (!(event->key() == Qt::Key_Comma || event->key() == Qt::Key_Period)){
        m_bFrameStepMode = false;
        emit SigToggleFrameStepMode(false);
    }
    qDebug() << "MainWid::keyPressEvent:" << event->key();
    switch (event->key())
    {
    case Qt::Key_Escape:
        if (m_bFullScreenPlay){
            emit SigPlayOrPause();
            OnFullScreenPlay();
            OnMinBtnClicked();
        }
        break;
    case Qt::Key_Enter:     // 小键盘回车
    case Qt::Key_Return://全屏
        OnFullScreenPlay();
        break;
    case Qt::Key_Comma://.
                    m_bFrameStepMode = true;
                    emit SigToggleFrameStepMode(true);
                emit SigStepFrameBackward();
        break;
    case Qt::Key_Period://,
                    m_bFrameStepMode = true;
                    emit SigToggleFrameStepMode(true);
                emit SigStepFrameForward();
        break;
    case Qt::Key_Left://后退5s
        emit SigSeekBack();
        break;
    case Qt::Key_PageDown://后退5s
        emit SigSeekForward10s();
        break;
    case Qt::Key_PageUp://后退5s
        emit SigSeekBack10s();
        break;
    case Qt::Key_Right://前进5s
        qDebug() << "前进5s";
        emit SigSeekForward();
        break;
    case Qt::Key_Up://增加10音量
        qDebug() << "加音量－－－－－－－－－－－－－－－+++++++++";
        emit SigAddVolume();
        break;
    case Qt::Key_Down://减少10音量
        qDebug() << "加音量－－－－－－－－－－－－－－－----------------";
        emit SigSubVolume();
        break;
    case Qt::Key_Space://暂停
        emit SigPlayOrPause();
        break;

    default:
        break;
    }
}


void MainWid::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        if (ui->TitleWid->geometry().contains(event->pos()) && GlobalVars::getWinState()==0)
        {
            m_bMoveDrag = true;
            m_DragPosition = event->globalPos() - this->pos();
        }
    }

    QWidget::mousePressEvent(event);
}

void MainWid::mouseReleaseEvent(QMouseEvent *event)
{
    m_bMoveDrag = false;
    // 停止调整大小
        if (m_bResizeDrag)
        {
            StopResize();
        }
    QWidget::mouseReleaseEvent(event);
}

void MainWid::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bMoveDrag) {
            move(event->globalPos() - m_DragPosition);
            // 拖动窗口时立即更新字幕位置
            if (ui->ShowWid) {
                // 调用Show类的更新函数
                ui->ShowWid->updateSubtitleWindowPosition();
                qDebug() << "拖动窗口，更新字幕位置------";
            }
        }
        // 如果正在调整大小，不处理移动窗口
        else if (m_bResizeDrag) {
            DoResize(event->globalPos());
        }
        // 否则更新鼠标光标
        else {
            UpdateCursorForBorder(event->pos());
        }
    // 全屏状态下，如果播放列表可见，处理鼠标悬停自动隐藏
        if (m_bFullScreenPlay && m_bPlaylistVisible) {
            QPoint mousePos = event->globalPos();
            QRect playlistRect = ui->PlaylistWid->geometry();

            // 如果鼠标不在播放列表区域，可以考虑启动隐藏定时器
            if (!playlistRect.contains(mousePos)) {
                // 这里可以添加自动隐藏逻辑
            }
        }
    QWidget::mouseMoveEvent(event);
}

void MainWid::contextMenuEvent(QContextMenuEvent* event)
{
    m_stMenu.exec(event->globalPos());
}
int optionBz = 0;
bool MainWid::eventFilter(QObject *watched, QEvent *event)
{
    // 处理按键释放事件
    if (event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
//        if (!(keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down))
//            UpdateMouseActivity();
        // 检查是否是CtrlBarWid或其子控件的按键事件
        if (watched == ui->CtrlBarWid ||
            watched == ui->ShowWid ||
            watched == ui->PlaylistWid ||
            watched == ui->TitleWid ||
            ui->CtrlBarWid->isAncestorOf(static_cast<QWidget*>(watched)))
        {
            // 直接调用keyReleaseEvent处理
            keyReleaseEvent(keyEvent);
            return true; // 事件已处理
        }
    }

    // 处理全屏状态下视频窗口的鼠标事件
        if (m_bFullScreenPlay && watched == ui->ShowWid)

        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            switch (event->type())
            {
            case QEvent::MouseButtonPress:
            {
                optionBz = 1;
                if (mouseEvent->button() == Qt::LeftButton) {
                     changeCtrlTileShow();   //切换显示与隐藏
                }else {
                    m_bFullscreenCtrlBarShow = false;
                    UpdateMouseActivity();
                }
                // 记录鼠标位置
                m_lastMousePos = mouseEvent->globalPos();
                // 如果点击视频区域且播放列表可见，隐藏播放列表
                if (m_bPlaylistVisible && m_bClickVideoToHidePlaylist)
                {
                    qDebug() << " 准备隐藏播放列表" ;
                    showFullscreenPlaylist(false);
                    return true;
                }
                break;
            }

            case QEvent::MouseButtonDblClick:
            {
                optionBz = 2;
                UpdateMouseActivity();
                // 记录鼠标位置
                m_lastMousePos = mouseEvent->globalPos();

                // 如果点击视频区域且播放列表可见，隐藏播放列表
                if (m_bPlaylistVisible && m_bClickVideoToHidePlaylist)
                {
                    // 检查点击位置是否在播放列表内
                    QRect playlistRect = ui->PlaylistWid->geometry();
                    if (!playlistRect.contains(mouseEvent->globalPos()))
                    {
                        // 点击位置不在播放列表内，隐藏播放列表
                        ui->PlaylistWid->hide();
                        // 恢复播放列表的父窗口和样式
                        ui->PlaylistWid->setParent(this);
                        ui->PlaylistWid->setWindowFlags(Qt::Widget);
                        ui->PlaylistWid->setStyleSheet("");
                        m_playlistHideTimer = nullptr;
                        // 重新将播放列表添加到分割器
                        if (m_splitter && m_splitter->indexOf(ui->PlaylistWid) == -1) {
                            m_splitter->addWidget(ui->PlaylistWid);
                        }
                        // 禁用点击视频隐藏功能
                        m_bClickVideoToHidePlaylist = false;
                       // m_bPlaylistVisible=false;
                    }
                }
                // 防止双击导致程序卡死
                qDebug() << "视频窗口双击事件，已放行！";
                //return true; // 阻止事件继续传递
            }
            case QEvent::Wheel:
               {optionBz = 3;
                qDebug() << "鼠标轮滚动－－－－－－－－！" << m_lastMouseActivityTime << " m_bFullscreenCtrlBarShow:"<< m_bFullscreenCtrlBarShow;
                        UpdateMouseActivity();
                        break;
                }
            case QEvent::MouseMove:
            {
                optionBz = 4;
                //qDebug() << "鼠标移动 m_bFullscreenCtrlBarShow=---------------" << m_bFullscreenCtrlBarShow;
                break;
            }
            default:
                break;
            }
        }
        // 处理边框的鼠标事件
        if (watched == m_borderFrame)
        {
            switch (event->type())
            {
            case QEvent::HoverEnter:
            case QEvent::HoverMove:
            {
                QHoverEvent *hoverEvent = static_cast<QHoverEvent*>(event);
                UpdateCursorForBorder(hoverEvent->pos());
                return true;
            }
            case QEvent::MouseButtonPress:
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton)
                {
                    Qt::Edges edges = GetEdgesForBorderPos(mouseEvent->pos());
                    if (edges != Qt::Edges())
                    {
                        StartResize(mouseEvent->globalPos());
                        return true;
                    }
                }
                break;
            }
            case QEvent::MouseMove:
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if (m_bResizeDrag)
                {
                    DoResize(mouseEvent->globalPos());
                    return true;
                }
                else
                {
                    UpdateCursorForBorder(mouseEvent->pos());
                }
                break;
            }
            case QEvent::MouseButtonRelease:
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton && m_bResizeDrag)
                {
                    StopResize();
                    return true;
                }
                break;
            }
            default:
                break;
            }
        }

    // 其他事件传递给基类处理
    return QMainWindow::eventFilter(watched, event);
}
// 边框调整大小相关函数的实现
Qt::CursorShape MainWid::GetCursorForBorderPos(const QPoint &pos) const
{
    if (!m_borderFrame) return Qt::ArrowCursor;

    QRect frameRect = m_borderFrame->rect();
    bool leftEdge = pos.x() <= m_nBorderWidth;
    bool rightEdge = pos.x() >= frameRect.width() - m_nBorderWidth;
    bool topEdge = pos.y() <= m_nBorderWidth;
    bool bottomEdge = pos.y() >= frameRect.height() - m_nBorderWidth;

    if (leftEdge && topEdge) return Qt::SizeFDiagCursor;
    if (leftEdge && bottomEdge) return Qt::SizeBDiagCursor;
    if (rightEdge && topEdge) return Qt::SizeBDiagCursor;
    if (rightEdge && bottomEdge) return Qt::SizeFDiagCursor;
    if (leftEdge || rightEdge) return Qt::SizeHorCursor;
    if (topEdge || bottomEdge) return Qt::SizeVerCursor;

    return Qt::ArrowCursor;
}

Qt::Edges MainWid::GetEdgesForBorderPos(const QPoint &pos) const
{
    if (!m_borderFrame) return Qt::Edges();

    QRect frameRect = m_borderFrame->rect();
    Qt::Edges edges;

    if (pos.x() <= m_nBorderWidth) edges |= Qt::LeftEdge;
    if (pos.x() >= frameRect.width() - m_nBorderWidth) edges |= Qt::RightEdge;
    if (pos.y() <= m_nBorderWidth) edges |= Qt::TopEdge;
    if (pos.y() >= frameRect.height() - m_nBorderWidth) edges |= Qt::BottomEdge;

    return edges;
}

void MainWid::UpdateCursorForBorder(const QPoint &pos)
{
    if (!m_borderFrame || m_bResizeDrag) return;

    // 如果窗口最大化或全屏，不显示调整大小光标
    if (isMaximized() || isFullScreen())
    {
        m_borderFrame->setCursor(Qt::ArrowCursor);
        return;
    }

    Qt::CursorShape cursor = GetCursorForBorderPos(pos);
    m_borderFrame->setCursor(cursor);
}

void MainWid::StartResize(const QPoint &pos)
{
    if (isMaximized() || isFullScreen()) return;

    m_bResizeDrag = true;
    m_ResizeStartPos = pos;
    m_ResizeStartGeometry = geometry();
    m_ResizeEdges = GetEdgesForBorderPos(m_borderFrame->mapFromGlobal(pos));

    // 设置鼠标捕获，确保在拖动过程中能接收到鼠标事件
    m_borderFrame->setMouseTracking(true);
    m_borderFrame->grabMouse();
}

void MainWid::DoResize(const QPoint &pos)
{
    if (!m_bResizeDrag || !m_borderFrame) return;

    QPoint delta = pos - m_ResizeStartPos;
    QRect newGeometry = m_ResizeStartGeometry;

    // 根据调整的边计算新的几何位置
    if (m_ResizeEdges & Qt::LeftEdge)
    {
        int newLeft = newGeometry.left() + delta.x();
        if (newLeft < newGeometry.right() - minimumWidth())
            newGeometry.setLeft(newLeft);
    }

    if (m_ResizeEdges & Qt::RightEdge)
    {
        int newRight = newGeometry.right() + delta.x();
        if (newRight > newGeometry.left() + minimumWidth())
            newGeometry.setRight(newRight);
    }

    if (m_ResizeEdges & Qt::TopEdge)
    {
        int newTop = newGeometry.top() + delta.y();
        if (newTop < newGeometry.bottom() - minimumHeight())
            newGeometry.setTop(newTop);
    }

    if (m_ResizeEdges & Qt::BottomEdge)
    {
        int newBottom = newGeometry.bottom() + delta.y();
        if (newBottom > newGeometry.top() + minimumHeight())
            newGeometry.setBottom(newBottom);
    }

    // 确保窗口不小于最小大小
    if (newGeometry.width() < minimumWidth())
    {
        if (m_ResizeEdges & Qt::LeftEdge)
            newGeometry.setLeft(newGeometry.right() - minimumWidth());
        else
            newGeometry.setRight(newGeometry.left() + minimumWidth());
    }

    if (newGeometry.height() < minimumHeight())
    {
        if (m_ResizeEdges & Qt::TopEdge)
            newGeometry.setTop(newGeometry.bottom() - minimumHeight());
        else
            newGeometry.setBottom(newGeometry.top() + minimumHeight());
    }

    setGeometry(newGeometry);
}

void MainWid::StopResize()
{
    if (!m_bResizeDrag) return;

    m_bResizeDrag = false;
    m_ResizeEdges = Qt::Edges();

    // 释放鼠标捕获
    if (m_borderFrame)
    {
        m_borderFrame->releaseMouse();
        m_borderFrame->setCursor(Qt::ArrowCursor);
    }
}
void MainWid::OnFullScreenPlay()
{
    if (m_bFullScreenPlay == false)
    {
        isShowPlaylistOnWin = m_bPlaylistVisible;//备份窗口模式列表是不是显示
        // 进入全屏前保存状态
        if (m_splitter) {
                m_preFullScreenSplitterState = m_splitter->saveState();
            }
        playlistWidth = ui->PlaylistWid->width() == 188 ? 300:ui->PlaylistWid->width();
        qDebug() << "进入全屏前列表宽度：－－－－－－－－－－－－－" << playlistWidth;
        m_preFullScreenSize = this->size();
        m_preFullScreenPos = this->pos();
        m_preFullScreenVideoSize = ui->ShowWid->size();
        m_bIsFullScreen = true;

        GlobalVars::getFullScreen()=true;
        GlobalVars::getWinState() = 4;
        m_bFullScreenPlay = true;

        // 如果播放列表当前可见，先隐藏它
        if (m_bPlaylistVisible) {
            playlistWidth = ui->PlaylistWid->width();
            ui->PlaylistWid->hide();
            m_bPlaylistVisible = false;
            updatePlaylistButtonIcon(false);
        }

        this->hide();

        // 设置视频窗口为独立窗口
        ui->ShowWid->setWindowFlags(Qt::Window);
        QScreen *pStCurScreen = qApp->screens().at(qApp->desktop()->screenNumber(this));
        ui->ShowWid->windowHandle()->setScreen(pStCurScreen);
        ui->ShowWid->showFullScreen();

        // 设置控制栏为独立窗口
        QRect stScreenRect = pStCurScreen->geometry();
        int nCtrlBarHeight = ui->CtrlBarWid->height();
        int nTitleBarHeight = ui->TitleWid->height(); // 获取标题栏高度
        int nPlayListHeight = stScreenRect.height() - nCtrlBarHeight - nTitleBarHeight;
        int nPlaylistWidth =  stScreenRect.width()*0.25;
        int nX0 = stScreenRect.width() - nPlaylistWidth;
        int nX = ui->ShowWid->x();
        m_stCtrlBarAnimationShow = QRect(nX, stScreenRect.height() - nCtrlBarHeight, stScreenRect.width(), nCtrlBarHeight);
        m_stCtrlBarAnimationHide = QRect(nX, stScreenRect.height(), stScreenRect.width(), nCtrlBarHeight);
        m_stTitleBarAnimationShow = QRect(nX, 0, stScreenRect.width(), nTitleBarHeight);
        m_stTitleBarAnimationHide = QRect(nX, -nTitleBarHeight, stScreenRect.width(), nTitleBarHeight);

        m_stPlayListAnimationShow = QRect(nX0, nTitleBarHeight, nPlaylistWidth, nPlayListHeight);
        m_stPlayListAnimationHide = QRect(stScreenRect.width()-1, nTitleBarHeight,  nPlaylistWidth, nPlayListHeight);
        m_stTitlebarAnimationShow = new QPropertyAnimation(ui->TitleWid, "geometry");
        m_stTitlebarAnimationHide = new QPropertyAnimation(ui->TitleWid, "geometry");
        m_stPlaylistAnimationShow = new QPropertyAnimation(ui->PlaylistWid, "geometry");
        m_stPlaylistAnimationHide = new QPropertyAnimation(ui->PlaylistWid, "geometry");

        m_stPlaylistAnimationShow->setStartValue(m_stPlayListAnimationHide);
        m_stPlaylistAnimationShow->setEndValue(m_stPlayListAnimationShow);
        m_stPlaylistAnimationShow->setDuration(2000);
        m_stPlaylistAnimationHide->setStartValue(m_stPlayListAnimationShow);
        m_stPlaylistAnimationHide->setEndValue(m_stPlayListAnimationHide);
        m_stPlaylistAnimationHide->setDuration(2000);

        m_stCtrlbarAnimationShow->setStartValue(m_stCtrlBarAnimationHide);
        m_stCtrlbarAnimationShow->setEndValue(m_stCtrlBarAnimationShow);
        m_stCtrlbarAnimationShow->setDuration(1000);
        m_stCtrlbarAnimationHide->setStartValue(m_stCtrlBarAnimationShow);
        m_stCtrlbarAnimationHide->setEndValue(m_stCtrlBarAnimationHide);
        m_stCtrlbarAnimationHide->setDuration(1000);

        m_stTitlebarAnimationShow->setStartValue(m_stTitleBarAnimationHide);
        m_stTitlebarAnimationShow->setEndValue(m_stTitleBarAnimationShow);
        m_stTitlebarAnimationShow->setDuration(1000);
        m_stTitlebarAnimationHide->setStartValue(m_stTitleBarAnimationShow);
        m_stTitlebarAnimationHide->setEndValue(m_stTitleBarAnimationHide);
        m_stTitlebarAnimationHide->setDuration(1000);

        ui->PlaylistWid->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
        ui->PlaylistWid->windowHandle()->setScreen(pStCurScreen);
        ui->PlaylistWid->raise();
        ui->PlaylistWid->setWindowOpacity(0.85);
        ui->PlaylistWid->showNormal();
        ui->PlaylistWid->windowHandle()->setScreen(pStCurScreen);

        ui->PlaylistWid->setGeometry(stScreenRect.width(), nTitleBarHeight,
                                   nPlaylistWidth, nPlayListHeight);
        // 全屏时设置播放列表样式和行高
                QString playlistStyle =
                        "QDockWidget {"  // 修改：设置整个DockWidget的样式
                                    "    background: #2e2f37;"
                                    "    border: 1px solid Black;"
                                    "    border-radius: 8px;"  // 整个播放列表区域圆角
                                    "}"
                                    ""
                                    "* {"  // 修改：应用给所有子控件
                                    "    background: transparent;"  // 子控件背景透明，显示父控件背景
                                    "    color: #cccccc;"
                                    "}"
                                    ""
                                    "QWidget#playlistWidget {"  // 为Playlist内部widget添加objectName
                                    "    background: transparent;"
                                    "    border: none;"
                                    "}"
                                    ""
                                                "QFrame#frameHeader {"
                                                "    background: transparent;"  // 透明背景
                                                "    border: none;"  // 无边框
                                                "    border-radius: 4px;"  // 有圆角
                        "    min-height: 24px;"  // 顶部高度设置为24px
                        "    max-height: 24px;"  // 顶部高度设置为24px
                                                "}"
                                                ""
                                                "/* 播放列表标题标签 - 全屏 */"
                                                "QPushButton#btnListTitle {"
//                                              "    color: aaaaaa;"  // 白色文字
                                                "    font-size: 20px;"  // 与全屏列表文字大小相同
                        "    font-family: 'Microsoft YaHei', '微软雅黑', sans-serif;"
                                                "    font-weight: bold;"  // 保持粗体
                                                "    background: transparent;"  // 透明背景
                                                "    border: none;"  // 无边框
                                                "}"
                                                ""
                                                "/* 列表索引标签 - 全屏 */"
                                                "QLabel#labelIndex {"
//                                                "    color: 888888;"  // 白色文字
                                                "    font-size: 20px;"  // 与全屏列表文字大小相同
                        "    font-family: 'Microsoft YaHei', '微软雅黑', sans-serif;"
                                                "    background: transparent;"  // 透明背景
                                                "    border: none;"  // 无边框
                                                "    padding-left: 10px; "
                                                "}"
                                                ""
                                                "/* 图标标签 - 全屏 */"
                                                            "QPushButton#labelIcon {"
                                                            "    min-width: 24px;"  // 图标尺寸变大
                                                            "    max-width: 24px;"  // 图标尺寸变大
                                                            "    min-height: 24px;"  // 图标尺寸变大
                                                            "    max-height: 24px;"  // 图标尺寸变大
                                                            "    padding: 0px;"
                                                            "    margin: 0px 0px 0px 4px;"
                                                            "}"
                                                            ""
                                    "QPushButton {"
                                    "    background: transparent;"
                                    "    color: #b0b0b0;"
                                    "    border: 1px solid transparent;"
                                    "    border-radius: 4px;"  // 按钮也有圆角
                                    "    padding: 4px;"
                                    "}"
                                    "QPushButton:hover {"
                                    "    color: Cyan;"
                                    "    background: rgba(255, 255, 255, 0.1);"
                                    "}"
                                    "QPushButton:pressed {"
                                    "    color: CadetBlue;"
                                    "    background: rgba(255, 255, 255, 0.2);"
                                    "}"
                                    ""
                                    "/*****列表*******/"
                                    "QListWidget {"
                                    "    border: 1px solid #555555;"
                                    "    alternate-background-color: #3a3b42;"
                                    "    show-decoration-selected: 1;"
                                    "    font-size: 20px;" /* 全屏时加大字体大小 */
                                    "    border-radius: 6px;" /* 列表内部圆角 */
                                    "    padding: 5px;"
                                    "    background: rgba(0, 0, 0, 0.3);" /* 半透明背景 */
                                    "}"
                                    "QListWidget::item {"
                                    "    background: transparent;"
                                    "    padding: 0px;"
                                    "    margin: 0px;"
                                    "    border: none;"
                                    "}"
                                    "QListWidget::item:hover {"
                                    "    color: Cyan;"
                                    "    border: 1px solid Cyan;"
                                    "    background: rgba(255, 255, 255, 0.1);"
                                    "    margin: 0px;"
                                    "    padding: 0px;"
                                    "}"
                                    "QListWidget::item:selected {"
                                    "    background: rgb(5, 146, 186);"
                                    "    color: black;"
                                    "    border: 0px solid rgb(5, 146, 186);"
                                    "    margin: 0px;"
                                    "    padding: 0px;"
                                    "}"
                                    ""
                                    "/* 修复滚动条设置 */"
                                    "QScrollBar:vertical {"
                                    "    width: 12px;"
                                    "    background: transparent;"
                                    "    border: none;"
                                    "}"
                                    "QScrollBar::handle:vertical {"
                                    "    min-height: 30px;"
                                    "    background: #202129;"
                                    "    margin: 0px;"
                                    "    border-radius: 6px;"
                                    "}"
                                    "QScrollBar::handle:vertical:hover {"
                                    "    background: rgb(80, 80, 80);"
                                    "}"
                                    "QScrollBar::sub-line:vertical,"
                                    "QScrollBar::add-line:vertical {"
                                    "    height: 0px;"
                                    "    background: transparent;"
                                    "    border: none;"
                                    "}"
                                    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                                    "    background: transparent;"
                                    "}";

                                ui->PlaylistWid->setStyleSheet(playlistStyle);
                                m_stPlaylist.setStyleSheet(""); // 清除Playlist内部的样式，让父控件样式生效
                                 m_stPlaylist.setRowHeight(40); // 全屏时使用较大的行高

                                 // 全屏时需要重新设置图标，因为图标大小改变了
                                 QPushButton *labelIcon = m_stPlaylist.findChild<QPushButton*>("labelIcon");
                                 if (labelIcon) {
                                         // 全屏时设置更大的图标（16px）
                                         GlobalHelper::SetIcon(labelIcon, 16, QChar(0xf00b)); // 列表图标
                                         // 调整按钮大小
                                         labelIcon->setFixedSize(28, 28);
                                         labelIcon->setIconSize(QSize(20, 20));
                                         QString btnStyleSheet = "QPushButton { "
                                                                 "border: none; "
                                                                 "padding: 1px 4px 0px 4px; "//内边距
                                                                 "margin: 0px -6px 0px 2px;"  //外边距上右下左
                                                                 "background-color: transparent; "
                                                                 "}"
                                                                 "QPushButton:hover { "
                                                                 "color : Cyan;"
                                                                 "background-color: #444444; "  // 浅灰色背景
                                                                 "border-radius: 2px; "
                                                                 "}"
                                                                 "QPushButton:pressed { "
                                                                 "background-color: #555555; "  // 按下时稍深
                                                                 "border-radius: 2px; "
                                                                 "}";
                                         labelIcon->setStyleSheet(btnStyleSheet);
                                 }
                                 // 重点：全屏时加大保存和删除按钮的图标尺寸
                                         // 获取保存和删除按钮并设置更大的图标
                                         QPushButton* btnSave = m_stPlaylist.findChild<QPushButton*>("btnSaveToNewlist");
                                         QPushButton* btnDelete = m_stPlaylist.findChild<QPushButton*>("btnDeleteCurrentList");

                                         if (btnSave) {
                                             // 全屏时设置更大的图标（16px）
                                             GlobalHelper::SetIcon(btnSave, 18, QChar(0xf2e5)); // 软盘图标
                                             // 调整按钮大小
                                             btnSave->setFixedSize(28, 28);
                                             btnSave->setIconSize(QSize(20, 20));
                                             QString btnStyleSheet = "QPushButton { "
                                                                     "border: none; "
                                                                     "padding: 0px 4px 0px 4px; "//内边距
                                                                     "margin: 0px 0px;"  //外边距
                                                                     "background-color: transparent; "
                                                                     "}"
                                                                     "QPushButton:hover { "
                                                                     "color : Cyan;"
                                                                     "background-color: #444444; "  // 浅灰色背景
                                                                     "border-radius: 2px; "
                                                                     "}"
                                                                     "QPushButton:pressed { "
                                                                     "background-color: #555555; "  // 按下时稍深
                                                                     "border-radius: 2px; "
                                                                     "}";

                                             btnSave->setStyleSheet(btnStyleSheet);

                                         }

                                         if (btnDelete) {
                                             // 全屏时设置更大的图标（16px）
                                             GlobalHelper::SetIcon(btnDelete, 18, QChar(0xf014)); // 垃圾桶图标
                                             // 调整按钮大小
                                             btnDelete->setFixedSize(28, 28);
                                             btnDelete->setIconSize(QSize(20, 20));
                                             QString btnStyleSheet = "QPushButton { "
                                                                     "border: none; "
                                                                     "padding: 0px 4px 2px 4px; "//内边距
                                                                     "margin: 0px 0px;"  //外边距
                                                                     "background-color: transparent; "
                                                                     "}"
                                                                     "QPushButton:hover { "
                                                                     "color : Cyan;"
                                                                     "background-color: #444444; "  // 浅灰色背景
                                                                     "border-radius: 2px; "
                                                                     "}"
                                                                     "QPushButton:pressed { "
                                                                     "background-color: #555555; "  // 按下时稍深
                                                                     "border-radius: 2px; "
                                                                     "}";
                                            btnDelete->setStyleSheet(btnStyleSheet);
                                         }
        ui->CtrlBarWid->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
        ui->CtrlBarWid->windowHandle()->setScreen(pStCurScreen);
        ui->CtrlBarWid->raise();
        ui->CtrlBarWid->setWindowOpacity(0.5);
        ui->CtrlBarWid->showNormal();
        ui->CtrlBarWid->windowHandle()->setScreen(pStCurScreen);

        ui->TitleWid->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
        ui->TitleWid->windowHandle()->setScreen(pStCurScreen);
        ui->TitleWid->raise();
        ui->TitleWid->setWindowOpacity(0.5);
        ui->TitleWid->showNormal();
        ui->TitleWid->windowHandle()->setScreen(pStCurScreen);
        ui->TitleWid->setStyleSheet("QDockWidget { border: 0px; }");

        // 确保控制栏在正确位置
        ui->CtrlBarWid->setGeometry(0, stScreenRect.height() - nCtrlBarHeight,
                                   stScreenRect.width(), nCtrlBarHeight);

        m_stCtrlbarAnimationShow->start();
        m_stTitlebarAnimationShow->start();
        m_bFullscreenCtrlBarShow = true;
        //m_stFullscreenMouseDetectTimer.start();

        // 隐藏边框
        if (m_borderFrame)
        {
            m_borderFrame->setFrameShape(QFrame::NoFrame);
            m_borderFrame->setStyleSheet("QFrame { border: 0px; }");
        }

        GlobalHelper::SetIcon(ui->CtrlBarWid->findChild<QPushButton*>("FullScreenBtn"), 12, QChar(0xf22f));

        // 为视频窗口安装事件过滤器
        ui->ShowWid->installEventFilter(this);
        m_lastMouseActivityTime = QDateTime::currentMSecsSinceEpoch();
                m_bMouseCursorHidden = false;
                m_lastMousePos = QPoint(-1, -1); // 重置最后鼠标位置
                m_stFullscreenMouseDetectTimer.start();
    }
    else
    {
        m_bPlaylistVisible = isShowPlaylistOnWin;//恢复窗口模式列表是不是显示
        ShowMouseCursor();
        m_stFullscreenMouseDetectTimer.stop();
        // 1. 停止所有动画
            if (m_stPlaylistAnimationShow) {
                m_stPlaylistAnimationShow->stop();
            }
            if (m_stPlaylistAnimationHide) {
                m_stPlaylistAnimationHide->stop();
            }
        // 如果播放列表在全屏状态下可见，先隐藏它
        if (ui->PlaylistWid->parent() != this || ui->PlaylistWid->windowFlags() != Qt::Widget) {
            qDebug() << "开始隐藏列表----------------------------1";
            //showFullscreenPlaylist(false);这个动画隐藏列表不行，m_playlistHideTimer＝nullptr；会卡死，使用下面直接快速的隐藏代码
            ui->PlaylistWid->hide();
            // 恢复播放列表的父窗口和样式
            ui->PlaylistWid->setParent(this);
            ui->PlaylistWid->setWindowFlags(Qt::Widget);
            ui->PlaylistWid->setStyleSheet("");
            m_playlistHideTimer = nullptr;
            // 重新将播放列表添加到分割器
            if (m_splitter && m_splitter->indexOf(ui->PlaylistWid) == -1) {
                m_splitter->addWidget(ui->PlaylistWid);
            }
            // 禁用点击视频隐藏功能
            m_bClickVideoToHidePlaylist = false;
        }
        m_stCtrlbarAnimationShow->stop();
        m_stCtrlbarAnimationHide->stop();
        ui->CtrlBarWid->setWindowOpacity(1);
        ui->CtrlBarWid->setWindowFlags(Qt::SubWindow);
        ui->ShowWid->setWindowFlags(Qt::SubWindow);
        ui->CtrlBarWid->showNormal();
        m_stTitlebarAnimationShow->stop(); //快速切换时，动画还没结束导致控制面板消失
        m_stTitlebarAnimationHide->stop();
        ui->TitleWid->setWindowOpacity(1);
        ui->TitleWid->setWindowFlags(Qt::SubWindow);
        ui->TitleWid->showNormal();
        ui->ShowWid->showNormal();
        // 恢复主窗口
        // 修改这里：恢复窗口模式下的样式
                ui->PlaylistWid->setStyleSheet("");

//                // 重新应用窗口模式下的样式表
//                QString qss = GlobalHelper::GetQssStr("://res/qss/mainwid.css");
//                this->setStyleSheet(qss);
                m_stPlaylist.setRowHeight(24); // 窗口模式的行高
                QString windowPlaylistStyle = GlobalHelper::GetQssStr("://res/qss/playlist.css");
                    m_stPlaylist.setStyleSheet(windowPlaylistStyle);
                    // 获取保存和删除按钮并恢复正常的图标尺寸
                    QPushButton *labelIcon = m_stPlaylist.findChild<QPushButton*>("labelIcon");
                    if (labelIcon) {
                            // 全屏时设置更大的图标（16px）
                            GlobalHelper::SetIcon(labelIcon, 11, QChar(0xf0ca)); // 列表图标
                            // 调整按钮大小
                            labelIcon->setFixedSize(16, 20);
                            labelIcon->setIconSize(QSize(12, 14));
                            QString btnStyleSheet = "QPushButton { "
                                                    "border: none; "
                                                    "padding: 1px 0px 0px -2px; "//内边距
                                                    "margin: 0px -12px 0px -12px;"  //外边距上右下左
                                                    "background-color: transparent; "
                                                    "}"
                                                    "QPushButton:hover { "
                                                    "color : Cyan;"
                                                    "background-color: #444444; "  // 浅灰色背景
                                                    "border-radius: 2px; "
                                                    "}"
                                                    "QPushButton:pressed { "
                                                    "background-color: #555555; "  // 按下时稍深
                                                    "border-radius: 2px; "
                                                    "}";
                            labelIcon->setStyleSheet(btnStyleSheet);
                    }
                            QPushButton* btnSave = m_stPlaylist.findChild<QPushButton*>("btnSaveToNewlist");
                            QPushButton* btnDelete = m_stPlaylist.findChild<QPushButton*>("btnDeleteCurrentList");

                            if (btnSave) {
                                // 窗口模式时恢复正常图标尺寸（10px）
                                GlobalHelper::SetIcon(btnSave, 12, QChar(0xf2e5)); // 软盘图标
                                // 恢复按钮大小
                                btnSave->setFixedSize(22, 22);
                                btnSave->setIconSize(QSize(16, 16));
                                QString btnStyleSheet = "QPushButton { "
                                                        "border: none; "
                                                        "padding: 0px 2px 1px 2px; "//内边距
                                                        "margin: 0px 0px;"  //外边距
                                                        "background-color: transparent; "
                                                        "}"
                                                        "QPushButton:hover { "
                                                        "color : Cyan;"
                                                        "background-color: #444444; "  // 浅灰色背景
                                                        "border-radius: 2px; "
                                                        "}"
                                                        "QPushButton:pressed { "
                                                        "background-color: #555555; "  // 按下时稍深
                                                        "border-radius: 2px; "
                                                        "}";
                                btnSave->setStyleSheet(btnStyleSheet);
                            }

                            if (btnDelete) {
                                // 窗口模式时恢复正常图标尺寸（10px）
                                GlobalHelper::SetIcon(btnDelete, 12, QChar(0xf014)); // 垃圾桶图标
                                // 恢复按钮大小
                                btnDelete->setFixedSize(20, 20);
                                btnDelete->setIconSize(QSize(16, 16));
                            }
        this->show();
        // 恢复窗口大小和位置
        if (!m_preFullScreenSize.isEmpty()) {
            // 如果之前有播放列表，恢复播放列表显示
            if (playlistWidth > 10 && m_bPlaylistVisible) {
                ui->PlaylistWid->show();
                if (m_splitter && !m_preFullScreenSplitterState.isEmpty()) {
                        m_splitter->restoreState(m_preFullScreenSplitterState);
                    }
            }
            this->resize(m_preFullScreenSize);
            this->move(m_preFullScreenPos);
        }
        // 恢复窗口置顶状态
        if (m_bAlwaysOnTop) {
            this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
            this->show();
        }
        m_stFullscreenMouseDetectTimer.stop();
        // 显示边框
        if (m_borderFrame)
        {
            m_borderFrame->setFrameShape(QFrame::Box);
            m_borderFrame->setStyleSheet("QFrame { border: 1px solid #333333; }");
        }
        GlobalHelper::SetIcon(ui->CtrlBarWid->findChild<QPushButton*>("FullScreenBtn"), 12, QChar(0xf22e));
        // 移除视频窗口的事件过滤器
        ui->ShowWid->removeEventFilter(this);
        GlobalVars::getFullScreen()=false;
        GlobalVars::getWinState() = 0;
        m_bFullScreenPlay = false;
        m_bIsFullScreen = false;
    }
    if (ui->ShowWid) {
        ui->ShowWid->updateSubtitleWindowPosition();
    }
    // 恢复播放列表的选中状态
    if (m_bPlaylistVisible && GlobalVars::currentPlayIndex() >= 0) {
        // 延迟一小段时间，确保播放列表已经正确显示
        QTimer::singleShot(100, this, [this]() {
            qDebug() << "恢复选中项索引:" << GlobalVars::currentPlayIndex();
            m_stPlaylist.SetSelectedIndex(GlobalVars::currentPlayIndex());
        });
    }
    qDebug() << "恢复到窗口：" <<  GlobalHelper::subtitleWindow()->isVisible() << "定时器状态：" << GlobalHelper::subtitleWindow()->m_updateTimer.isActive() ;
}

void MainWid::updatePlaylistButtonState()
{
    if (m_bFullScreenPlay && m_bPlaylistVisible) {
        GlobalHelper::SetIcon(ui->CtrlBarWid->findChild<QPushButton*>("PlaylistCtrlBtn"), 12, QChar(0xf03b));
    } else if (m_bFullScreenPlay && !m_bPlaylistVisible) {
        GlobalHelper::SetIcon(ui->CtrlBarWid->findChild<QPushButton*>("PlaylistCtrlBtn"), 12, QChar(0xf03c));
    }
}
void MainWid::OnCtrlBarAnimationTimeOut()
{
    QApplication::setOverrideCursor(Qt::BlankCursor);
}


void MainWid::OnCtrlBarHideTimeOut()
{
    if (m_bFullScreenPlay)
    {
        m_stCtrlbarAnimationHide->start();
        m_stTitlebarAnimationHide->start();
    }
    //setCursor(Qt::BlankCursor);

}

void MainWid::OnShowMenu()
{
    CreateDefaultMenu();
    // 在菜单显示前更新"实际大小"菜单项状态
    QList<QAction*> actions = m_stMenu.actions();
    for (QAction* action : actions) {
        if (action->text() == "实际大小") {
            action->setEnabled(isVideoPlaying());
            if (!isVideoPlaying()) {
                action->setToolTip("当前播放的是音频文件或没有播放");
            } else {
                action->setToolTip(QString("调整窗口到视频实际大小 (%1×%2)")
                                  .arg(m_currentVideoWidth)
                                  .arg(m_currentVideoHeight));
            }
            break;
        }
    }
    m_stMenu.exec(cursor().pos());
}

void MainWid::OnShowAbout()
{
    m_stAboutWidget.move(cursor().pos().x() - m_stAboutWidget.width()/2, cursor().pos().y() - m_stAboutWidget.height()/2);
    m_stAboutWidget.show();
}

void MainWid::OpenFile()
{
     QString lastDir = GlobalVars::GetLastOpenDir();
     QString fileType = GlobalVars::getMediaFileType();
    QString strFileName = QFileDialog::getOpenFileName(this, "打开文件", lastDir,fileType);

    if (!strFileName.isEmpty()) {
            QFileInfo fileInfo(strFileName);
            QString selectedDir = fileInfo.absolutePath();
            GlobalVars::SaveLastOpenDir(selectedDir);

            emit SigOpenFile(strFileName);
        }
}

void MainWid::OnShowSettingWid()
{
    // 在显示前手动定位
    QRect parentRect = this->window()->frameGeometry();
    QRect dialogRect = m_stSettingWid.frameGeometry();
    // 确保不超出屏幕
    QRect screenRect = QApplication::primaryScreen()->availableGeometry();

    // 计算对话框位置（例如在父窗口右侧显示）
    int x = parentRect.left()> screenRect.width()/2 ? parentRect.left()-dialogRect.width():parentRect.right();
    int y = parentRect.top() + 5;  // 距离父窗口顶部50像素
    if (y < screenRect.top()) y = screenRect.top();
    if (y + dialogRect.height() > screenRect.bottom()) {
        y = screenRect.bottom() - dialogRect.height();
    }
    if (x < screenRect.left()) x = screenRect.left();
    if (x + dialogRect.width() > screenRect.right()) {
        x = screenRect.right() - dialogRect.width();
    }
    m_stSettingWid.move(x, y);
    m_stSettingWid.show();
}
void MainWid::onShowSubtitle()
{
    m_bShowSubtitle = !m_bShowSubtitle;
    GlobalHelper::isSubtitleVisible() = m_bShowSubtitle;
    qDebug() << "字幕显示状态:" << (m_bShowSubtitle ? "显示" : "隐藏");
    // 保存设置到配置文件
    // GlobalHelper::SaveShowSubtitleSetting(show);
    TransparentSubtitleWindow* subtitleWindow = GlobalHelper::subtitleWindow();
        if (subtitleWindow)
    {
         if (m_bShowSubtitle)
            {if (!subtitleWindow->m_updateTimer.isActive()){
                subtitleWindow->show();
                subtitleWindow->m_updateTimer.start();
                }
            }
         else
         {
           subtitleWindow->hide();
            subtitleWindow->m_updateTimer.stop();
            subtitleWindow->setSubtitleText("");

        }
    }
}
void MainWid::setExtend()
{
    int extend=GlobalVars::getExtend();
    GlobalVars::getExtend()=extend==0?1:0;
    ui->ShowWid->ChangeShow();
}
void MainWid::InitMenu()
{
//    QString menu_json_file_name = ":/res/menu.json";
//    QByteArray ba_json;
//    QFile json_file(menu_json_file_name);
//    if (json_file.open(QIODevice::ReadOnly))
//    {
//        ba_json = json_file.readAll();
//        json_file.close();
//    }

//    QJsonDocument json_doc = QJsonDocument::fromJson(ba_json);

//    if (json_doc.isObject())
//    {
//        //QJsonObject json_obj = json_doc.object();
//        //MenuJsonParser(json_obj, &m_stMenu);
//    }
    //CreateDefaultMenu();
}

// 辅助函数：创建带颜色的图标
QIcon MainWid::createMenuIcon(const QString& svgPath, const QColor& normalColor, const QColor& hoverColor)
{
    QIcon icon;

    // 正常状态图标
    QPixmap normalPixmap = createColoredSvg(svgPath, normalColor, QSize(16, 16));

    // 悬停状态图标
    QPixmap hoverPixmap = createColoredSvg(svgPath, hoverColor, QSize(16, 16));

    // 设置图标状态
    icon.addPixmap(normalPixmap, QIcon::Normal);
    icon.addPixmap(hoverPixmap, QIcon::Active);
    icon.addPixmap(hoverPixmap, QIcon::Selected);

    return icon;
}

QPixmap MainWid::createColoredSvg(const QString& svgPath, const QColor& color, QSize size)
{
    // 读取 SVG 文件
    QFile file(svgPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }

    QString svgContent = QString::fromUtf8(file.readAll());
    file.close();

    // 替换颜色（假设 SVG 中有 fill="#666666" 或类似）
    // 将任何 fill 属性替换为指定颜色
    QString colorHex = color.name();
    svgContent.replace(QRegularExpression("fill\\s*=\\s*\"[^\"]*\""),
                       QString("fill=\"%1\"").arg(colorHex));

    // 渲染 SVG
    QSvgRenderer renderer(svgContent.toUtf8());
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    renderer.render(&painter);

    return pixmap;
}
void MainWid::SetMenuStyle(QMenu* menu)
{
    // 将颜色转换为字符串
        QString normalColorStr = m_menuNormalColor.name();
        QString hoverColorStr = m_menuHoverColor.name();
    // 设置菜单的样式表
    menu->setStyleSheet(
        // 菜单整体样式
        "QMenu {"
        "   background-color: #2b2b2b;"           // 背景色
        "   border: 1px solid #3c3c3c;"          // 边框
        "   border-radius: 4px;"                 // 圆角
        "   padding: 4px 0px;"                   // 上下内边距
        "}"

        // 菜单项样式
        "QMenu::item {"
        "   background-color: transparent;"      // 透明背景
        "   color: " + normalColorStr + ";"     // 使用变量：文字颜色
        "   padding: 4px 16px 4px 32px;"         // 内边距上右下左（左32px为图标留空间）
        "   margin: 2px 4px;"                    // 外边距：上下、左右
        "   border-radius: 3px;"                 // 圆角
        "   min-height: 20px;"                   // 最小高度
        "   min-width: 120px;"                   // 最小宽度
        "}"
                // 菜单项选中且悬停样式
                "QMenu::item:checked:selected {"
                "   background-color: #404040;"          // checked 状态悬停时的背景色
                "   color: " + hoverColorStr + ";"       // 使用变量：选中且悬停文字颜色
                "}"
        // 菜单项悬停样式
        "QMenu::item:selected {"
        "   background-color: #404040;"          // 选中背景色
        "   color: " + hoverColorStr + ";"      // 使用变量：选中文字颜色
        "}"
        // 菜单项禁用样式
        "QMenu::item:disabled {"
        "   color: #808080;"                     // 禁用颜色
        "}"

        // 菜单项选中但非悬停样式（如带复选框的项）
        "QMenu::item:checked {"
        "   background-color: #2b2b2b;"          // 选中背景色
        "}"
                // =============== 新增：勾选框样式（调整位置和大小） ===============
                // 先重置indicator的位置
                        "QMenu::indicator {"
                        "   /* 移除所有默认样式 */"
                        "   border: none;"
                        "   background: none;"
                        "   width: 16px;"
                        "   height: 16px;"
                        "}"

                        // 未选中状态，勾选标志
                        "QMenu::indicator:unchecked {"
                        "   image: url(:/res/icons/unSelected.svg);"
                        "   /* 使用position和left定位 */"
                        "   position: relative;"
                        "   left: 15px;"  // 距离菜单左边15px
                        "}"

                        // 选中状态，勾选标志
                        "QMenu::indicator:checked {"
                        "   image: url(:/res/icons/selected.svg);"
                        "   /* 使用position和left定位 */"
                        "   position: relative;"
                        "   left: 15px;"  // 距离菜单左边15px
                        "}"

                        // 勾选框未选中+悬停样式
                        "QMenu::indicator:unchecked:hover {"
                        "   image: url(:/res/icons/unCheckOn.svg);"
                        "}"

                        // 勾选框选中+悬停样式
                        "QMenu::indicator:checked:hover {"
                        "   image: url(:/res/icons/selected.svg);"
                        "}"
        // 分隔线样式
        "QMenu::separator {"
        "   height: 1px;"                        // 分隔线高度
        "   background-color: #3c3c3c;"          // 分隔线颜色
        "   margin: 4px 8px;"                    // 分隔线外边距
        "}"

        // 图标样式
        "QMenu::icon {"
        "   margin-left: 18px;"                   // 图标左边距
        "   width: 16px;"                        // 图标宽度
        "   height: 16px;"                       // 图标高度
        "}"

        // 子菜单指示器（箭头）
        "QMenu::indicator {"
        "   width: 10px;"                        // 指示器宽度
        "   height: 10px;"                       // 指示器高度
        "}"

        // 子菜单样式
        "QMenu::menu-arrow {"
        "   image: url(:/res/menu_arrow.svg);"   // 自定义箭头图标（可选）
        "   width: 8px;"
        "   height: 8px;"
        "}"
    );

    // 设置字体（可选）
    QFont menuFont("Microsoft YaHei", 9);
    menu->setFont(menuFont);
}
void MainWid::CreateDefaultMenu()
{
    m_stMenu.clear();
    m_menuNormalColor=QColor("#aaaaaa");  // 初始化正常颜色
    m_menuHoverColor=QColor("#eeeeee");   // 初始化悬停颜色
    SetMenuStyle(&m_stMenu);
    // 定义所有颜色变量
    // 加载 FontAwesome 字体
    QFont fontAwesome;
    int fontId = QFontDatabase::addApplicationFont(":/res/fontawesome-webfont.ttf");
    if (fontId >= 0) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            fontAwesome.setFamily(fontFamilies.at(0));
            fontAwesome.setPixelSize(12);
        }
    }
    // ============ 可切换状态的菜单项 ============
    QIcon openIcon = createMenuIcon(":/res/icons/file-open.svg",
                                         m_menuNormalColor,  // 正常颜色
                                         m_menuHoverColor); // 悬停颜色
    QAction* openAction = m_stMenu.addAction(openIcon,"打开文件...");
    connect(openAction, &QAction::triggered, this, &MainWid::OpenFile);
    m_stMenu.addAction(openAction);
    m_stMenu.addSeparator();

    // ============ 可切换状态的菜单项 ============
    m_stActFullscreen.setText("全屏");
    m_stActFullscreen.setCheckable(true);

    //m_bFullScreenPlay=GlobalVars::getFullScreen();
    m_stActFullscreen.setChecked(m_bFullScreenPlay);  // 同步当前全屏状态
    m_stActFullscreen.setShortcut(QKeySequence(Qt::Key_Return));
    //connect(&m_stActFullscreen, &QAction::triggered, this, &MainWid::OnFullScreenPlay);
    m_stMenu.addAction(&m_stActFullscreen);
    // 拉伸动作
        m_stActExtend.setText("拉伸");
    int extend = GlobalVars::getExtend();
        m_stActExtend.setCheckable(true);
        m_stActExtend.setChecked(extend == 1);  // 同步当前拉伸状态
        m_stActExtend.setShortcut(QKeySequence(Qt::Key_Enter));
        //connect(&m_stActExtend, &QAction::triggered, this, &MainWid::setExtend);
        m_stMenu.addAction(&m_stActExtend);
    // 列表
    m_stActPlaylist.setText("播放列表");
    m_stActPlaylist.setCheckable(true);
    m_stActPlaylist.setShortcut(QKeySequence(Qt::Key_F12));
    //connect(&m_stActPlaylist, &QAction::triggered, this, &MainWid::OnShowOrHidePlaylist);
    m_stMenu.addAction(&m_stActPlaylist);
    m_stActShowSubtitle.setText("显示字幕");
        m_stActShowSubtitle.setCheckable(true);
        m_stActShowSubtitle.setChecked(m_bShowSubtitle); // 默认勾选（显示）
        m_stMenu.addAction(&m_stActShowSubtitle);
        // 置顶动作 - 新增
            m_stActAlwaysOnTop.setText("窗口置顶");
            m_stActAlwaysOnTop.setCheckable(true);
            m_stActAlwaysOnTop.setChecked(m_bAlwaysOnTop);
            m_stActAlwaysOnTop.setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T)); // Ctrl+T 快捷键
           // connect(&m_stActAlwaysOnTop, &QAction::triggered, this, &MainWid::OnAlwaysOnTopBtnClicked);
            m_stMenu.addAction(&m_stActAlwaysOnTop);


            m_stMenu.addSeparator();
        // 实际大小动作 - 新增
            QIcon actualSizeIcon = createMenuIcon(":/res/icons/actual_size.svg",
                                                  m_menuNormalColor,
                                                  m_menuHoverColor);
            QAction* actualSizeAction = m_stMenu.addAction(actualSizeIcon, "实际大小");
            actualSizeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1)); // Ctrl+1 快捷键
            connect(actualSizeAction, &QAction::triggered, this, &MainWid::onActualSize);
            // 初始状态禁用（没有视频播放时）
            actualSizeAction->setEnabled(isVideoPlaying());
            m_stMenu.addAction(actualSizeAction);
        // 均衡器设置
            QIcon eqIcon = createMenuIcon(":/res/icons/junhengqi.svg",
                                                 m_menuNormalColor,  // 正常颜色
                                                 m_menuHoverColor); // 悬停颜色
            QAction* eqAction = m_stMenu.addAction(eqIcon,"均衡器...");
            connect(eqAction, &QAction::triggered, this, &MainWid::onEqualizerSettings);
            m_stMenu.addAction(eqAction);
                m_stMenu.addSeparator();
            // 播放控制子菜单
            openIcon = createMenuIcon(":/res/icons/playCtrl.svg",
                                                 m_menuNormalColor,  // 正常颜色
                                                 m_menuHoverColor); // 悬停颜色
                QMenu *playControlMenu = m_stMenu.addMenu(openIcon,"播放控制");
                // 播放/暂停
                openIcon = createMenuIcon(":/res/icons/playPause.svg",
                                                     m_menuNormalColor,  // 正常颜色
                                                     m_menuHoverColor); // 悬停颜色
                QAction *playPauseAction = playControlMenu->addAction(openIcon,"播放/暂停");
                playPauseAction->setShortcut(QKeySequence(Qt::Key_Space));
                connect(playPauseAction, &QAction::triggered, this, &MainWid::SigPlayOrPause);
                // 上一曲
                openIcon = createMenuIcon(":/res/icons/shangyiqu.svg",
                                                     m_menuNormalColor,  // 正常颜色
                                                     m_menuHoverColor); // 悬停颜色
                QAction *prevAction = playControlMenu->addAction(openIcon,"上一曲");
                connect(prevAction, &QAction::triggered, ui->CtrlBarWid, &CtrlBar::SigBackwardPlay);
                // 下一曲
                openIcon = createMenuIcon(":/res/icons/xiayiqu.svg",
                                                     m_menuNormalColor,  // 正常颜色
                                                     m_menuHoverColor); // 悬停颜色
                QAction *nextAction = playControlMenu->addAction(openIcon,"下一曲");
                connect(nextAction, &QAction::triggered, ui->CtrlBarWid, &CtrlBar::SigForwardPlay);


                // ============ 声音控制子菜单 ============
                // 声音控制子菜单
                openIcon = createMenuIcon(":/res/icons/soundCtrl.svg",
                                                     m_menuNormalColor,  // 正常颜色
                                                     m_menuHoverColor); // 悬停颜色
                    QMenu *soundControlMenu = m_stMenu.addMenu(openIcon,"声音控制");
                    // 音量+
                    openIcon = createMenuIcon(":/res/icons/volumeMiddle.svg",
                                                         m_menuNormalColor,  // 正常颜色
                                                         m_menuHoverColor); // 悬停颜色
                    QAction *volumeUpAction = soundControlMenu->addAction(openIcon,"音量 +");
                    volumeUpAction->setShortcut(QKeySequence(Qt::Key_Up));
                    connect(volumeUpAction, &QAction::triggered, this, &MainWid::SigAddVolume);
                    // 音量-
                    openIcon = createMenuIcon(":/res/icons/volumeLow.svg",
                                                         m_menuNormalColor,  // 正常颜色
                                                         m_menuHoverColor); // 悬停颜色
                    QAction *volumeDownAction = soundControlMenu->addAction(openIcon,"音量 -");
                    volumeDownAction->setShortcut(QKeySequence(Qt::Key_Down));
                    connect(volumeDownAction, &QAction::triggered, this, &MainWid::SigSubVolume);

                    // 静音
                    openIcon = createMenuIcon(":/res/icons/muteSound.svg",
                                                         m_menuNormalColor,  // 正常颜色
                                                         m_menuHoverColor); // 悬停颜色
                    QAction* muteAction = soundControlMenu->addAction(openIcon,"静音");
                    connect(muteAction, &QAction::triggered, this, [this]() {
                        // 调用CtrlBar的静音切换方法
                            if (ui->CtrlBarWid) {
                                ui->CtrlBarWid->toggleMute();
                            }
                    });
                m_stMenu.addSeparator();

            // 选项
                openIcon = createMenuIcon(":/res/icons/setting.svg",
                                                     m_menuNormalColor,  // 正常颜色
                                                     m_menuHoverColor); // 悬停颜色
            QAction* optionsAction = m_stMenu.addAction(openIcon,"选项");
            connect(optionsAction, &QAction::triggered, this, &MainWid::OnShowSettingWid);
            m_stMenu.addAction(optionsAction);

    // 媒体信息
            openIcon = createMenuIcon(":/res/icons/mediaInfo.svg",
                                                 m_menuNormalColor,  // 正常颜色
                                                 m_menuHoverColor); // 悬停颜色
    QAction* mediaInfoAction = m_stMenu.addAction(openIcon,"媒体信息");
    // 检查当前是否有媒体正在播放或暂停
    bool hasCurrentMedia = false;
    if (GlobalVars::currentPlayIndex() >= 0) {
        // 检查播放器状态
        hasCurrentMedia = (GlobalVars::runState()>0?true :false);
    }
    mediaInfoAction->setEnabled(hasCurrentMedia);
    if (hasCurrentMedia) {
        connect(mediaInfoAction, &QAction::triggered, this, &MainWid::onShowMediaInfo);
    } else {
        mediaInfoAction->setToolTip("请先播放媒体文件");
    }
    m_stMenu.addAction(mediaInfoAction);
    // 文件所在文件夹
    openIcon = createMenuIcon(":/res/icons/folderHeart.svg",
                                         m_menuNormalColor,  // 正常颜色
                                         m_menuHoverColor); // 悬停颜色
    QAction* mediaPathAction = m_stMenu.addAction(openIcon,"文件所在目录");
    // 检查当前是否有媒体正在播放或暂停
    mediaPathAction->setEnabled(hasCurrentMedia);
    if (hasCurrentMedia) {
        connect(mediaPathAction, &QAction::triggered, this, &MainWid::onShowMediaPath);
    } else {
        mediaPathAction->setToolTip("请先播放媒体文件");
    }
    m_stMenu.addAction(mediaPathAction);

    m_stMenu.addSeparator();

    // 关于
    openIcon = createMenuIcon(":/res/icons/about.svg",
                                         m_menuNormalColor,  // 正常颜色
                                         m_menuHoverColor); // 悬停颜色
    QAction* aboutAction = m_stMenu.addAction(openIcon, "关于");
    connect(aboutAction, &QAction::triggered, this, &MainWid::OnShowAbout);
    m_stMenu.addAction(aboutAction);

    // 退出
    openIcon = createMenuIcon(":/res/icons/exit.svg",
                                         m_menuNormalColor,  // 正常颜色
                                         m_menuHoverColor); // 悬停颜色
    QAction* exitAction = m_stMenu.addAction(openIcon,"退出");
    connect(exitAction, &QAction::triggered, this, &MainWid::OnCloseBtnClicked);
    m_stMenu.addAction(exitAction);

}

void MainWid::OnCloseBtnClicked()
{
    if (!m_bFullScreenPlay && !isMinimized() && !isMaximized())
    {
        if(m_bPlaylistVisible) playlistWidth = ui->PlaylistWid->width() < 200 ? 300:ui->PlaylistWid->width();
        QSettings settings("XyPlayer", "PlayerSettings");
                settings.beginGroup("Window");
        settings.setValue("Geometry", this->geometry());
        settings.setValue("PlaylistVisible", m_bPlaylistVisible);
        settings.setValue("PlaylistWidth", playlistWidth);
        settings.endGroup();
        qDebug() << "保存窗口位置：" << this->geometry() << "是否显示列播放表：" << m_bPlaylistVisible << " 列表宽度：" << playlistWidth;
    }
    if (GlobalVars::runState() == 1){
    emit SigFadeVolmeClose(2);
    }
    else this->close();
}

void MainWid::OnMinBtnClicked()
{
    GlobalVars::getWinState() = 2;//最小化
    this->showMinimized();
}
void MainWid::OnAlwaysOnTopBtnClicked()
{
    qDebug() << "置顶操作开始－－－－－－－－－－－－－－－－－－－－111";
    //ShowAllFontAwesomeIcons();
    m_bAlwaysOnTop = !m_bAlwaysOnTop;

        // 更新窗口标志
        Qt::WindowFlags flags = windowFlags();

        if (m_bAlwaysOnTop) {
            // 设置窗口置顶
            flags |= Qt::WindowStaysOnTopHint;
            setWindowFlags(flags);
            show(); // 必须重新显示窗口才能使标志生效

            // 更新按钮图标和工具提示
            GlobalHelper::SetIcon(ui->TitleWid->findChild<QPushButton*>("AlwaysOnTopBtn"), 9, QChar(0xf08d));
            if (ui->TitleWid->findChild<QPushButton*>("AlwaysOnTopBtn")) {
                ui->TitleWid->findChild<QPushButton*>("AlwaysOnTopBtn")->setToolTip("取消置顶");
            }

        } else {
            // 取消窗口置顶
            flags &= ~Qt::WindowStaysOnTopHint;
            setWindowFlags(flags);
            show(); // 必须重新显示窗口才能使标志生效

            // 更新按钮图标和工具提示
            GlobalHelper::SetIcon(ui->TitleWid->findChild<QPushButton*>("AlwaysOnTopBtn"), 9, QChar(0xf2e3));//06c
            if (ui->TitleWid->findChild<QPushButton*>("AlwaysOnTopBtn")) {
                ui->TitleWid->findChild<QPushButton*>("AlwaysOnTopBtn")->setToolTip("窗口置顶");
            }


        }
}
void MainWid::OnMaxBtnClicked()
{
    if (isMaximized())
    {
        if (m_bFullScreenPlay){
            OnFullScreenPlay();
        }else {
            showNormal();
            GlobalVars::getWinState()=0;
            emit SigShowMax(false);
        }
    }
    else
    {
        if (m_bFullScreenPlay) {
                    // 先退出全屏
                    OnFullScreenPlay();
                    // 延迟一小段时间后最大化
                    QTimer::singleShot(50, this, [this]() {
                        GlobalVars::getWinState()=1;
                        showMaximized();
                        emit SigShowMax(true);
                    });
        }else {
            showMaximized();
            GlobalVars::getWinState()=1;
            emit SigShowMax(true);
        }
    }
}
static int playlistWidth = 300;
static int video_width = 592; // 固定视频宽度

void MainWid::OnShowOrHidePlaylist()
{
    // 获取准确的窗口状态
    bool isFullScreen = m_bFullScreenPlay;  // 使用全屏播放标志
    bool isMaximized = this->isMaximized();

    qDebug() << "=== OnShowOrHidePlaylist ===";
    qDebug() << "全屏状态:" << isFullScreen << "最大化状态:" << isMaximized;
    qDebug() << "播放列表当前可见:" << m_bPlaylistVisible;

    // 停止自动隐藏定时器
    if (m_playlistHideTimer && m_playlistHideTimer->isActive()) {
        m_playlistHideTimer->stop();
    }

    if ((isFullScreen && !m_bPlaylistVisible) || ui->PlaylistWid->isHidden() || ui->PlaylistWid->width() < 5)
    {
        // 显示播放列表
        qDebug() << "显示播放列表";

        // 确保播放列表有最小宽度
        playlistWidth = playlistWidth < 10 ? 300 : playlistWidth;

        if (isFullScreen)
        {
            // 全屏状态 - 特殊处理
            showFullscreenPlaylist(true);
        }
        else if (isMaximized)
        {
            // 最大化状态
            showPlaylistMaximized(true);
        }
        else
        {
            // 正常窗口状态
            showPlaylistNormal(true);
        }

        // 更新按钮图标
        updatePlaylistButtonIcon(true);
        m_bPlaylistVisible = true;
    }
    else
    {
        // 隐藏播放列表
        qDebug() << "隐藏播放列表";

        // 保存当前播放列表宽度
        if (!isFullScreen) {
            playlistWidth = ui->PlaylistWid->width();
            video_width = ui->ShowWid->width();
            qDebug() << "保存宽度 - 播放列表:" << playlistWidth << "视频:" << video_width;
        }

        if (isFullScreen)
        {
            // 全屏状态
            showFullscreenPlaylist(false);
        }
        else if (isMaximized)
        {
            // 最大化状态
            showPlaylistMaximized(false);
        }
        else
        {
            // 正常窗口状态
            showPlaylistNormal(false);
        }

        // 更新按钮图标
        updatePlaylistButtonIcon(false);
        m_bPlaylistVisible = false;
    }
    this->update();
}

// 正常窗口状态显示/隐藏播放列表
void MainWid::showPlaylistNormal(bool show)
{

    static int windowBorder = 4; // 窗口边框和边距
    if (show)
    {
             playlistWidth=playlistWidth<200?300:playlistWidth;
             // 显示播放列表
             qDebug() << "=== 显示播放列表 ===" << playlistWidth;
             // 保存当前视频区域宽度（隐藏时的状态）
             int currentVideoWidth = ui->ShowWid->width();
             int currentWindowWidth = this->width();
             qDebug() << "当前状态 - 窗口:" << currentWindowWidth
                      << "视频宽度:" << currentVideoWidth;
                video_width=currentVideoWidth;
                // 显示播放列表
                //ui->PlaylistWid->show();
                if (m_splitter) {
                    m_splitter->widget(1)->show();
                    // 获取手柄宽度
                    int handleWidth = m_splitter->handleWidth();

                    // 计算新窗口宽度
                    // 目标：视频区域 = 591，播放列表 = playlistWidth
                    int newWindowWidth = video_width + playlistWidth + handleWidth + windowBorder;

                    qDebug() << "计算新窗口宽度:" << newWindowWidth
                             << " = 视频" << video_width
                             << " + 播放列表" << playlistWidth
                             << " + 手柄" << handleWidth
                             << " + 边框" << windowBorder;

                    // 方法1：先调整窗口大小，再设置分割器
                   // this->resize(newWindowWidth, this->height());
                    this->setGeometry(this->x(),this->y(), newWindowWidth, this->height());

                    // 强制分割器使用固定大小
                    QList<int> sizes;
                    sizes << video_width << playlistWidth;
                    m_splitter->setSizes(sizes);

                }


                GlobalHelper::SetIcon(ui->CtrlBarWid->findChild<QPushButton*>("PlaylistCtrlBtn"), 12, QChar(0xf03b));
                m_bPlaylistVisible = true;
    }
    else
    {
        // 隐藏播放列表
        // 保存当前宽度
                int savedPlaylistWidth = ui->PlaylistWid->width();
                int savedVideoWidth = ui->ShowWid->width();
                playlistWidth = savedPlaylistWidth;

                qDebug() << "隐藏前 - 播放列表宽度:" << savedPlaylistWidth
                         << "视频宽度:" << savedVideoWidth
                         << "当前窗口宽度:" << this->width()
                         << "窗口最小宽度:" << this->minimumWidth();

                // 临时禁用窗口更新
                //this->setUpdatesEnabled(false);
                // 临时将最小宽度设置为0，允许窗口缩小
               // int oldMinWidth = this->minimumWidth();
                // 3. 计算新窗口宽度
                int newWindowWidth = savedVideoWidth + windowBorder;
               this->setGeometry(this->x(),this->y(), newWindowWidth, this->height());
                // 方法1：逐步调整
                // 1. 先调整分割器
                if (m_splitter) {
                    // 先设置分割器尺寸
                    QList<int> sizes;
                    sizes << savedVideoWidth << 0;
                    m_splitter->setSizes(sizes);
                    m_splitter->widget(1)->hide();
                }

                m_bPlaylistVisible = false;
    }
    mainwin_width = this->width();
    qDebug() << "m_bPlaylistVisible === " << m_bPlaylistVisible << "this->minimumWidth=" << this->minimumWidth() ;
}

// 最大化窗口状态显示/隐藏播放列表
void MainWid::showPlaylistMaximized(bool show)
{
    if (!m_splitter) return;

    this->setUpdatesEnabled(false);

    if (show)
    {
        // 显示播放列表 - 视频缩小
        ui->PlaylistWid->show();
        m_splitter->widget(1)->show();

        // 计算可用宽度（考虑控制面板）
        int totalWidth = this->width();
        // 设置视频和播放列表的比例（例如 7:3）
        int videoWidth = totalWidth * 70 / 100;
        int playlistW = totalWidth * 30 / 100;

        // 确保控制面板与视频窗口同步收缩
        if (ui->CtrlBarWid) {
            ui->CtrlBarWid->setFixedWidth(videoWidth);
            // 将控制面板置于视频窗口下层
            ui->CtrlBarWid->lower();
        }

        QList<int> sizes;
        sizes << videoWidth << playlistW;
        m_splitter->setSizes(sizes);

        qDebug() << "最大化显示 - 视频:" << videoWidth << "播放列表:" << playlistW;
    }
    else
    {
        // 隐藏播放列表 - 视频恢复全宽
        ui->PlaylistWid->hide();
        m_splitter->widget(1)->hide();

        int totalWidth = this->width();

        // 恢复控制面板宽度
        if (ui->CtrlBarWid) {
            ui->CtrlBarWid->setFixedWidth(totalWidth);
        }

        QList<int> sizes;
        sizes << totalWidth << 0;
        m_splitter->setSizes(sizes);
    }

    this->setUpdatesEnabled(true);
    this->update();
}

void MainWid::showFullscreenPlaylist(bool show)
{
    qDebug() << "showFullscreenPlaylist: " << show;

    if (!m_bFullScreenPlay) {
        qDebug() << "错误：当前不在全屏状态";
        return;
    }
    if (show)
    {
        if (m_bPlaylistVisible) {
                    qDebug() << "播放列表已经显示，跳过";
                    return;
                }
            ui->PlaylistWid->raise();
            m_stPlaylistAnimationShow->start();
            m_stPlaylistAnimationHide->stop();
            m_bPlaylistVisible = true;
            // 恢复播放列表的选中状态
            if (GlobalVars::currentPlayIndex() >= 0) {
                // 延迟一小段时间，确保播放列表已经正确显示
                QTimer::singleShot(100, this, [this]() {
                    qDebug() << "恢复选中项索引:" << GlobalVars::currentPlayIndex();
                    m_stPlaylist.SetSelectedIndex(GlobalVars::currentPlayIndex());
                });
            }
            qDebug() << "播放列表:" << ui->PlaylistWid->geometry();

    }
    else
    {
        if (!m_bPlaylistVisible) {
                    qDebug() << "播放列表已经隐藏，跳过";
                    return;
                }
            m_stPlaylistAnimationShow->stop();
            m_stPlaylistAnimationHide->start();
            m_bPlaylistVisible = false;
            qDebug() << "播放列表:" << ui->PlaylistWid->geometry();


    }

}

// 更新播放列表按钮图标
void MainWid::updatePlaylistButtonIcon(bool visible)
{
    if (visible) {
        GlobalHelper::SetIcon(ui->CtrlBarWid->findChild<QPushButton*>("PlaylistCtrlBtn"), 12, QChar(0xf03b));
    } else {
        GlobalHelper::SetIcon(ui->CtrlBarWid->findChild<QPushButton*>("PlaylistCtrlBtn"), 12, QChar(0xf03c));
    }
}

void MainWid::OnAutoPlayNext()
{
    // 获取当前播放模式
    int mode = GlobalVars::playMode();
    int currentIndex = GlobalVars::currentPlayIndex();
    if (GlobalVars::isRenameing())return;
    // 如果不是顺序播放且已到最后一首，不自动播放
    if (mode == PLAY_MODE_SEQUENCE &&
        currentIndex == GlobalVars::playlistCount() - 1) {
        qDebug() << "顺序播放已到最后一首，停止播放";
        GlobalVars::runState() = 0;
        return;
    }

    // 获取下一个索引
    int nextIndex = m_stPlaylist.GetNextIndex(currentIndex);
    if (nextIndex >= 0 && nextIndex < GlobalVars::playlistCount()) {
        qDebug() << "自动播放下一个，索引：" << nextIndex;
        // 延迟一小段时间后自动播放，避免与停止操作冲突
        QTimer::singleShot(100, this, [this, nextIndex]() {
            m_stPlaylist.PlayByIndex(nextIndex);
        });
    }
}
// 添加均衡器设置槽函数
void MainWid::onEqualizerSettings()
{
    if (!m_equalizerDialog) {
        m_equalizerDialog = new EqualizerDialog(this);
        connect(m_equalizerDialog, &EqualizerDialog::equalizerChanged,
                this, &MainWid::onEqualizerChanged);
        connect(m_equalizerDialog, &EqualizerDialog::finished,
                this, [this]() {
                    // 保存均衡器设置到配置文件
                    saveEqualizerSettings();
                });

        // 加载保存的设置
        loadEqualizerSettings();
    }

    m_equalizerDialog->show();
    m_equalizerDialog->raise();
    m_equalizerDialog->activateWindow();
}

void MainWid::onEqualizerChanged(const QList<int>& gains, int balance, bool enabled)
{
    m_eqGains = gains;
    m_balance = balance;
    m_eqEnabled = enabled;

    // 通知VideoCtl应用新的均衡器设置
    applyEqualizerToAudio();
}

void MainWid::applyEqualizerToAudio()
{
    // 这里需要将均衡器设置应用到音频处理
    // 由于VideoCtl是单例，我们可以直接调用

    // 注意：需要先在VideoCtl中添加相应的接口
    // VideoCtl::GetInstance()->setEqualizer(m_eqGains, m_balance, m_eqEnabled);
}

void MainWid::saveEqualizerSettings()
{
    QSettings settings("XyPlayer", "Equalizer");
    settings.setValue("Enabled", m_eqEnabled);
    settings.setValue("Balance", m_balance);

    // 保存10段均衡器增益
    for (int i = 0; i < m_eqGains.size(); i++) {
        settings.setValue(QString("Gain%1").arg(i), m_eqGains[i]);
    }
}

void MainWid::loadEqualizerSettings()
{
    QSettings settings("XyPlayer", "Equalizer");
    m_eqEnabled = settings.value("Enabled", false).toBool();
    m_balance = settings.value("Balance", 0).toInt();

    // 加载10段均衡器增益
    for (int i = 0; i < 10; i++) {
        m_eqGains[i] = settings.value(QString("Gain%1").arg(i), 0).toInt();
    }

    if (m_equalizerDialog) {
        m_equalizerDialog->setEqualizerEnabled(m_eqEnabled);
        m_equalizerDialog->setBalance(m_balance);
        m_equalizerDialog->setEqualizerGains(m_eqGains);
    }
}

void MainWid::onShowMediaInfo()
{
    int currentIndex = GlobalVars::currentPlayIndex();
    if (currentIndex >= 0 && currentIndex < GlobalVars::playlistCount()) {
        // 调用 Playlist 的 ShowFileInfoByIndex 函数
        m_stPlaylist.ShowFileInfoByIndex(currentIndex);
    } else {
        // 如果没有当前播放的文件，可以显示一个提示
        qDebug() << "提示", "请先播放一个媒体文件";
    }
}
void MainWid::onShowMediaPath()
{
    int currentIndex = GlobalVars::currentPlayIndex();
    if (currentIndex >= 0 && currentIndex < GlobalVars::playlistCount()) {
        // 调用 Playlist 的 ShowFileInfoByIndex 函数
        m_stPlaylist.ShowFilePathByIndex(currentIndex);
    } else {
        // 如果没有当前播放的文件，可以显示一个提示
        qDebug() << "提示", "请先播放一个媒体文件";
    }
}
//切换标题栏与控制栏的显示与隐藏
void MainWid::changeCtrlTileShow(){
    m_lastMouseActivityTime = QDateTime::currentMSecsSinceEpoch();
    // 如果鼠标指针已隐藏，则显示
    if (m_bMouseCursorHidden) {
        ShowMouseCursor();
    }
    // 如果控制栏已隐藏，则显示
    if (m_bFullScreenPlay) {
            if (ui->TitleWid->y() <0){
                ui->CtrlBarWid->raise();
                ui->TitleWid->raise();
                m_stCtrlbarAnimationShow->start();
                m_stTitlebarAnimationShow->start();
                m_stCtrlbarAnimationHide->stop();
                m_stTitlebarAnimationHide->stop();
                m_bFullscreenCtrlBarShow = true;
            }else {
                m_stCtrlbarAnimationHide->start();
                m_stTitlebarAnimationHide->start();
                m_bFullscreenCtrlBarShow = false;
            }
            qDebug()<< "标题栏位置:" << ui->TitleWid->geometry();
      }
}
// 更新鼠标活动时间
void MainWid::UpdateMouseActivity()
{
    //if(GlobalHelper::subtitleWindow()->m_bMouseHover)return;
    m_lastMouseActivityTime = QDateTime::currentMSecsSinceEpoch();
    // 如果鼠标指针已隐藏，则显示
    if (m_bMouseCursorHidden) {
        ShowMouseCursor();
    }
    // 如果控制栏已隐藏，则显示
    if (m_bFullScreenPlay && (!m_bFullscreenCtrlBarShow || ui->TitleWid->y() <0)) {
        ShowFullscreenControls();
    }
}

// 检查鼠标活动状态
void MainWid::CheckMouseActivity()
{
    if (!m_bFullScreenPlay) return;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 inactiveTime = currentTime - m_lastMouseActivityTime;
    qDebug() << "无操作4秒开始隐藏标题栏和控制栏…………现在空闲时间：……" << inactiveTime;
    // 3秒无活动：隐藏控制栏和标题栏
    if (inactiveTime > 4000 && m_bFullscreenCtrlBarShow) {
        qDebug() << "无操作3秒开始隐藏标题栏和控制栏………………" ;
        HideFullscreenControls();
    }

    // 5秒无活动：隐藏鼠标指针
    if (inactiveTime > 6000 && !m_bMouseCursorHidden) {
        qDebug() << "无操作5秒开始隐藏鼠标指针………………" ;
        HideMouseCursor();
    }
}

// 隐藏鼠标指针
void MainWid::HideMouseCursor()
{
    if (m_bFullScreenPlay && !m_bMouseCursorHidden) {
        // 保存当前光标
        // 设置空白光标
        QApplication::setOverrideCursor(Qt::BlankCursor);
        m_bMouseCursorHidden = true;
        qDebug() << "隐藏鼠标指针";
    }
}

// 显示鼠标指针
void MainWid::ShowMouseCursor()
{
    if (m_bMouseCursorHidden) {
        // 恢复光标
        QApplication::restoreOverrideCursor();
        m_bMouseCursorHidden = false;
        qDebug() << "显示鼠标指针";
    }
}
// 显示全屏控制栏
void MainWid::ShowFullscreenControls()
{
    if (!m_bFullscreenCtrlBarShow) {
        ui->CtrlBarWid->raise();
        ui->TitleWid->raise();
        if (ui->TitleWid->y() <0){
            m_stCtrlbarAnimationShow->start();
            m_stTitlebarAnimationShow->start();
            m_stCtrlbarAnimationHide->stop();
            m_stTitlebarAnimationHide->stop();
        }
        m_bFullscreenCtrlBarShow = true;
        qDebug()
                     << "标题栏位置:" << ui->TitleWid->geometry();
    }
}

// 隐藏全屏控制栏
void MainWid::HideFullscreenControls()
{
    if (m_bFullscreenCtrlBarShow) {
        m_stCtrlbarAnimationHide->start();
        m_stTitlebarAnimationHide->start();
        if (ui->PlaylistWid->x()<m_stPlayListAnimationHide.x())m_stPlaylistAnimationHide->start();
        m_bFullscreenCtrlBarShow = false;
        qDebug() << "隐藏全屏控制栏";
    }
}
void MainWid::changeEvent(QEvent *event)
{
    if (!m_bFullScreenPlay){
        if (event->type() == QEvent::ActivationChange){
             GlobalVars::getWinState()= 0;
        }else if (event->type() == QEvent::WindowStateChange){
            GlobalVars::getWinState()= 2;
            qDebug() << "窗口最小化了！" << GlobalVars::getWinState();
        }
    }
    if (m_bFullScreenPlay && GlobalVars::getWinState() >=2 ){
     if (event->type() == QEvent::ActivationChange){
qDebug() << "全屏最小化恢复全屏,定时器" <<  GlobalHelper::subtitleWindow()->m_updateTimer.isActive() << "字幕窗口：" << GlobalHelper::subtitleWindow()->isVisible();
          ui->PlaylistWid->setGeometry(m_stPlayListAnimationHide);

     }else if (event->type() == QEvent::WindowStateChange){
         qDebug() << "全屏到最小化，定时器" <<  GlobalHelper::subtitleWindow()->m_updateTimer.isActive() << "字幕窗口：" << GlobalHelper::subtitleWindow()->isVisible();
         GlobalVars::getWinState()= 3;
     }
    }
}
// 新的定时器回调函数
void MainWid::OnMouseActivityCheckTimeOut()
{
    if (!m_bFullScreenPlay) return;
    int moveDistance = (m_lastMousePos - cursor().pos()).manhattanLength();
    if (moveDistance > 3){
        m_lastMousePos = cursor().pos();
    }
    if (moveDistance > 3 || m_stTitleBarAnimationShow.contains(cursor().pos()) || m_stPlayListAnimationShow.contains(cursor().pos()))
           {
            UpdateMouseActivity();
            if (m_stPlayListAnimationHide.contains(cursor().pos()))
            {
                showFullscreenPlaylist(true);
            }
            //判断鼠标是否在控制面板上面
            if (moveDistance > 3 || ui->TitleWid->geometry().contains(cursor().pos()) || m_bPlaylistVisible)
            {
                //继续显示
                m_bFullscreenCtrlBarShow = true;
            }
            else
            {   qDebug() << "显示中－－－－－－－－－－－－－－－1-m_lastMousePos:" << m_lastMousePos << "cursor().pos():" << cursor().pos();
                //需要显示
                if (ui->TitleWid->y()<0){
                ui->CtrlBarWid->raise();
                ui->TitleWid->raise();
                m_stCtrlbarAnimationShow->start();
                m_stTitlebarAnimationShow->start();
                m_stCtrlbarAnimationHide->stop();
                m_stTitlebarAnimationHide->stop();
                stCtrlBarHideTimer.stop();
                }
            }
    }else  if (m_stCtrlBarAnimationShow.contains(cursor().pos()))
    {
        UpdateMouseActivity();
        //判断鼠标是否在控制面板上面
        if (ui->CtrlBarWid->geometry().contains(cursor().pos()))
        {
            //继续显示
            m_bFullscreenCtrlBarShow = true;
        }
        else
        {
            qDebug() << "显示中－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－2";
            if (ui->TitleWid->y()<0){
                ui->CtrlBarWid->raise();
                ui->TitleWid->raise();
                m_stCtrlbarAnimationShow->start();
                m_stTitlebarAnimationShow->start();
                m_stCtrlbarAnimationHide->stop();
                m_stTitlebarAnimationHide->stop();
                stCtrlBarHideTimer.stop();
            }
        }

    }
    CheckMouseActivity();
}

// 视频尺寸变化时的处理
void MainWid::onVideoSizeChanged(int width, int height)
{
    m_currentVideoWidth = width;
    m_currentVideoHeight = height;
    qDebug() << "视频实际尺寸:" << width << "x" << height;
}

// 检查是否正在播放视频
bool MainWid::isVideoPlaying() const
{
    qDebug() << "m_currentVideoWidth = " << m_currentVideoWidth << "m_currentVideoHeight = " << m_currentVideoHeight;
    // 检查是否有当前播放项且不是音频模式
    return (GlobalVars::currentPlayIndex() >= 0 &&
            GlobalVars::isVideoPlaying() &&
            m_currentVideoWidth > 0 && m_currentVideoHeight > 0);
}

// 更新实际大小菜单项状态
void MainWid::onUpdateActualSizeMenuState()
{
    // 在菜单显示前更新状态
    // 这个函数会被调用，但我们主要依赖菜单弹出时的更新
}

// 调整窗口到视频实际大小
void MainWid::adjustWindowToVideoSize()
{
    m_currentVideoWidth = m_currentVideoWidth < 150 ? 150 : m_currentVideoWidth;//最小限制视频窗口
    m_currentVideoHeight = m_currentVideoHeight < 100 ?100 : m_currentVideoHeight;
    // 获取屏幕可用区域
    // 计算需要的窗口尺寸
    int titleBarHeight = ui->TitleWid->height();
    int ctrlBarHeight = ui->CtrlBarWid->height();
    int borderWidth = 4; // 窗口边框宽度（左右各2像素）
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    // 检查视频尺寸是否超过屏幕尺寸
    if (m_currentVideoWidth > screenGeometry.width() ||
        m_currentVideoHeight > (screenGeometry.height() - titleBarHeight - ctrlBarHeight - borderWidth)) {
        qDebug() << "视频尺寸超过屏幕，切换到全屏";
        if (!m_bFullScreenPlay)OnFullScreenPlay();
        return;
    }
    // 如果当前是全屏状态，先退出全屏
        if (m_bFullScreenPlay) {
            OnFullScreenPlay();
        }
    int playlistWidth = (m_bPlaylistVisible && !m_bFullScreenPlay) ? ui->PlaylistWid->width() : 0;
    int splitterHandleWidth = m_bPlaylistVisible ? m_splitter->handleWidth() : 0;
    int requiredWidth = m_currentVideoWidth + borderWidth + playlistWidth + splitterHandleWidth;
    int requiredHeight = titleBarHeight + m_currentVideoHeight + ctrlBarHeight + borderWidth;
    int minWidth =this->minimumWidth();
    int minheight =this->minimumHeight();
    if (requiredWidth < minWidth)this->setMinimumWidth(requiredWidth);
    if (requiredHeight < minheight)this->setMinimumHeight(requiredHeight);
    qDebug() << "需要的窗口尺寸:" << requiredWidth << "x" << requiredHeight;
    qDebug() << "屏幕可用尺寸:" << screenGeometry.width() << "x" << screenGeometry.height();
    // 检查窗口尺寸是否超过屏幕
    if (requiredWidth > screenGeometry.width() ||
        requiredHeight > screenGeometry.height()) {
        qDebug() << "窗口尺寸超过屏幕，调整到合适大小";

        // 计算缩放比例
        float widthRatio = (float)screenGeometry.width() / requiredWidth;
        float heightRatio = (float)(screenGeometry.height() - 100) / requiredHeight; // 减去一些边距
        float scaleRatio = qMin(widthRatio, heightRatio);

        // 确保缩放比例不超过1（不放大）
        scaleRatio = qMin(scaleRatio, 1.0f);

        requiredWidth = requiredWidth * scaleRatio;
        requiredHeight = requiredHeight * scaleRatio;

        qDebug() << "缩放后尺寸:" << requiredWidth << "x" << requiredHeight;
    }

    // 保存当前窗口位置，用于居中
    QPoint currentPos = this->pos();

    // 调整窗口大小
    this->resize(requiredWidth, requiredHeight);

    // 调整分割器中视频窗口的大小
    if (m_splitter) {
        QList<int> sizes = m_splitter->sizes();
        if (sizes.size() >= 2) {
            sizes[0] = m_currentVideoWidth;
            if (m_bPlaylistVisible && sizes.size() > 1) {
                // 保持播放列表宽度比例
                int totalWidth = sizes[0] + sizes[1] + splitterHandleWidth;
                sizes[1] = playlistWidth * requiredWidth / totalWidth;
            }
            m_splitter->setSizes(sizes);
        }
    }

//    // 将窗口移动到屏幕中央
//    int x = screenGeometry.x() + (screenGeometry.width() - requiredWidth) / 2;
//    int y = screenGeometry.y() + (screenGeometry.height() - requiredHeight) / 2;
//    this->move(x, y);

//    qDebug() << "已调整窗口到实际大小";
}

// 实际大小菜单项响应
void MainWid::onActualSize()
{
    if (!isVideoPlaying()) {
        qDebug() << "当前没有播放视频文件";
        return;
    }

    adjustWindowToVideoSize();
}

void MainWid::onSubtitleSettingsChanged(const QString& fontFamily, int fontSize)
{
    // 更新全局变量
    GlobalVars::subtitleFontFamily() = fontFamily;
    GlobalVars::subtitleFontSize() = fontSize;

    // 传递给Show控件
    if (ui->ShowWid) {
        ui->ShowWid->setSubtitleFont(fontFamily, fontSize);
    }

    qDebug() << "字幕设置已更新:" << fontFamily << fontSize;
}
