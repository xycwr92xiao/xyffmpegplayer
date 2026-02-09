#include <QDebug>
#include <QDir>
#include <QThread>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QtConcurrent>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QFontMetrics>
#include <QMessageBox>

#include "playlist.h"
#include "ui_playlist.h"
#include "GlobalVars.h"
#include "globalhelper.h"

//PlaylistItemDelegate 的实现
PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent), m_rowHeight(24) // 初始化为24
{
}

void PlaylistItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return;
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    // 检查是否是正在播放的项
       bool isPlaying = (index.row() == GlobalVars::currentPlayIndex());
    // 保存 painter 状态
    painter->save();

    // 绘制背景（选中状态等）
    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

    // 获取数据
    QString text = index.data(Qt::DisplayRole).toString();
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QString duration = index.data(Qt::UserRole + 1).toString(); // 时长存储在 UserRole+1 中

    // 计算绘制区域
    QRect rect = opt.rect;
    int iconSize = m_rowHeight == 24 ?16:24;
    int iconLeftMargin = 4;
    int textLeftMargin = iconLeftMargin + iconSize + 6; // 图标宽度 + 间距
    int durationWidth = m_rowHeight == 24 ? ((duration.contains(':') && duration.count(':') == 1)?40:60) :
                                            ((duration.contains(':') && duration.count(':') == 1)?60:84); // 时长显示宽度
    int rightMargin = 4;

    // 绘制图标
        if (!icon.isNull()) {
            QRect iconRect(rect.left() + iconLeftMargin,
                          rect.top() + (rect.height() - iconSize) / 2,
                          iconSize, iconSize);
                // 根据状态选择合适的图标模式
                QIcon::Mode mode = QIcon::Normal;
                if (opt.state & QStyle::State_Selected) {
                    mode = QIcon::Selected;
                } else if (opt.state & QStyle::State_MouseOver) {
                    mode = QIcon::Active;
                } else if (isPlaying) {
                    mode = QIcon::Disabled;
                }
                // 绘制图标
                icon.paint(painter, iconRect, Qt::AlignCenter, mode, QIcon::On);
        }

    // 绘制文件名（左对齐）
    QRect textRect(rect.left() + textLeftMargin,
                  rect.top(),
                  rect.width() - textLeftMargin - durationWidth - rightMargin,
                  rect.height());

    // 设置文本颜色
    QColor textColor ;
    //= opt.state & QStyle::State_Selected ? opt.palette.highlightedText().color() : opt.palette.text().color();
    // 优先级：选中状态 > 悬停状态 > 正常状态
        if (opt.state & QStyle::State_Selected) {
            // 选中状态：高亮文本颜色
            textColor = opt.palette.highlightedText().color();
        } else if (opt.state & QStyle::State_MouseOver) {
            // 悬停状态：青色（Cyan）
            textColor = QColor(255, 255, 255); //
        } else {
            if (isPlaying)textColor = QColor(0, 255, 255); // 青色
            else {
                // 正常状态：默认文本颜色
                textColor = opt.palette.text().color();
            }
        }
    painter->setPen(textColor);

    // 绘制文件名，超出部分用省略号
    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    // 绘制时长（右对齐，灰色）
    QRect durationRect(rect.right() - durationWidth - rightMargin,
                      rect.top(),
                      durationWidth,
                      rect.height());
    if (opt.state & QStyle::State_Selected) {
        // 选中状态：与文件名相同的高亮文本颜色
        painter->setPen(QColor(186, 186, 186));
    } else {
        // 未选中状态：灰色
        painter->setPen(QColor(136, 136, 136));
    }
    painter->drawText(durationRect, Qt::AlignRight | Qt::AlignVCenter, duration);

    // 恢复 painter 状态
    painter->restore();
}

QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    size.setHeight(m_rowHeight); // 固定高度，可以根据需要调整
    return size;
}

Playlist::Playlist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Playlist),
    m_pItemDelegate(nullptr), // 初始化委托指针
    m_lastPlayPosition(0),
    m_currentPlaylistName("默认列表"),
    m_playlistMenu(nullptr)  // 新增：初始化菜单指针
{
    ui->setupUi(this);
    GlobalVars::selectedIndex() = -1;  // 初始化选中索引
    //m_nCurrentPlayListIndex = -1;
    m_itemData.clear(); // 清空旧数据
    // 确保列表为空
    m_playlistInfos.clear();
    // 初始化播放列表索引
    loadPlaylistIndex();
    // 设置默认播放列表路径
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_currentPlaylistJson = documentsPath + "/MyPlayer/playlist_config.json";
    qDebug() << "  m_playlistInfos = ---------------------------" << m_playlistInfos.isEmpty();

    loadPlaylistData();
    // 设置播放列表中所有按钮不获取焦点
        QList<QPushButton*> buttons = this->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            button->setFocusPolicy(Qt::NoFocus);
        }
}

Playlist::~Playlist()
{
    // 删除委托
        if (m_pItemDelegate) {
            delete m_pItemDelegate;
            m_pItemDelegate = nullptr;
        }

    saveAllData();  // 统一保存所有数据

    QStringList strListPlayList;
    for (int i = 0; i < ui->List->count(); i++)
    {
        strListPlayList.append(ui->List->item(i)->toolTip());
    }
    //GlobalHelper::SavePlaylist(strListPlayList);
    m_nCurrentPlayListIndex=GlobalVars::currentPlayIndex();
    qDebug() << "保存 nCurrentPlayListIndex：" << GlobalVars::currentPlayIndex();
    // 保存当前播放索引
    GlobalHelper::SavePlayIndex(m_nCurrentPlayListIndex);

    // 保存播放模式
    GlobalHelper::SavePlayMode(GlobalVars::playMode());

    delete ui;
}

bool Playlist::Init()
{
    if (ui->List->Init() == false)
    {
        return false;
    }

    if (InitUi() == false)
    {
        return false;
    }

    if (ConnectSignalSlots() == false)
    {
        return false;
    }

    setAcceptDrops(true);

    return true;
}

