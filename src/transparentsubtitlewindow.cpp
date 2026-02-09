#include "TransparentSubtitleWindow.h"
#include <QPainter>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include "globalvars.h"
TransparentSubtitleWindow::TransparentSubtitleWindow(QWidget *parent)
    : QWidget(parent), m_fontSize(24)
    , m_textColor(Qt::white)
        , m_strokeColor(Qt::black)
        , m_hoverBgColor(QColor(0, 0, 0, 150))
        , m_leaveBgColor(QColor(0, 0, 0, 100))
        , m_keepBackground(false)
{
    // 设置窗口属性
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    m_fontFamily = GlobalVars::subtitleFontFamily();
    // 从全局变量加载颜色设置
        m_textColor = GlobalVars::subtitleTextColor();
    qDebug() << "现在使用的字体是：－－" << m_fontFamily << "颜色是：－－" << m_textColor;
        m_strokeColor = GlobalVars::subtitleStrokeColor();
        m_hoverBgColor = GlobalVars::subtitleHoverBgColor();
        m_leaveBgColor = GlobalVars::subtitleLeaveBgColor();
        m_keepBackground = GlobalVars::subtitleKeepBackground();
    setMouseTracking(true);
    // 设置窗口标志
    updateWindowFlags();

    // 设置样式
    setStyleSheet("background-color: transparent; border: none;");
    //setStyleSheet("background-color: rgba(0, 0, 0, 0.5); border: 1px ;");
    // 设置定时器，定期重绘
    m_updateTimer.setInterval(1000);
    connect(&m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
    });

    // 初始隐藏
    hide();
}

TransparentSubtitleWindow::~TransparentSubtitleWindow()
{
    m_updateTimer.stop();
}

void TransparentSubtitleWindow::setSubtitleText(const QString &text)
{
    if (m_subtitleText != text) {
        m_subtitleText = text;
        if (!text.isEmpty()) {
            show();
            raise();
        }
        update();
    }
}
void TransparentSubtitleWindow::setFontFamily(const QString& fontFamily)
{
    if (m_fontFamily != fontFamily) {
        m_fontFamily = fontFamily;
        update();  // 触发重绘
    }
}
void TransparentSubtitleWindow::setFontSize(int size)
{
    if (m_fontSize != size && size > 0) {
        m_fontSize = size;
        update();
    }
}
void TransparentSubtitleWindow::updateBackgroundAlpha()
{
    // 根据鼠标悬停状态和设置决定背景颜色
        if (m_bMouseHover) {
            m_bgAlpha = m_hoverBgColor.alpha();
        } else {
            if (m_keepBackground) {
                m_bgAlpha = m_leaveBgColor.alpha();
            } else {
                m_bgAlpha = 0;    // 完全透明
            }
        }
}
void TransparentSubtitleWindow::setTextColor(const QColor &color)
{
    if (m_textColor != color) {
        m_textColor = color;
        update();
    }
}

void TransparentSubtitleWindow::setStrokeColor(const QColor &color)
{
    if (m_strokeColor != color) {
        m_strokeColor = color;
        update();
    }
}

void TransparentSubtitleWindow::setHoverBgColor(const QColor &color)
{
    if (m_hoverBgColor != color) {
        m_hoverBgColor = color;
        updateBackgroundAlpha();
        update();
    }
}

void TransparentSubtitleWindow::setLeaveBgColor(const QColor &color)
{
    if (m_leaveBgColor != color) {
        m_leaveBgColor = color;
        updateBackgroundAlpha();
        update();
    }
}

