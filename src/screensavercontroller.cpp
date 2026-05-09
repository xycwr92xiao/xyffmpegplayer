#include "screensavercontroller.h"
#include <QDebug>

// 根据操作系统包含不同的头文件
#ifdef Q_OS_WIN
    #include <windows.h>
#elif defined(Q_OS_MAC)
    #include <IOKit/pwr_mgt/IOPMLib.h>
    #include <objc/objc-auto.h>
#elif defined(Q_OS_LINUX)
    #include <QScreen>
    #include <QGuiApplication>
#endif

ScreenSaverController& ScreenSaverController::instance()
{
    static ScreenSaverController controller;
    return controller;
}

ScreenSaverController::~ScreenSaverController()
{
    restore();
}

void ScreenSaverController::inhibit()
{
    if (m_isActive) return;

#ifdef Q_OS_WIN
    // Windows: 使用 SetThreadExecutionState API
    // ES_SYSTEM_REQUIRED: 防止系统进入睡眠
    // ES_DISPLAY_REQUIRED: 防止显示器进入睡眠/屏保
    // ES_CONTINUOUS: 通知系统这个状态应该一直保持，直到下一次调用
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    m_wasSet = true;
    m_isActive = true;
    qDebug() << "[ScreenSaver] Windows: System sleep and screensaver inhibited";

#elif defined(Q_OS_MAC)
    // macOS: 使用 IOPMAssertionCreateWithName API
    CFStringRef reasonForActivity = CFSTR("XyPlayer is playing video in fullscreen");
    IOPMAssertionID assertionID;
    IOReturn success = IOPMAssertionCreateWithName(
        kIOPMAssertionTypeNoDisplaySleep,   // 防止显示器休眠
        kIOPMAssertionLevelOn,
        reasonForActivity,
        &assertionID
    );

    if (success == kIOReturnSuccess) {
        m_assertionID = (void*)(size_t)assertionID;
        m_isActive = true;
        qDebug() << "[ScreenSaver] macOS: System sleep and screensaver inhibited";
    } else {
        qWarning() << "[ScreenSaver] macOS: Failed to inhibit sleep";
    }

#elif defined(Q_OS_LINUX)
    // Linux/X11: 使用 Qt 自带的 QScreen 接口
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        screen->setScreenSaverEnabled(false);
        m_isActive = true;
        qDebug() << "[ScreenSaver] Linux: Screensaver disabled via QScreen";
    } else {
        qWarning() << "[ScreenSaver] Linux: No primary screen found";
    }

#endif
}

void ScreenSaverController::restore()
{
    if (!m_isActive) return;

#ifdef Q_OS_WIN
    // 恢复原来的设置
    SetThreadExecutionState(ES_CONTINUOUS);
    m_wasSet = false;
    m_isActive = false;
    qDebug() << "[ScreenSaver] Windows: Sleep and screensaver restored";

#elif defined(Q_OS_MAC)
    if (m_assertionID) {
        IOPMAssertionID assertionID = (IOPMAssertionID)(size_t)m_assertionID;
        IOReturn success = IOPMAssertionRelease(assertionID);
        if (success == kIOReturnSuccess) {
            m_assertionID = nullptr;
            m_isActive = false;
            qDebug() << "[ScreenSaver] macOS: System sleep and screensaver restored";
        } else {
            qWarning() << "[ScreenSaver] macOS: Failed to restore sleep settings";
        }
    }

#elif defined(Q_OS_LINUX)
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        screen->setScreenSaverEnabled(true);
        m_isActive = false;
        qDebug() << "[ScreenSaver] Linux: Screensaver re-enabled";
    }

#endif
}