bool Playlist::InitUi()
{
    setStyleSheet(GlobalHelper::GetQssStr("://res/qss/playlist.css"));
    //ui->List->hide();
    //this->setFixedWidth(ui->HideOrShowBtn->width());
    //GlobalHelper::SetIcon(ui->HideOrShowBtn, 12, QChar(0xf104));
    // 启用拖动重新排序
//    QPixmap listIcon("://res/icons/list.svg");
//    int iconSize = 16; // 窗口模式默认16x16
//    ui->labelIcon->setFixedSize(iconSize, iconSize);
//    QPixmap scaledIcon = listIcon.scaled(iconSize, iconSize,
//        Qt::KeepAspectRatio, Qt::SmoothTransformation);
//    ui->labelIcon->setPixmap(scaledIcon);
//    ui->labelIcon->setAlignment(Qt::AlignCenter);
//    ui->labelIcon->setStyleSheet("");
    QString btnStyleSheet = "QPushButton { "
                            "border: none; "
                            "padding: 1px 4px 0px 4px; "//内边距
                            "margin: 0px -3px 0px -6px;"  //外边距上右下左
                            "background-color: transparent; "
                            "}"
                            "QPushButton:hover { "
                            "color : Cyan;"
                            "background-color: #444444; "  // 浅灰色背景
                            "border-radius: 2px; "
                            "}"
                            "QPushButton:pressed { "
                            "background-color: #555555; "  // 按下时稍深
                            "border-radius: 2px; "
                            "}";
    ui->labelIcon->setStyleSheet(btnStyleSheet);
    // 设置列表标题按钮样式
    ui->btnListTitle->setStyleSheet("QPushButton { "
                                        "border: none; "
                                        "padding: 2px 6px; "
                                    "margin-left: -4px;"  //外边距
                                        "background-color: transparent; "
                                        "}"
                                        "QPushButton:hover { "
                                        "color : Cyan;"
                                        "background-color: #444444; "  // 浅灰色背景
                                        "border-radius: 2px; "
                                        "}"
                                        "QPushButton:pressed { "
                                        "background-color: #555555; "  // 按下时稍深
                                        "border-radius: 2px; "
                                        "}");
    btnStyleSheet = "QPushButton { "
                            "border: none; "
                            "padding: 0px 4px 0px 4px; "//内边距
                            "margin: 0px 0px;"  //外边距
                            "background-color: transparent; "
                            "}"
                            "QPushButton:hover { "
                            "color : Cyan;"
                            "background-color: #444444; "  // 浅灰色背景
                            "border-radius: 2px; "
                            "}"
                            "QPushButton:pressed { "
                            "background-color: #555555; "  // 按下时稍深
                            "border-radius: 2px; "
                            "}";
    QString btnStyleSheet2 = "QPushButton { "
                            "border: none; "
                            "padding: 0px 4px 0px 4px; "//内边距
                            "margin: 0px 0px;"  //外边距
                            "background-color: transparent; "
                            "}"
                            "QPushButton:hover { "
                            "color : Cyan;"
                            "background-color: #444444; "  // 浅灰色背景
                            "border-radius: 2px; "
                            "}"
                            "QPushButton:pressed { "
                            "background-color: #555555; "  // 按下时稍深
                            "border-radius: 2px; "
                            "}";
    ui->btnSaveToNewlist->setStyleSheet(btnStyleSheet);
    ui->btnDeleteCurrentList->setStyleSheet(btnStyleSheet2);
    // 设置保存和删除按钮图标
     GlobalHelper::SetIcon(ui->labelIcon, 12, QChar(0xf0ca));
    GlobalHelper::SetIcon(ui->btnSaveToNewlist, 11, QChar(0xf2e5));     // 保存图标（软盘）c7

        // 删除图标：使用垃圾桶图标 - 与下方的删除按钮使用相同的图标
    GlobalHelper::SetIcon(ui->btnDeleteCurrentList, 11, QChar(0xf2e6)); // 删除图标（垃圾桶）1f8

    ui->List->setDragEnabled(true);
    ui->List->setAcceptDrops(true);
    ui->List->setDropIndicatorShown(true);
    ui->List->setDragDropMode(QAbstractItemView::InternalMove);
    ui->List->setDefaultDropAction(Qt::MoveAction);
    ui->List->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // 设置自定义委托
    m_pItemDelegate = new PlaylistItemDelegate(this);
    ui->List->setItemDelegate(m_pItemDelegate);
    // 设置按钮图标（使用图标字库）
    GlobalHelper::SetIcon(ui->btnAdd, 10, QChar(0xf052));      // 加号图标
    GlobalHelper::SetIcon(ui->btnDelete, 10, QChar(0xf2e7));   // 垃圾桶图标
    GlobalHelper::SetIcon(ui->btnMoveUp, 14, QChar(0xf0d8));   // 向上箭头
    GlobalHelper::SetIcon(ui->btnMoveDown, 14, QChar(0xf0d7)); // 向下箭头
    GlobalHelper::SetIcon(ui->btnMoveTop, 10, QChar(0xf116));  // 双向上箭头102,093
    GlobalHelper::SetIcon(ui->btnMoveBottom, 10, QChar(0xf117)); // 双向下箭头103,019
    GlobalHelper::SetIcon(ui->btnSort, 10, QChar(0xf15d));     // 排序图标

    // 连接按钮信号
    connect(ui->List, &QListWidget::currentRowChanged, this, &Playlist::on_currentRowChanged);
    // 初始化播放列表菜单
    initListMenu();
    // 更新当前播放列表标题
    updateCurrentPlaylistTitle();

    ui->List->clear();
    qDebug()<< "开始加载列表-----------------------------------------------------2";
    // 将 m_itemData 转换为列表并按 orderIndex 排序
        QList<PlaylistItemData> sortedItems;
        for (auto it = m_itemData.begin(); it != m_itemData.end(); ++it) {
            sortedItems.append(it.value());
        }

        // 按 orderIndex 排序
        std::sort(sortedItems.begin(), sortedItems.end(),
                  [](const PlaylistItemData &a, const PlaylistItemData &b) {
                      return a.orderIndex < b.orderIndex;
                  });

        // 按顺序创建列表项
        for (int i = 0; i < sortedItems.size(); i++) {
            const PlaylistItemData& data = sortedItems[i];

            QListWidgetItem *pItem = new QListWidgetItem(ui->List);
            pItem->setData(Qt::UserRole, QVariant(data.filePath));
            pItem->setToolTip(data.filePath);

            int iconSize = 24;
            QIcon formatIcon = GlobalHelper::GetFormatIcon(data.filePath, iconSize);
            pItem->setIcon(formatIcon);

            ui->List->addItem(pItem);
            updateItemDisplay(i);
        }
    //修复列表2次加载最后修改
        if (ui->List->count() > 0)
            {
                ui->List->setCurrentRow(0);
                GlobalVars::selectedIndex() = 0;
            }
    // 连接选中项变化信号
    GlobalVars::selectedIndex()=0;
//        connect(ui->List, &QListWidget::currentRowChanged,
//                [](int nRow) {
//                    GlobalVars::selectedIndex() = nRow;
//                    qDebug() << "选中索引更新为：" << nRow;
//                });

        // 更新播放列表总数
     GlobalVars::playlistCount() = ui->List->count();
    //ui->List->addItems(strListPlaylist);
     // 恢复播放模式
     int savedMode = GlobalHelper::GetPlayMode();
     GlobalVars::playMode()=savedMode;
     emit SigPlayModeRestored(savedMode);
     // 恢复播放索引
         int lastIndex = GlobalHelper::GetPlayIndex();
         if (lastIndex >= 0 && lastIndex < ui->List->count())
         {
             ui->List->setCurrentRow(lastIndex);
             GlobalVars::selectedIndex() = lastIndex;
             m_nCurrentPlayListIndex = lastIndex;

             qDebug() << "自动播放2 ------------:" << GlobalHelper::GetAutoPlay();

             // 自动播放上一次的项
             if (GlobalHelper::GetAutoPlay())
             {
                 GlobalVars::currentPlayIndex()=lastIndex;
                 QTimer::singleShot(100, this, [this, lastIndex]() {
                     QListWidgetItem *pItem = ui->List->item(lastIndex);
                     if (pItem)
                     {
                         qDebug() << "自动播放3 ------------:" << GlobalHelper::GetAutoPlay();
                         if(GlobalHelper::haveCommandLine()){
                             // 确保只有一个选中项
                             ui->List->clearSelection();
                             pItem = ui->List->item(0);
                         }else on_List_itemDoubleClicked(pItem);
                         //ui->List->setCurrentRow(GlobalHelper::haveCommandLine()?0:lastIndex);
                     }
                 });
             }
         }
         else if (ui->List->count() > 0)
         {
             ui->List->setCurrentRow(0);
             GlobalVars::selectedIndex() = 0;
         }

         // 连接选中项变化信号
//         connect(ui->List, &QListWidget::currentRowChanged,
//                 [](int nRow) {
//                     GlobalVars::selectedIndex() = nRow;
//                     qDebug() << "选中索引更新为：" << nRow;
//                 });
    updateButtonStates();
    // 为所有没有时长的项启动后台加载
       for (int i = 0; i < ui->List->count(); i++) {
           QListWidgetItem* item = ui->List->item(i);
           if (item) {
               QString filePath = item->data(Qt::UserRole).toString();
               if (m_itemData.contains(filePath)) {
                   if (!m_itemData[filePath].durationLoaded) {
                       loadDurationInBackground(i);
                   }
               }
           }
       }
    return true;
}

bool Playlist::ConnectSignalSlots()
{
    QList<bool> listRet;
    bool bRet;
    bRet = connect(ui->btnAdd, &QPushButton::clicked, ui->List, &MediaList::AddFile);
        listRet.append(bRet);
    // 连接当前行变化信号
        bRet = connect(ui->List, &QListWidget::currentRowChanged,
                       this, &Playlist::onCurrentItemChanged);
        listRet.append(bRet);
    // 新增信号连接
       bRet = connect(ui->List, &MediaList::SigRemoveFile, this, [this](int index) {
           if (index >= 0 && index < ui->List->count()) {
               qDebug() << "移除了索引为： －－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－：" << index;
               ui->List->takeItem(index);
               // 更新播放列表计数
               GlobalVars::playlistCount() = ui->List->count();
               if (index == GlobalVars::currentPlayIndex()) {emit SigStop();
                   if (ui->List->count() !=0)
                   {if (index >= ui->List->count())index =ui->List->count()-1;
                       qDebug() << "播放索引为： －－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－：" << index;
                    if (index >= 0)PlayByIndex(index);
                   }
               }
            updateButtonStates();
           }
       });
       listRet.append(bRet);

       bRet = connect(ui->List, &MediaList::SigPlayOrPause, this, [this](int index) {
           qDebug() << "播放列表接收到播放/暂停信号，索引：" << index <<" 正在播放："<< GlobalVars::currentPlayIndex();
           if (index >= 0 && index < ui->List->count()) {
               QListWidgetItem *pItem = ui->List->item(index);
               if (pItem) {
                   // 如果当前正在播放此项，则暂停/播放
                   if (GlobalVars::currentPlayIndex() == index) {
                       qDebug() << "正在发生暂停信号 --->>>>>>>>>>>>>>>>>>>> ";
                       emit SigPlayOrPause(); // 发送播放/暂停信号
                   } else {
                       // 否则播放此项
                       on_List_itemDoubleClicked(pItem);
                   }
               }
           }
       });
       listRet.append(bRet);

       bRet = connect(ui->List, &MediaList::SigPlayFile, this, [this](int index) {
           if (index >= 0 && index < ui->List->count()) {
               QListWidgetItem *pItem = ui->List->item(index);
               if (pItem) {
                   on_List_itemDoubleClicked(pItem);
               }
           }
       });
       listRet.append(bRet);
       // 新增：连接重命名信号
           bRet = connect(ui->List, &MediaList::SigRenameFile,
                          this, &Playlist::onFileRenamed);
           listRet.append(bRet);

           // 新增：连接播放位置请求信号（需要与主窗口播放器配合）
           bRet = connect(ui->List, &MediaList::SigRequestPlayPosition,
                          this, [this]() {
                              // 获取当前播放位置
                              emit SigGetPlayPosition();
                          });
           listRet.append(bRet);

           // 新增：连接恢复播放信号
           bRet = connect(ui->List, &MediaList::SigResumePlay,
                          this, [this](int index, qint64 position) {
                              // 如果是指定的索引，从指定位置播放
                              if (index == GlobalVars::currentPlayIndex()) {
                                  // 播放
                                  PlayByIndex(index);
                                  // 等待文件加载完成
                                  QTimer::singleShot(20, this, [this, position]() {
                                      // 设置播放位置
                                      GlobalVars::isRenameing()=false;
                                      emit SigSetPlayPosition(position);
                                  });
                              }
                          });
           listRet.append(bRet);
    bRet = connect(ui->List, &MediaList::SigAddFile, this, &Playlist::OnAddFile);
    listRet.append(bRet);
    bRet = connect(ui->List, &MediaList::SigSaveSubtitleFile, this, &Playlist::SigSaveSubtitleFile);
    listRet.append(bRet);
    bRet = connect(ui->List, &MediaList::SigStop, this, &Playlist::SigStop);
    listRet.append(bRet);
    // 新增信号连接,这里无需手动连接，Qt 会自动连接他们
        bRet = connect(ui->labelIcon, &QPushButton::clicked, this, &Playlist::on_btnListTitle_clicked);
        listRet.append(bRet);

//        bRet = connect(ui->btnSaveToNewlist, &QPushButton::clicked, this, &Playlist::on_btnSaveToNewlist_clicked);
//        listRet.append(bRet);

//        bRet = connect(ui->btnDeleteCurrentList, &QPushButton::clicked, this, &Playlist::on_btnDeleteCurrentList_clicked);
//        listRet.append(bRet);
    for (bool bReturn : listRet)
    {
        if (bReturn == false)
        {
            return false;
        }
    }

    return true;
}

