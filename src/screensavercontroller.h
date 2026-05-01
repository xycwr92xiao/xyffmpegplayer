// ScreenSaverController.h
#ifndef SCREENSAVERCONTROLLER_H
#define SCREENSAVERCONTROLLER_H

#include <QObject>

class ScreenSaverController : public QObject
{
    Q_OBJECT

public:
    static ScreenSaverController& instance();
    void inhibit();     // 禁用屏保/睡眠
    void restore();     // 恢复系统设置

private:
    ScreenSaverController() = default;
    ~ScreenSaverController();
    bool m_isActive = false;

#ifdef Q_OS_WIN
    bool m_wasSet = false;
#elif defined(Q_OS_MAC)
    void* m_assertionID = nullptr;
#endif
};

#endif // SCREENSAVERCONTROLLER_H
