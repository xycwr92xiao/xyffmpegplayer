// 创建自定义消息框类 CustomMessageBox.h
#ifndef CUSTOMMESSAGEBOX_H
#define CUSTOMMESSAGEBOX_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QMessageBox>

class CustomMessageBox : public QDialog
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

    explicit CustomMessageBox(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setText(const QString &text);
    void setIcon(IconType icon);
    void setButtons(QMessageBox::StandardButtons buttons);

    static int showMessage(QWidget *parent, const QString &title, const QString &text,
                          IconType icon = Information,
                          QMessageBox::StandardButtons buttons = QMessageBox::Ok);

private:
    void setupUI();

private:
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QPushButton *m_okButton;
    QPushButton *m_yesButton;
    QPushButton *m_noButton;
    QPushButton *m_cancelButton;
    QMessageBox::StandardButton m_result;
};

#endif // CUSTOMMESSAGEBOX_H
