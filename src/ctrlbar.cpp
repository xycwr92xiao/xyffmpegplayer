/*
 * @file 	ctrlbar.cpp
 * @date 	2018/03/10 22:27
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	控制界面
 * @note
 */
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include "ctrlbar.h"
#include "ui_ctrlbar.h"
#include "playlist.h"
#include "globalhelper.h"
#include "transparentsubtitlewindow.h"
#include "videoctl.h"

CtrlBar::CtrlBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CtrlBar)
{
    ui->setupUi(this);
this->setContentsMargins(0, 0, 0, 1);  // 左边1像素间距
    m_dLastVolumePercent = 1.0;
    // 初始化倍速列表
        m_speedList = {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5};
}

CtrlBar::~CtrlBar()
{
    delete ui;
}

bool CtrlBar::Init()
{
    setStyleSheet(GlobalHelper::GetQssStr("://res/qss/ctrlbar.css"));

    // 设置斜杠标签样式
    ui->TimeSplitLabel->setStyleSheet(
        "QLabel {"
        "    color: white;"  // 设置文字颜色
        "    background: transparent;"  // 透明背景
        "    border: none;"  // 无边框
        "    font-size: 11px;"  // 设置字体大小
        "}"
    );

    GlobalHelper::SetIcon(ui->PlayOrPauseBtn, 12, QChar(0xf04b));
//    GlobalHelper::SetIcon(ui->StopBtn, 12, QChar(0xf04d));
    GlobalHelper::SetIcon(ui->Backward5Btn, 12, QChar(0xf04a));
    GlobalHelper::SetIcon(ui->Forward5Btn, 12, QChar(0xf04e));
    GlobalHelper::SetIcon(ui->VolumeBtn, 12, QChar(0xf028));
    GlobalHelper::SetIcon(ui->PlaylistCtrlBtn, 12, QChar(0xf03c));
    GlobalHelper::SetIcon(ui->ForwardBtn, 12, QChar(0xf051));
    GlobalHelper::SetIcon(ui->BackwardBtn, 12, QChar(0xf048));
    GlobalHelper::SetIcon(ui->FullScreenBtn, 12, QChar(0xf22e));//065

    UpdatePlayModeButton();
//    GlobalHelper::SetIcon(ui->speedBtn, 12, QChar(0xf013));
    ui->Backward5Btn->setToolTip("快退5秒");
    ui->Forward5Btn->setToolTip("快进5秒");

    ui->PlaylistCtrlBtn->setToolTip("播放列表");
    ui->FullScreenBtn->setToolTip("全屏");
    ui->VolumeBtn->setToolTip("静音");
    ui->ForwardBtn->setToolTip("下一个");
    ui->BackwardBtn->setToolTip("上一个");
//    ui->StopBtn->setToolTip("停止");
    ui->PlayOrPauseBtn->setToolTip("播放");
    ui->speedBtn->setToolTip("倍速");
    initSpeedMenu();
    ConnectSignalSlots();

    double dPercent = -1.0;
    GlobalHelper::GetPlayVolume(dPercent);
    if (dPercent != -1.0)
    {
        emit SigPlayVolume(dPercent);
        OnVideopVolume(dPercent);
    }
    ui->PlaySlider->setObjectName("PlaySlider");
    ui->VolumeSlider->setObjectName("VolumeSlider");
    return true;

}
void CtrlBar::UpdatePlayModeButton()
{
    int mode = GlobalVars::playMode();
    QString tooltip;
    QChar iconChar;

    switch (mode) {
    case PLAY_MODE_SEQUENCE:
        iconChar = QChar(0xf0c9);  // 下箭头
        tooltip = "顺序播放";
        break;
    case PLAY_MODE_REPEAT_ALL:
        iconChar = QChar(0xf021);  // 循环箭头
        tooltip = "循环全部";
        break;
    case PLAY_MODE_REPEAT_ONE:
        iconChar = QChar(0xf01e);  // 循环箭头
        tooltip = "循环单曲";
        break;
    case PLAY_MODE_RANDOM:
        iconChar = QChar(0xf074);  // 随机图标
        tooltip = "随机播放";
        break;
    default:
        iconChar = QChar(0xf0da);
        tooltip = "顺序播放";
        break;
    }

    GlobalHelper::SetIcon(ui->PlayModeBtn, 12, iconChar);
    ui->PlayModeBtn->setToolTip(tooltip);
}
bool CtrlBar::ConnectSignalSlots()
{
    QList<bool> listRet;


    connect(ui->PlaylistCtrlBtn, &QPushButton::clicked, this, &CtrlBar::SigShowOrHidePlaylist);
    connect(ui->PlaySlider, &CustomSlider::SigCustomSliderValueChanged, this, &CtrlBar::OnPlaySliderValueChanged);
    connect(ui->VolumeSlider, &CustomSlider::SigCustomSliderValueChanged, this, &CtrlBar::OnVolumeSliderValueChanged);
    connect(ui->BackwardBtn, &QPushButton::clicked, this, &CtrlBar::SigBackwardPlay);
    connect(ui->ForwardBtn, &QPushButton::clicked, this, &CtrlBar::SigForwardPlay);
    connect(ui->FullScreenBtn, &QPushButton::clicked, this, &CtrlBar::SigFullScreenBtn);
    // 连接新按钮
    ui->Forward5Btn->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->Backward5Btn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Backward5Btn, &QPushButton::customContextMenuRequested, this, &CtrlBar::on_Backward5Btn_rightClicked);
    connect(ui->Forward5Btn, &QPushButton::customContextMenuRequested, this, &CtrlBar::on_Forward5Btn_rightClicked);
    return true;
}