void Playlist::on_List_itemDoubleClicked(QListWidgetItem *item)
{
    int prevPlayingIndex = GlobalVars::currentPlayIndex();
    int index = ui->List->row(item);
    ui->List->clearSelection();
    QString filePath = item->data(Qt::UserRole).toString();
    GlobalVars::isVideoPlaying()= GlobalHelper::IsVideo(filePath);
    GlobalVars::currentPlayFileName()=filePath;
    qDebug() << "当前播放的文件：＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝" << GlobalVars::currentPlayFileName();
    // 检查文件是否存在
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(this,
                tr("文件不存在"),
                tr("文件 '%1' 不存在或已被删除。\n\n"
                   "是否从播放列表中移除该文件？").arg(fileInfo.fileName()));

            // 用户点击确定后从播放列表中移除
            int reply = QMessageBox::question(this,
                tr("移除文件"),
                tr("从播放列表中移除 '%1'？").arg(fileInfo.fileName()),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                // 从播放列表中移除
                ui->List->takeItem(index);

                // 从数据映射中移除
                if (m_itemData.contains(filePath)) {
                    m_itemData.remove(filePath);
                }

                // 更新播放列表计数
                GlobalVars::playlistCount() = ui->List->count();
                SetSelectedIndex(GlobalVars::currentPlayIndex());
                // 更新按钮状态
                updateButtonStates();

                // 保存更改
                saveAllData();
            }

            return; // 不播放，直接返回
        }
        // 先停止当前播放
        if (GlobalVars::currentPlayIndex() >= 0) {
            emit SigStop();
            // 等待一小段时间让播放器完全停止
            QEventLoop loop;
            QTimer::singleShot(50, &loop, &QEventLoop::quit);
            loop.exec();
        }
        // 如果还没有获取时长，在后台获取
        if (m_itemData.contains(filePath)) {
            if (!m_itemData[filePath].durationLoaded) {
                loadDurationInBackground(index);
            }
        } else {
            // 如果映射中没有此项，创建并获取时长
            PlaylistItemData data;
            data.filePath = filePath;
            data.fileName = QFileInfo(filePath).fileName();
            data.duration = 0;
            data.durationLoaded = false;
            m_itemData[filePath] = data;
            loadDurationInBackground(index);
        }
    emit SigPlay(item->data(Qt::UserRole).toString());
        GlobalVars::currentPlayIndex() = index;
        ui->List->setCurrentRow(index);
        GlobalVars::selectedIndex() = index;
    // 方法2：更精确地只更新受影响的行（推荐）
    if (prevPlayingIndex >= 0 && prevPlayingIndex < ui->List->count()) {
        // 更新之前播放的行
        QModelIndex prevModelIndex = ui->List->model()->index(prevPlayingIndex, 0);
        emit ui->List->model()->dataChanged(prevModelIndex, prevModelIndex);
    }
}

bool Playlist::GetPlaylistStatus()
{
    if (this->isHidden())
    {
        return false;
    }

    return true;
}

int Playlist::GetCurrentIndex()
{
    return 0;
}
void Playlist::SetSelectedIndex(int index)
{
    if ( index >= 0 && index < ui->List->count()) {
        ui->List->setCurrentRow(index);
        // 确保选中项可见
        ui->List->scrollToItem(ui->List->item(index),
                                   QAbstractItemView::PositionAtCenter);
    }
}
void Playlist::OnAddFile(QString strFileName)
{
    bool bSupportMedia = GlobalHelper::IsSupportedMedia(strFileName);
        if (!bSupportMedia) return;

        QFileInfo fileInfo(strFileName);
        QString filePath = fileInfo.filePath();

        // 检查是否已存在
        bool exists = false;
        for (int i = 0; i < ui->List->count(); i++) {
            if (ui->List->item(i)->data(Qt::UserRole).toString() == filePath) {
                exists = true;
                break;
            }
        }

        if (exists) return;

        // 创建新项
        QListWidgetItem *pItem = new QListWidgetItem(ui->List);
        pItem->setData(Qt::UserRole, QVariant(filePath));
        pItem->setToolTip(filePath);

        // 设置图标
        int iconSize = 24; // 默认值
//            if (m_pItemDelegate) {
//                // 根据行高决定图标大小：行高24对应16x16，行高更大对应24x24
//                iconSize = (m_pItemDelegate->rowHeight() == 24) ? 16 : 24;
//            }
        QIcon formatIcon = GlobalHelper::GetFormatIcon(filePath,iconSize);
        pItem->setIcon(formatIcon);

        ui->List->addItem(pItem);

        // 存储数据（如果已有保存的数据，使用保存的数据）
        PlaylistItemData data;
        data.filePath = filePath;
        data.fileName = fileInfo.fileName();

        if (m_itemData.contains(filePath)) {
            // 使用已保存的时长数据
            data.duration = m_itemData[filePath].duration;
            data.durationLoaded = m_itemData[filePath].durationLoaded;
            qDebug() << "使用已保存的时长数据：" << filePath << data.duration << "ms";
        } else {
            data.duration = 0;
            data.durationLoaded = false;

            // 如果没有时长数据，在后台获取
            int index = ui->List->count() - 1;
            loadDurationInBackground(index);
        }

        m_itemData[filePath] = data;

        // 更新显示
        updateItemDisplay(ui->List->count() - 1);

        // 更新播放列表总数
        GlobalVars::playlistCount() = ui->List->count();

        // 保存到配置文件
        saveAllData();

        updateButtonStates();
}

void Playlist::OnAddFileAndPlay(QString strFileName)
{
    bool bSupportMedia = GlobalHelper::IsSupportedMedia(strFileName);
    if (!bSupportMedia)
    {
        return;
    }

    QFileInfo fileInfo(strFileName);
    QList<QListWidgetItem *> listItem = ui->List->findItems(fileInfo.fileName(), Qt::MatchExactly);
    QListWidgetItem *pItem = nullptr;
    if (listItem.isEmpty())
    {
        pItem = new QListWidgetItem(ui->List);
        pItem->setData(Qt::UserRole, QVariant(fileInfo.filePath()));  // 用户数据
        pItem->setText(fileInfo.fileName());  // 显示文本
        pItem->setToolTip(fileInfo.filePath());
        ui->List->addItem(pItem);
    }
    else
    {
        pItem = listItem.at(0);
    }
    on_List_itemDoubleClicked(pItem);
}

void Playlist::OnBackwardPlay()
{
    if (ui->List->count() == 0) return;

        int prevIndex = GetPrevIndex(m_nCurrentPlayListIndex);
        if (prevIndex >= 0 && prevIndex < ui->List->count()) {
            m_nCurrentPlayListIndex = prevIndex;
            QListWidgetItem *pItem = ui->List->item(m_nCurrentPlayListIndex);
            if (pItem) {
                on_List_itemDoubleClicked(pItem);
                ui->List->setCurrentRow(m_nCurrentPlayListIndex);
            }
        }
}

void Playlist::OnForwardPlay()
{
    if (ui->List->count() == 0) return;

        int nextIndex = GetNextIndex(m_nCurrentPlayListIndex);
        qDebug() << " 下一曲索引：" << nextIndex;
        if (nextIndex >= 0 && nextIndex < ui->List->count()) {
            m_nCurrentPlayListIndex = nextIndex;
            QListWidgetItem *pItem = ui->List->item(m_nCurrentPlayListIndex);
            if (pItem) {
                on_List_itemDoubleClicked(pItem);
                ui->List->setCurrentRow(m_nCurrentPlayListIndex);
            }
        }
}

void Playlist::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
    {
        return;
    }

    for (QUrl url : urls)
    {
        QString strFileName = url.toLocalFile();

        OnAddFile(strFileName);
    }
}

void Playlist::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

// 在 playlist.cpp 中添加新的函数
void Playlist::PlayByIndex(int nIndex)
{
    qDebug() << "播放选中索引" << nIndex;
    if (nIndex >= 0 && nIndex < ui->List->count())
    {
        QListWidgetItem *pItem = ui->List->item(nIndex);
        if (pItem)
        {
            // 检查文件是否存在
                        QString filePath = pItem->data(Qt::UserRole).toString();
                        QFileInfo fileInfo(filePath);
                        if (!fileInfo.exists()) {
                            QMessageBox::warning(this,
                                tr("文件不存在"),
                                tr("文件 '%1' 不存在或已被删除。\n\n"
                                   "已从播放列表中移除该文件。").arg(fileInfo.fileName()));
                            // 从播放列表中移除
                            ui->List->takeItem(nIndex);

                            // 从数据映射中移除
                            if (m_itemData.contains(filePath)) {
                                m_itemData.remove(filePath);
                            }

                            // 更新播放列表计数
                            GlobalVars::playlistCount() = ui->List->count();

                            // 更新按钮状态
                            updateButtonStates();

                            // 保存更改
                            saveAllData();

                            qDebug() << "文件不存在，已从播放列表移除：" << filePath;
                            return;
                        }
                        // 确保只有一个选中项
                         ui->List->clearSelection();
            on_List_itemDoubleClicked(pItem);
            // 确保UI更新选中状态
            ui->List->setCurrentRow(nIndex);
            GlobalVars::selectedIndex() = nIndex;
            GlobalVars::currentPlayIndex() = nIndex;
            qDebug() << "通过播放按钮开始播放索引：" << nIndex;
        }
    }
    else
    {
        qDebug() << "无效的播放索引：" << nIndex;
    }
}

