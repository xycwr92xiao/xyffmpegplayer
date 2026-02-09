#include "mainwid.h"
#include <QApplication>
#include <QFontDatabase>
#include <QDebug>
#include <QFileInfo>

int main(int argc, char *argv[])
{
//    qDebug() << "123";
    QApplication a(argc, argv);
    
    //使用第三方字库，用来作为UI图片 ://res/fa-solid-900.ttf
    QFontDatabase::addApplicationFont("://res/fontawesome-webfont.ttf");
    // 设置应用程序名称和组织名，用于QSettings
        QCoreApplication::setOrganizationName("XyPlayer");
        QCoreApplication::setApplicationName("PlayerSettings");

        // 在创建MainWid之前，可以预先加载全局设置
        QSettings settings("XyPlayer", "PlayerSettings");
        settings.beginGroup("Subtitle");
        GlobalVars::subtitleFontFamily() = settings.value("FontFamily", "Microsoft YaHei").toString();
        GlobalVars::subtitleFontSize() = settings.value("FontSize", 16).toInt();
        GlobalVars::subtitleTextColor() = QColor(settings.value("TextColor", "#FFFFFFFF").toString());
        GlobalVars::subtitleStrokeColor() = QColor(settings.value("StrokeColor", "#FF000000").toString());
        GlobalVars::subtitleHoverBgColor() = QColor(settings.value("HoverBgColor", "#96000000").toString());
        GlobalVars::subtitleLeaveBgColor() = QColor(settings.value("LeaveBgColor", "#64000000").toString());
        GlobalVars::subtitleKeepBackground() = settings.value("KeepBackground", false).toBool();
        settings.endGroup();
        qDebug() << "撷取配置文件：－－－－－－－－－－－－－－－" << GlobalVars::subtitleFontFamily()<< GlobalVars::subtitleFontSize();
    MainWid w;
    if (w.Init() == false)
    {
        return -1;
    }
    // 检查命令行参数
        QStringList args = QApplication::arguments();
        if (args.size() > 1) {
            // 跳过第一个参数（程序自身路径），处理后续参数
            for (int i = 1; i < args.size(); i++) {
                QString filePath = args.at(i);
                QFileInfo fileInfo(filePath);

                // 检查文件是否存在
                if (fileInfo.exists() && fileInfo.isFile()) {
                    qDebug() << "通过命令行参数打开文件1:--------------------------------:" << filePath;
                    // 发送打开文件信号
                    w.openFileFromCommandLine(filePath);
                    break; // 只处理第一个有效的媒体文件
                } else {
                    qWarning() << "命令行参数指定的文件不存在:" << filePath;
                }
            }
            qDebug() << "通过命令行参数打开文件数量2:---------------------------------:" << args.size()-1;
        }
    w.show();

    return a.exec();
}