void CtrlBar::OnVideoTotalSeconds(int nSeconds)
{
    m_nTotalPlaySeconds = nSeconds;

    int thh, tmm, tss;
    thh = nSeconds / 3600;
    tmm = (nSeconds % 3600) / 60;
    tss = (nSeconds % 60);
    QTime TotalTime(thh, tmm, tss);

    ui->VideoTotalTimeTimeEdit->setTime(TotalTime);
}

void CtrlBar::OnVideopVolume(double dPercent)
{
    ui->VolumeSlider->setValue(dPercent * MAX_SLIDER_VALUE);
    m_dLastVolumePercent = dPercent;
    if (m_dLastVolumePercent == 0)
    {
        GlobalHelper::SetIcon(ui->VolumeBtn, 12, QChar(0xf026));
    }
    else
    {
        GlobalHelper::SetIcon(ui->VolumeBtn, 12, QChar(0xf028));
    }
    GlobalHelper::SavePlayVolume(dPercent);
}

void CtrlBar::on_PlayOrPauseBtn_clicked()
{
    qDebug() << "当前播放列表索引0：" << GlobalVars::currentPlayIndex();  // 这样读取
    qDebug() << "选中索引：" << GlobalVars::selectedIndex();
    qDebug() << "列表总数：" << GlobalVars::playlistCount();
    int tatolCount = GlobalVars::playlistCount();
    // 新增：检查是否有当前播放索引，如果没有则尝试播放选中的文件
        if (GlobalVars::currentPlayIndex() < 0 || GlobalVars::currentPlayIndex()> tatolCount-1)
        {
            // 没有当前播放项，尝试播放选中的项
            if (GlobalVars::selectedIndex() != -1 && tatolCount > 0)
            {
                qDebug() << "发出播放信号";
                GlobalVars::currentPlayIndex()=GlobalVars::selectedIndex();
                emit SigPlaySelected(GlobalVars::selectedIndex());
                return;
            }
            else if (tatolCount > 0)
            {
                // 没有选中项但有播放列表，播放第一项
                emit SigPlaySelected(0);
                return;
            }
        }
        else {
            qDebug() << "GlobalVars::runState()" << GlobalVars::runState();
            if (GlobalVars::runState()==0){
                emit SigPlaySelected(GlobalVars::selectedIndex());
            }
            else emit SigPlayOrPause();
        }
}