int Playlist::GetNextIndex(int currentIndex)
{
    if (ui->List->count() == 0) return -1;

    int mode = GlobalVars::playMode();
    int total = ui->List->count();

    switch (mode) {
    case PLAY_MODE_SEQUENCE:    // 顺序播放
        if (currentIndex == total - 1) {
            return -1;  // 播放到最后一首，不自动播放
        } else {
            return currentIndex + 1;
        }
        break;

    case PLAY_MODE_REPEAT_ALL:  // 循环全部
        return (currentIndex + 1) % total;
        break;

    case PLAY_MODE_REPEAT_ONE:  // 循环单曲
        return currentIndex;  // 返回当前索引，重复播放
        break;

    case PLAY_MODE_RANDOM:      // 随机播放
        // 生成随机索引
        return qrand() % total;
        break;

    default:
        return (currentIndex + 1) % total;
    }
}

int Playlist::GetPrevIndex(int currentIndex)
{
    if (ui->List->count() == 0) return -1;

    int mode = GlobalVars::playMode();
    int total = ui->List->count();

    switch (mode) {
    case PLAY_MODE_SEQUENCE:    // 顺序播放
        if (currentIndex == 0) {
            return -1;  // 已经是第一首，无法后退
        } else {
            return currentIndex - 1;
        }
        break;

    case PLAY_MODE_REPEAT_ALL:  // 循环全部
        return (currentIndex - 1 + total) % total;
        break;

    case PLAY_MODE_REPEAT_ONE:  // 循环单曲
        return currentIndex;  // 返回当前索引，重复播放
        break;

    case PLAY_MODE_RANDOM:      // 随机播放
        // 生成随机索引
        return qrand() % total;
        break;

    default:
        return (currentIndex - 1 + total) % total;
    }
}
// 添加文件按钮
//void Playlist::on_btnAdd_clicked()
//{
//    // 调用 MediaList 的添加文件功能
//    //ui->List->AddFile();
//    qDebug() << " 你点击了添加文件按钮！";
//    MediaList::AddFile();
//    GlobalVars::playlistCount() = ui->List->count();
//    updateButtonStates();
//}

// 删除按钮
void Playlist::on_btnDelete_clicked()
{
    int currentRow = ui->List->currentRow();
    if (currentRow >= 0)
    {
        emit ui->List->SigRemoveFile(currentRow);
        //ui->List->takeItem(currentRow);
        GlobalVars::playlistCount() = ui->List->count();
        updateButtonStates();
    }
}

// 上移按钮
void Playlist::on_btnMoveUp_clicked()
{
    int currentRow = ui->List->currentRow();
        if (currentRow > 0)
        {
            // 临时阻塞信号，避免触发多次事件
            ui->List->blockSignals(true);

            QListWidgetItem* currentItem = ui->List->takeItem(currentRow);
            ui->List->insertItem(currentRow - 1, currentItem);
            ui->List->setCurrentRow(currentRow - 1);

            // 更新当前播放索引
            if (GlobalVars::currentPlayIndex() == currentRow) {
                GlobalVars::currentPlayIndex() = currentRow - 1;
            } else if (GlobalVars::currentPlayIndex() == currentRow - 1) {
                GlobalVars::currentPlayIndex() = currentRow;
            }
            qDebug() << "调整后 GlobalVars::currentPlayIndex()：" << GlobalVars::currentPlayIndex();
            updateButtonStates();
        }
}

// 下移按钮
void Playlist::on_btnMoveDown_clicked()
{
    int currentRow = ui->List->currentRow();
    if (currentRow >= 0 && currentRow < ui->List->count() - 1)
    {
        QListWidgetItem* currentItem = ui->List->takeItem(currentRow);
        ui->List->insertItem(currentRow + 1, currentItem);
        ui->List->setCurrentRow(currentRow + 1);

        // 更新当前播放索引
        if (GlobalVars::currentPlayIndex() == currentRow) {
            GlobalVars::currentPlayIndex() = currentRow + 1;
        } else if (GlobalVars::currentPlayIndex() == currentRow + 1) {
            GlobalVars::currentPlayIndex() = currentRow;
        }
        qDebug() << "调整后 GlobalVars::currentPlayIndex()：" << GlobalVars::currentPlayIndex();
        updateButtonStates();
    }
}

// 置顶按钮
void Playlist::on_btnMoveTop_clicked()
{
    int currentRow = ui->List->currentRow();
    if (currentRow > 0)
    {
        QListWidgetItem* currentItem = ui->List->takeItem(currentRow);
        ui->List->insertItem(0, currentItem);
        ui->List->setCurrentRow(0);

        // 更新当前播放索引
        int playIndex = GlobalVars::currentPlayIndex();
        if (playIndex == currentRow) {
            GlobalVars::currentPlayIndex() = 0;
        } else if (playIndex < currentRow) {
            GlobalVars::currentPlayIndex() = playIndex + 1;
        }

        updateButtonStates();
    }
}

// 置底按钮
void Playlist::on_btnMoveBottom_clicked()
{
    int currentRow = ui->List->currentRow();
    int lastRow = ui->List->count() - 1;

    if (currentRow >= 0 && currentRow < lastRow)
    {
        QListWidgetItem* currentItem = ui->List->takeItem(currentRow);
        ui->List->insertItem(lastRow, currentItem);
        ui->List->setCurrentRow(lastRow);

        // 更新当前播放索引
        int playIndex = GlobalVars::currentPlayIndex();
        if (playIndex == currentRow) {
            GlobalVars::currentPlayIndex() = lastRow;
        } else if (playIndex > currentRow) {
            GlobalVars::currentPlayIndex() = playIndex - 1;
        }

        updateButtonStates();
    }
}

// 排序按钮
void Playlist::on_btnSort_clicked()
{
    QMenu sortMenu(this);

    QAction* sortByNameAsc = sortMenu.addAction("按名称升序");
    QAction* sortByNameDesc = sortMenu.addAction("按名称降序");
    QAction* sortByPathAsc = sortMenu.addAction("按路径升序");
    QAction* sortByPathDesc = sortMenu.addAction("按路径降序");

    QAction* selected = sortMenu.exec(ui->btnSort->mapToGlobal(QPoint(0, ui->btnSort->height())));

    if (selected == sortByNameAsc) {
        // 按名称升序排序
        ui->List->sortItems(Qt::AscendingOrder);
    } else if (selected == sortByNameDesc) {
        // 按名称降序排序
        ui->List->sortItems(Qt::DescendingOrder);
    } else if (selected == sortByPathAsc || selected == sortByPathDesc) {
        // 按文件路径排序
        sortByPath(selected == sortByPathAsc);
    }

    // 排序后重新选择当前项目
    //updateSelectionAfterSort();
}

// 按文件路径排序
void Playlist::sortByPath(bool ascending)
{
    // 创建一个项目列表，用于排序
        QList<QPair<QString, QListWidgetItem*>> items;

        for (int i = 0; i < ui->List->count(); ++i) {
            QListWidgetItem* item = ui->List->item(i);
            QString filePath = item->toolTip(); // 文件路径存储在toolTip中
            items.append(qMakePair(filePath, item));
        }

        // 排序
        if (ascending) {
            std::sort(items.begin(), items.end(),
                      [](const QPair<QString, QListWidgetItem*>& a,
                         const QPair<QString, QListWidgetItem*>& b) {
                          return a.first < b.first;
                      });
        } else {
            std::sort(items.begin(), items.end(),
                      [](const QPair<QString, QListWidgetItem*>& a,
                         const QPair<QString, QListWidgetItem*>& b) {
                          return a.first > b.first;
                      });
        }

        // 清空列表并重新添加排序后的项目
        ui->List->clear();
        for (const auto& item : items) {
            ui->List->addItem(item.second);
        }

        // 重新更新所有项的显示
        for (int i = 0; i < ui->List->count(); i++) {
            updateItemDisplay(i, false);
        }
}

// 排序后更新选择
void Playlist::updateSelectionAfterSort()
{
    // 排序后尝试重新选择当前播放的项目
    int playIndex = GlobalVars::currentPlayIndex();
    if (playIndex >= 0 && playIndex < ui->List->count()) {
        ui->List->setCurrentRow(playIndex);
        GlobalVars::selectedIndex() = playIndex;
    }
}

// 更新按钮状态
void Playlist::updateButtonStates()
{
    int currentRow = ui->List->currentRow();
    int count = ui->List->count();
    bool hasSelection = (currentRow >= 0);
    bool canMoveUp = hasSelection && (currentRow > 0);
    bool canMoveDown = hasSelection && (currentRow < count - 1);

    ui->btnDelete->setEnabled(hasSelection);
    ui->btnMoveUp->setEnabled(canMoveUp);
    ui->btnMoveDown->setEnabled(canMoveDown);
    ui->btnMoveTop->setEnabled(canMoveUp);
    ui->btnMoveBottom->setEnabled(canMoveDown);
    ui->btnSort->setEnabled(count > 1);
    int displayIndex = (currentRow >= 0) ? currentRow + 1 : 0;
    QString indexText = QString("%1/%2").arg(displayIndex).arg(count);
    ui->labelIndex->setText(indexText);
    // 同时更新全局变量中的播放列表总数（确保一致性）
    GlobalVars::playlistCount() = count;
}

