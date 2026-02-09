/*
 * @file 	globalhelper.h
 * @date 	2018/01/07 10:41
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	公共接口
 * @note
 */
#ifndef GLOBALHELPER_H
#define GLOBALHELPER_H

#pragma execution_character_set("utf-8")

enum ERROR_CODE
{
    NoError = 0,
    ErrorFileInvalid
};



#include <QString>
#include <QPushButton>
#include <QDebug>
#include <QStringList>
class TransparentSubtitleWindow; // 前向声明
class GlobalHelper      // 工具类
{
public:
    static bool& isKeyFrameSparse()
    {
        static bool m_isKeyFrameSparse = false;
        return m_isKeyFrameSparse;
    }
    static bool& isSubtitleVisible()
    {
        static bool m_bisSubtitleVisible = true;
        return m_bisSubtitleVisible;
    }
    static TransparentSubtitleWindow*& subtitleWindow()
        {
            static TransparentSubtitleWindow* s_subtitleWindow = nullptr;
            return s_subtitleWindow;
        }
    static bool& haveCommandLine()
    {
        static bool m_haveCommandLine = false;
        return m_haveCommandLine;
    }
    static int& getIsSeeking()
    {
        static int m_bIsSeeking = 0;
        return m_bIsSeeking;
    }
    GlobalHelper();
    /**
     * 获取样式表
     *
     * @param	strQssPath 样式表文件路径
     * @return	样式表
     * @note
     */
    static QString GetQssStr(QString strQssPath);

    /**
     * 为按钮设置显示图标
     *
     * @param	btn 按钮指针
     * @param	iconSize 图标大小
     * @param	icon 图标字符
     */
    static void SetIcon(QPushButton* btn, int iconSize, QChar icon);


    static void SavePlaylist(QStringList& playList);    // 保存播放列表
    static void GetPlaylist(QStringList& playList);     // 获取播放列表
    static void SavePlayVolume(double& nVolume);        // 保存音量
    static void GetPlayVolume(double& nVolume);         // 获取音量
    static void SavePlayIndex(int index);
    static int GetPlayIndex();
    static void SavePlayMode(int mode);
    static int GetPlayMode();
    static void SaveAutoPlay(bool autoPlay);
    static bool GetAutoPlay();
    static bool IsSupportedMedia(const QString& fileName) {
            QString lowerName = fileName.toLower();
            bool isSupportedMovie = IsMusic(lowerName);
            // 视频格式
            if (isSupportedMovie){
                return true;
            }
            bool isSupportedVideo = IsVideo(lowerName);
            // 视频格式
            if (isSupportedVideo){
                return true;
            }
            return false;
        }

        static bool IsMusic(const QString& fileName) {
            QString lowerName = fileName.toLower();

            static QStringList audioFormats = {
                ".mp3", ".wav", ".aac", ".flac", ".m4a", ".wma", ".ape",
//                ".ogg", ".opus", ".wv", ".dts",
                ".tta"
            };

            for (const QString& format : audioFormats) {
                if (lowerName.endsWith(format)) {
                    return true;
                }
            }

            return false;
        }

        static bool IsVideo(const QString& fileName) {
            QString lowerName = fileName.toLower();

            static QStringList videoFormats = {
                ".mkv", ".rmvb", ".rm", ".mp4", ".avi", ".flv", ".wmv", ".mov",".mpg", ".3gp", ".vob", ".dat", ".bin", ".mts"
            };

            for (const QString& format : videoFormats) {
                if (lowerName.endsWith(format)) {
                    return true;
                }
            }

            return false;
        }

    static QString GetAppVersion();
    //列表新增时长功能
    static qint64 GetMediaDuration(const QString& filePath);
    static QString FormatDuration(qint64 duration); // 格式化为 HH:MM:SS
    static QIcon GetFormatIcon(const QString& filePath, int size = 24); // 根据文件格式获取图标
    static QPixmap createColoredSvg(const QString& svgPath, const QColor& color, QSize size);
    static QIcon createMenuIcon(const QString& svgPath,
                                      const QColor& normalColor,
                                      const QColor& hoverColor,
                                      const QColor& selectedColor,const QColor& playingColor,const int size);
    static QIcon createButtonIcon(const QString& svgPath,
                                      const QColor& normalColor,
                                      const QColor& hoverColor,
                                      const QColor& pressedColor,
                                      const int size);
};

//必须加以下内容,否则编译不能通过,为了兼容C和C99标准
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

extern "C"{
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
#include "SDL.h"
}




#define MAX_SLIDER_VALUE 65536



#endif // GLOBALHELPER_H