void CtrlBar::OnPauseStat(bool bPaused)
{
    if (bPaused)
    {
        GlobalHelper::SetIcon(ui->PlayOrPauseBtn, 12, QChar(0xf04b));
        ui->PlayOrPauseBtn->setToolTip("播放");
        GlobalVars::runState()=2;
    }
    else
    {
        GlobalHelper::SetIcon(ui->PlayOrPauseBtn, 12, QChar(0xf04c));
        ui->PlayOrPauseBtn->setToolTip("暂停");
        GlobalVars::runState()=1;
    }
    emit sigInfoMessage(GlobalVars::runState()==2 ? "已暂停" : "");
    qDebug() << "完成GlobalVars::runState():" << GlobalVars::runState();
}

void CtrlBar::OnStopFinished()
{
    ui->PlaySlider->setValue(0);
    QTime StopTime(0, 0, 0);
    ui->VideoTotalTimeTimeEdit->setTime(StopTime);
    ui->VideoPlayTimeTimeEdit->setTime(StopTime);
    GlobalHelper::SetIcon(ui->PlayOrPauseBtn, 12, QChar(0xf04b));
    ui->PlayOrPauseBtn->setToolTip("播放");
    //GlobalVars::currentPlayIndex() = -1;
}

void CtrlBar::OnSpeed(float speed)
{
    // 更新倍速按钮文本
            ui->speedBtn->setText(QString("倍速:%1x").arg(speed, 0, 'f', 2));

            // 更新菜单选中状态
            for (QAction* action : m_speedMenu->actions()) {
                float menuSpeed = action->data().toFloat();
                action->setChecked(qFuzzyCompare(menuSpeed, speed));
            }

}



void CtrlBar::OnVolumeSliderValueChanged()
{
    double dPercent = ui->VolumeSlider->value()*1.0 / ui->VolumeSlider->maximum();
    emit SigPlayVolume(dPercent);
    emit sigInfoMessage(QString("音量：%1 %").arg(QString::number(dPercent*100, 'f', 1)));
    OnVideopVolume(dPercent);
}



