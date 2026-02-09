/*
 * @file 	show.cpp
 * @date 	2018/01/22 23:07
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	显示控件
 * @note
 */

#include <QPainter>
#include <QDebug>
#include <QMutex>
#include "show.h"
#include "ui_show.h"
#include "globalvars.h"
#include "globalhelper.h"
#include "SimpleMessageBox.h"

#pragma execution_character_set("utf-8")

QMutex g_show_rect_mutex;

Show::Show(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Show),
    m_stActionGroup(this),
    m_stMenu(this),
    m_pSubtitle(nullptr),
    m_pSubtitleLabel(nullptr),
    m_pSubtitleTimer(nullptr),
    m_bDrag(false)  // 初始化拖动标志
{
    ui->setupUi(this);
    this->setContentsMargins(1, 0, 0, 0);  // 左边1像素间距
    //加载样式
    setStyleSheet(GlobalHelper::GetQssStr("://res/qss/show.css"));
    setAcceptDrops(true);

	
    //防止过度刷新显示
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    //ui->label->setAttribute(Qt::WA_OpaquePaintEvent);

    ui->label->setUpdatesEnabled(false);

    this->setMouseTracking(true);
    


    m_nLastFrameWidth = 0; ///< 记录视频宽高
    m_nLastFrameHeight = 0;

    m_stActionGroup.addAction("全屏");
    m_stActionGroup.addAction("暂停");
    m_stActionGroup.addAction("停止");

    m_stMenu.addActions(m_stActionGroup.actions());

    // 初始化字幕系统
        m_pSubtitle = new Subtitle(this);

        // 创建独立的透明字幕窗口
            m_pSubtitleWindow = new TransparentSubtitleWindow();
            GlobalHelper::subtitleWindow() = m_pSubtitleWindow;
            m_pSubtitleWindow->setParent(nullptr);  // 确保是顶级窗口

        // 创建字幕更新定时器
        m_pSubtitleTimer = new QTimer(this);
        m_pSubtitleTimer->setInterval(100); // 每100毫秒更新一次字幕
        // 连接字幕窗口的右键点击信号到Show的右键菜单信号
            connect(m_pSubtitleWindow, &TransparentSubtitleWindow::sigRightClicked,
                    this, &Show::SigShowMenu);
    // 连接字幕更新信号
        connect(this, &Show::SigSubtitleTextChanged, this, &Show::OnSubtitleChanged);
        connect(m_pSubtitleTimer, &QTimer::timeout, this, &Show::OnUpdateSubtitle);

    // 创建音频信息显示控件
            m_pAudioInfoWidget = new QWidget(this);
            m_pAudioInfoWidget->setObjectName("audioInfoWidget");
            m_pAudioInfoWidget->setStyleSheet(
                "QWidget {"
                "   background-color: rgba(40, 40, 40, 200);"
                "   border-radius: 8px;"
                "}"
            );

            m_pAudioInfoLayout = new QHBoxLayout(m_pAudioInfoWidget);
            m_pAudioInfoLayout->setContentsMargins(10, 5, 5, 5);
            m_pAudioInfoLayout->setSpacing(20);

            // 左侧：封面图片
            m_pCoverLabel = new QLabel("无封面");
            m_pCoverLabel->setObjectName("coverLabel");
            m_pCoverLabel->setFixedSize(200, 200);
            m_pCoverLabel->setStyleSheet(
                "QLabel {"
                "   border: 2px solid rgba(255, 255, 255, 100);"
                "   border-radius: 5px;"
                "   background-color: rgba(60, 60, 60, 150);"
                "   color: rgba(200, 200, 200, 150);"
                "   font-size: 12px;"
                "}"
            );
            m_pCoverLabel->setAlignment(Qt::AlignCenter);
            m_pCoverLabel->setScaledContents(true); // 不自动缩放，手动控制
            // 启用鼠标跟踪和双击事件
                m_pCoverLabel->setMouseTracking(true);
                m_pCoverLabel->installEventFilter(this);

                // 连接双击事件
                connect(m_pCoverLabel, &QLabel::customContextMenuRequested, []() {
                    // 如果需要右键菜单可以在这里添加
                });
            // 右侧：文本信息
            QWidget* textWidget = new QWidget();
            QVBoxLayout* textLayout = new QVBoxLayout(textWidget);
            m_pAudioTextLabel = textWidget;
            textLayout->setContentsMargins(0, 0, 0, 0);
            textLayout->setSpacing(5);

            m_pAudioInfoLabel = new QLabel(textWidget);
            m_pAudioInfoLabel->setObjectName("audioInfoLabel");
            m_pAudioInfoLabel->setStyleSheet(
                "QLabel {"
                "   color: white;"
                "   padding : 0px 5px 0px 5px;"
                "   border-top-left-radius: 6px;"
                "   border-top-right-radius: 6px;"
                "   border-bottom-right-radius: 0px;"
                "   border-bottom-left-radius: 0px;"
                "   font-size: 16px;"
                "   font-family: 'Microsoft YaHei', '微软雅黑', sans-serif;"
                "}"
            );
            m_pAudioInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            m_pAudioInfoLabel->setWordWrap(true);
            textLayout->addWidget(m_pAudioInfoLabel);
            textLayout->addStretch();

            // 修改布局顺序：先添加封面（左侧），再添加文本信息（右侧）
            m_pAudioInfoLayout->addWidget(m_pCoverLabel, 1);  // 封面占1份
            m_pAudioInfoLayout->addWidget(textWidget, 3);     // 文本信息占3份

            m_pAudioInfoWidget->hide();

            // 连接音频元数据信号
            connect(VideoCtl::GetInstance(), &VideoCtl::SigAudioMetadataReceived,this, &Show::OnAudioMetadataReceived);
            connect(VideoCtl::GetInstance(), &VideoCtl::SigAudioCoverReceived,this, &Show::OnAudioCoverReceived);
}

Show::~Show()
{
    delete ui;
    if (m_pSubtitleTimer) {
        m_pSubtitleTimer->stop();
        delete m_pSubtitleTimer;
    }
    if (m_pSubtitleWindow) {
        delete m_pSubtitleWindow;
    }
    if (m_pSubtitle) {
        delete m_pSubtitle;
    }
    if (m_pAudioInfoWidget) {
        delete m_pAudioInfoWidget;
    }
}

bool Show::Init()
{
    if (ConnectSignalSlots() == false)
    {
        return false;
    }

	//ui->label->setUpdatesEnabled(false);




	return true;
}

