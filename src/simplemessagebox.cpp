// SimpleMessageBox.cpp
#include "SimpleMessageBox.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QStyle>
#include <QApplication>

SimpleMessageBox::SimpleMessageBox(QWidget *parent)
    : QDialog(parent)
{
    // 设置窗口属性
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_StyledBackground);
}

int SimpleMessageBox::question(QWidget *parent, const QString &title, const QString &text,int width, int height)
{
    SimpleMessageBox dialog;
    return dialog.execDialog(parent, title, text, Question, OkCancelButtons, width, height, true);
}

int SimpleMessageBox::information(QWidget *parent, const QString &title, const QString &text,int width, int height)
{
    SimpleMessageBox dialog;
    return dialog.execDialog(parent, title, text, Information, OkButton, width, height, true);
}

int SimpleMessageBox::warning(QWidget *parent, const QString &title, const QString &text,int width, int height)
{
    SimpleMessageBox dialog;
    return dialog.execDialog(parent, title, text, Warning, OkButton, width, height, true);
}

int SimpleMessageBox::critical(QWidget *parent, const QString &title, const QString &text,int width, int height)
{
    SimpleMessageBox dialog;
    return dialog.execDialog(parent, title, text, Critical, OkButton, width, height, true);
}
// 播放对应图标类型的声音
void SimpleMessageBox::playSoundForIcon(IconType icon)
{
    QString soundFile;
    switch (icon) {
    case Information:
        soundFile = ":/sounds/information.wav";
        break;
    case Warning:
        soundFile = ":/sounds/warning.wav";
        break;
    case Critical:
        soundFile = ":/sounds/error.wav";
        break;
    case Question:
        soundFile = ":/sounds/question.wav";
        break;
    default:
        return;
    }
     QApplication::beep();
            return;


}
int SimpleMessageBox::execDialog(QWidget *parent, const QString &title, const QString &text,
                                IconType icon, ButtonType buttonType,int width, int height, bool playSound)
{
    // 如果需要播放声音
        if (playSound) {
            playSoundForIcon(icon);
        }
    // 创建对话框
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setFixedSize(450, 180); // 稍微调小一点高度
    // 设置对话框大小，使用传入的宽度和高度
        if (width > 0 && height > 0) {
            dialog.setFixedSize(width, height);
        } else {
            dialog.setFixedSize(450, 180); // 默认大小
        }
        // 设置样式
            QString dialogStyle = QString(
                "QDialog {"
                "   background-color: #2b2b2b;"
                "   border: 1px solid #4a4a4a;"
                "   border-radius: 8px;"
                "}"
                "QLabel {"
                "   color: white;"
                "   font-size: 14px;"
                "   font-family: 'Microsoft YaHei', '微软雅黑', sans-serif;"
                "   background-color: transparent;"
                "   border: none;"
                "}"
            );
    // 根据图标类型调整样式
        switch (icon) {
        case Critical:
            dialogStyle +=
                "QDialog {"
                "   border-top: 0px solid #ff4757;"
                "}";
            break;
        case Warning:
            dialogStyle +=
                "QDialog {"
                "   border-top: 0px solid #ffa502;"
                "}";
            break;
        case Information:
            dialogStyle +=
                "QDialog {"
                "   border-top: 0px solid #2ed573;"
                "}";
            break;
        case Question:
            dialogStyle +=
                "QDialog {"
                "   border-top: 0px solid #1e90ff;"
                "}";
            break;
        default:
            break;
        }

        dialog.setStyleSheet(dialogStyle);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 内容布局
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setAlignment(Qt::AlignTop);

    // 图标
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(48, 48);
    iconLabel->setStyleSheet("background-color: transparent; border: none;");

    QIcon iconPix;
    switch (icon) {
    case Information:
        iconPix = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
        break;
    case Warning:
        iconPix = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
        break;
    case Critical:
        iconPix = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
        break;
    case Question:
        iconPix = QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion);
        break;
    default:
        break;
    }

    if (!iconPix.isNull()) {
        iconLabel->setPixmap(iconPix.pixmap(48, 48));
        contentLayout->addWidget(iconLabel);
        contentLayout->addSpacing(15);
    }

    // 文本
    QLabel *textLabel = new QLabel(text);
    textLabel->setWordWrap(true);
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    contentLayout->addWidget(textLabel, 1);

    mainLayout->addLayout(contentLayout, 1);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QString buttonStyle =
            "QPushButton {"
            "   background-color: #4a4a4a;"
            "   color: white;"
            "   border: 1px solid #5a5a5a;"
            "   border-radius: 4px;"
            "   padding: 8px 20px;"
            "   font-size: 13px;"
            "   min-width: 80px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #5a5a5a;"
            "}"
            "QPushButton:pressed {"
            "   background-color: #3a3a3a;"
            "}"
            "QPushButton:focus {"
            "   outline: none;"
            "   border: 1px solid #1e90ff;"
            "}";

    int result = QMessageBox::Cancel;

    switch (buttonType) {
    case OkButton: {
        // 只有一个"确定"按钮
        QPushButton *okButton = new QPushButton("确定");
        okButton->setStyleSheet(buttonStyle);
        buttonLayout->addWidget(okButton);

        QObject::connect(okButton, &QPushButton::clicked, &dialog, [&dialog, &result]() {
            result = QMessageBox::Ok;
            dialog.accept();
        });
        break;
    }
    case OkCancelButtons: {
            // "确定"和"取消"按钮
            QPushButton *okButton = new QPushButton("确定");
            QPushButton *cancelButton = new QPushButton("取消");

            okButton->setStyleSheet(buttonStyle);
            cancelButton->setStyleSheet(buttonStyle);
            okButton->setDefault(true); // 确定按钮为默认

            buttonLayout->addWidget(okButton);
            buttonLayout->addSpacing(10);
            buttonLayout->addWidget(cancelButton);

            QObject::connect(okButton, &QPushButton::clicked, &dialog, [&dialog, &result]() {
                result = QMessageBox::Ok;
                dialog.accept();
            });
            QObject::connect(cancelButton, &QPushButton::clicked, &dialog, [&dialog, &result]() {
                result = QMessageBox::Cancel;
                dialog.reject();
            });
            break;
        }
    case YesNoButtons: {
        // "是"和"否"按钮
        QPushButton *yesButton = new QPushButton("是");
        QPushButton *noButton = new QPushButton("否");

        yesButton->setStyleSheet(buttonStyle);
        noButton->setStyleSheet(buttonStyle);

        buttonLayout->addWidget(yesButton);
        buttonLayout->addSpacing(10);
        buttonLayout->addWidget(noButton);

        QObject::connect(yesButton, &QPushButton::clicked, &dialog, [&dialog, &result]() {
            result = QMessageBox::Yes;
            dialog.accept();
        });
        QObject::connect(noButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        break;
    }
    case YesNoCancelButtons: {
        // "是"、"否"、"取消"按钮
        QPushButton *yesButton = new QPushButton("是");
        QPushButton *noButton = new QPushButton("否");
        QPushButton *cancelButton = new QPushButton("取消");

        yesButton->setStyleSheet(buttonStyle);
        noButton->setStyleSheet(buttonStyle);
        cancelButton->setStyleSheet(buttonStyle);

        buttonLayout->addWidget(yesButton);
        buttonLayout->addSpacing(10);
        buttonLayout->addWidget(noButton);
        buttonLayout->addSpacing(10);
        buttonLayout->addWidget(cancelButton);

        QObject::connect(yesButton, &QPushButton::clicked, &dialog, [&dialog, &result]() {
            result = QMessageBox::Yes;
            dialog.accept();
        });
        QObject::connect(noButton, &QPushButton::clicked, &dialog, [&dialog, &result]() {
            result = QMessageBox::No;
            dialog.reject();
        });
        QObject::connect(cancelButton, &QPushButton::clicked, &dialog, [&dialog, &result]() {
            result = QMessageBox::Cancel;
            dialog.reject();
        });
        break;
    }
    }

    mainLayout->addLayout(buttonLayout);
    // 设置Enter键和Esc键的行为
        dialog.setFocus();
    dialog.exec();
    return result;
}
