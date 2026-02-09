/*
 * @file 	videoctl.h
 * @date 	2018/01/07 10:48
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	视频控制类
 * @note
 */
#ifndef VIDEOCTL_H
#define VIDEOCTL_H

#include <QObject>
#include <QThread>
#include <QString>
#include <vector>
#include <complex>
#include <QList>

#include "globalhelper.h"
#include "datactl.h"
#include "sonic.h"

#define FFP_PROP_FLOAT_PLAYBACK_RATE                    10003       // 设置播放速率
#define FFP_PROP_FLOAT_PLAYBACK_VOLUME                  10006

#define PLAYBACK_RATE_MIN           0.25     // 最慢
#define PLAYBACK_RATE_MAX           3.0     // 最快
#define PLAYBACK_RATE_SCALE         0.25    // 变速刻度
//单例模式
struct KeyFrameAnalysis {
    bool isKeyFrameSparse;      // 是否关键帧稀少
    double avgKeyFrameInterval; // 平均关键帧间隔(秒)
    double maxKeyFrameInterval; // 最大关键帧间隔
    int totalKeyFrames;         // 关键帧总数
};
class VideoCtl : public QObject
{
    Q_OBJECT



public:
    KeyFrameAnalysis analyzeKeyFrameDistribution(AVFormatContext* fmt_ctx, int video_stream);
        bool m_bKeyFrameSparse;     // 标记当前视频关键帧是否稀少
        KeyFrameAnalysis m_KeyFrameInfo;
    //explicit VideoCtl(QObject *parent = nullptr);

    static VideoCtl* GetInstance();
    ~VideoCtl();
    /**
    * @brief	开始播放
    *
    * @param	strFileName 文件完整路径
    * @param	widPlayWid 播放窗口id
    * @return	true 成功 false 失败
    * @note
    */
    bool StartPlay(QString strFileName, WId widPlayWid);
    int audio_decode_frame(VideoState *is);
    void update_sample_display(VideoState *is, short *samples, int samples_size);
    void set_clock_at(Clock *c, double pts, int serial, double time);
    void sync_clock_to_slave(Clock *c, Clock *slave);

    float     ffp_get_property_float(int id, float default_value);
    void      ffp_set_property_float(int id, float value);
    int getFileType(const QString& strFileName);
//    int64_t   ffp_get_property_int64(int id, int64_t default_value);
//    void      ffp_set_property_int64(int id, int64_t value);
    void StepFrameForward();      // 向前步进一帧
    void StepFrameBackward();     // 向后步进一帧
    void SetFrameStepMode(bool enabled);  // 设置逐帧播放模式
signals:
    void SigSpeed(float speed);
    void SigPlayMsg(QString strMsg);//< 错误信息
    void SigFrameDimensionsChanged(int nFrameWidth, int nFrameHeight); //<视频宽高发生变化
    void SigFrameStep(bool forward);  // 帧步进信号
    void SigVideoTotalSeconds(int nSeconds);
    void SigVideoPlaySeconds(int nSeconds);

    void SigVideoVolume(double dPercent);
    void SigPauseStat(bool bPaused);
    void SigCloseWin();
    void SigStop();
    void SigStopFinished();//停止播放完成

    void SigStartPlay(QString strFileName);
    void SigAudioInfo(QString info);  // 音频信息
       void SigAudioBitrate(int bitrate); // 音频比特率
       void SigAudioSampleRate(int rate); // 采样率
       void SigAudioChannels(int channels); // 声道数
    void SigAudioSpectrumData(float* data, int bands); // 频谱数据信号
    //void SigPlayFinished();  // 播放完成信号
    void SigAudioMetadataReceived(const QString& title, const QString& artist,
                                  const QString& album, const QString& genre,
                                  int year, int bitrate, int sampleRate);
    void SigAudioCoverReceived(const QPixmap& cover);
public:
    void SetVideoDisplaySize(int width, int height);
    void OnSpeed(float speed);
    void OnPlaySeek(double dPercent);
    void OnPlayVolume(double dPercent);

    void OnAddVolume();
    void OnSubVolume();
    void OnPause();
    void OnStop();
    // 均衡器设置接口
        void setEqualizer(const QList<int>& gains, int balance, bool enabled);
        void setEqualizerEnabled(bool enabled);
        int getCurrentPlayTimeMs(); // 新增：获取当前播放时间（毫秒）
public slots:
    void SetAudioOnlyMode(bool audioOnly); // 设置纯音频模式
    void onGetPlayPosition();  // 获取当前播放位置
    void onSetPlayPosition();  // 设置播放位置
    void runFadeProc(int optionYype);
private:
    bool m_bFrameStepMode =false;        // 逐帧播放模式标志
    bool m_FrameSeek = false;
    double m_savedVolume =0.7;         // 保存的音量值
    double m_savedPosition =0;       // 进入逐帧模式时的位置
    QTimer* m_fadeTimer = nullptr;
    int m_lastVolume = 0;
    int m_fadeInterval= 5;
    int m_runTimes = 0;
    int finishOper = 0;
    void onFadeTimer();
    // 添加成员函数读取音频元数据
        void extractAudioMetadata(AVFormatContext* formatCtx);
        QPixmap extractAlbumArt(AVFormatContext* formatCtx);
    explicit VideoCtl(QObject *parent = nullptr);
    /**
     * @brief	初始化
     *
     * @return	true 成功 false 失败
     * @note
     */
    bool Init();

