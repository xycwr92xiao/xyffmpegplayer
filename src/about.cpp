#include "about.h"
#include "ui_about.h"

#include "globalhelper.h"



About::About(QWidget *parent)
    : QWidget(parent)
{
    ui = new Ui::About();
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
}

About::~About()
{
    delete ui;
}

bool About::Init()
{
    this->setWindowModality(Qt::ApplicationModal);

    this->setStyleSheet(
        "About { background-color: #2b2b2b; }"
        "QLabel, QTextBrowser, QTabWidget { color: #e0e0e0; }"
        "QTabWidget::pane { background-color: #3c3c3c; border: 1px solid #555; }"
        "QTabBar::tab { background-color: #2b2b2b; color: #e0e0e0; padding: 6px; }"
        "QTabBar::tab:selected { background-color: #3c3c3c; }"
        "QTabBar::tab:hover { background-color: #4a4a4a; }"
        "QTextBrowser { background-color: #2b2b2b; color: #e0e0e0; border: none; }"
        "QPushButton { background-color: #4a4a4a; color: white; border: 1px solid #6a6a6a; border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #5a5a5a; }"
    );

    this->setWindowIcon(QIcon("://res/player.png"));
    ui->LogoLabel->setPixmap(QPixmap("://res/player.png").scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QString strVersion = QString("版本：%1\n时间：%2").arg(GlobalHelper::GetAppVersion()).arg(QString(__DATE__) + " " + QString(__TIME__));
    ui->VersionLabel->setText(strVersion);

    return true;
}



void About::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        m_bMoveDrag = true;
        m_DragPosition = event->globalPos() - this->pos();
    }

    QWidget::mousePressEvent(event);
}

void About::mouseReleaseEvent(QMouseEvent *event)
{
    m_bMoveDrag = false;

    QWidget::mouseReleaseEvent(event);
}

void About::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bMoveDrag)
    {
        move(event->globalPos() - m_DragPosition);
    }

    QWidget::mouseMoveEvent(event);
}


void About::on_ClosePushButton_clicked()
{
    hide();
}