void CtrlBar::on_VolumeBtn_clicked()
{
    if (ui->VolumeBtn->text() == QChar(0xf028))
    {
        GlobalHelper::SetIcon(ui->VolumeBtn, 12, QChar(0xf026));
        ui->VolumeSlider->setValue(0);
        emit SigPlayVolume(0);
    }
    else
    {
        GlobalHelper::SetIcon(ui->VolumeBtn, 12, QChar(0xf028));
        ui->VolumeSlider->setValue(m_dLastVolumePercent * MAX_SLIDER_VALUE);
        emit SigPlayVolume(m_dLastVolumePercent);
    }

}
void CtrlBar::on_seekSecond(int nSeconds)
{
    //if (ui->PlaySlider->value() == 0)return;
    GlobalHelper::getIsSeeking() = 2;
    double currentPercent = ui->PlaySlider->value() * 1.0 / ui->PlaySlider->maximum();
    double targetVaue = currentPercent + (nSeconds * 1.0 / m_nTotalPlaySeconds);
        if (targetVaue < 0) targetVaue = 0;
        if (targetVaue > 1) targetVaue = 1;
        // 立即更新进度条到目标位置
    emit SigPlaySeek(targetVaue);
    QTimer::singleShot(50, this, []() {
            GlobalHelper::getIsSeeking() = 0;
    });
}
void CtrlBar::on_seekBySecond(double dSeconds)
{
    GlobalHelper::getIsSeeking() = 2;
    VideoCtl* vctl = VideoCtl::GetInstance();
        vctl-> OnPlaySeek(dSeconds);
            if (GlobalHelper::subtitleWindow()) {
                GlobalHelper::subtitleWindow()->setSubtitleText("");
            }
        if(GlobalVars::runState()==2){//暂停更新进度条位置
            double targetVaue = dSeconds * MAX_SLIDER_VALUE / m_nTotalPlaySeconds;
                if (targetVaue < 0) targetVaue = 0;
                if (targetVaue > MAX_SLIDER_VALUE)targetVaue = MAX_SLIDER_VALUE;
                // 立即更新进度条到目标位置
                ui->PlaySlider->blockSignals(true);
                ui->PlaySlider->setValue(int(targetVaue));
                ui->PlaySlider->blockSignals(false);
                int ndSeconds = (int)dSeconds;
                QTime targetTime(ndSeconds / 3600, (ndSeconds % 3600) / 60, ndSeconds % 60);
                ui->VideoPlayTimeTimeEdit->setTime(targetTime);
        }
        QTimer::singleShot(100, this, []() {
            GlobalHelper::getIsSeeking() = 0;
        });
}
void CtrlBar::on_seekByPercent(double nSeconds)
{
    GlobalHelper::getIsSeeking() = 2;
    double targetVaue = ui->PlaySlider->value() + nSeconds * MAX_SLIDER_VALUE / m_nTotalPlaySeconds;
        if (targetVaue < 0) targetVaue = 0;
        if (targetVaue > MAX_SLIDER_VALUE)targetVaue = MAX_SLIDER_VALUE;
        // 立即更新进度条到目标位置
        ui->PlaySlider->blockSignals(true);
        ui->PlaySlider->setValue(int(targetVaue));
        ui->PlaySlider->blockSignals(false);
        OnPlaySliderValueChanged();
}
// 添加新按钮的槽函数实现
void CtrlBar::on_Backward5Btn_rightClicked()
{
    double step =(GlobalHelper::isKeyFrameSparse() && GlobalVars::isVideoPlaying()) ? -10.0 : (VideoCtl::GetInstance()->m_bKeyFrameSparse>90?-2.0:-5.0);
    on_seekByPercent(step);
    emit sigInfoMessage(QString("快退: %1秒").arg(QString::number((0-step), 'f', 1)));
}
// 添加新按钮的槽函数实现

void CtrlBar::on_Backward5Btn_clicked()
{
    if (GlobalVars::isVideoPlaying() && GlobalHelper::isKeyFrameSparse()){
        VideoCtl* vctl = VideoCtl::GetInstance();
        double currentSec =m_nSeconds;//vctl->get_master_clock(vctl->m_CurStream);
        //if (std::isnan(currentSec))currentSec = m_nSeconds;
        qDebug() << " 快退，跳转到了 ：currentSec : "<< currentSec <<" m_nSeconds====" <<m_nSeconds;
        if (vctl->isKeyframeIndexReady()){
            double nextKey = vctl->getPreKeyframe(currentSec);
            m_nSeconds=nextKey;
            qDebug() << " 快退，跳转到了 ：currentSec : "<< currentSec << "nextKey :" << nextKey;
            emit sigInfoMessage(QString("快退(关键帧): %1秒").arg(QString::number((currentSec-nextKey), 'f', 1)));
            on_seekBySecond(nextKey);
            m_nSeconds=nextKey;
            return;
        }
    }
    on_seekByPercent(-10);
    emit sigInfoMessage("快退10秒");
}

void CtrlBar::on_Forward5Btn_rightClicked()
{
    if (GlobalVars::runState()==1){
        if (!GlobalVars::isVideoPlaying()){
            on_seekByPercent(10);
        } else if (GlobalHelper::isKeyFrameSparse())on_seekSecond(15);
        else {
            VideoCtl* vctl = VideoCtl::GetInstance();
           double currentSec = m_nSeconds;
          if (vctl->m_KeyFrameInfo.maxKeyFrameInterval<=2){
              double step=(int)(vctl->m_KeyFrameInfo.maxKeyFrameInterval+3.99);
              on_seekBySecond(currentSec+step);
              m_nSeconds=currentSec+step;//防止连续快进时，m_nSeconds没有更新，这里强制更新
              sigInfoMessage(QString("快进: %1秒").arg(QString::number((step), 'f', 1)));
              return;}
          else {on_seekByPercent(10);}
        }
    }
    else {if (GlobalVars::runState()==2)
            on_seekByPercent(10);}
    emit sigInfoMessage("快进10秒");
}