// 更新字幕窗口位置
void Show::updateSubtitleWindowPosition()
{
    //qDebug() << "准备更新字幕窗口－当前位置 ：GlobalVars::getWinState() =---------------" << GlobalVars::getWinState();
    if (!m_pSubtitleWindow) return;
    // 计算字幕窗口位置（底部居中）
    int windowWidth = this->width();
    int windowHeight = this->height();
    int fontSize;
    if (GlobalVars::getFullScreen()) {
        // 全屏时，字体更大一些
        fontSize = qMax(28, windowHeight / 20);
    } else {
        fontSize = qMax(20, windowHeight / 25);
    }
    m_pSubtitleWindow->setFontSize(fontSize);
    int subtitleWidth = windowWidth * 0.8 <200 ? 200 :(windowWidth * 0.8);
    int subtitleHeight = windowHeight * 0.14 <50 ? 50: windowHeight * 0.14;
    int newY=m_pSubtitleWindow->y();
    // 设置字幕窗口位置和大小
        QFont font(GlobalVars::subtitleFontFamily(), m_pSubtitleWindow->m_fontSize, QFont::Bold);
        QFontMetrics fm(font);
        // 如果文本太长，换行
        int maxWidth = width() *0.8;
       // qDebug() << "text:" << m_currentSubtitleText << "fm.horizontalAdvance(text)" << fm.horizontalAdvance(m_currentSubtitleText) << " maxWidth:"<<  maxWidth;
        if (fm.horizontalAdvance(m_currentSubtitleText) > maxWidth) {
            qDebug() <<" 太长了，准备缩小字体！";
            subtitleWidth = width()-20;
                 int currentFontSize = m_pSubtitleWindow->m_fontSize;
            while (fm.horizontalAdvance(m_currentSubtitleText) > width()-20){
                //缩小字体
                    currentFontSize--;
                    QFont font(GlobalVars::subtitleFontFamily(), currentFontSize, QFont::Bold);
                    QFontMetrics fm(font);
                    if (fm.horizontalAdvance(m_currentSubtitleText) <= width()-20){
                     m_pSubtitleWindow->m_fontSize = currentFontSize;
                        break;}
            }
            QFont font(GlobalVars::subtitleFontFamily(), m_pSubtitleWindow->m_fontSize, QFont::Bold);
            QFontMetrics fm(font);
            m_pSubtitleWindow->setGeometry(10, newY, subtitleWidth, subtitleHeight);
        }else m_pSubtitleWindow->setPosition(m_pSubtitleWindow->x(), newY, subtitleWidth, subtitleHeight);
    int winState = GlobalVars::getWinState();
        if (winState > 1 && !isReSizeWin) {
            qDebug() << "2最小化，保持字幕窗口当前位置，不重新计算";
            return;
        }
    // 获取当前窗口在屏幕上的位置
    QPoint globalPos = mapToGlobal(QPoint(0, 0));
    // 字幕窗口尺寸
    // 计算位置
    int x = globalPos.x() + (windowWidth - subtitleWidth) / 2;
    int y = globalPos.y() + windowHeight - subtitleHeight - 10;
    // 全屏模式下调整Y坐标，让字幕更靠上
        if (GlobalVars::getFullScreen()) {
            // 全屏时，让字幕距离底部有一定距离，避免被控制栏遮挡
            int bottomMargin = 80;  // 调整这个值来改变字幕离底部的距离
            y = globalPos.y() + windowHeight - subtitleHeight - bottomMargin;
        } else {
            // 窗口模式，距离底部10像素
            int bottomMargin = 10;
            y = globalPos.y() + windowHeight - subtitleHeight - bottomMargin;
        }
        m_pSubtitleWindow->setPosition(x, y, subtitleWidth, subtitleHeight);
           // 根据窗口高度动态设置字体大小
}

void Show::OnFrameDimensionsChanged(int nFrameWidth, int nFrameHeight)
{
    qDebug() << "Show::OnFrameDimensionsChanged" << nFrameWidth << nFrameHeight;
    m_nLastFrameWidth = nFrameWidth;
    m_nLastFrameHeight = nFrameHeight;
    // 发射视频尺寸变化信号
    emit SigVideoSizeChanged(nFrameWidth, nFrameHeight);
    ChangeShow();
}

void Show::ChangeShow()
{
    g_show_rect_mutex.lock();

    if (m_nLastFrameWidth == 0 && m_nLastFrameHeight == 0)
    {
        ui->label->setGeometry(0, 0, width(), height());
    }
    else
    {
        float aspect_ratio;
        int width, height, x, y;
        int scr_width = this->width();
        int scr_height = this->height();

        aspect_ratio = (float)m_nLastFrameWidth / (float)m_nLastFrameHeight;

        // 根据 isExtend 的值选择不同的显示模式
        int extend = GlobalVars::getExtend();

        if (extend == 1) {
            // 模式1：拉伸适应窗口（不保持宽高比）
            width=scr_width; height=scr_height; x=0; y=0;
            ui->label->setGeometry(0, 0, width, height);
        }
        else {
            // 模式0：等比填充（保持宽高比）
            height = scr_height;
            width = lrint(height * aspect_ratio) & ~1;
            if (width > scr_width)
            {
                width = scr_width;
                height = lrint(width / aspect_ratio) & ~1;
            }
            x = (scr_width - width) / 2;
            y = (scr_height - height) / 2;
            ui->label->setGeometry(x, y, width, height);
        }
        // 添加：全屏切换时强制通知视频渲染器重新设置显示尺寸
                if (GlobalVars::getFullScreen()) {
                    QTimer::singleShot(50, this, [this, width, height]() {
                        // 通过 VideoCtl 重新设置视频输出尺寸
                        VideoCtl::GetInstance()->SetVideoDisplaySize(width, height);
                    });
                }
    }
    g_show_rect_mutex.unlock();
}

void Show::dragEnterEvent(QDragEnterEvent *event)
{
//    if(event->mimeData()->hasFormat("text/uri-list"))
//    {
//        event->acceptProposedAction();
//    }
    event->acceptProposedAction();
}

void Show::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    ChangeShow();

    // 调整音频信息控件位置
    if (m_pAudioInfoWidget) {
        int infoWidth = this->width() * 0.9;
        int infoHeight = m_nAudioInfoHeight;
        int x = (this->width() - infoWidth) / 2;
        int y = 10; // 顶部留10像素边距

        m_pAudioInfoWidget->setGeometry(x, y, infoWidth, infoHeight);
        m_pAudioInfoWidget->raise();
    }
    // 更新字幕窗口位置
        if (m_pSubtitleWindow && m_pSubtitleWindow->isVisible()) {
            isReSizeWin = true;
            updateSubtitleWindowPosition();
            isReSizeWin = false;
        }
        updateSpectrumDisplayArea();
}

