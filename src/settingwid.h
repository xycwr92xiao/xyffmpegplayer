#pragma once

#include <QDialog>
#include <QSettings>
#include <QFontDatabase>
#include "ui_settingwid.h"

class SettingWid : public QDialog
{
    Q_OBJECT

public:
    explicit SettingWid(QWidget *parent = nullptr);
    ~SettingWid();

    // 保存设置到配置文件
    void saveSettings();

    // 从配置文件加载设置
    void loadSettings();

    // 获取字幕设置
    QString getSubtitleFontFamily() const;
    int getSubtitleFontSize() const;
    // 获取颜色设置
        QColor getSubtitleTextColor() const;
        QColor getSubtitleStrokeColor() const;
        QColor getSubtitleHoverBgColor() const;
        QColor getSubtitleLeaveBgColor() const;
        bool getSubtitleKeepBackground() const;
    // 设置字幕设置
    void setSubtitleFont(const QString &family, int size);
    void setSubtitleColors(const QColor &textColor, const QColor &strokeColor,
                              const QColor &hoverBgColor, const QColor &leaveBgColor,
                              bool keepBackground);
signals:
    // 设置变化的信号（用于后续实现立即生效）
    void subtitleSettingsChanged(const QString &fontFamily, int fontSize);
    void subtitleColorSettingsChanged(const QColor &textColor, const QColor &strokeColor,
                                         const QColor &hoverBgColor, const QColor &leaveBgColor,
                                         bool keepBackground);
private slots:
    void onFontFamilyChanged(int index);
    void onFontSizeChanged(int value);
    // 颜色按钮点击槽函数
    void onTextColorClicked();
    void onStrokeColorClicked();
    void onHoverBgColorClicked();
    void onLeaveBgColorClicked();
    void onKeepBackgroundChanged(int state);
    //按钮
    void onApplyClicked();
    void onCancelClicked();
    void onOkClicked();

private:
    void initUI();
    void populateFontFamilies();
    void setupConnections();
    void updatePreview();
    void updateColorButton(QPushButton *btn, const QColor &color);
    void updateColorLabel(QLabel *label, const QColor &color);
    QString colorToString(const QColor &color) const;
    Ui::SettingWid ui;
    QSettings m_settings;

    // 当前设置
    QString m_currentFontFamily;
    int m_currentFontSize;
    // 颜色设置
        QColor m_currentTextColor;
        QColor m_currentStrokeColor;
        QColor m_currentHoverBgColor;
        QColor m_currentLeaveBgColor;
        bool m_currentKeepBackground;

    // 临时设置（用于取消时恢复）
    QString m_tempFontFamily;
    int m_tempFontSize;

    // 临时颜色设置
        QColor m_tempTextColor;
        QColor m_tempStrokeColor;
        QColor m_tempHoverBgColor;
        QColor m_tempLeaveBgColor;
        bool m_tempKeepBackground;
};