// 当前行变化时更新按钮状态
void Playlist::on_currentRowChanged(int currentRow)
{
    Q_UNUSED(currentRow);
        GlobalVars::selectedIndex() = currentRow;  // 更新全局变量
        qDebug() << "选中索引更新为：" << currentRow;
        updateButtonStates();
}


void Playlist::saveAllData()
{
    if (m_currentPlaylistJson.isEmpty()) {
           return;
       }
    // 1. 收集所有数据
    QJsonArray playlistArray;

    for (int i = 0; i < ui->List->count(); i++) {
        QString filePath = ui->List->item(i)->data(Qt::UserRole).toString();
        QFileInfo fileInfo(filePath);

        QJsonObject itemData;
        itemData["orderIndex"] = i;
        itemData["filePath"] = filePath;
        itemData["fileName"] = fileInfo.fileName();

        // 保存时长信息
        if (m_itemData.contains(filePath) && m_itemData[filePath].durationLoaded) {
            itemData["duration"] = m_itemData[filePath].duration;
            itemData["durationLoaded"] = true;
        } else {
            itemData["duration"] = 0;
            itemData["durationLoaded"] = false;
            qDebug() << "保存无时长的数据：" << fileInfo.fileName();
        }

        playlistArray.append(itemData);
    }

    // 2. 创建完整的配置对象
    QJsonObject configData;
    configData["playlist"] = playlistArray;
    configData["currentIndex"] = GlobalVars::currentPlayIndex();
    configData["playMode"] = GlobalVars::playMode();
    configData["playlistCount"] = ui->List->count();
    configData["autoPlay"] = GlobalHelper::GetAutoPlay();

    // 3. 保存到统一的 JSON 文件
    QJsonDocument doc(configData);
    QByteArray jsonData = doc.toJson();

    // 选择保存位置 - 使用文档目录
    QFile file(m_currentPlaylistJson);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonData);
        file.close();
        qDebug() << "所有数据已保存到：" << m_currentPlaylistJson;
        qDebug() << "数据大小：" << jsonData.size() << "字节";
    } else {
        qDebug() << "无法保存数据到：" << m_currentPlaylistJson;
    }
}

void Playlist::loadPlaylistData()
{
    qDebug() << "m_currentPlaylistJson==  " << m_currentPlaylistJson;
    if (m_currentPlaylistJson.isEmpty()) {
            return;
        }
    // 查找配置文件
    QFile file(m_currentPlaylistJson);
    if (!file.exists()) {
        qDebug() << "配置文件不存在：" << m_currentPlaylistJson;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开配置文件：" << m_currentPlaylistJson;
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "配置文件格式错误：" << m_currentPlaylistJson;
        return;
    }
    ui->List->clear();
    m_itemData.clear();

    QJsonObject configData = doc.object();

    if (configData.contains("playlist") && configData["playlist"].isArray()) {
            QJsonArray playlistArray = configData["playlist"].toArray();

            for (int i = 0; i < playlistArray.size(); i++) {
                QJsonObject itemData = playlistArray[i].toObject();

                QString filePath = itemData["filePath"].toString();
                QFileInfo fileInfo(filePath);

                QString canonicalPath = fileInfo.canonicalFilePath();
                if (canonicalPath.isEmpty()) {
                    canonicalPath = filePath;
                }

                if (fileInfo.exists()) {
                    PlaylistItemData data;
                    data.filePath = canonicalPath;
                    data.fileName = fileInfo.fileName();
                    data.orderIndex = itemData["orderIndex"].toInt();

                    if (itemData.contains("duration") && itemData.contains("durationLoaded")) {
                        data.duration = itemData["duration"].toInt();
                        data.durationLoaded = itemData["durationLoaded"].toBool();
                    } else {
                        data.duration = 0;
                        data.durationLoaded = false;
                    }

                    m_itemData[canonicalPath] = data;
                }
            }
        }
    // 2. 加载其他设置
    if (configData.contains("currentIndex")) {
        int savedIndex = configData["currentIndex"].toInt();
        GlobalVars::currentPlayIndex() = savedIndex;
        qDebug() << "加载当前播放索引：" << savedIndex;
    }

    if (configData.contains("playMode")) {
        int savedMode = configData["playMode"].toInt();
        GlobalVars::playMode() = savedMode;
        qDebug() << "加载播放模式：" << savedMode;
    }

    if (configData.contains("autoPlay")) {
        bool autoPlay = configData["autoPlay"].toBool();
        GlobalHelper::SaveAutoPlay(autoPlay);
        qDebug() << "加载自动播放设置：" << autoPlay;
    }
}
// 添加更新项显示的函数
void Playlist::updateItemDisplay(int index, bool forceUpdate)
{
    if (index < 0 || index >= ui->List->count()) return;

        QListWidgetItem *item = ui->List->item(index);
        if (!item) return;

        QString filePath = item->data(Qt::UserRole).toString();
        QFileInfo fileInfo(filePath);

        // 获取文件名（不包含路径）
        QString fileName = fileInfo.fileName();

        // 获取时长显示文本
        QString durationStr = "--:--"; // 默认值

        // 优先从 m_itemData 中获取时长
        if (m_itemData.contains(filePath)) {
            PlaylistItemData& data = m_itemData[filePath];
//qDebug() << "index=" << index << " data.durationLoaded=" << data.durationLoaded << "data.duration = " << data.duration;
            if (data.durationLoaded && data.duration > 0) {
                durationStr = formatDurationForDisplay(data.duration);
            } else if (!data.durationLoaded) {
                // 如果数据存在但尚未加载时长，可以触发后台加载
                // 这里不触发，由其他地方控制
            }
        }

        // 设置显示文本（仅文件名）
        item->setText(fileName);
        // 将时长存储在 UserRole+1 中，委托会读取这个值
        item->setData(Qt::UserRole + 1, durationStr);
        // 设置图标（如果强制更新或图标为空）
        if (forceUpdate || item->icon().isNull()) {
            QIcon formatIcon = GlobalHelper::GetFormatIcon(filePath,24);
            item->setIcon(formatIcon);
        }

        // 触发重绘
        item->setData(Qt::UserRole + 2, QVariant(true)); // 标记需要重绘
}

QString Playlist::formatDurationForDisplay(qint64 durationMs)
{
    if (durationMs <= 0) {
            return "--:--";
        }

        return GlobalHelper::FormatDuration(durationMs);
}
// 后台加载时长的函数
void Playlist::loadDurationInBackground(int index)
{
    // 使用 QtConcurrent 在后台线程获取时长
    if (index < 0 || index >= ui->List->count()) return;

    QListWidgetItem *item = ui->List->item(index);
    if (!item) return;

    QString filePath = item->data(Qt::UserRole).toString();

    // 在后台线程获取时长
    QtConcurrent::run([this, index, filePath]() {
        qint64 duration = GlobalHelper::GetMediaDuration(filePath);

        // 回到主线程更新UI
        QMetaObject::invokeMethod(this, [this, index, duration]() {
            onDurationLoaded(index, duration);
        }, Qt::QueuedConnection);
    });
}

void Playlist::onDurationLoaded(int index, qint64 duration)
{
    if (index < 0 || index >= ui->List->count()) return;

    QListWidgetItem *item = ui->List->item(index);
        if (!item) return;

        QString filePath = item->data(Qt::UserRole).toString();

        // 更新数据，使用文件路径作为键
        if (!m_itemData.contains(filePath)) {
            PlaylistItemData data;
            data.filePath = filePath;
            data.fileName = QFileInfo(filePath).fileName();
            m_itemData[filePath] = data;
        }

        m_itemData[filePath].duration = duration;
        m_itemData[filePath].durationLoaded = true;

       // qDebug() << "时长数据加载完成：" << filePath << duration << "ms";

    // 更新显示
    updateItemDisplay(index, true);

    // 保存更新后的数据
    //savePlaylistData();
}

// 添加当前项变化槽函数
void Playlist::onCurrentItemChanged(int currentRow)
{
    GlobalVars::selectedIndex() = currentRow;
    qDebug() << "选中索引更新为：" << currentRow;

    // 如果选中了项且没有时长，尝试获取时长
        if (currentRow >= 0 && currentRow < ui->List->count()) {
            QListWidgetItem *item = ui->List->item(currentRow);
            if (item) {
                QString filePath = item->data(Qt::UserRole).toString();

                if (m_itemData.contains(filePath)) {
                    if (!m_itemData[filePath].durationLoaded) {
                        loadDurationInBackground(currentRow);
                    }
                } else {
                    // 如果映射中没有此项，创建并获取时长
                    PlaylistItemData data;
                    data.filePath = filePath;
                    data.fileName = QFileInfo(filePath).fileName();
                    data.duration = 0;
                    data.durationLoaded = false;
                    m_itemData[filePath] = data;
                    loadDurationInBackground(currentRow);
                }
            }
        }

    updateButtonStates();
}

void Playlist::ShowCurrentFileInfo()
{
    int currentIndex = ui->List->currentRow();
    if (currentIndex >= 0 && currentIndex < ui->List->count()) {
        // 调用 MediaList 的 ShowFileInfo 函数
        ui->List->ShowFileInfo();
    }
}