void Show::keyReleaseEvent(QKeyEvent *event)
{
    qDebug() << "Show::keyPressEvent:" << event->key();
    switch (event->key())
    {
    case Qt::Key_Escape:
        if (GlobalVars::getFullScreen()){
            emit SigPlayOrPause();
            SigFullScreen();
            emit MinBtnClicked();
        }
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return://全屏
        SigFullScreen();
        break;
    case Qt::Key_Left://后退5s
        emit SigSeekBack();
        break;
    case Qt::Key_Right://前进5s
        qDebug() << "前进5s";
        emit SigSeekForward();
        break;
    case Qt::Key_Up://增加10音量
        emit SigAddVolume();
        break;
    case Qt::Key_Down://减少10音量
        emit SigSubVolume();
        break;
    case Qt::Key_Space://减少10音量
        emit SigPlayOrPause();
        break;

    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

// void Show::contextMenuEvent(QContextMenuEvent* event)
// {
//     //m_stMenu.exec(event->globalPos());
//     qDebug() << "Show::contextMenuEvent";
// }
void Show::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::RightButton)
    {
        emit SigShowMenu();
    }
    else if (event->buttons() & Qt::LeftButton)
        {
        // 检查是否点击在音频信息文本区域（包括边框）内
             if (m_pAudioInfoWidget && m_pAudioInfoWidget->isVisible()) {
                 QPoint labelTopLeft = m_pAudioInfoLabel->mapTo(this, QPoint(0, 0));
                 QSize labelSize = m_pAudioInfoLabel->size();
                 QRect expandedLabelRect = QRect(labelTopLeft, labelSize);
                      qDebug() << " 真实的文字区域：" << expandedLabelRect << "窗口状态：" << GlobalVars::getWinState() << "全屏状态：" << GlobalVars::getFullScreen();
                if (expandedLabelRect.contains(event->pos())) {
                     qDebug() << "鼠标被按下------------------------qqqq:" << event->pos();
                    // 点击在音频信息文本区域（包括边框）内，忽略拖动
                    qDebug() << "点击在音频信息文本区域内（包括边框），忽略拖动";
                    m_bDrag = false;  // 明确设置为不拖动
                    setCursor(Qt::ArrowCursor);  // 恢复光标
                    event->ignore();  // 让事件继续传递
                    return;
                }
            }
            // 获取父窗口（MainWid）
            QWidget* mainWindow = this->window();
            if (mainWindow && !mainWindow->isFullScreen() && GlobalVars::getWinState()==0)
            {
                m_bDrag = true;
                m_DragStartPos = event->globalPos();
                m_WindowStartPos = mainWindow->pos();
                // 设置光标形状
                setCursor(Qt::ClosedHandCursor);
                event->accept();
                return;
            }
        }

    QWidget::mousePressEvent(event);
}
void Show::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_bDrag)
    {
        m_bDrag = false;
        setCursor(Qt::ArrowCursor);  // 恢复光标
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void Show::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bDrag && (event->buttons() & Qt::LeftButton))
        {
            QWidget* mainWindow = this->window();
            if (mainWindow)
            {
                QPoint delta = event->globalPos() - m_DragStartPos;
                mainWindow->move(m_WindowStartPos + delta);

                // 拖动窗口时实时更新字幕位置
                if (m_pSubtitleWindow && m_pSubtitleWindow->isVisible()) {
                    updateSubtitleWindowPosition();
                }

                event->accept();
                return;
            }
        }

        QWidget::mouseMoveEvent(event);
}

void Show::mouseDoubleClickEvent(QMouseEvent *event)
{
    //Q_UNUSED(event);
    m_bDrag = false;
    setCursor(Qt::ArrowCursor);
    SigFullScreen();
    //qDebug() << "双击了视频窗口，------------------";
    //emit SigFullScreen();  // 发射全屏信号
    //QWidget::mouseDoubleClickEvent(event);
}

void Show::OnDisplayMsg(QString strMsg)
{
	qDebug() << "Show::OnDisplayMsg " << strMsg;
}

void Show::OnPlay(QString strFile)
{
    QString strFileName = strFile.toLower();
    m_strCurrentAudioFile = strFile;
    GlobalVars::currentPlayFileName()=strFile;
            // 检查文件类型
            bool isAudio = GlobalHelper::IsMusic(strFileName);
            bool isVideo = GlobalHelper::IsVideo(strFileName);
            if (!isAudio && !isVideo)
            {
                int fileType = VideoCtl::GetInstance()->getFileType(strFileName);
                if (fileType<=0){
                    QMessageBox::warning(this,"播放失败",QString("无法播放该文件：%1\n"
                                                             "错误的代码：%2\n"
                                                             "0. 文件已损坏\n"
                                                             "-1. 不支持的编码格式\n"
                                                             "-2. 文件不包含有效的音频或视频流").arg(strFile).arg(fileType),QMessageBox::Close);
                    return ;
                }else {
                    isAudio = fileType == 1;
                    isVideo = fileType == 2;
                }

            }
            // 停止之前的字幕定时器
                if (m_pSubtitleTimer) {
                    qDebug() << "停止之前的字幕定时器";
                            m_pSubtitleTimer->stop();

                            // 断开旧的连接
                            disconnect(m_pSubtitleTimer, &QTimer::timeout, this, &Show::OnUpdateSubtitle);
                }

                // 清除之前的字幕
                if (m_pSubtitle) {
                    m_pSubtitle->clear();
                }

                // 加载字幕文件
                if (m_pSubtitle->loadSubtitle(strFile)) {
                    qDebug() << "字幕加载成功，共" << m_pSubtitle->hasSubtitles() << "条";

                                // 重新连接定时器信号槽
                                bool connected = connect(m_pSubtitleTimer, &QTimer::timeout,
                                                       this, &Show::OnUpdateSubtitle,
                                                       Qt::QueuedConnection);
                                qDebug() << "重新连接定时器信号槽:" << (connected ? "成功" : "失败");

                                // 延迟启动定时器
                                QTimer::singleShot(300, this, [this]() {
                                    if (m_pSubtitleTimer) {
                                        qDebug() << "启动字幕更新定时器";
                                        m_pSubtitleTimer->start();
                                        qDebug() << "定时器状态 - 是否活跃:" << m_pSubtitleTimer->isActive();

                                        // 立即测试一次调用
                                        QTimer::singleShot(50, this, &Show::OnUpdateSubtitle);
                                    }
                                });
                } else {
                    qDebug() << "未找到字幕文件或加载失败";
                }
            if (isVideo) {
                // 视频文件
                SetAudioMode(false);
                VideoCtl::GetInstance()->SetAudioOnlyMode(false);
                VideoCtl::GetInstance()->StartPlay(strFile, ui->label->winId());
            } else if (isAudio) {
                // 音频文件
                SetAudioMode(true);
                VideoCtl::GetInstance()->SetAudioOnlyMode(true);
                // 清除视频尺寸信息
                m_nLastFrameWidth = 0;
                m_nLastFrameHeight = 0;

                // 发射视频尺寸为0的信号（表示音频模式）
                emit SigVideoSizeChanged(0, 0);
                // 显示音频频谱提示
    //            ui->label->setText("正在播放音频文件\n显示频谱...");
    //            ui->label->setStyleSheet("color: white; font-size: 16px; background-color: black;");
    //            ui->label->setAlignment(Qt::AlignCenter);

                // 开始播放音频
                VideoCtl::GetInstance()->StartPlay(strFile, ui->label->winId());
            }
}
// 添加频谱数据处理
void Show::OnAudioSpectrumData(float* data, int bands)
{
  //  qDebug() << "m_showSpectrum=======================" << m_showSpectrum;
    if (!m_bAudioMode) return;

    m_spectrumBands = bands;
    m_spectrumData.clear();
    for (int i = 0; i < bands; i++) {
        m_spectrumData.append(data[i]);
    }
    m_showSpectrum = true;
    update();  // 触发重绘
}
void Show::SetAudioMode(bool audioMode)
{
    m_bAudioMode = audioMode;
    m_showSpectrum = audioMode;
    if (audioMode) {
        // 音频模式
        // 显示音频信息控件
                if (m_pAudioInfoWidget) {
                    m_pAudioInfoWidget->show();
                }
                // 立即隐藏label
                ui->label->setVisible(false);
                // 清空频谱数据
                        m_spectrumData.clear();
                        // 显示频谱
                        m_showSpectrum = true;
                // 强制清空label内容
                //ui->label->clear();
                //ui->label->setText("");
                //m_spectrumData.clear();
                // 设置label样式为透明
                ui->label->setStyleSheet("background-color: transparent;");
                // 立即强制绘制黑色背景
                this->repaint();  // 使用repaint而不是update，强制同步重绘
                qDebug() << "音频模式设置完成，已强制重绘";
    }else {
        // 视频模式：隐藏音频信息控件
                if (m_pAudioInfoWidget) {
                    m_pAudioInfoWidget->hide();
                }
        // 视频模式：显示label
        ui->label->show();
        ui->label->setStyleSheet("");
    }
    //update();这个打开会造成有一个长方形的白色区域闪一下。
}
void Show::OnStopFinished()
{
    if (m_pSubtitleTimer) {
            m_pSubtitleTimer->stop();
        }
        // 清除视频尺寸信息
            m_nLastFrameWidth = 0;
            m_nLastFrameHeight = 0;

            // 发射视频尺寸为0的信号
            emit SigVideoSizeChanged(0, 0);
        // 不清除字幕列表(加载之前其实的清空动作)，只清除当前显示的字幕
        m_currentSubtitleText.clear();
        // 隐藏字幕窗口
            if (m_pSubtitleWindow) {
                m_pSubtitleWindow->setSubtitleText("");
            }
    update();
}