void CtrlBar::on_Forward5Btn_clicked()
{
    double step = GlobalHelper::isKeyFrameSparse() && GlobalVars::isVideoPlaying()  ? 11.0 : 5.0;
    if (GlobalVars::runState()==1 && GlobalVars::isVideoPlaying()){
          VideoCtl* vctl = VideoCtl::GetInstance();
          double currentSec = m_nSeconds;
        if (vctl->m_KeyFrameInfo.maxKeyFrameInterval<=2){
            step=(int)(vctl->m_KeyFrameInfo.maxKeyFrameInterval+1.99);
            on_seekBySecond(currentSec+step);
            m_nSeconds=currentSec+step;//防止连续快进时，m_nSeconds没有更新，这里强制更新
            sigInfoMessage(QString("快进: %1秒").arg(QString::number((step), 'f', 1)));
            //qDebug() << " 跳转到了 ：currentSec : "<< currentSec << "currentSec+step :" << currentSec+step << "MAX_SLIDER_VALUE=" << MAX_SLIDER_VALUE;
            return;}

            if (GlobalHelper::isKeyFrameSparse() && vctl->isKeyframeIndexReady()) {
                double nextKey = vctl->getNextKeyframeAfter(currentSec);
                if (nextKey > 0) {
                    // 精确跳到下一个关键帧
                    double targetSec = nextKey;
                    step = targetSec - currentSec;
                   // qDebug() << " 跳转到了 ：currentSec : "<< currentSec << "targetSec :" << targetSec << "MAX_SLIDER_VALUE=" << MAX_SLIDER_VALUE;
                     emit sigInfoMessage(QString("快进(关键帧): %1秒").arg(QString::number((targetSec-currentSec), 'f', 1)));
                     on_seekBySecond(targetSec);
                     return;
                }
            }else {//if (vctl->m_bKeyFrameSparse>90)step=2;
                    on_seekByPercent(step);
            }
    }
    else {if (GlobalVars::runState()==2 || !GlobalVars::isVideoPlaying())on_seekByPercent(5);}
    emit sigInfoMessage(QString("快进: %1秒").arg(QString::number((step), 'f', 1)));
}
void CtrlBar::OnPlaySliderValueChanged()
{
    double dPercent = ui->PlaySlider->value()*1.0 / ui->PlaySlider->maximum();
    int targetSeconds = dPercent * m_nTotalPlaySeconds;
        // 更新当前时间显示
    QTime targetTime(targetSeconds / 3600, (targetSeconds % 3600) / 60, targetSeconds % 60);
    ui->VideoPlayTimeTimeEdit->setTime(targetTime);
    emit SigPlaySeek(dPercent);
    //qDebug() << " 跳转到了 ："<< targetTime << "GlobalHelper::getIsSeeking()--" << GlobalHelper::getIsSeeking();
    // 通过全局变量清除字幕
        if (GlobalHelper::subtitleWindow()) {
            GlobalHelper::subtitleWindow()->setSubtitleText("");
        }
    QTimer::singleShot((GlobalHelper::getIsSeeking() == 3?300:50), this, []() {
        GlobalHelper::getIsSeeking() = 0;
    });
}
void CtrlBar::OnVideoPlaySeconds(double nSeconds)
{
    //if(nSeconds==-2147483648)return;

    if (GlobalHelper::getIsSeeking() == 0) {
     m_nSeconds = nSeconds;
    int thh, tmm, tss;
    thh = nSeconds / 3600;
    tmm = ((int)nSeconds % 3600) / 60;
    tss = ((int)nSeconds % 60);
    QTime TotalTime(thh, tmm, tss);

    ui->VideoPlayTimeTimeEdit->setTime(TotalTime);

    ui->PlaySlider->setValue((int)(nSeconds * 1.0 / m_nTotalPlaySeconds * MAX_SLIDER_VALUE));

    }
}
void CtrlBar::on_speedBtn_clicked()
{
    // 设置变速
    //emit SigSpeed();
    QPoint pos = ui->speedBtn->mapToGlobal(QPoint(0, ui->speedBtn->height()));
        m_speedMenu->exec(pos);
}