void Playlist::ShowFileInfoByIndex(int index)
{
    if (index >= 0 && index < ui->List->count()) {
        // 先选中该项，然后显示信息
        ui->List->setCurrentRow(index);
        ui->List->ShowFileInfo();
    }
}
void Playlist::ShowFilePathByIndex(int index)
{
    if (index >= 0 && index < ui->List->count()) {
        // 先选中该项，然后显示信息
        ui->List->setCurrentRow(index);
        ui->List->OpenFileLocation();
    }
}
void Playlist::setRowHeight(int height)
{
    if (m_pItemDelegate) {
        m_pItemDelegate->setRowHeight(height);
        // 强制更新列表
        ui->List->update();
        ui->List->reset(); // 重置视图以应用新尺寸
    }
}

// 新增：重命名当前选中文件
void Playlist::RenameCurrentFile()
{
    int currentRow = ui->List->currentRow();
    if (currentRow >= 0) {
        // 触发MediaList的重命名功能
        ui->List->startRenameCurrentItem();
    }
}

// 新增：重命名指定文件
bool Playlist::RenameFile(int index, const QString &newName)
{
    if (index >= 0 && index < ui->List->count()) {
        return ui->List->renameFileInList(index, newName);
    }
    return false;
}

// 新增：处理文件重命名信号
void Playlist::onFileRenamed(int index, QString oldPath, QString newPath)
{
    qDebug() << "文件重命名:" << oldPath << "->" << newPath;

    // 更新内部数据映射
    handleFileRenamed(index, oldPath, newPath);

    // 更新播放索引（如果正在播放的文件被重命名）
    if (GlobalVars::currentPlayIndex() == index) {
        // 当前播放的文件被重命名，需要更新播放器的文件路径
        // 这里可以发送信号给主窗口，让播放器重新加载新文件
        emit SigPlay(newPath);
    }

    // 保存数据
    saveAllData();
}

// 新增：处理文件重命名，更新内部数据结构
void Playlist::handleFileRenamed(int index, const QString &oldPath, const QString &newPath)
{
    // 更新 m_itemData
    updateItemDataPath(oldPath, newPath);

    // 更新列表显示
    updateItemDisplay(index, true);

    // 更新全局变量
    if (GlobalVars::currentPlayIndex() == index) {
        // 当前播放文件被重命名，路径已更新
        qDebug() << "当前播放文件已重命名，新路径:" << newPath;
    }
}

// 新增：更新数据映射中的文件路径
void Playlist::updateItemDataPath(const QString &oldPath, const QString &newPath)
{
    if (m_itemData.contains(oldPath)) {
        PlaylistItemData data = m_itemData.value(oldPath);
        data.filePath = newPath;
        data.fileName = QFileInfo(newPath).fileName();

        // 移除旧键，添加新键
        m_itemData.remove(oldPath);
        m_itemData.insert(newPath, data);

        qDebug() << "已更新数据映射:" << oldPath << "->" << newPath;
    } else {
        // 如果映射中没有，创建一个新条目
        PlaylistItemData data;
        data.filePath = newPath;
        data.fileName = QFileInfo(newPath).fileName();
        data.duration = 0;
        data.durationLoaded = false;
        m_itemData.insert(newPath, data);

        qDebug() << "创建新的数据映射条目:" << newPath;
    }
}
void Playlist::appToIndexAndPlay(QStringList filesList,int index)
{
    if (filesList.isEmpty()) {
        qDebug() << "命令行文件列表为空";
        return;
    }
    if (index < 0) {
        index = 0;
    }
    appendItemToPosition(filesList,index);
    if ( m_firstIndex>=0 ) PlayByIndex(m_firstIndex);
}
void Playlist::appendItemToPosition(QStringList filesList, int index)
{
    if (filesList.isEmpty()) {
        qDebug() << "命令行文件列表为空";
        return;
    }

    qDebug() << QString("将命令行文件添加到播放列表第%1个位置，数量：%2").arg(index).arg(filesList.size());

    // 验证index的有效性
    if (index < 0) {
        index = 0;
    }

    int itemCount = ui->List->count();
    if (index > itemCount) {
        index = itemCount;
        qDebug() << "索引超出范围，调整到列表末尾";
    }

    // 临时阻塞信号，避免频繁更新UI
    ui->List->blockSignals(true);
    m_firstIndex = index;
    // 顺序插入到指定位置
    for (int i = 0; i < filesList.size(); ++i) {
        QString filePath = filesList[i];
        QFileInfo fileInfo(filePath);

        // 检查文件是否存在
        if (!fileInfo.exists()) {
            qDebug() << "文件不存在，跳过：" << filePath;
            continue;
        }
        // 检查列表中是否已存在
        bool exists = false;
        for (int j = 0; j < ui->List->count(); j++) {
            if (ui->List->item(j)->data(Qt::UserRole).toString() == filePath.replace('\\', '/')) {
                exists = true;
                if (i == filesList.size() - 1) m_firstIndex = j;
                break;
            }
        }
        if (exists) continue;
        // 创建新项，不要传递父对象
        QListWidgetItem *pItem = new QListWidgetItem();
        pItem->setData(Qt::UserRole, QVariant(filePath));
        pItem->setToolTip(filePath);

        // 设置图标
        int iconSize = 24;
        QIcon formatIcon = GlobalHelper::GetFormatIcon(filePath, iconSize);
        pItem->setIcon(formatIcon);

        // 插入到指定位置，每插入一个项目后，下一个项目的插入位置要+1
        ui->List->insertItem(index + i, pItem);

        // 存储数据
        PlaylistItemData data;
        data.filePath = filePath;
        data.fileName = fileInfo.fileName();
        data.duration = 0;
        data.durationLoaded = false;

        // 注意：由于允许重复，我们不检查m_itemData是否已包含此文件
        m_itemData[filePath] = data;

        qDebug() << QString("已添加到播放列表第%1个位置：%2").arg(index + i).arg(fileInfo.fileName());
    }

    // 恢复信号
    ui->List->blockSignals(false);

    // 更新所有项的显示
    for (int i = 0; i < ui->List->count(); i++) {
        updateItemDisplay(i, true);
    }

    // 更新播放列表总数
    GlobalVars::playlistCount() = ui->List->count();

    // 设置选中项为插入的第一个文件（如果有成功插入的文件）
    if (ui->List->count() > 0 && index < ui->List->count()) {
        ui->List->setCurrentRow(index);
        GlobalVars::selectedIndex() = index;
    }

    // 为所有没有时长的项启动后台加载
    for (int i = 0; i < filesList.size(); i++) {
        QString filePath = filesList[i];
        if (m_itemData.contains(filePath) && !m_itemData[filePath].durationLoaded) {
            // 找到该文件在列表中的实际索引
            for (int j = 0; j < ui->List->count(); j++) {
                if (ui->List->item(j)->data(Qt::UserRole).toString() == filePath) {
                    loadDurationInBackground(j);
                    break;
                }
            }
        }
    }

    // 更新按钮状态
    updateButtonStates();

    // 保存到配置文件
    saveAllData();

    qDebug() << QString("命令行文件已成功添加到播放列表第%1个位置").arg(index);
}
//void Playlist::appendItemTotop(QStringList filesList)
//{
//    if (filesList.isEmpty()) {
//        qDebug() << "命令行文件列表为空";
//        return;
//    }
//    qDebug() << "将命令行文件添加到播放列表顶部，数量：" << filesList.size();
//    // 临时阻塞信号，避免频繁更新UI
//    ui->List->blockSignals(true);
//    // 逆序插入，因为我们要插入到顶部，但保持原始顺序
//    // 例如：文件列表是[A, B, C]，插入后顺序应该是[A, B, C, 原有项目]
//    // 所以我们从最后一个文件开始插入到索引0的位置
//    for (int i = filesList.size() - 1; i >= 0; --i) {
//        QString filePath = filesList[i];
//        QFileInfo fileInfo(filePath);
//        // 检查文件是否存在
//        if (!fileInfo.exists()) {
//            qDebug() << "文件不存在，跳过：" << filePath;
//            continue;
//        }
//        // 检查列表中是否已存在
//        bool exists = false;
//        for (int j = 0; j < ui->List->count(); j++) {
//            if (ui->List->item(j)->data(Qt::UserRole).toString() == filePath) {
//                exists = true;
//                break;
//            }
//        }
//        if (exists) continue;
//        // 创建新项，不要传递父对象
//                QListWidgetItem *pItem = new QListWidgetItem();  // 不传递父对象
//                pItem->setData(Qt::UserRole, QVariant(filePath));
//                pItem->setToolTip(filePath);

//        // 设置图标
//        int iconSize = 24;
//        QIcon formatIcon = GlobalHelper::GetFormatIcon(filePath, iconSize);
//        pItem->setIcon(formatIcon);

//        // 插入到列表顶部（索引0）
//        ui->List->insertItem(0, pItem);

//        // 存储数据
//        PlaylistItemData data;
//        data.filePath = filePath;
//        data.fileName = fileInfo.fileName();
//        data.duration = 0;
//        data.durationLoaded = false;

//        // 注意：由于允许重复，我们不检查m_itemData是否已包含此文件
//        m_itemData[filePath] = data;

//        qDebug() << "已添加到播放列表顶部：" << fileInfo.fileName();
//    }

//    // 恢复信号
//    ui->List->blockSignals(false);

//    // 更新所有项的显示
//    for (int i = 0; i < ui->List->count(); i++) {
//        qDebug() << "i:" << i << " 内容：" << ui->List->item(i)->text();
//        updateItemDisplay(i, true);
//    }
//    // 更新播放列表总数
//    GlobalVars::playlistCount() = ui->List->count();

//    // 设置选中项为第一个（顶部新增的第一个文件）
//    if (ui->List->count() > 0) {
//        ui->List->setCurrentRow(0);
//        GlobalVars::selectedIndex() = 0;
//    }

