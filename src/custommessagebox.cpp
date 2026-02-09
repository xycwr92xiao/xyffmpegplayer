// CustomMessageBox.cpp
#include "CustomMessageBox.h"
#include <QStyle>
#include <QApplication>
#include <QFont>

CustomMessageBox::CustomMessageBox(QWidget *parent)
    : QDialog(parent)
    , m_result(QMessageBox::Cancel)
{
    setupUI();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(450, 200);
}

void CustomMessageBox::setupUI()
{
    // 首先清除所有默认样式
    setAttribute(Qt::WA_StyledBackground); // 启用样式表背景

    // 设置对话框样式
    setStyleSheet(
        "CustomMessageBox {"
        "   background-color: #2b2b2b;"
        "   border: 1px solid #4a4a4a;"
        "}"
    );

    // 设置窗口背景色
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(43, 43, 43));
    palette.setColor(QPalette::WindowText, Qt::white);
    setPalette(palette);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 创建水平布局用于图标和文本
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // 图标标签 - 彻底清除边框
    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setStyleSheet(
        "QLabel {"
        "   background-color: transparent;"
        "   border: none;"  // 关键：去掉边框
        "   padding: 0px;"  // 去掉内边距
        "   margin: 0px;"   // 去掉外边距
        "}"
    );
    m_iconLabel->setAttribute(Qt::WA_TranslucentBackground); // 启用透明背景
    contentLayout->addWidget(m_iconLabel);

    // 文本标签 - 彻底清除边框
    m_textLabel = new QLabel();
    m_textLabel->setStyleSheet(
        "QLabel {"
        "   color: #ffffff;"
        "   font-size: 14px;"
        "   font-family: 'Microsoft YaHei', '微软雅黑', sans-serif;"
        "   background-color: transparent;"
        "   border: none;"    // 关键：去掉边框
        "   padding: 0px;"    // 去掉内边距
        "   margin: 0px;"     // 去掉外边距
        "}"
    );
    m_textLabel->setWordWrap(true);
    m_textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_textLabel->setAttribute(Qt::WA_TranslucentBackground); // 启用透明背景
    contentLayout->addWidget(m_textLabel, 1);

    mainLayout->addLayout(contentLayout, 1);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    // 创建按钮 - 简化样式
    QString buttonStyle =
        "QPushButton {"
        "   background-color: #4a4a4a;"
        "   color: white;"
        "   border: 1px solid #5a5a5a;"
        "   border-radius: 4px;"
        "   padding: 6px 12px;"
        "   font-size: 13px;"
        "   min-width: 80px;"
        "   font-family: 'Microsoft YaHei', '微软雅黑', sans-serif;"
        "}"
        "QPushButton:hover {"
        "   background-color: #5a5a5a;"
        "   border-color: #6a6a6a;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3a3a3a;"
        "}";

    m_okButton = new QPushButton("确定");
    m_okButton->setObjectName("okButton");
    m_okButton->setFixedSize(80, 30);
    m_okButton->setStyleSheet(buttonStyle);
    connect(m_okButton, &QPushButton::clicked, this, [this]() {
        m_result = QMessageBox::Ok;
        accept();
    });
    buttonLayout->addWidget(m_okButton);

    m_yesButton = new QPushButton("是");
    m_yesButton->setObjectName("yesButton");
    m_yesButton->setFixedSize(80, 30);
    m_yesButton->setStyleSheet(buttonStyle);
    connect(m_yesButton, &QPushButton::clicked, this, [this]() {
        m_result = QMessageBox::Yes;
        accept();
    });
    buttonLayout->addWidget(m_yesButton);

    m_noButton = new QPushButton("否");
    m_noButton->setObjectName("noButton");
    m_noButton->setFixedSize(80, 30);
    m_noButton->setStyleSheet(buttonStyle);
    connect(m_noButton, &QPushButton::clicked, this, [this]() {
        m_result = QMessageBox::No;
        reject();
    });
    buttonLayout->addWidget(m_noButton);

    m_cancelButton = new QPushButton("取消");
    m_cancelButton->setObjectName("cancelButton");
    m_cancelButton->setFixedSize(80, 30);
    m_cancelButton->setStyleSheet(buttonStyle);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        m_result = QMessageBox::Cancel;
        reject();
    });
    buttonLayout->addWidget(m_cancelButton);

    // 默认隐藏所有按钮
    m_okButton->hide();
    m_yesButton->hide();
    m_noButton->hide();
    m_cancelButton->hide();

    mainLayout->addLayout(buttonLayout);
}

void CustomMessageBox::setTitle(const QString &title)
{
    setWindowTitle(title);
}

void CustomMessageBox::setText(const QString &text)
{
    m_textLabel->setText(text);
}

void CustomMessageBox::setIcon(IconType icon)
{
    QPixmap pixmap;
    switch (icon) {
    case Information:
        pixmap = style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(32, 32);
        break;
    case Warning:
        pixmap = style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32, 32);
        break;
    case Critical:
        pixmap = style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(32, 32);
        break;
    case Question:
        pixmap = style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(32, 32);
        break;
    default:
        break;
    }
    m_iconLabel->setPixmap(pixmap);
}

void CustomMessageBox::setButtons(QMessageBox::StandardButtons buttons)
{
    // 根据按钮类型显示对应的按钮
    if (buttons & QMessageBox::Ok) m_okButton->show();
    if (buttons & QMessageBox::Yes) m_yesButton->show();
    if (buttons & QMessageBox::No) m_noButton->show();
    if (buttons & QMessageBox::Cancel) m_cancelButton->show();

    // 设置默认按钮
    if (buttons & QMessageBox::Yes) m_yesButton->setFocus();
    else if (buttons & QMessageBox::Ok) m_okButton->setFocus();
    else if (buttons & QMessageBox::No) m_noButton->setFocus();
}

int CustomMessageBox::showMessage(QWidget *parent, const QString &title, const QString &text,
                                 IconType icon, QMessageBox::StandardButtons buttons)
{
    CustomMessageBox dialog(parent);
    dialog.setTitle(title);
    dialog.setText(text);
    dialog.setIcon(icon);
    dialog.setButtons(buttons);

    dialog.exec();
    return dialog.m_result;
}
