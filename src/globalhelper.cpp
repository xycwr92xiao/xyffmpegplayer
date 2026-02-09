#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QSvgRenderer>
#include <QRegularExpression>
#include <QPainter>

#include "globalhelper.h"
#include "globalvars.h"

const QString PLAYER_CONFIG_BASEDIR = QDir::tempPath();

const QString PLAYER_CONFIG = "player_config.ini";

const QString APP_VERSION = "0.1.0";

GlobalHelper::GlobalHelper()
{

}

QString GlobalHelper::GetQssStr(QString strQssPath)
{
    QString strQss;
    QFile FileQss(strQssPath);
    if (FileQss.open(QIODevice::ReadOnly))
    {
        strQss = FileQss.readAll();
        FileQss.close();
    }
    else
    {
        qDebug() << "读取样式表失败" << strQssPath;
    }
    return strQss;
}

void GlobalHelper::SetIcon(QPushButton* btn, int iconSize, QChar icon)
{
    QFont font;
    font.setFamily("FontAwesome");
    font.setPointSize(iconSize);

    btn->setFont(font);
    btn->setText(icon);
}

void GlobalHelper::SavePlaylist(QStringList& playList)
{
    //QString strPlayerConfigFileName = QCoreApplication::applicationDirPath() + QDir::separator() + PLAYER_CONFIG;
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);
    settings.beginWriteArray("playlist");
    for (int i = 0; i < playList.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("movie", playList.at(i));
    }
    settings.endArray();
}

void GlobalHelper::GetPlaylist(QStringList& playList)
{
    //QString strPlayerConfigFileName = QCoreApplication::applicationDirPath() + QDir::separator() + PLAYER_CONFIG;
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);

    int size = settings.beginReadArray("playlist");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        playList.append(settings.value("movie").toString());
    }
    settings.endArray();
}

void GlobalHelper::SavePlayVolume(double& nVolume)
{
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);
    settings.setValue("volume/size", nVolume);
}

void GlobalHelper::GetPlayVolume(double& nVolume)
{
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);
    QString str = settings.value("volume/size").toString();
    nVolume = settings.value("volume/size", nVolume).toDouble();
}

QString GlobalHelper::GetAppVersion()
{
    return APP_VERSION;
}

void GlobalHelper::SavePlayIndex(int index)
{
    QSettings settings("MyCompany", "MyPlayer");
    settings.setValue("PlayIndex", index);
}

int GlobalHelper::GetPlayIndex()
{
    QSettings settings("MyCompany", "MyPlayer");
    return settings.value("PlayIndex", -1).toInt();
}

void GlobalHelper::SavePlayMode(int mode)
{
    QSettings settings("MyCompany", "MyPlayer");
    settings.setValue("PlayMode", mode);
}

int GlobalHelper::GetPlayMode()
{
    QSettings settings("MyCompany", "MyPlayer");
    return settings.value("PlayMode", PLAY_MODE_SEQUENCE).toInt();
}

void GlobalHelper::SaveAutoPlay(bool autoPlay)
{
    QSettings settings("MyCompany", "MyPlayer");
    settings.setValue("AutoPlay", autoPlay);
}

bool GlobalHelper::GetAutoPlay()
{
    QSettings settings("MyCompany", "MyPlayer");
    return settings.value("AutoPlay", true).toBool();
}
//----------------------------------------------------------以下到结束是增加时长的函数
qint64 GlobalHelper::GetMediaDuration(const QString& filePath)
{
    qint64 duration = 0;

    avformat_network_init();
    AVFormatContext* formatContext = nullptr;

    // 打开媒体文件
    if (avformat_open_input(&formatContext, filePath.toLocal8Bit().constData(), nullptr, nullptr) == 0) {
        // 获取流信息
        if (avformat_find_stream_info(formatContext, nullptr) >= 0) {
            // 计算总时长（毫秒）
            if (formatContext->duration != AV_NOPTS_VALUE) {
                duration = formatContext->duration / 1000; // 转换为毫秒
            } else {
                // 如果AVFormatContext中没有duration，则计算所有流的时长
                for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
                    if (formatContext->streams[i]->duration != AV_NOPTS_VALUE) {
                        qint64 streamDuration = formatContext->streams[i]->duration *
                                               av_q2d(formatContext->streams[i]->time_base) * 1000;
                        duration = qMax(duration, streamDuration);
                    }
                }
            }
        }
        avformat_close_input(&formatContext);
    }

    avformat_network_deinit();
    return duration;
}