//    // 为所有没有时长的项启动后台加载
//    for (int i = 0; i < filesList.size(); i++) {
//        QString filePath = filesList[i];
//        if (m_itemData.contains(filePath) && !m_itemData[filePath].durationLoaded) {
//            // 找到该文件在列表中的实际索引
//            for (int j = 0; j < ui->List->count(); j++) {
//                if (ui->List->item(j)->data(Qt::UserRole).toString() == filePath) {
//                    loadDurationInBackground(j);
//                    break;
//                }
//            }
//        }
//    }
//    // 更新按钮状态
//    updateButtonStates();
//    // 保存到配置文件
//    saveAllData();
//    qDebug() << "命令行文件已成功添加到播放列表顶部";
//}

// 初始化播放列表菜单
void Playlist::initListMenu()
{
    if (!m_playlistMenu) {
        m_playlistMenu = new QMenu(this);
        // 设置菜单样式
                m_playlistMenu->setStyleSheet(R"(
                    /* 菜单整体样式 */
                    QMenu {
                        background-color: #1e1e1e;
                        border: 1px solid #444;
                        border-radius: 3px;
                        padding: 8px 0px;
                        color: #f0f0f0;
                        font-size: 14px;
                        font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
                    }

                    /* 菜单项样式 */
                    QMenu::item {
                        padding: 2px 10px 2px 10px;  /* 左侧留出勾选图标位置 */
                        margin: 1px 10px;
                        border-radius: 4px;
                        min-height: 20px;  /* 设置最小行高 */
                        line-height: 2px;  /* 行距调整 */
                    }

                    /* 悬停状态 */
                    QMenu::item:selected {
                        background-color: #2a5ea0;
                        color: #ffffff;
                    }

                    /* 禁用状态 */
                    QMenu::item:disabled {
                        color: #888;
                    }

                    /* 分隔线样式 */
                    QMenu::separator {
                        height: 1px;
                        background-color: #444;
                        margin: 5px 12px;
                    }

                    /* 选中项的勾选标记样式 */
                    QMenu::indicator {
                        width: 16px;
                        height: 16px;
                               margin-left: 16px;
                    }

//                    QMenu::indicator:checked {
//                        /* 使用图片作为勾选图标 */
//                        image: url("://res/icons/check.svg");
//                    }

                    /* 图标样式（如果有图标） */
                    QMenu::icon {
                        left: 8px;  /* 图标位置 */
                        width: 16px;
                        height: 16px;
                    }
                )");
        connect(m_playlistMenu, &QMenu::triggered, this, &Playlist::onPlaylistMenuTriggered);
    }
    m_playlistMenu->clear();

    // 添加默认列表
    QAction *defaultAction = m_playlistMenu->addAction("默认列表");
    defaultAction->setData("default");
    // 设置默认列表为可勾选
    defaultAction->setCheckable(true);
    // 如果当前是默认列表，则勾选
        if (m_currentPlaylistName == "默认列表") {
            defaultAction->setChecked(true);
        }
    // 添加分隔线
    m_playlistMenu->addSeparator();
    // 如果有用户自定义列表，添加分隔线和列表项
        if (!m_playlistInfos.isEmpty()) {
            m_playlistMenu->addSeparator();

            // 添加用户自定义列表
            for (const PlaylistInfo &info : m_playlistInfos) {
                if (!info.listName.isEmpty()) {
                    QAction *action = m_playlistMenu->addAction(info.listName);
                    action->setData(info.jsonPath);
                    // 设置可勾选
                    action->setCheckable(true);
                    // 如果当前是这个列表，则勾选
                    if (info.listName == m_currentPlaylistName) {
                        action->setChecked(true);
                    }
                    qDebug() << "添加播放列表到菜单：" << info.listName;
                }
            }
        }
        else {
                // 如果没有自定义列表，添加一个提示项
                QAction *emptyAction = m_playlistMenu->addAction("（暂无自定义列表）");
                emptyAction->setEnabled(false);
            }
}

// 加载播放列表索引文件
void Playlist::loadPlaylistIndex()
{
    m_playlistInfos.clear();
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString indexFile = documentsPath + "/MyPlayer/playlist_index.json";

    QFile file(indexFile);
    if (!file.exists()) {
        qDebug() << "播放列表索引文件不存在：" << indexFile;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开播放列表索引文件：" << indexFile;
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "播放列表索引文件格式错误：" << indexFile;
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray playlistArray = root["playlistName"].toArray();

    for (int i = 0; i < playlistArray.size(); i++) {
        QJsonObject obj = playlistArray[i].toObject();
        PlaylistInfo info;
        info.id = obj["id"].toInt();
        info.listName = obj["listName"].toString();
        info.jsonPath = obj["jsonPath"].toString();
        info.orderIndex = obj["orderIndex"].toInt();
        m_playlistInfos.append(info);
    }

    qDebug() << "加载了" << m_playlistInfos.size() << "个播放列表";
}

// 保存播放列表索引文件
void Playlist::savePlaylistIndex()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString configDir = documentsPath + "/MyPlayer/";
    QDir().mkpath(configDir);
    QString indexFile = configDir + "playlist_index.json";

    QJsonArray playlistArray;
    for (const PlaylistInfo &info : m_playlistInfos) {
        QJsonObject obj;
        obj["id"] = info.id;
        obj["listName"] = info.listName;
        obj["jsonPath"] = info.jsonPath;
        obj["orderIndex"] = info.orderIndex;
        playlistArray.append(obj);
    }

    QJsonObject root;
    root["playlistName"] = playlistArray;
    root["listCount"] = m_playlistInfos.size();

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson();

    QFile file(indexFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonData);
        file.close();
        qDebug() << "播放列表索引已保存到：" << indexFile;
    } else {
        qDebug() << "无法保存播放列表索引到：" << indexFile;
    }
}

// 切换到指定播放列表
void Playlist::switchToPlaylist(const QString &listName, const QString &jsonPath)
{
    // 保存当前播放列表
    saveAllData();
    // 先停止当前播放
        if (GlobalVars::currentPlayIndex() >= 0) {
            emit SigStop();
            // 等待一小段时间让播放器完全停止
            QEventLoop loop;
            QTimer::singleShot(50, &loop, &QEventLoop::quit);
            loop.exec();
        }
    // 清空当前数据


    // 更新当前播放列表信息
    m_currentPlaylistName = listName;
    m_currentPlaylistJson = jsonPath;
    m_isDefaultPlaylist = (listName == "默认列表");

    qDebug() << " 切换中－－－－－－－－－－－－－－－－－－－－－－－－－：" <<  m_currentPlaylistJson;
    // 更新按钮状态
    ui->btnDeleteCurrentList->setEnabled(!m_isDefaultPlaylist);

    // 更新标题
    updateCurrentPlaylistTitle();

    // 加载新的播放列表数据
    loadPlaylistFromFile(jsonPath);

    // 更新全局变量
    GlobalVars::playlistCount() = ui->List->count();

    // 发送播放列表改变信号
    emit SigPlaylistChanged();
    // 根据保存的 currentIndex 自动播放
       int savedIndex = GlobalVars::currentPlayIndex();
       if (savedIndex >= 0 && savedIndex < ui->List->count()) {
           qDebug() << "切换播放列表后自动播放索引：" << savedIndex;

           // 使用单次定时器延迟播放，确保UI已更新
           QTimer::singleShot(100, this, [this, savedIndex]() {
               PlayByIndex(savedIndex);
           });
       } else if (ui->List->count() > 0) {
           // 如果保存的索引无效，但列表不为空，选中第一项
           ui->List->setCurrentRow(0);
           GlobalVars::selectedIndex() = 0;
       }
}

// 从文件加载播放列表
void Playlist::loadPlaylistFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "播放列表文件不存在：" << filePath;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开播放列表文件：" << filePath;
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "播放列表文件格式错误：" << filePath;
        return;
    }

    QJsonObject configData = doc.object();
    QJsonArray playlistArray = configData["playlist"].toArray();

    m_itemData.clear();
    ui->List->clear();

    for (int i = 0; i < playlistArray.size(); i++) {
        QJsonObject itemData = playlistArray[i].toObject();

        QString filePath = itemData["filePath"].toString();
        QFileInfo fileInfo(filePath);

        if (fileInfo.exists()) {
            PlaylistItemData data;
            data.filePath = filePath;
            data.fileName = fileInfo.fileName();
            data.orderIndex = i;

            if (itemData.contains("duration") && itemData.contains("durationLoaded")) {
                data.duration = itemData["duration"].toInt();
                data.durationLoaded = itemData["durationLoaded"].toBool();
            } else {
                data.duration = 0;
                data.durationLoaded = false;
            }

            m_itemData[filePath] = data;

            QListWidgetItem *pItem = new QListWidgetItem(ui->List);
            pItem->setData(Qt::UserRole, QVariant(filePath));
            pItem->setToolTip(filePath);

            int iconSize = 24;
            QIcon formatIcon = GlobalHelper::GetFormatIcon(filePath, iconSize);
            pItem->setIcon(formatIcon);

            ui->List->addItem(pItem);
            updateItemDisplay(i);
        }
    }

    // 恢复播放索引
    if (configData.contains("currentIndex")) {
        int savedIndex = configData["currentIndex"].toInt();
        GlobalVars::currentPlayIndex() = savedIndex;  // 先设置全局变量
        if (savedIndex >= 0 && savedIndex < ui->List->count()) {
            ui->List->setCurrentRow(savedIndex);
            GlobalVars::selectedIndex() = savedIndex;
            m_nCurrentPlayListIndex = savedIndex;
            GlobalVars::currentPlayIndex() = savedIndex;
        }
    }else {
        // 如果没有保存的索引，重置为-1
        GlobalVars::currentPlayIndex() = -1;
        m_nCurrentPlayListIndex = -1;
    }

    // 如果列表不为空但当前索引无效，选中第一项
        if (ui->List->count() > 0 && (GlobalVars::currentPlayIndex() < 0 || GlobalVars::currentPlayIndex() >= ui->List->count())) {
            ui->List->setCurrentRow(0);
            GlobalVars::selectedIndex() = 0;
        }

    // 为所有没有时长的项启动后台加载
    for (int i = 0; i < ui->List->count(); i++) {
        QListWidgetItem* item = ui->List->item(i);
        if (item) {
            QString filePath = item->data(Qt::UserRole).toString();
            if (m_itemData.contains(filePath)) {
                if (!m_itemData[filePath].durationLoaded) {
                    loadDurationInBackground(i);
                }
            }
        }
    }

    updateButtonStates();
    qDebug() << "从文件加载播放列表：" << filePath << "，包含" << ui->List->count() << "个文件";
}