void Show::OnTimerShowCursorUpdate()
{
    //qDebug() << "Show::OnTimerShowCursorUpdate()";
    //setCursor(Qt::BlankCursor);
}

void Show::OnActionsTriggered(QAction *action)
{
    QString strAction = action->text();
    if (strAction == "全屏")
    {
        emit SigFullScreen();
    }
    else if (strAction == "停止")
    {
        emit SigStop();
    }
    else if (strAction == "暂停" || strAction == "播放")
    {
        emit SigPlayOrPause();
    }
}

bool Show::ConnectSignalSlots()
{
	QList<bool> listRet;
	bool bRet;

	bRet = connect(this, &Show::SigPlay, this, &Show::OnPlay);
    listRet.append(bRet);

    timerShowCursor.setInterval(2000);
    bRet = connect(&timerShowCursor, &QTimer::timeout, this, &Show::OnTimerShowCursorUpdate);
    listRet.append(bRet);

    connect(&m_stActionGroup, &QActionGroup::triggered, this, &Show::OnActionsTriggered);

	for (bool bReturn : listRet)
	{
		if (bReturn == false)
		{
			return false;
		}
	}

	return true;
}

void Show::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if(urls.isEmpty())
    {
        return;
    }
    QStringList dropFiles;
    for(QUrl url: urls)
    {
        QString strFileName = url.toLocalFile();
        qDebug() << strFileName;
        dropFiles.append(strFileName);
    }
    int index = GlobalVars::currentPlayIndex();
    emit SigAddFilesAndPlay(dropFiles,index < 0 ? 0 : index);
}
// 修改paintEvent函数
void Show::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    // 始终先绘制黑色背景，避免白色闪烁
    painter.fillRect(this->rect(), QColor(0, 0, 0));
    // 计算频谱显示区域（考虑音频信息区域）
        int topMargin = 0;
        if (m_pAudioInfoWidget && m_pAudioInfoWidget->isVisible()) {
            topMargin = m_pAudioInfoWidget->height() + 10; // 信息区域高度 + 间隔
        }
    if (m_showSpectrum && !m_spectrumData.isEmpty() ) {
        int width = this->width();
        int height = this->height();

        // 考虑底部字幕
                int bottomMargin = 0;
                if (m_pSubtitleLabel && m_pSubtitleLabel->isVisible()) {
                    bottomMargin = m_pSubtitleLabel->height() + 20;
                }

                int availableHeight = height - topMargin - bottomMargin - 10;

                // 确保频谱有最小高度
                if (availableHeight < m_nSpectrumMinHeight) {
                    availableHeight = m_nSpectrumMinHeight;
                }

                int spectrumY = topMargin + 5;
                int spectrumHeight = availableHeight - 3;

        // 如果频谱显示区域高度太小，不绘制频谱
        if (spectrumHeight <= 0) {
            return;
        }
        int margin =3 ;
        int spectrumWidth = width - 2 * margin;
        spectrumY = margin + (m_pAudioInfoWidget ? m_nAudioInfoHeight :0) + 18;
        // 绘制边框
        painter.setPen(QColor(100, 100, 100));
        painter.drawRect(margin, spectrumY, spectrumWidth, spectrumHeight);

        // 绘制频谱标题
        painter.setPen(QColor(255, 255, 255));
        painter.setFont(QFont("Arial", 14, QFont::Bold));
        QString title = "音频频谱";
       // painter.drawText(width/2 - painter.fontMetrics().horizontalAdvance(title)/2,
                   //    spectrumY - 10, title);

        // 绘制频谱
        if (m_spectrumBands > 0) {
            int barSpacing = 4;
            int barWidth = (spectrumWidth - (m_spectrumBands -1) * barSpacing - margin*2) / m_spectrumBands;
            margin=(spectrumWidth-barSpacing*(m_spectrumBands-1)-barWidth*m_spectrumBands)/2;

            for (int i = 0; i < m_spectrumBands; i++) {
                float value = m_spectrumData[i];
                int barHeight = value * spectrumHeight;
                int x = margin + i * (barWidth + barSpacing) + barSpacing;
                int y = spectrumY + spectrumHeight - barHeight;

                // 根据频率位置使用不同颜色
                QColor color;
                float pos = (float)i / m_spectrumBands;

                if (pos < 0.33f) {
                    // 低频：红色到橙色
                    int red = 255;
                    int green = static_cast<int>(150 * pos * 3);
                    color = QColor(red, green, 0);
                } else if (pos < 0.66f) {
                    // 中频：橙色到绿色
                    int red = static_cast<int>(255 * (1 - (pos - 0.33f) * 3));
                    int green = 255;
                    color = QColor(red, green, 0);
                } else {
                    // 高频：绿色到紫色
                    int green = static_cast<int>(255 * (1 - (pos - 0.66f) * 3));
                    int blue = 255;
                    color = QColor(0, green, blue);
                }

                painter.setBrush(color);
                painter.setPen(Qt::NoPen);
                painter.drawRect(x, y, barWidth, barHeight);
            }
        }
    }
}
// 更新字幕

