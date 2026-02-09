// SimpleMessageBox.h
#ifndef SIMPLEMESSAGEBOX_H
#define SIMPLEMESSAGEBOX_H

#include <QDialog>
#include <QMessageBox>

class SimpleMessageBox : public QDialog
{
    Q_OBJECT

public:
    enum IconType {
        NoIcon,
        Information,
        Warning,
        Critical,
        Question
    };

    enum ButtonType {
        OkButton,      // 只有一个"确定"按钮
        OkCancelButtons,    // "确定"和"取消"按钮
        YesNoButtons,  // "是"和"否"按钮
        YesNoCancelButtons // "是"、"否"、"取消"按钮
    };

    explicit SimpleMessageBox(QWidget *parent = nullptr);

    static int question(QWidget *parent, const QString &title, const QString &text,
                        int width = 450, int height = 180);
    static int information(QWidget *parent, const QString &title, const QString &text,
                           int width = 450, int height = 180);
    static int warning(QWidget *parent, const QString &title, const QString &text,
                       int width = 450, int height = 180);
    static int critical(QWidget *parent, const QString &title, const QString &text,
                        int width = 450, int height = 180);
    // 通用方法，可以指定所有参数
    static int showMessage(QWidget *parent, const QString &title, const QString &text,
                              IconType icon, ButtonType buttons = OkButton,
                              int width = 450, int height = 180, bool playSound = true);
private:
    int execDialog(QWidget *parent, const QString &title, const QString &text,
                       IconType icon, ButtonType buttonType = OkButton,
                       int width = 450, int height = 180, bool playSound = true);

        void playSoundForIcon(IconType icon);
};

#endif // SIMPLEMESSAGEBOX_H
