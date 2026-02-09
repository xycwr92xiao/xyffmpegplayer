#ifndef TRANSPARENTSUBTITLEWINDOW_H
#define TRANSPARENTSUBTITLEWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QColor>

class TransparentSubtitleWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TransparentSubtitleWindow(QWidget *parent = nullptr);
    ~TransparentSubtitleWindow();
    QTimer m_updateTimer;
    bool m_bMouseHover;  // 鼠标是否悬停
    int m_fontSize;
    void setSubtitleText(const QString &text);
    void setFontSize(int size);
    void setPosition(int x, int y, int width, int height);
    void setMouseHover(bool hover);
    void setFontFamily(const QString& fontFamily);
    // 颜色设置函数
        void setTextColor(const QColor &color);
        void setStrokeColor(const QColor &color);
        void setHoverBgColor(const QColor &color);
        void setLeaveBgColor(const QColor &color);
        void setKeepBackground(bool keep);
signals:
    void sigRightClicked();  // 新增：右键点击信号
protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    // 添加鼠标事件
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private:
    QString m_subtitleText;
    int m_bgAlpha;       // 背景透明度
    QString m_fontFamily;
    // 颜色设置
        QColor m_textColor;          // 文字颜色
        QColor m_strokeColor;        // 描边颜色
        QColor m_hoverBgColor;       // 鼠标悬停背景颜色
        QColor m_leaveBgColor;       // 鼠标离开时的背景颜色
        bool m_keepBackground;       // 鼠标离开时是否保留背景
    // 添加拖动相关变量
        bool m_bDrag = false;           ///< 拖动标志
        QPoint m_DragStartPos;  ///< 拖动起始位置
    void updateWindowFlags();
    void updateBackgroundAlpha();
};

#endif // TRANSPARENTSUBTITLEWINDOW_H
