#ifndef GLOBALVARS_H
#define GLOBALVARS_H
#include <QString>
#include <QSettings>
#include <QDir>
#include <QColor>

enum PlayMode {
    PLAY_MODE_SEQUENCE = 0,    // 顺序播放
    PLAY_MODE_REPEAT_ALL = 1,  // 循环全部
    PLAY_MODE_REPEAT_ONE = 2,  // 循环单曲
    PLAY_MODE_RANDOM = 3       // 随机播放
};

class GlobalVars
{
public:
    static QString& subtitleFontFamily(){
       static QString subtitleFontFamily = "Microsoft YaHei";
        return subtitleFontFamily;
    }
    static int& subtitleFontSize(){
       static int subtitleFontSize = 16;
        return subtitleFontSize;
    }
    static QColor& subtitleTextColor() {
        static QColor color(Qt::white);
        return color;
    }

    static QColor& subtitleStrokeColor() {
        static QColor color(Qt::black);
        return color;
    }

    static QColor& subtitleHoverBgColor() {
        static QColor color(0, 0, 0, 150);  // 半透明黑色
        return color;
    }

    static QColor& subtitleLeaveBgColor() {
        static QColor color(0, 0, 0, 100);  // 浅透明黑色
        return color;
    }

    static bool& subtitleKeepBackground() {
        static bool keep = false;
        return keep;
    }
    // 获取当前播放索引的引用
    static int& currentPlayIndex()
    {
        static int s_nCurrentPlayIndex = -1;
        return s_nCurrentPlayIndex;
    }
    static QString& currentPlayFileName()
    {
        static QString s_currentPlayFileName = "";
        return s_currentPlayFileName;
    }
    static double& currentPlaytime()
    {
        static double s_currentPlaytime = 0;
        return s_currentPlaytime;
    }
    static bool& isRenameing()
    {
        static bool m_isRenameing = false;
        return m_isRenameing;
    }
    static QString& getMediaFileType()
    {
        static QString filter =
                "所有媒体文件(*.mkv *.rmvb *.rm *.mp4 *.avi *.flv *.wmv *.mpg *.mov *.3gp *.mp3 *.wav *.aac *.flac *.m4a *.wma *.ape *.tta);;"
                        "视频文件(*.mkv *.rm *.rmvb *.mp4 *.avi *.flv *.wmv *.mpg *.mov *.3gp);;"
                        "音频文件(*.mp3 *.wav *.aac *.flac *.m4a *.wma *.ape *.tta)";
        return filter;
    }
    // 选中索引（用户当前选中的项，高亮显示）- 新增
    static int& selectedIndex()
    {
        static int s_nSelectedIndex = -1;
        return s_nSelectedIndex;
    }
    // 播放列表总项数（可选）- 新增
    static int& playlistCount()
    {
        static int s_nPlaylistCount = 0;
        return s_nPlaylistCount;
    }
    // 播放状态- 0:停止，1：播放 2：暂停
        static int& runState()
        {
            static int m_run = 0;
            return m_run;
        }
        static int& getWinState()
        {
            static int m_WinState = 0;//0:窗口，1：最大化 2:最小化,3:全屏后点击了最小化,4:全屏
            return m_WinState;
        }
        // 播放模式
            static int& playMode()
            {
                static int s_nPlayMode = 0; // 默认顺序播放
                return s_nPlayMode;
            }
            static bool& getFullScreen()
            {
                static bool isFullScreen = false; // 默认顺序播放
                return isFullScreen;
            }
            static int& getExtend()
            {
                static int isExtend = 0; // 默认顺序播放
                return isExtend;
            }
            // 音量百分比
            static double& volumePercent()
            {
                static double s_dVolumePercent = 0.7; // 默认80%音量
                return s_dVolumePercent;
            }

            // 自动播放标志
            static bool& autoPlay()
            {
                static bool s_bAutoPlay = true;
                return s_bAutoPlay;
            }
            // 自动播放标志
            static bool& isVideoPlaying()
            {
                static bool isVideo = false;
                return isVideo;
            }
        // 重置所有状态（可选）- 新增
        static void resetAll()
        {
            currentPlayIndex() = -1;
            selectedIndex() = -1;
            playlistCount() = 0;
            runState() = 0;
            playMode() = 0; // 重置为顺序播放
            volumePercent() = 0.7; // 重置为默认音量
            autoPlay() = true; // 重置为自动播放
            getFullScreen() = false;
        }
        // 获取上次打开的目录
        static QString GetLastOpenDir(int dirType = 0) {
            QSettings settings("MyPlayer", "MediaPlayer");
            QString lastDir = settings.value("LastOpenDir", QDir::homePath()).toString();
            QString lastCoverDir = settings.value("LastCoverDir", QDir::homePath()).toString();
            // 检查目录是否存在
            if (dirType == 0){
                QDir dir(lastDir);
                if (!dir.exists()) {
                    return QDir::homePath();
                }
                return lastDir;
            }else {
                QDir dir(lastCoverDir);
                if (!dir.exists()) {
                    return QDir::homePath();
                }
                return lastCoverDir;
            }
        }
        // 保存上次打开的目录SaveLastOpenDir
        static void SaveLastOpenDir(const QString& dir,int dirType=0) {
            QSettings settings("MyPlayer", "MediaPlayer");
            if(dirType ==0 )settings.setValue("LastOpenDir", dir);
            else settings.setValue("LastCoverDir", dir);
        }
        static bool IsMusic(const QString& fileName) {
            QString lowerName = fileName.toLower();

            static QStringList audioFormats = {
                ".mp3", ".wav", ".aac", ".flac", ".m4a", ".wma", ".ape",
                ".ogg", ".opus", ".wv", ".dts",
                ".tta"
            };

            for (const QString& format : audioFormats) {
                if (lowerName.endsWith(format)) {
                    return true;
                }
            }

            return false;
        }
};

#endif // GLOBALVARS_H