void Show::OnUpdateSubtitle()
{
    if (!GlobalHelper::isSubtitleVisible()){
        return;
    }
    static QString lastValidSubtitle;    // 上一个有效的字幕
        static QTime lastSubtitleTime;       // 上一次有字幕的时间
        static bool isBetweenLyrics = false; // 是否在两行歌词之间
        static int betweenCounter = 0;       // 在歌词间空白的时间计数

        if (!m_pSubtitle || !m_pSubtitle->hasSubtitles()) {
            return;
        }
        if (GlobalVars::runState()!=1) return;

        // 从VideoCtl获取当前播放时间
        int currentMs = VideoCtl::GetInstance()->getCurrentPlayTimeMs();
        QTime currentTime = QTime(0, 0, 0).addMSecs(currentMs);

        // 获取当前字幕
        QString subtitleText = m_pSubtitle->getCurrentSubtitle(currentTime);
       if (!subtitleText.isEmpty()) {
            // 有新字幕
            betweenCounter = 0;
            isBetweenLyrics = false;

            if (subtitleText != lastValidSubtitle) {
                lastValidSubtitle = subtitleText;
                lastSubtitleTime = currentTime;
                m_currentSubtitleText = subtitleText;

                // 更新字幕窗口
                if (m_pSubtitleWindow) {
                    updateSubtitleWindowPosition();
                    m_pSubtitleWindow->setSubtitleText(subtitleText);
                    qDebug() << "显示新字幕: " << subtitleText;
                }
            }
        } else {
            // 没有获取到字幕（歌词之间）
            if (!lastValidSubtitle.isEmpty()) {
                // 检查是否已经进入歌词间状态
                if (!isBetweenLyrics) {
                    // 第一次进入歌词间状态
                    isBetweenLyrics = true;
                    betweenCounter = 0;
                  //  qDebug() << "进入歌词之间区域，开始计时";
                } else {
                    // 已经在歌词间状态，增加计数器
                    betweenCounter++;

                    // 每100毫秒调用一次，100次 = 10秒
                    if (betweenCounter > 100) {
                        // 超过10秒，清除字幕
                        lastValidSubtitle.clear();
                        m_currentSubtitleText.clear();
                        isBetweenLyrics = false;
                        betweenCounter = 0;

                        if (m_pSubtitleWindow) {
                            m_pSubtitleWindow->setSubtitleText("");
                        }
                    } else {
                        // 少于10秒，保持显示上一句歌词
                        // 不需要更新字幕窗口，因为已经显示着
                        // 确保字幕窗口仍然显示
                        if (m_pSubtitleWindow) {
                            if (m_pSubtitleWindow->isHidden()) {
                                m_pSubtitleWindow->show();
                            }
                            // 刷新一下，确保显示正确
                            m_pSubtitleWindow->update();
                        }
                    }
                }
            }
        }
}

// 字幕文本变化处理
void Show::OnSubtitleChanged(const QString& text)
{
    if (!m_pSubtitleLabel) {
        return;
    }

    if (text.isEmpty()) {
        m_pSubtitleLabel->hide();
    } else {
        m_pSubtitleLabel->setText(text);
        m_pSubtitleLabel->show();
        m_pSubtitleLabel->raise();
    }
}

// 更新频谱显示区域
void Show::updateSpectrumDisplayArea()
{
    if (!m_bAudioMode || !m_showSpectrum) return;

    int width = this->width();
    int height = this->height();

    // 顶部音频信息区域高度
    int topMargin = 0;
    if (m_pAudioInfoWidget && m_pAudioInfoWidget->isVisible()) {
        topMargin = m_nAudioInfoHeight;
    }

    // 底部字幕区域高度
    int bottomMargin = 0;
    // 计算可用高度
    int availableHeight = height - topMargin - bottomMargin - 10; // 减去间隔

    // 如果可用高度小于最小高度，调整窗口
    if (availableHeight < m_nSpectrumMinHeight) {
        // 可以触发调整窗口大小或者压缩其他区域
        // 这里我们可以动态调整音频信息区域的高度
        if (m_pAudioInfoWidget && m_pAudioInfoWidget->isVisible()) {
            // 压缩音频信息区域
            int reducedHeight = m_nSpectrumMinHeight - availableHeight;
            m_pAudioInfoWidget->setFixedHeight(m_nAudioInfoHeight - reducedHeight);
            m_pCoverLabel->setFixedSize(80, 80); // 缩小封面
        }
    }
}

// 接收音频元数据
void Show::OnAudioMetadataReceived(const QString& title, const QString& artist,
                                   const QString& album, const QString& genre,
                                   int year, int bitrate, int sampleRate)
{
    if (!m_pAudioInfoLabel) return;

    QString infoText;
    infoText += QString("<b>路＿径:</b> %1<br>").arg(m_strCurrentAudioFile);
    if (!title.isEmpty()) {
        infoText += QString("<b>标＿题:</b> %1<br>").arg(title);
    }
    if (!artist.isEmpty()) {
        infoText += QString("<b>艺术家:</b> %1<br>").arg(artist);
    }
    if (!album.isEmpty()) {
        infoText += QString("<b>专＿辑:</b> %1<br>").arg(album);
    }
    if (year > 0) {
        infoText += QString("<b>年＿份:</b> %1<br>").arg(year);
    }
    if (!genre.isEmpty()) {
        infoText += QString("<b>流＿派:</b> %1<br>").arg(genre);
    }
    if (bitrate > 0) {
        infoText += QString("<b>比特率:</b> %1 kbps<br>").arg(bitrate);
    }
    if (sampleRate > 0) {
        infoText += QString("<b>采样率:</b> %1 Hz<br>").arg(sampleRate);
    }

    m_pAudioInfoLabel->setText(infoText);
}

// 接收封面图片
void Show::OnAudioCoverReceived(const QPixmap& cover)
{
    if (!m_pCoverLabel) return;

    if (!cover.isNull()) {
        // 移除占位符
        QWidget* placeholder = m_pCoverLabel->findChild<QLabel*>();
        if (placeholder) {
            placeholder->hide();
        }

        // 设置封面图片
        QPixmap scaledCover = cover.scaled(m_pCoverLabel->size(),
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation);
        m_pCoverLabel->setPixmap(scaledCover);
    } else {

        // 没有封面时，显示"无封面"文本
        loadDefaultCover();
    }
}

