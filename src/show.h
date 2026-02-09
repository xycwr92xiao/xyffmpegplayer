/*
 * @file 	displaywid.h
 * @date 	2018/01/07 11:11
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	显示控件
 * @note
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <QWidget>
#include <QMimeData>
#include <QDebug>
#include <QTimer>
#include <QDragEnterEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QActionGroup>
#include <QAction>
#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>
#include "TransparentSubtitleWindow.h"
#include "videoctl.h"
#include "subtitle.h"

namespace Ui {
class Show;
}

class Show : public QWidget
{
    Q_OBJECT

public:
    Show(QWidget *parent);
    ~Show();
	/**
	 * @brief	初始化
	 */
	bool Init();
    void updateSubtitleWindowPosition();
protected:
	/**
	 * @brief	放下事件
	 * 
	 * @param	event 事件指针
	 * @note 	
	 */
    void dropEvent(QDropEvent *event) override;
	/**
	 * @brief	拖动事件
	 *
	 * @param	event 事件指针
	 * @note
	 */
    void dragEnterEvent(QDragEnterEvent *event) override;
    /**
     * @brief	窗口大小变化事件
     * 
     * @param	event 事件指针
     * @note
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief	按键事件
     * 
     * @param	
     * @return	
     * @note 	
     */
    void keyReleaseEvent(QKeyEvent *event) override;


    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    //void contextMenuEvent(QContextMenuEvent* event);
    void mouseDoubleClickEvent(QMouseEvent *event) override;
public:
    /**
    * @brief	播放
    *
    * @param	strFile 文件名
    * @note
    */
    void OnPlay(QString strFile);
    void OnStopFinished();

    /**
     * @brief	调整显示画面的宽高，使画面保持原比例
     *
     * @param	nFrameWidth 宽
     * @param	nFrameHeight 高
     * @return
     * @note
     */
    void OnFrameDimensionsChanged(int nFrameWidth, int nFrameHeight);
    void ChangeShow();
public slots:
    void OnAudioSpectrumData(float* data, int bands);
    void SetAudioMode(bool audioMode);
    void setSubtitleFont(const QString& fontFamily, int fontSize);
    void OnSubtitleColorSettingsChanged(const QColor& textColor, const QColor& strokeColor,
                                           const QColor& hoverBgColor, const QColor& leaveBgColor,
                                           bool keepBackground);
    void OnLyricDownloaded(const QString& lrcPath);  // 新增：处理歌词下载完成信号
private:
    QVector<float> m_spectrumData;
    int m_spectrumBands;
    bool m_bAudioMode;
    bool m_showSpectrum;
    bool isReSizeWin = false;
	/**
	 * @brief	显示信息
	 * 
	 * @param	strMsg 信息内容
	 * @note 	
	 */
	void OnDisplayMsg(QString strMsg);


    void OnTimerShowCursorUpdate();

    void OnActionsTriggered(QAction *action);
protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
	/**
	 * @brief	连接信号槽	
	 */
	bool ConnectSignalSlots();

    // 字幕相关
        Subtitle* m_pSubtitle;
        QLabel* m_pSubtitleLabel;
        TransparentSubtitleWindow* m_pSubtitleWindow;

        QTimer* m_pSubtitleTimer;
        int m_nCurrentPlayTimeMs;
        QString m_currentSubtitleText;
     // 音频信息显示相关
        QLabel* m_pAudioInfoLabel;
        QLabel* m_pCoverLabel;
        QWidget* m_pAudioInfoWidget;
        QWidget* m_pAudioTextLabel;
        QHBoxLayout* m_pAudioInfoLayout;

        // 频谱最小高度
        const int m_nSpectrumMinHeight = 20;

        // 音频信息区域高度
        const int m_nAudioInfoHeight = 220;

        // 更新频谱显示区域
        void updateSpectrumDisplayArea();

        // 绘制音频信息
        void drawAudioInfo(QPainter& painter);
        void loadDefaultCover();
private slots:
    void OnUpdateSubtitle();
    void OnSubtitleChanged(const QString& text);
    void OnAudioMetadataReceived(const QString& title, const QString& artist,
                                 const QString& album, const QString& genre,
                                 int year, int bitrate, int sampleRate);
    void OnAudioCoverReceived(const QPixmap& cover);
signals:
    void SigSubtitleTextChanged(const QString& text);
    void SigVideoSizeChanged(int width, int height);  // 新增：视频尺寸变化信号
    void SigOpenFile(QString strFileName);///< 增加视频文件
    void SigAddFilesAndPlay(QStringList strFileName,int index);///< 增加视频文件
	void SigPlay(QString strFile); ///<播放
	void SigFullScreen();//全屏播放
    void SigPlayOrPause();
    void SigStop();
    void SigShowMenu();
    void MinBtnClicked();
    void SigSeekForward();
    void SigSeekBack();
    void SigAddVolume();
    void SigSubVolume();

private:
    Ui::Show *ui;

    int m_nLastFrameWidth; ///< 记录视频宽高
    int m_nLastFrameHeight;

    QTimer timerShowCursor;

    QMenu m_stMenu;
    QActionGroup m_stActionGroup;
    // 添加拖动相关变量
        bool m_bDrag;           ///< 拖动标志
        QPoint m_DragStartPos;  ///< 拖动起始位置
        QPoint m_WindowStartPos; ///< 窗口起始位置
        //添加封面相关
        QString m_strCurrentAudioFile;  // 当前音频文件路径
        // 双击封面处理函数
        void onCoverLabelDoubleClicked();

        // 保存封面到音频文件
        bool saveCoverToAudioFile(const QString& audioFile, const QString& coverImageFile);

        // 获取支持的音频格式
        QString getSupportedAudioFormats();
};

#endif // DISPLAY_H