QString GlobalHelper::FormatDuration(qint64 durationMs)
{
    if (durationMs <= 0) return "--:--";

    qint64 totalSeconds = durationMs / 1000;
    qint64 hours = totalSeconds / 3600;
    qint64 minutes = (totalSeconds % 3600) / 60;
    qint64 seconds = totalSeconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
}

QIcon GlobalHelper::GetFormatIcon(const QString& filePath, int size)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    // 根据文件后缀返回对应的图标
    // 使用统一的颜色方案
        QColor normalAudioColor = QColor("#aaaaaa");  // 正常颜色
        QColor hoverAudioColor = QColor("#efefef");   // 悬停颜色
        QColor selectedAudioColor = QColor("#ffffff"); // 选中颜色
        QColor playingAudioColor = QColor("#00ffff"); // 选中颜色

        QColor normalVideoColor = QColor("#eeb508");  // 正常颜色
        QColor hoverVideoColor = QColor("#e1e73c");   // 悬停颜色
        QColor selectedVideoColor = QColor("#feef02"); // 选中颜色
        QColor playingVideoColor = QColor("#00ffff"); // 选中颜色
        //int size = GlobalVars::getFullScreen() ? 24 : 16;
        // 根据文件后缀返回对应的图标
        if (suffix == "mp4" || suffix == "m4v") {
            return createMenuIcon(":/res/icons/mp4.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "mp3") {
            return createMenuIcon(":/res/icons/mp3.svg", normalAudioColor, hoverAudioColor, selectedAudioColor,playingAudioColor,size);
        } else if (suffix == "avi" || suffix == "divx") {
            return createMenuIcon(":/res/icons/avi.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "rm" || suffix == "rmvb") {
            return createMenuIcon(":/res/icons/rm.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "mkv") {
            return createMenuIcon(":/res/icons/mkv.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "mov") {
            return createMenuIcon(":/res/icons/mov.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "wmv") {
            return createMenuIcon(":/res/icons/wmv.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "mpg") {
            return createMenuIcon(":/res/icons/mpg.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "wma") {
            return createMenuIcon(":/res/icons/wma.svg", normalAudioColor, hoverAudioColor, selectedAudioColor,playingAudioColor,size);
        } else if (suffix == "flv") {
            return createMenuIcon(":/res/icons/flv.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
        } else if (suffix == "wav") {
            return createMenuIcon(":/res/icons/wav.svg", normalAudioColor, hoverAudioColor, selectedAudioColor,playingAudioColor,size);
        } else if (suffix == "flac") {
            return createMenuIcon(":/res/icons/flac.svg", normalAudioColor, hoverAudioColor, selectedAudioColor,playingAudioColor,size);
        } else if (suffix == "ape") {
            return createMenuIcon(":/res/icons/ape.svg", normalAudioColor, hoverAudioColor, selectedAudioColor,playingAudioColor,size);
        } else if (suffix == "aac" || suffix == "m4a") {
            return createMenuIcon(":/res/icons/aac.svg", normalAudioColor, hoverAudioColor, selectedAudioColor,playingAudioColor,size);
        } else {
            // 默认图标
            if (GlobalHelper::IsVideo(filePath)) {
                return createMenuIcon(":/res/icons/video.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
            } else if (GlobalHelper::IsMusic(filePath)) {
                return createMenuIcon(":/res/icons/audio.svg", normalAudioColor, hoverAudioColor, selectedAudioColor,playingAudioColor,size);
            }
        }

        return createMenuIcon(":/res/icons/file.svg", normalVideoColor, hoverVideoColor, selectedVideoColor,playingVideoColor,size);
}

QIcon GlobalHelper::createMenuIcon(const QString& svgPath,
                                  const QColor& normalColor,
                                  const QColor& hoverColor,
                                  const QColor& selectedColor,
                                  const QColor& playingColor,const int size)
{
    QIcon icon;

    // 正常状态图标
    QPixmap normalPixmap = createColoredSvg(svgPath, normalColor, QSize(size, size));

    // 悬停状态图标
    QPixmap hoverPixmap = createColoredSvg(svgPath, hoverColor, QSize(size, size));

    // 选中状态图标
    QPixmap selectedPixmap = createColoredSvg(svgPath, selectedColor, QSize(size, size));

    // 选中状态图标
    QPixmap playingPixmap = createColoredSvg(svgPath, playingColor, QSize(size, size));

    // 设置图标状态
    icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);    // 正常未选中
    icon.addPixmap(hoverPixmap, QIcon::Active, QIcon::Off);     // 悬停
    icon.addPixmap(playingPixmap, QIcon::Disabled, QIcon::Off); // 正在播放
    icon.addPixmap(selectedPixmap, QIcon::Selected, QIcon::On); // 选中

    // 当鼠标悬停时也使用悬停图标
    icon.addPixmap(hoverPixmap, QIcon::Active, QIcon::Off);

    return icon;
}

QIcon GlobalHelper::createButtonIcon(const QString& svgPath,
                                  const QColor& normalColor,
                                  const QColor& hoverColor,
                                  const QColor& pressedColor,
                                  const int size)
{
    QIcon icon;

    // 正常状态图标
    QPixmap normalPixmap = createColoredSvg(svgPath, normalColor, QSize(size, size));

    // 悬停状态图标
    QPixmap hoverPixmap = createColoredSvg(svgPath, hoverColor, QSize(size, size));

    // 选中状态图标
    QPixmap selectedPixmap = createColoredSvg(svgPath, pressedColor, QSize(size, size));


    // 设置图标状态
    icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);    // 正常未选中
    icon.addPixmap(hoverPixmap, QIcon::Active, QIcon::Off);     // 悬停
    icon.addPixmap(selectedPixmap, QIcon::Selected, QIcon::On); // 选中
    // 当鼠标悬停时也使用悬停图标
    icon.addPixmap(hoverPixmap, QIcon::Active, QIcon::Off);

    return icon;
}

QPixmap GlobalHelper::createColoredSvg(const QString& svgPath, const QColor& color, QSize size)
{
    // 读取 SVG 文件
        QFile file(svgPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "无法打开SVG文件：" << svgPath;
            // 返回一个占位图标
            QPixmap pixmap(size);
            pixmap.fill(Qt::transparent);
            return pixmap;
        }

        QString svgContent = QString::fromUtf8(file.readAll());
        file.close();

        // 替换颜色 - 更全面的正则表达式
        QString colorHex = color.name();

        // 替换所有 fill 属性
        svgContent.replace(QRegularExpression("fill\\s*=\\s*\"[^\"]*\""),
                           QString("fill=\"%1\"").arg(colorHex));

        // 替换所有 stroke 属性（如果有描边）
        svgContent.replace(QRegularExpression("stroke\\s*=\\s*\"[^\"]*\""),
                           QString("stroke=\"%1\"").arg(colorHex));

        // 替换内联样式中的颜色
        svgContent.replace(QRegularExpression("fill\\s*:\\s*[^;]+;"),
                           QString("fill:%1;").arg(colorHex));

        // 渲染 SVG
        QSvgRenderer renderer(svgContent.toUtf8());
        if (!renderer.isValid()) {
            qDebug() << "无效的SVG内容：" << svgPath;
            QPixmap pixmap(size);
            pixmap.fill(Qt::transparent);
            return pixmap;
        }

        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        renderer.render(&painter);

        return pixmap;
}