// 加载默认封面图片的函数
void Show::loadDefaultCover()
{
    m_strCurrentAudioFile = GlobalVars::currentPlayFileName();
    qDebug()<< "准备查找并加载存在封面文件－－－－－－－－－－－－－－:11222222222222222" <<  GlobalVars::currentPlayFileName();
    // 首先尝试加载歌曲文件同目录下的封面图片
        if (!m_strCurrentAudioFile.isEmpty()) {
            QFileInfo audioFileInfo(m_strCurrentAudioFile);
            QString baseName = audioFileInfo.completeBaseName(); // 获取不带扩展名的文件名
            QString dirPath = audioFileInfo.absolutePath();

            // 支持的图片格式
            QStringList imageExtensions = {"_cover.jpg", "_cover.png", "_cover.jpeg", "_cover.bmp", "_cover.gif"};

            for (const QString &ext : imageExtensions) {
                QString coverPath = dirPath + "/" + baseName + ext;

                if (QFile::exists(coverPath)) {

                    QPixmap cover(coverPath);
                    if (!cover.isNull()) {
                        // 缩放图片到合适大小
                        int height =cover.height();
                        int width =cover.width();
                        qDebug()<< "存在封面文件，准备加载－－－－－－－－－height－width:－－－－:"<< height << " : "<< width;
                        if (height < width){
                            width = (width * 200)/height ;
                            width = width>300 ? 300 : width;
                            height = 200;
                            m_pCoverLabel->setFixedSize(width, height);
                            qDebug()<< "存在封面文件，准备加载－－－－－－－－－－－－－－width:"<< width;
                        }else {
                            width = 200;
                            height = 200;
                        }
                        QPixmap scaledCover = cover.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        m_pCoverLabel->setPixmap(scaledCover);
                        return; // 成功加载封面，直接返回
                    }
                }
            }
        }
    QPixmap defaultCover(":/res/default_cover.png");

    // 如果资源文件加载失败，创建一个简单的默认图片
    if (defaultCover.isNull()) {
        // 创建一个200x200的默认图片
        defaultCover = QPixmap(200, 200);
        defaultCover.fill(QColor(60, 60, 60, 150));

        QPainter painter(&defaultCover);
        painter.setPen(QColor(200, 200, 200, 150));
        painter.setFont(QFont("Arial", 12));

        // 绘制音符图标
        QPainterPath path;
        path.addEllipse(70, 40, 60, 60);
        path.moveTo(100, 100);
        path.lineTo(100, 160);
        path.moveTo(100, 120);
        path.lineTo(120, 140);
        path.moveTo(100, 120);
        path.lineTo(80, 140);

        painter.drawPath(path);

        // 绘制"无封面"文字
        painter.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
        painter.drawText(defaultCover.rect(), Qt::AlignCenter | Qt::AlignBottom, "无封面");

        painter.end();
    }

    // 缩放图片到合适大小
    QPixmap scaledCover = defaultCover.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_pCoverLabel->setPixmap(scaledCover);
}
// 在 Show 类中添加事件过滤器
bool Show::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_pCoverLabel) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                onCoverLabelDoubleClicked();
                return true;
            }
        }
    }
    // 处理父窗口（主窗口）移动事件
//        if ("MainWid" == this->window()->objectName() && event->type() == QEvent::Move) {
//            // 窗口移动时实时更新字幕窗口位置
//            if (m_pSubtitleWindow && m_pSubtitleWindow->isVisible()) {
//                updateSubtitleWindowPosition();
//            }
//        }
        // 新增：处理父窗口隐藏和显示事件（对应最小化和恢复）
//        if ("MainWid" == this->window()->objectName()) {
//            if (GlobalVars::getWinState()==4 && (event->type() == QEvent::Hide || event->type() == QEvent::WindowDeactivate)) {
//                // 停止字幕定时器，防止重新显示
//                if (m_pSubtitleTimer && m_pSubtitleTimer->isActive()) {
//                    m_pSubtitleTimer->stop();
//                    qDebug() << "停止字幕定时器";
//                }

//                if (m_pSubtitleWindow) {
//                    m_pSubtitleWindow->hide();
//                    qDebug() << "字幕窗口已隐藏";
//                }
//            } else if (event->type() == QEvent::Show || event->type() == QEvent::WindowActivate) {
//                qDebug() << "Show::showSubtitleWindow 被调用";
//                if (m_pSubtitleWindow && !m_currentSubtitleText.isEmpty()) {
//                    // 如果之前定时器被停止，重新启动它
//                    if (m_pSubtitleTimer && !m_pSubtitleTimer->isActive() && m_pSubtitle->hasSubtitles()) {
//                        m_pSubtitleTimer->start();
//                        qDebug() << "启动字幕定时器";
//                    }

//                    updateSubtitleWindowPosition();
//                    m_pSubtitleWindow->show();
//                    qDebug() << "字幕窗口已显示";
//                } else {
//                    qDebug() << "没有字幕内容，不显示字幕窗口";
//                }

//            }
//        }
    return QWidget::eventFilter(obj, event);
}
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include <QRegularExpression>