    /**
     * @brief	连接信号槽
     *
     * @return	true 成功 false 失败
     * @note
     */
    bool ConnectSignalSlots();

    int get_video_frame(VideoState *is, AVFrame *frame);
    int audio_thread(void *arg);
    int video_thread(void *arg);
    int subtitle_thread(void *arg);

    int synchronize_audio(VideoState *is, int nb_samples);

    int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params);
    int stream_component_open(VideoState *is, int stream_index);
    int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue);
    int is_realtime(AVFormatContext *s);
    void ReadThread(VideoState *CurStream);
    void LoopThread(VideoState *CurStream);
    VideoState *stream_open(const char *filename);

    void stream_cycle_channel(VideoState *is, int codec_type);
    void refresh_loop_wait_event(VideoState *is, SDL_Event *event);
    void seek_chapter(VideoState *is, int incr);
    void video_refresh(void *opaque, double *remaining_time);
    int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
    //更新音量
    void UpdateVolume(int sign, double step);

    void video_display(VideoState *is);
    int video_open(VideoState *is);
    void do_exit(VideoState* &is);

    int realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width, int new_height, SDL_BlendMode blendmode, int init_texture);
    void calculate_display_rect(SDL_Rect *rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height, int pic_width, int pic_height, AVRational pic_sar);
    int upload_texture(SDL_Texture *tex, AVFrame *frame, struct SwsContext **img_convert_ctx);
    void video_image_display(VideoState *is);
    void stream_component_close(VideoState *is, int stream_index);
    void stream_close(VideoState *is);
    double get_clock(Clock *c);

    void set_clock(Clock *c, double pts, int serial);
    void set_clock_speed(Clock *c, double speed);
    void init_clock(Clock *c, int *queue_serial);
    
    int get_master_sync_type(VideoState *is);
    double get_master_clock(VideoState *is);
    void check_external_clock_speed(VideoState *is);
    void stream_seek(VideoState *is, int64_t pos, int64_t rel);
    void stream_toggle_pause(VideoState *is);
    void toggle_pause(VideoState *is);
    void step_to_next_frame(VideoState *is);
    double compute_target_delay(double delay, VideoState *is);
    double vp_duration(VideoState *is, Frame *vp, Frame *nextvp);
    void update_video_pts(VideoState *is, double pts, int64_t pos, int serial);
    // 均衡器相关
        bool m_eqEnabled;
        int m_balance;  // -100 to 100
        std::vector<float> m_eqGains;  // 10段增益 (dB)
        std::vector<std::vector<float>> m_eqCoefficients;  // 二阶滤波器系数，每个频段5个系数

        // 更新滤波器系数
        void updateEqualizerCoefficients(int sampleRate);
        // 双二阶滤波器
        struct BiquadFilter {
            float b0, b1, b2, a1, a2;
            float x1, x2, y1, y2;

            BiquadFilter() : b0(1), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}

            float process(float x) {
                float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1;
                x1 = x;
                y2 = y1;
                y1 = y;
                return y;
            }

            void reset() {
                x1 = x2 = y1 = y2 = 0;
            }
        };

        std::vector<std::vector<BiquadFilter>> m_eqFilterInstances;  // 每个声道每个频段一个滤波器

public:
        // 声道平衡计算
            void applyBalance(float* audioData, int samples, int channels);

            // 均衡器处理
            void applyEqualizer(float* audioData, int samples, int channels, int sampleRate);
    void ffp_set_playback_rate(float rate);
    float ffp_get_playback_rate();

    int ffp_get_playback_rate_change();
    void ffp_set_playback_rate_change(int change);

    int64_t get_target_frequency();
    int     get_target_channels();
    int   is_normal_playback_rate();
    bool m_bAudioOnly;  // 纯音频播放标志
    bool m_bNeedUpdateSpectrum; // 需要更新频谱标志
    void calculateSpectrum(const float* audioData, int samples);
private:
        int m_nSpectrumBands; // 频谱分段数量
        float* m_pSpectrumData; // 频谱数据
        SDL_mutex* m_pSpectrumMutex; // 频谱数据互斥锁
        SDL_Texture* m_pSpectrumTexture; // 频谱纹理

        int m_nAudioChannels; // 音频通道数
        int m_nSampleRate; // 采样率
    QWidget* m_pAudioVisualizer; // 音频可视化控件
    // 频谱计算相关

        void updateSpectrumTexture();
        void drawSpectrum();

    static VideoCtl* m_pInstance; //< 单例指针

    bool m_bInited;	//< 初始化标志
    bool m_bPlayLoop; //刷新循环标志

    VideoState* m_CurStream;

    SDL_Window *window;
    SDL_Renderer *renderer;
    WId play_wid;//播放窗口


    /* options specified by the user */
    int screen_width;
    int screen_height;
    int startup_volume;

    //播放刷新循环线程
    std::thread m_tPlayLoopThread;

    int m_nFrameW;
    int m_nFrameH;



    float       pf_playback_rate;           // 播放速率
    int         pf_playback_rate_changed;   // 播放速率改变
public:
    // 变速相关
    sonicStreamStruct *audio_speed_convert;
};

#endif // VIDEOCTL_H