void TransparentSubtitleWindow::setKeepBackground(bool keep)
{
    if (m_keepBackground != keep) {
        m_keepBackground = keep;
        updateBackgroundAlpha();
        update();
    }
}
void TransparentSubtitleWindow::setPosition(int x, int y, int width, int height)
{
    setGeometry(x, y, width, height);
}
void TransparentSubtitleWindow::setMouseHover(bool hover)
{
    if (m_bMouseHover != hover) {
        m_bMouseHover = hover;
        updateBackgroundAlpha();
        update();
    }
}
void TransparentSubtitleWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (m_subtitleText.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 清除背景（完全透明）
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    // 绘制背景（根据透明度）
        if (m_bgAlpha > 0) {
            // 绘制圆角矩形背景
            QPainterPath path;
            int borderRadius = 8;
            path.addRoundedRect(rect().adjusted(1, 1, -1, -1), borderRadius, borderRadius);

            // 根据鼠标状态选择背景颜色
                    QColor bgColor;
                    if (m_bMouseHover) {
                        bgColor = m_hoverBgColor;
                    } else if (m_keepBackground) {
                        bgColor = m_leaveBgColor;
                    } else {
                        bgColor = Qt::transparent;
                    }

                    if (bgColor.alpha() > 0) {
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(bgColor);
                        painter.drawPath(path);

                        // 可选：绘制边框
                        if (m_bMouseHover) {
                            painter.setPen(QColor(100, 100, 100, 100));
                            painter.setBrush(Qt::NoBrush);
                            painter.drawPath(path);
                        }
                    }
        }

//    // 设置字体
    QFont font(m_fontFamily.isEmpty() ? "Microsoft YaHei" : m_fontFamily, m_fontSize, QFont::Bold);
    painter.setFont(font);

//    // 计算文本尺寸
    QFontMetrics fm(font);
    //QString displayText = m_subtitleText;

//    // 如果文本太长，换行
//    int maxWidth = width() - 10;
//    if (fm.horizontalAdvance(displayText) > maxWidth) {
//        qDebug() << "准备分行显示：fm.horizontalAdvance(displayText)" << fm.horizontalAdvance(displayText) << "maxWidth : " << maxWidth;
//        QStringList lines;
//        QString currentLine;
//        QStringList words = displayText.split(' ');

//        for (const QString &word : words) {
//            QString testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
//            if (fm.horizontalAdvance(testLine) <= maxWidth) {
//                currentLine = testLine;
//            } else {
//                if (!currentLine.isEmpty()) {
//                    lines.append(currentLine);
//                }
//                currentLine = word;
//            }
//        }
//        if (!currentLine.isEmpty()) {
//            lines.append(currentLine);
//        }
//        displayText = lines.join('\n');
//        qDebug() << "displayText:" << displayText;
//    }

    // 绘制文本（带描边）
        int lineHeight = QFontMetrics(font).height();
        QStringList lines = m_subtitleText.split('\n');
        int totalHeight = lines.size() * lineHeight;
        int startY = (height() - totalHeight) / 2 + QFontMetrics(font).ascent();

    for (int i = 0; i < lines.size(); i++) {
        QString line = lines[i];
        int textWidth = QFontMetrics(font).horizontalAdvance(line);
        int x = (width() - textWidth) / 2;
        int y = startY + i * lineHeight;

        // 绘制描边
        painter.setPen(QPen(m_strokeColor, 4));
        painter.drawText(x-1, y, line);
        painter.drawText(x+1, y, line);
        painter.drawText(x, y-1, line);
        painter.drawText(x, y+1, line);

        // 绘制文字
        painter.setPen(m_textColor);
        painter.drawText(x, y, line);
    }
}

void TransparentSubtitleWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    m_updateTimer.start();
    updateWindowFlags();
    raise();
}

void TransparentSubtitleWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);
    m_updateTimer.stop();
}

void TransparentSubtitleWindow::updateWindowFlags()
{
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint |
//                   Qt::WindowTransparentForInput | 打开这个就不接受鼠标事件了
                   Qt::WindowDoesNotAcceptFocus);
}
// 鼠标进入事件
void TransparentSubtitleWindow::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    setMouseHover(true);
    setCursor(Qt::OpenHandCursor);
}

// 鼠标离开事件
void TransparentSubtitleWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    setMouseHover(false);
    setCursor(Qt::ArrowCursor);
}

#include <QEnterEvent>

// 事件处理
bool TransparentSubtitleWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Enter:
        enterEvent(static_cast<QEnterEvent*>(event));
        return true;
    case QEvent::Leave:
        leaveEvent(event);
        return true;
    default:
        return QWidget::event(event);
    }
}

// 鼠标按下事件 - 开始拖动
void TransparentSubtitleWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::RightButton)
        {
            // 右键点击，发射信号触发主窗口菜单
            emit sigRightClicked();
            event->accept();
            return;
        }
        else if (event->buttons() & Qt::LeftButton)
    {
        m_bDrag = true;
        m_DragStartPos = event->globalPos();
        // 设置光标形状为闭合的手形，表示正在拖动
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

// 鼠标释放事件 - 结束拖动
void TransparentSubtitleWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_bDrag)
    {
        m_bDrag = false;
        // 恢复光标形状
        if (m_bMouseHover) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

// 鼠标移动事件 - 处理拖动
void TransparentSubtitleWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bDrag && (event->buttons() & Qt::LeftButton))
    {
        QPoint delta = event->globalPos() - m_DragStartPos;
        move(pos() + delta);
        m_DragStartPos = event->globalPos();

        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}