void Show::onCoverLabelDoubleClicked()
{
    if (!m_bAudioMode || m_strCurrentAudioFile.isEmpty()) {
            SimpleMessageBox::warning(this, "警告", "没有正在播放的音频文件！");
            return;
        }
    // 检查文件格式是否支持封面嵌入
    QString suffix = QFileInfo(m_strCurrentAudioFile).suffix().toLower();
        QList<QString> supportedFormats = {"mp3", "flac", "m4a", "ogg", "wma", "aac", "alac"};
        int reply= 0;
        if (!supportedFormats.contains(suffix)) {
           reply=  SimpleMessageBox::question(this, "格式不支持",
                QString("格式（.%1）不支持嵌入封面, 确认使用外挂吗？\n"
                       "支持格式：MP3,FLAC,M4A,OGG,WMA,AAC,ALAC")
                       .arg(suffix),450,140);
            if (reply != QMessageBox::Ok) {
                return;
            }
        }

        if (reply== 0){//是支持格式，检查ffmpeg
            QProcess process;
            process.start("ffmpeg", QStringList() << "-version");

            if (!process.waitForStarted(3000)) {
                // 尝试其他可能的名称
                process.start("ffmpeg.exe", QStringList() << "-version");
                if (!process.waitForStarted(3000)) {
                    SimpleMessageBox::warning(this, "警告", "您可能没有安装 FFmpeg 或未配置环境变量\n无法更换封面！",400,140);
                    return ;
                }
            }
            if (!process.waitForFinished(3000)) {
                process.kill();
                SimpleMessageBox::warning(this, "警告", "获取 FFmpeg 版本超时！",400,140);
                return ;
            }
            QByteArray output = process.readAllStandardOutput();
            QByteArray error = process.readAllStandardError();
            QString allOutput = QString::fromLocal8Bit(output) + QString::fromLocal8Bit(error);

            // 从输出中提取版本信息
            QRegularExpression versionRegex(R"(ffmpeg\s+version\s+(\S+))",
                                            QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = versionRegex.match(allOutput);

            if (match.hasMatch()) {
                qDebug()<< "您安装的 FFmpeg 版本:"<< match.captured(1);
            }
        }

    // 打开文件对话框选择图片
        QString lastBmpDir = GlobalVars::GetLastOpenDir(1);
    QString filter = "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)";
    QString imageFile = QFileDialog::getOpenFileName(this,
        "选择封面图片",
        lastBmpDir,
        filter);

    if (imageFile.isEmpty()) {
        return;
    }
    GlobalVars::SaveLastOpenDir(QFileInfo(imageFile).absolutePath(),1);
    QFileInfo seleFile(imageFile);
    lastBmpDir = seleFile.absolutePath();
    // 验证图片文件
    QPixmap cover;
    if (!cover.load(imageFile)) {
        SimpleMessageBox::critical(this, "错误", "无法加载图片文件！",400,140);
        return;
    }
    // 检查图片尺寸（建议不要太大）
    QImage image(imageFile);
    if (image.width() > 1500 || image.height() > 1500) {
        SimpleMessageBox::warning(this, "图片过大",
                                  "图片尺寸较大，建议压缩到1500x1500以内以提高兼容性。"
                                  ,450,140);
        return ;
    }
           qDebug() <<"reply = ----------------1:" << reply;
    if (reply== 0){
        // 使用自定义消息框
        reply = SimpleMessageBox::question(this, "确认",
                    QString("是否将选中的图片保存为音频文件的封面？\n"
                           "音频文件：%1\n"
                           "图片文件：%2")
                           .arg(QFileInfo(m_strCurrentAudioFile).fileName())
                           .arg(QFileInfo(imageFile).fileName()));
        if (reply != QMessageBox::Ok) {
            return;
        }
    }
    if (saveCoverToAudioFile(m_strCurrentAudioFile, imageFile)) {
            // 更新显示
            QPixmap scaledCover = cover.scaled(m_pCoverLabel->size(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation);
            m_pCoverLabel->setPixmap(scaledCover);

            SimpleMessageBox::information(this, "成功", "封面已保存到音频文件中！",450,140);
        } else {
                bool success = false;
                qDebug() << "尝试使用简单方法...";
                // 将封面图片保存到音频文件所在目录
                QString audioDir = QFileInfo(m_strCurrentAudioFile).absolutePath();
                QString coverFileName = QFileInfo(m_strCurrentAudioFile).completeBaseName() + "_cover." + QFileInfo(imageFile).suffix().toLower();
                QString coverPath = audioDir + "/" + coverFileName;
                // 如果已存在则删除
                if (QFile::exists(coverPath)) {
                    QFile::remove(coverPath);
                }

                if (QFile::copy(imageFile, coverPath)) {
                    qDebug() << "封面图片已保存到: " << coverPath;
                    // 更新应用程序的封面缓存
                    QPixmap cover(coverPath);

                    if (!cover.isNull()) {
                        // 缩放图片到合适大小
                        QPixmap scaledCover = cover.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        m_pCoverLabel->setPixmap(scaledCover);
                        return; // 成功加载封面，直接返回
                    }
                    success = true;
                } else {
                    qDebug() << "保存封面图片失败";
                }
           if (success) SimpleMessageBox::information(this, "外挂成功", "封面图片保存到歌曲目录！",400,140);
           else SimpleMessageBox::critical(this, "错误", "保存封面失败！",400,140);
    }

}



bool Show::saveCoverToAudioFile(const QString& audioFile, const QString& coverImageFile)
{
        bool success = false;
        QString suffix = QFileInfo(m_strCurrentAudioFile).suffix().toLower();
        // 创建临时文件
        QString tempFile = QDir::tempPath() + "/temp_audio_with_cover." + suffix;

        // 构建 ffmpeg 命令 - 使用和手动运行相同的参数
        QString program = "ffmpeg";
        QStringList arguments;

        // 输入文件
        arguments << "-i" << audioFile;
        arguments << "-i" << coverImageFile;

        // 映射流
        arguments << "-map" << "0";
        arguments << "-map" << "1";

        // 复制所有编解码器
        arguments << "-c" << "copy";

        // 元数据 - 注意使用和手动命令相同的comment
        arguments << "-metadata:s:v" << "title=\"Album cover\"";
        arguments << "-metadata:s:v" << "comment=\"Cover (front)\"";  // 注意：和手动命令一样使用"front"
        arguments << "-disposition:v" << "attached_pic";

        // 输出文件
        arguments << tempFile;

        qDebug() << "完整命令: " << program << arguments;

        // 执行 ffmpeg 命令
        QProcess ffmpegProcess;
        ffmpegProcess.setProcessChannelMode(QProcess::MergedChannels); // 合并标准输出和错误

        // 设置工作目录为音频文件所在目录，避免路径问题
        QString audioDir = QFileInfo(audioFile).absolutePath();
        ffmpegProcess.setWorkingDirectory(audioDir);

        // 连接输出信号
        QObject::connect(&ffmpegProcess, &QProcess::readyRead, [&]() {
            QByteArray output = ffmpegProcess.readAll();
            qDebug() << "ffmpeg输出: " << QString::fromLocal8Bit(output);
        });

        // 连接错误信号
        QObject::connect(&ffmpegProcess, &QProcess::errorOccurred, [&](QProcess::ProcessError error) {
            qDebug() << "ffmpeg进程错误: " << error;
        });

        qDebug() << "启动ffmpeg进程...";
        ffmpegProcess.start(program, arguments);

        // 等待进程启动
        if (!ffmpegProcess.waitForStarted(5000)) {
            qDebug() << "无法启动ffmpeg进程";
            qDebug() << "错误字符串: " << ffmpegProcess.errorString();
            return false;
        }

        qDebug() << "ffmpeg进程已启动，等待完成...";

        // 等待进程完成，设置较长的超时时间（10秒）
        if (ffmpegProcess.waitForFinished(10000)) {
            int exitCode = ffmpegProcess.exitCode();
            QByteArray allOutput = ffmpegProcess.readAll();
            qDebug() << "ffmpeg进程完成，退出码: " << exitCode;
            qDebug() << "完整输出: \n" << QString::fromLocal8Bit(allOutput);

            QFileInfo fileInfo(tempFile);
            // 执行 ffmpeg 命令成功后，进行文件替换
            if (exitCode == 0 && fileInfo.exists() && fileInfo.size() > 0) {
                    qDebug() << "临时文件创建成功，大小: " << fileInfo.size() << "字节";

                    // 首先停止播放该文件
                    emit SigStop(); // 假设有这个信号可以停止播放

                    // 等待一小段时间确保文件被释放
                    QThread::msleep(200);

                    // 尝试重命名原文件（比直接删除更安全）
                    QString backupFile = audioFile + ".backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

                    bool renameSuccess = false;
                    int retryCount = 0;

                    // 重试机制
                    while (retryCount < 3 && !renameSuccess) {
                        if (QFile::exists(audioFile)) {
                            renameSuccess = QFile::rename(audioFile, backupFile);
                            if (!renameSuccess) {
                                qDebug() << "重命名尝试" << (retryCount + 1) << "失败，等待后重试...";
                                QThread::msleep(100);
                                retryCount++;
                            }
                        } else {
                            renameSuccess = true; // 文件不存在，直接跳过
                        }
                    }

                    if (!renameSuccess) {
                        qDebug() << "无法重命名原文件，可能文件被占用";

                        // 尝试直接复制覆盖
                        qDebug() << "尝试直接复制覆盖...";
                        if (QFile::remove(audioFile)) {
                            qDebug() << "删除原文件成功";
                        } else {
                            qDebug() << "无法删除原文件，尝试强制复制";
                        }
                    }

                    // 复制临时文件到目标位置
                    bool copySuccess = QFile::copy(tempFile, audioFile);

                    if (copySuccess) {
                        qDebug() << "新文件复制成功";

                        // 验证新文件
                        QFileInfo newFileInfo(audioFile);
                        if (newFileInfo.exists() && newFileInfo.size() > 0) {
                            success = true;
                            qDebug() << "封面添加成功，新文件大小: " << newFileInfo.size() << "字节";

                            // 清理备份文件
                            if (QFile::exists(backupFile)) {
                                QFile::remove(backupFile);
                                qDebug() << "备份文件已删除";
                            }

                            // 更新显示
                            QPixmap coverPixmap(coverImageFile);
                            if (!coverPixmap.isNull()) {
                                OnAudioCoverReceived(coverPixmap);
                            }
                        } else {
                            qDebug() << "新文件验证失败";

                            // 恢复备份
                            if (renameSuccess && QFile::exists(backupFile)) {
                                if (QFile::remove(audioFile) && QFile::rename(backupFile, audioFile)) {
                                    qDebug() << "已恢复备份文件";
                                }
                            }
                        }
                    } else {
                        qDebug() << "复制临时文件失败";

                        // 恢复备份
                        if (renameSuccess && QFile::exists(backupFile)) {
                            QFile::rename(backupFile, audioFile);
                            qDebug() << "已恢复备份文件";
                        }
                    }

                    // 删除临时文件
                    if (QFile::exists(tempFile)) {
                        QFile::remove(tempFile);
                        qDebug() << "临时文件已删除";
                    }
                } else {
                qDebug() << "ffmpeg执行失败或未生成有效文件";
                qDebug() << "临时文件存在: " << fileInfo.exists();
                qDebug() << "临时文件大小: " << fileInfo.size();
            }
        } else {
            qDebug() << "ffmpeg执行超时";
            ffmpegProcess.kill(); // 终止进程
        }

        if (success) {
            // 重新播放音频文件
            qDebug() << "封面保存成功，重新播放音频文件";
            emit SigPlay(audioFile);
        }
        return success;
    }

void Show::setSubtitleFont(const QString& fontFamily, int fontSize)
{
    if (m_pSubtitleWindow) {
        // 这里需要修改TransparentSubtitleWindow类，添加设置字体的函数
        // 当前TransparentSubtitleWindow只有setFontSize，需要添加setFontFamily
        m_pSubtitleWindow->setFontFamily(fontFamily);
        //m_pSubtitleWindow->setFontSize(fontSize);
    }

    // 保存到全局变量（也可以在MainWid中已经保存了）
    GlobalVars::subtitleFontFamily() = fontFamily;
    GlobalVars::subtitleFontSize() = fontSize;

    qDebug() << "Show: 字幕字体设置为" << fontFamily << fontSize;
}
void Show::OnSubtitleColorSettingsChanged(const QColor& textColor, const QColor& strokeColor,
                                         const QColor& hoverBgColor, const QColor& leaveBgColor,
                                         bool keepBackground)
{

    if (m_pSubtitleWindow) {
        m_pSubtitleWindow->setTextColor(textColor);
        m_pSubtitleWindow->setStrokeColor(strokeColor);
        m_pSubtitleWindow->setHoverBgColor(hoverBgColor);
        m_pSubtitleWindow->setLeaveBgColor(leaveBgColor);
        m_pSubtitleWindow->setKeepBackground(keepBackground);
        m_pSubtitleWindow->update();
    }
}

void Show::OnLyricDownloaded(const QString& lrcPath)
{
    qDebug() << "收到歌词下载完成信号:" << lrcPath;

    // 检查当前是否正在播放
    if (m_strCurrentAudioFile.isEmpty() || GlobalVars::runState() != 1) {
        qDebug() << "当前没有播放音频文件或不在播放状态，不加载歌词";
        return;
    }

    // 检查歌词文件是否与当前播放文件匹配
    QFileInfo lrcFileInfo(lrcPath);
    QFileInfo currentFileInfo(m_strCurrentAudioFile);

    // 比较基本文件名是否相同（去除扩展名）
    if (lrcFileInfo.completeBaseName().compare(currentFileInfo.completeBaseName(), Qt::CaseInsensitive) != 0) {
        qDebug() << "歌词文件名与当前播放文件不匹配，不加载";
        return;
    }

    // 重新加载字幕
    if (m_pSubtitle) {
        // 停止当前定时器
        if (m_pSubtitleTimer && m_pSubtitleTimer->isActive()) {
            m_pSubtitleTimer->stop();
        }

        // 清除当前字幕
        m_pSubtitle->clear();
        m_currentSubtitleText.clear();

        // 加载新下载的字幕文件
        bool loaded = m_pSubtitle->loadSubtitleFile(lrcPath);
        if (loaded) {
            qDebug() << "成功加载新下载的字幕文件，共" << m_pSubtitle->hasSubtitles() << "条字幕";

            // 重新启动字幕定时器
            if (m_pSubtitleTimer) {
                // 重新连接信号槽
                disconnect(m_pSubtitleTimer, &QTimer::timeout, this, &Show::OnUpdateSubtitle);
                connect(m_pSubtitleTimer, &QTimer::timeout, this, &Show::OnUpdateSubtitle, Qt::QueuedConnection);

                // 延迟启动定时器
                QTimer::singleShot(300, this, [this]() {
                    if (m_pSubtitleTimer) {
                        qDebug() << "重新启动字幕更新定时器";
                        m_pSubtitleTimer->start();

                        // 立即更新一次字幕，从当前位置开始
                        OnUpdateSubtitle();
                    }
                });
            }

            // 立即显示字幕窗口
            if (m_pSubtitleWindow && GlobalHelper::isSubtitleVisible()) {
                updateSubtitleWindowPosition();
                m_pSubtitleWindow->show();
                qDebug() << "立即显示字幕窗口";
            }
        } else {
            qDebug() << "加载新字幕文件失败";
        }
    }
}
