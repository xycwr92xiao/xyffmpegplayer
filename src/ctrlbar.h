/*
 * @file 	ctrlbar.h
 * @date 	2018/01/07 10:46
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	控制面板界面
 * @note
 */
#ifndef CTRLBAR_H
#define CTRLBAR_H

#include <QWidget>
#include "CustomSlider.h"
#include <QMenu>

namespace Ui {
class CtrlBar;
}

class CtrlBar : public QWidget
{
    Q_OBJECT

public:
    explicit CtrlBar(QWidget *parent = 0);
    ~CtrlBar();
	/**
	 * @brief	初始化UI
	 * 
	 * @return	true 成功 false 失败
	 * @note 	
	 */
    bool Init();

public:
    void OnVideoTotalSeconds(int nSeconds);
    void OnVideoPlaySeconds(int nSeconds);
    void OnVideopVolume(double dPercent);
    void OnPauseStat(bool bPaused);
    void OnStopFinished();
    void OnSpeed(float speed);
    void UpdatePlayModeButton();
    void toggleMute(); // 切换静音状态
        double getCurrentVolume() const; // 获取当前音量
        void setVolumeSliderValue(double percent); // 设置音量条值
public slots:
        void OnPlayModeChanged(int mode);
        //放在slots中将与按钮自动连接
        void on_Backward5Btn_clicked();   // 快退5秒
        void on_Forward5Btn_clicked();    // 快进5秒
        void on_Forward5Btn_rightClicked();
        void on_Backward5Btn_rightClicked();
private:
        void on_seekSecond(int nSeconds);
        void on_seekByPercent(double nSeconds);
    void OnPlaySliderValueChanged();
    void OnVolumeSliderValueChanged();
    void initSpeedMenu();
    QMenu* m_speedMenu;  // 倍速菜单
    QList<float> m_speedList;  // 倍速列表

private slots:
    void onSpeedActionTriggered(QAction* action);  // 添加参数
    void on_PlayOrPauseBtn_clicked();
    void on_VolumeBtn_clicked();
//    void on_StopBtn_clicked();


    /**
    * @brief	连接信号槽
    *
    * @param
    * @return
    * @note
    */
    bool ConnectSignalSlots();
    void on_speedBtn_clicked();

    void on_PlayModeBtn_clicked();

signals:
    void SigShowOrHidePlaylist();	//< 显示或隐藏信号
    void SigPlaySeek(double dPercent); ///< 调整播放进度
    void SigPlayVolume(double dPercent);
    void SigPlayOrPause();
//    void SigStop();
    void SigForwardPlay();
    void SigBackwardPlay();
    void SigShowMenu();
    void SigShowSetting();
    void SigSpeed(float speed);
    void SigPlaySelected(int nIndex);  // 新增：播放指定索引的文件
    void SigFullScreenBtn();  //全屏信号
private:
    Ui::CtrlBar *ui;
    int m_nTotalPlaySeconds;
    double m_dLastVolumePercent;
    double m_nSeconds=0;
};

#endif // CTRLBAR_H
