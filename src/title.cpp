/*
 * @file 	title.cpp
 * @date 	2018/01/22 23:08
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	播放器标题栏
 * @note
 */

#include <QPainter>
#include <QFileInfo>
#include <QFontMetrics>
#include <QMessageBox>
#include <QFileDialog>

#include "title.h"
#include "ui_title.h"
#include "globalvars.h"
#include "globalhelper.h"

#pragma execution_character_set("utf-8")

Title::Title(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Title),
    m_stActionGroup(this),
    m_stMenu(this)
{
    ui->setupUi(this);

    connect(ui->CloseBtn, &QPushButton::clicked, this, &Title::SigCloseBtnClicked);
    connect(ui->MinBtn, &QPushButton::clicked, this, &Title::SigMinBtnClicked);
    connect(ui->MaxBtn, &QPushButton::clicked, this, &Title::SigMaxBtnClicked);
    connect(ui->SettingBtn, &QPushButton::clicked, this, &Title::SigEqualizerClicked);
    connect(ui->AlwaysOnTopBtn, &QPushButton::clicked, this, &Title::SigAlwaysOnTopBtnClicked);
    connect(ui->MenuBtn, &QPushButton::clicked, this, &Title::SigShowMenu);
 
    m_stMenu.addAction("最大化", this, &Title::SigMaxBtnClicked);
    m_stMenu.addAction("最小化", this, &Title::SigMinBtnClicked);
    m_stMenu.addAction("退出", this, &Title::SigCloseBtnClicked);

    QMenu* stMenu = m_stMenu.addMenu("打开");
    stMenu->addAction("打开文件", this, &Title::OpenFile);

    ui->MenuBtn->setToolTip("显示主菜单");
    ui->MinBtn->setToolTip("最小化");
    ui->MaxBtn->setToolTip("最大化");
    ui->CloseBtn->setToolTip("关闭");
    ui->SettingBtn->setToolTip("设置");
    ui->AlwaysOnTopBtn->setToolTip("窗口置顶");
    // 获取布局
    // 设置列的拉伸因子，实现指定的压缩顺序
        // 压缩顺序：MovieNameLab (1) -> MenuBtn (0) -> 按钮 (2-5)
        QGridLayout* layout = ui->gridLayout;

        // MenuBtn 列：压缩优先级低
        layout->setColumnStretch(0, 1);
        layout->setColumnMinimumWidth(0, 100);  // 最小宽度

        // MovieNameLab 列：最先被压缩
        layout->setColumnStretch(1, 3);  // 设置较高的拉伸因子，使其先被压缩
        layout->setColumnMinimumWidth(1, 35);   // 最小宽度

        // 按钮列：固定宽度，最后被压缩
        layout->setColumnStretch(2, 0);  // MinBtn
        layout->setColumnMinimumWidth(2, 35);

        layout->setColumnStretch(3, 0);  // MaxBtn
        layout->setColumnMinimumWidth(3, 35);

        layout->setColumnStretch(4, 0);  // SettingBtn
        layout->setColumnMinimumWidth(4, 35);

        layout->setColumnStretch(5, 0);  // CloseBtn
        layout->setColumnMinimumWidth(5, 35);
        layout->setColumnStretch(6, 0);  // CloseBtn
        layout->setColumnMinimumWidth(6, 35);
        ui->MenuBtn->setStyleSheet(
            "QPushButton {"
            "   margin-right: -20px;"
            "}"
        );
        // 设置播放列表中所有按钮不获取焦点
            QList<QPushButton*> buttons = this->findChildren<QPushButton*>();
            for (QPushButton* button : buttons) {
                button->setFocusPolicy(Qt::NoFocus);
            }
}

Title::~Title()
{
    delete ui;
}

bool Title::Init()
{
    if (InitUi() == false)
    {
        return false;
    }

    return true;
}

bool Title::InitUi()
{
    ui->MovieNameLab->clear();

    //保证窗口不被绘制上的部分透明
    setAttribute(Qt::WA_TranslucentBackground);

    setStyleSheet(GlobalHelper::GetQssStr("://res/qss/title.css"));

    GlobalHelper::SetIcon(ui->MaxBtn, 9, QChar(0xf2d0));
    GlobalHelper::SetIcon(ui->MinBtn, 9, QChar(0xf068));
    GlobalHelper::SetIcon(ui->CloseBtn, 9, QChar(0xf00d));//011是电源标志
    GlobalHelper::SetIcon(ui->AlwaysOnTopBtn, 9, QChar(0xf2e3));//0fb
    GlobalHelper::SetIcon(ui->SettingBtn, 9, QChar(0xf013));
    
    //ui->LogoLab->setToolTip("显示主菜单");

    if (about.Init() == false)
    {
        return false;
    }

    return true;
}

void Title::OpenFile()
{
    QString strFileName = QFileDialog::getOpenFileName(this, "打开文件", QDir::homePath(), 
        "视频文件(*.mkv *.rmvb *.mp4 *.avi *.flv *.wmv *.3gp)");

    emit SigOpenFile(strFileName);
}

void Title::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
}

void Title::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        emit SigDoubleClicked();
    }
}

void Title::resizeEvent(QResizeEvent *event)
{
    //ChangeMovieNameShow();
}

// void Title::mouseMoveEvent(QMouseEvent *event)
// {
//     qDebug() << "Title::mouseMoveEvent";
// }

void Title::ChangeMovieNameShow()
{
    QFontMetrics font_metrics(ui->MovieNameLab->font());
    QRect rect = font_metrics.boundingRect(m_strMovieName);
    int font_width = rect.width();
    int show_width = ui->MovieNameLab->width();
    if (font_width > show_width)
    {
        QString str = font_metrics.elidedText(m_strMovieName, Qt::ElideRight, ui->MovieNameLab->width());//返回一个带有省略号的字符串
        ui->MovieNameLab->setText(str);
    }
    else
    {
        ui->MovieNameLab->setText(m_strMovieName);
    }
}

void Title::OnChangeMaxBtnStyle(bool bIfMax)
{
    if (bIfMax)
    {
        GlobalHelper::SetIcon(ui->MaxBtn, 9, QChar(0xf2d2));
        ui->MaxBtn->setToolTip("还原");
    }
    else
    {
        GlobalHelper::SetIcon(ui->MaxBtn, 9, QChar(0xf2d0));
        ui->MaxBtn->setToolTip("最大化");
    }
}

void Title::OnPlay(QString strMovieName)
{
    qDebug() << "Title::OnPlay";
    QFileInfo fileInfo(strMovieName);
    m_strMovieName = fileInfo.fileName();
    ui->MovieNameLab->setText(m_strMovieName);
    //ChangeMovieNameShow();
}

void Title::OnStopFinished()
{
    qDebug() << "Title::OnStopFinished";
    ui->MovieNameLab->clear();
}