void CtrlBar::on_PlayModeBtn_clicked()
{
    // 切换播放模式
        int mode = GlobalVars::playMode();
        mode = (mode + 1) % 4;  // 四种模式循环
        GlobalVars::playMode() = mode;

        // 更新按钮图标
        UpdatePlayModeButton();

        // 保存播放模式
        //GlobalHelper::SavePlayMode(mode);

        // 发送模式改变信号
       // emit SigPlayModeChanged(mode);
}
void CtrlBar::OnPlayModeChanged(int mode)
{
    // 设置播放模式
    GlobalVars::playMode() = mode;

    // 更新按钮图标
    UpdatePlayModeButton();
}

// 添加初始化倍速菜单的函数
void CtrlBar::initSpeedMenu()
{
    m_speedMenu = new QMenu(this);
        m_speedMenu->setStyleSheet(
            "QMenu {"
            "    background-color: #2b2b2b;"
            "    border: 1px solid #555555;"
            "    padding: 2px;"
            "}"
            "QMenu::item {"
            "    color: #ffffff;"
            "    padding: 3px 15px;"
            "    margin: 0px 0px;"
            "    border-radius: 3px;" // 可选：添加圆角
            "}"
            "QMenu::item:selected {"
            "    background-color: #4a90e2;"
            "}"
            "QMenu::separator {"
            "    height: 1px;"
            "    background: #555555;"
            "    margin: 5px 0px;"
            "}"
        );

        // 添加倍速选项
        for (float speed : m_speedList) {
        QAction* action = m_speedMenu->addAction(QString("%1x").arg(speed, 0, 'f', 2));
        action->setData(speed);
        action->setCheckable(true); // 每个动作都设置为可选中
        if (qFuzzyCompare(speed, 1.0f)) {
        action->setChecked(true); // 默认选中1.0x
        }
        }
        // 使用 lambda 表达式连接信号
        connect(m_speedMenu, &QMenu::triggered, [this](QAction* action) {
            if (!action) return;

            float speed = action->data().toFloat();

            // 更新选中状态
            for (QAction* menuAction : m_speedMenu->actions()) {
                menuAction->setChecked(false);
            }
            action->setChecked(true);

            // 发射倍速改变信号（带参数）
            emit SigSpeed(speed);

            // 更新按钮显示
            ui->speedBtn->setText(QString("倍速:%1x").arg(speed, 0, 'f', int(speed*100)==int(speed*10)*10?1:2));
        });
}
// 添加倍速菜单触发槽函数
void CtrlBar::onSpeedActionTriggered(QAction* action)
{
    if (!action) return;

    float speed = action->data().toFloat();

    // 更新选中状态
    for (QAction* menuAction : m_speedMenu->actions()) {
        menuAction->setChecked(false);
    }
    action->setChecked(true);

    // 发射倍速改变信号
    emit SigSpeed(speed);

    // 更新按钮显示
    ui->speedBtn->setText(QString("倍速:%1x").arg(speed, 0, 'f', 2));
}

double CtrlBar::getCurrentVolume() const
{
    return ui->VolumeSlider->value() * 1.0 / ui->VolumeSlider->maximum();
}

void CtrlBar::setVolumeSliderValue(double percent)
{
    ui->VolumeSlider->setValue(percent * MAX_SLIDER_VALUE);
    // 触发音量变化信号
    OnVolumeSliderValueChanged();
}

void CtrlBar::toggleMute()
{
    // 模拟点击音量按钮
    on_VolumeBtn_clicked();
    ui->speedBtn->setText("静音");
}