// 另存为新播放列表
void Playlist::saveAsNewPlaylist(const QString &newListName)
{
    if (newListName.isEmpty()) {
        return;
    }

    // 检查是否已存在同名列表
    for (const PlaylistInfo &info : m_playlistInfos) {
        if (info.listName == newListName) {
            QMessageBox::warning(this, "警告", "已存在同名播放列表，请使用其他名称。");
            return;
        }
    }

    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString configDir = documentsPath + "/MyPlayer/";

    // 生成新文件名
    QString baseName = newListName;
    baseName = baseName.replace(" ", "_").replace("/", "_").replace("\\", "_");
    QString newJsonFile = configDir + "playlist_" + baseName + ".json";

    // 保存当前播放列表到新文件
    QJsonArray playlistArray;

    for (int i = 0; i < ui->List->count(); i++) {
        QString filePath = ui->List->item(i)->data(Qt::UserRole).toString();
        QFileInfo fileInfo(filePath);

        QJsonObject itemData;
        itemData["orderIndex"] = i;
        itemData["filePath"] = filePath;
        itemData["fileName"] = fileInfo.fileName();

        if (m_itemData.contains(filePath) && m_itemData[filePath].durationLoaded) {
            itemData["duration"] = m_itemData[filePath].duration;
            itemData["durationLoaded"] = true;
        } else {
            itemData["duration"] = 0;
            itemData["durationLoaded"] = false;
        }

        playlistArray.append(itemData);
    }

    QJsonObject configData;
    configData["playlist"] = playlistArray;
    configData["currentIndex"] = GlobalVars::currentPlayIndex();
    configData["playMode"] = GlobalVars::playMode();
    configData["playlistCount"] = ui->List->count();
    configData["autoPlay"] = GlobalHelper::GetAutoPlay();

    QJsonDocument doc(configData);
    QByteArray jsonData = doc.toJson();

    QFile file(newJsonFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonData);
        file.close();
        qDebug() << "播放列表已另存为：" << newJsonFile;
    } else {
        QMessageBox::warning(this, "错误", "无法保存播放列表文件。");
        return;
    }

    // 添加到索引文件
    PlaylistInfo newInfo;
    newInfo.id = m_playlistInfos.isEmpty() ? 0 : m_playlistInfos.last().id + 1;
    newInfo.listName = newListName;
    newInfo.jsonPath = newJsonFile;
    newInfo.orderIndex = m_playlistInfos.size();

    m_playlistInfos.append(newInfo);
    savePlaylistIndex();

    // 更新菜单
    initListMenu();

    QMessageBox::information(this, "成功", QString("播放列表已另存为: %1").arg(newListName));
}

// 删除当前播放列表
void Playlist::deleteCurrentPlaylist()
{
    if (m_isDefaultPlaylist) {
        QMessageBox::warning(this, "警告", "默认列表不能删除。");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  QString("确定要删除播放列表 '%1' 吗？").arg(m_currentPlaylistName),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    // 删除JSON文件
    QFile file(m_currentPlaylistJson);
    if (file.exists()) {
        if (!file.remove()) {
            QMessageBox::warning(this, "错误", "无法删除播放列表文件。");
            return;
        }
    }

    // 从索引中移除
    for (int i = 0; i < m_playlistInfos.size(); i++) {
        if (m_playlistInfos[i].jsonPath == m_currentPlaylistJson) {
            m_playlistInfos.removeAt(i);
            break;
        }
    }

    // 保存索引
    savePlaylistIndex();

    // 切换到默认列表
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString defaultJson = documentsPath + "/MyPlayer/playlist_config.json";
    switchToPlaylist("默认列表", defaultJson);

    // 更新菜单
    initListMenu();

    QMessageBox::information(this, "成功", "播放列表已删除。");
}

// 更新当前播放列表标题
void Playlist::updateCurrentPlaylistTitle()
{
    ui->btnListTitle->setText(m_currentPlaylistName);
    ui->btnDeleteCurrentList->setEnabled(!m_isDefaultPlaylist);
}

// 点击列表标题
void Playlist::on_btnListTitle_clicked()
{
    if (m_playlistMenu) {
        m_playlistMenu->exec(ui->btnListTitle->mapToGlobal(
            QPoint(0, ui->btnListTitle->height())));
    }
}

// 另存为新列表按钮点击
// 另存为新列表按钮点击
#include <QInputDialog>
// 另存为新列表按钮点击
void Playlist::on_btnSaveToNewlist_clicked()
{
    QInputDialog dialog(this);

    // 移除帮助按钮，让对话框更简洁
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 设置窗口属性
    dialog.setWindowTitle("另存为新播放列表");
    dialog.setLabelText("请输入新播放列表名称:");
    dialog.setTextValue("");
    dialog.setInputMode(QInputDialog::TextInput);

    // **关键：关闭固定大小，改用resize并允许调整**
    dialog.setFixedSize(400, 100); // 修改为250高度

    // 暗色调样式表（修复了标签背景和高度问题）
    dialog.setStyleSheet(R"(
        /* 对话框整体样式 */
        QInputDialog {
            background-color: #1a1a1a;
            color: #e0e0e0;
            font-family: "Segoe UI", "Microsoft YaHei";
            border: 1px solid #333;
            border-radius: 8px;
        }

        /* 标签样式 - 关键：设置背景透明 */
        QLabel {
            color: #cccccc;
            font-size: 14px;
            height: 24px;
            font-weight: normal;
            background-color: transparent;  /* 设置为透明 */
            padding: 0px;
            margin: 0px 5px;
            border: none;  /* 移除边框 */
        }

        /* 输入框样式 */
        QLineEdit {
            background-color: #2a2a2a;
            border: 2px solid #444;
            border-radius: 6px;
            color: #ffffff;
            padding: 8px;
            font-size: 14px;
            selection-background-color: #3a6ea5;
            margin: 10px 5px;
            min-height: 20px;  /* 增加最小高度 */
        }

        /* 输入框聚焦状态 */
        QLineEdit:focus {
            border-color: #4a9eff;
            background-color: #252525;
        }

        /* 输入框占位符文本 */
        QLineEdit::placeholder {
            color: #888;
            font-style: italic;
        }

        /* 按钮容器样式 */
        QDialogButtonBox {
            background-color: transparent;
            spacing: 15px;
            padding-top: 15px;
            border-top: 1px solid #333;
            margin-top: 15px;
        }

        /* 按钮通用样式 */
        QPushButton {
            background-color: #2c3e50;
            color: #ecf0f1;
            border: 1px solid #34495e;
            border-radius: 5px;
            padding: 10px 28px;
            margin: 10px 5px;
            font-weight: 600;
            font-size: 14px;
            min-width: 60px;
            min-height: 25px;
        }

        /* 按钮悬停效果 */
        QPushButton:hover {
            background-color: #3498db;
            border-color: #2980b9;
            transform: translateY(-1px);
        }

        /* 按钮按下效果 */
        QPushButton:pressed {
            background-color: #2980b9;
            border-color: #1c5980;
            transform: translateY(0px);
        }

        /* 确定按钮特殊样式 */
        QPushButton#OK_Button {
            background-color: #3498db;
            border-color: #2980b9;
        }

        QPushButton#OK_Button:hover {
            background-color: #5dade2;
            border-color: #3498db;
        }

        /* 取消按钮样式 */
        QPushButton#Cancel_Button {
            background-color: #555;
            border-color: #666;
        }

        QPushButton#Cancel_Button:hover {
            background-color: #777;
            border-color: #888;
        }
    )");

    // 使用 exec() 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        QString newListName = dialog.textValue();
        if (!newListName.isEmpty()) {
            saveAsNewPlaylist(newListName);
        }
        qDebug() << "输入的内容:" << newListName;
    }
}

// 删除当前列表按钮点击
void Playlist::on_btnDeleteCurrentList_clicked()
{
    deleteCurrentPlaylist();
}

// 修改播放列表菜单项触发函数，确保正确更新勾选状态
void Playlist::onPlaylistMenuTriggered(QAction *action)
{
    if (!action) return;

    QString data = action->data().toString();

    // 首先取消所有动作的选中状态
    QList<QAction*> actions = m_playlistMenu->actions();
    for (QAction* act : actions) {
        if (act->isCheckable()) {
            act->setChecked(false);
        }
    }

    // 设置当前动作为选中状态
    action->setChecked(true);

    if (data == "default") {
        QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QString defaultJson = documentsPath + "/MyPlayer/playlist_config.json";
        switchToPlaylist("默认列表", defaultJson);
    } else {
        // 查找对应的播放列表信息
        for (const PlaylistInfo &info : m_playlistInfos) {
            if (info.jsonPath == data) {
                switchToPlaylist(info.listName, info.jsonPath);
                break;
            }
        }
    }
}

