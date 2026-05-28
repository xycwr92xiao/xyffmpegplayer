#include "settingwid.h"
#include <QDebug>
#include <QScreen>
#include <QMessageBox>
#include <QFontDatabase>
#include <QColorDialog>
#include "globalvars.h"
SettingWid::SettingWid(QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
    , m_settings("XyPlayer", "PlayerSettings")
    , m_currentFontSize(16)
    , m_currentTextColor(Qt::white)
    , m_currentStrokeColor(Qt::black)
    , m_currentHoverBgColor(QColor(0, 0, 0, 150))
    , m_currentLeaveBgColor(QColor(0, 0, 0, 100))
    , m_currentKeepBackground(false)
    , m_currentMiaobianOrYinying(1)
{
    ui.setupUi(this);
    setWindowModality(Qt::WindowModal);
    setWindowTitle("设置");
    // 设置为对话框样式
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() | Qt::Tool);
    qreal scale = 1.0;
        if (QScreen *screen = this->screen()) {
            scale = screen->logicalDotsPerInch() / 96.0;
        } else if (QScreen *primaryScreen = QGuiApplication::primaryScreen()) {
            scale = primaryScreen->logicalDotsPerInch() / 96.0;
        }
        qDebug() << "scale =------------------------------------------------------------ " << scale;
    int nHeight = scale==1?620:(scale==1.25?650:(scale==1.5?690:720));
    int nWidth = scale==1?550:(scale==1.25?560:(scale==1.5?570:580));
    resize(nWidth, nHeight);  // 宽度500，高度600
    // 设置对话框的最小大小
    setMinimumSize(550, 600);
    // 初始化UI
    initUI();

    // 加载保存的设置
    loadSettings();

    // 设置临时值
    m_tempFontFamily = m_currentFontFamily;
    m_tempFontSize = m_currentFontSize;
    m_tempspectrumMode = m_spectrumMode;
    // 设置临时值
    m_tempFontFamily = m_currentFontFamily;
    m_tempFontSize = m_currentFontSize;
    m_tempTextColor = m_currentTextColor;
    m_tempStrokeColor = m_currentStrokeColor;
    m_tempHoverBgColor = m_currentHoverBgColor;
    m_tempLeaveBgColor = m_currentLeaveBgColor;
    m_tempKeepBackground = m_currentKeepBackground;
    m_tempMiaobianOrYinying = m_currentMiaobianOrYinying;
    // 最后确保预览更新
    updatePreview();
}

SettingWid::~SettingWid()
{
}

void SettingWid::initUI()
{
    // 填充字体列表
    populateFontFamilies();
    ui.labelPreview->setStyleSheet("");  // 清除样式，避免冲突
    ui.labelPreview->setScaledContents(false); // 不缩放，保持 pixmap 原始大小
    // 设置字号范围
    ui.spinFontSize->setRange(8, 72);
    ui.spinFontSize->setSingleStep(1);
    // 设置颜色按钮样式
        QString btnStyle = "QPushButton { min-width: 60px; max-width: 60px; min-height: 25px; max-height: 25px; border: 1px solid #666; }";
        ui.btnTextColor->setStyleSheet(btnStyle + "background-color: " + m_currentTextColor.name() + ";");
        ui.btnStrokeColor->setStyleSheet(btnStyle + "background-color: " + m_currentStrokeColor.name() + ";");
        ui.btnHoverBgColor->setStyleSheet(btnStyle + "background-color: " + m_currentHoverBgColor.name() + ";");
        ui.btnLeaveBgColor->setStyleSheet(btnStyle + "background-color: " + m_currentLeaveBgColor.name() + ";");

        // 更新颜色标签
        updateColorLabel(ui.labelTextColorDisplay, m_currentTextColor);
        updateColorLabel(ui.labelStrokeColorDisplay, m_currentStrokeColor);
        updateColorLabel(ui.labelHoverBgColorDisplay, m_currentHoverBgColor);
        updateColorLabel(ui.labelLeaveBgColorDisplay, m_currentLeaveBgColor);

        // 设置复选框
        ui.checkKeepBackground->setChecked(m_currentKeepBackground);
        m_spectrumMode = GlobalVars::spectrumMode();
        ui.sepMode1->setChecked(m_spectrumMode==0);
        ui.sepMode2->setChecked(m_spectrumMode==1);
        ui.sepMode3->setChecked(m_spectrumMode==2);
        ui.btnLeaveBgColor->setEnabled(m_currentKeepBackground);
        ui.labelLeaveBgColorDisplay->setEnabled(m_currentKeepBackground);
        ui.checkMiaobian->setChecked(m_currentMiaobianOrYinying==1 || m_currentMiaobianOrYinying==3);
        ui.checkYinying->setChecked(m_currentMiaobianOrYinying>1);
        ui.labelPreview->setScaledContents(false);  // 保持预览图片原始大小
        ui.labelPreview->setMinimumHeight(190);        // 确保即使在窗口最小时也能显示足够内容
            ui.labelPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 连接信号槽
    setupConnections();
}
void SettingWid::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    updatePreview();
}
void SettingWid::populateFontFamilies()
{
    ui.comboFontFamily->clear();

    // 获取系统可用字体
    QFontDatabase fontDb;
    QStringList fontFamilies = fontDb.families();

    // 过滤出常用中文字体和英文字体
    QStringList preferredFonts = {
        "Microsoft YaHei",      // 微软雅黑
        "SimHei",              // 黑体
        "SimSun",              // 宋体
        "KaiTi",               // 楷体
        "FangSong",            // 仿宋
        "NSimSun",             // 新宋体
        "Arial",
        "Times New Roman",
        "Verdana",
        "Tahoma",
        "Courier New"
    };

    // 添加首选字体
    for (const QString &font : preferredFonts) {
        if (fontFamilies.contains(font)) {
            ui.comboFontFamily->addItem(font);
        }
    }

    // 添加分隔线
    ui.comboFontFamily->insertSeparator(ui.comboFontFamily->count());

    // 添加其他字体
    for (const QString &font : fontFamilies) {
        if (!preferredFonts.contains(font)) {
            ui.comboFontFamily->addItem(font);
        }
    }
}

void SettingWid::setupConnections()
{
    // 字体选择变化
    connect(ui.comboFontFamily, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingWid::onFontFamilyChanged);

    // 字号变化
    connect(ui.spinFontSize, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingWid::onFontSizeChanged);
    // 颜色按钮点击
        connect(ui.btnTextColor, &QPushButton::clicked,
                this, &SettingWid::onTextColorClicked);
        connect(ui.btnStrokeColor, &QPushButton::clicked,
                this, &SettingWid::onStrokeColorClicked);
        connect(ui.btnHoverBgColor, &QPushButton::clicked,
                this, &SettingWid::onHoverBgColorClicked);
        connect(ui.btnLeaveBgColor, &QPushButton::clicked,
                this, &SettingWid::onLeaveBgColorClicked);

        // 复选框变化
        connect(ui.checkKeepBackground, &QCheckBox::stateChanged,
                this, &SettingWid::onKeepBackgroundChanged);
        connect(ui.checkMiaobian, &QCheckBox::stateChanged,
                this, &SettingWid::onMiaobianChanged);
        connect(ui.checkYinying, &QCheckBox::stateChanged,
                this, &SettingWid::onYinyingChanged);
        connect(ui.sepMode1, &QRadioButton::clicked,
                this, &SettingWid::onSpectrumModeChanged1);
        connect(ui.sepMode2, &QRadioButton::clicked,
                this, &SettingWid::onSpectrumModeChanged2);
        connect(ui.sepMode3, &QRadioButton::clicked,
                this, &SettingWid::onSpectrumModeChanged3);
    // 按钮点击
    connect(ui.btnApply, &QPushButton::clicked,
            this, &SettingWid::onApplyClicked);
    connect(ui.btnCancel, &QPushButton::clicked,
            this, &SettingWid::onCancelClicked);
    connect(ui.btnOk, &QPushButton::clicked,
            this, &SettingWid::onOkClicked);
}

void SettingWid::saveSettings()
{
    // 保存字幕设置
    m_settings.beginGroup("Subtitle");
    m_settings.setValue("FontFamily", m_currentFontFamily);
    m_settings.setValue("FontSize", m_currentFontSize);
    m_settings.setValue("spectrumMode", m_spectrumMode);
    m_settings.setValue("TextColor", m_currentTextColor.name(QColor::HexArgb));
    m_settings.setValue("StrokeColor", m_currentStrokeColor.name(QColor::HexArgb));
    m_settings.setValue("HoverBgColor", m_currentHoverBgColor.name(QColor::HexArgb));
    m_settings.setValue("LeaveBgColor", m_currentLeaveBgColor.name(QColor::HexArgb));
    m_settings.setValue("KeepBackground", m_currentKeepBackground);
    m_settings.setValue("MiaobianOrYinying", m_currentMiaobianOrYinying);
    m_settings.endGroup();
    m_settings.sync();
    qDebug() << "设置已保存：" << m_currentFontFamily << m_currentFontSize;
}

void SettingWid::loadSettings()
{
    // 加载字幕设置
    m_settings.beginGroup("Subtitle");
    m_currentFontFamily = m_settings.value("FontFamily", "Microsoft YaHei").toString();
    m_currentFontSize = m_settings.value("FontSize", 16).toInt();
    m_spectrumMode= m_settings.value("spectrumMode", 0).toInt();
    m_currentTextColor = QColor(m_settings.value("TextColor", "#FFFFFFFF").toString());
        m_currentStrokeColor = QColor(m_settings.value("StrokeColor", "#FF000000").toString());
        m_currentHoverBgColor = QColor(m_settings.value("HoverBgColor", "#96000000").toString());
        m_currentLeaveBgColor = QColor(m_settings.value("LeaveBgColor", "#64000000").toString());
        m_currentKeepBackground = m_settings.value("KeepBackground", false).toBool();
        m_currentMiaobianOrYinying = m_settings.value("MiaobianOrYinying", 1).toInt();
    m_settings.endGroup();

    // 更新UI
    int fontIndex = ui.comboFontFamily->findText(m_currentFontFamily);
    if (fontIndex >= 0) {
        ui.comboFontFamily->setCurrentIndex(fontIndex);
    } else {
        ui.comboFontFamily->setCurrentIndex(0); // 默认第一个
    }

    ui.spinFontSize->setValue(m_currentFontSize);

    // 更新颜色按钮和标签
        updateColorButton(ui.btnTextColor, m_currentTextColor);
        updateColorButton(ui.btnStrokeColor, m_currentStrokeColor);
        updateColorButton(ui.btnHoverBgColor, m_currentHoverBgColor);
        updateColorButton(ui.btnLeaveBgColor, m_currentLeaveBgColor);

        updateColorLabel(ui.labelTextColorDisplay, m_currentTextColor);
        updateColorLabel(ui.labelStrokeColorDisplay, m_currentStrokeColor);
        updateColorLabel(ui.labelHoverBgColorDisplay, m_currentHoverBgColor);
        updateColorLabel(ui.labelLeaveBgColorDisplay, m_currentLeaveBgColor);

        ui.checkKeepBackground->setChecked(m_currentKeepBackground);
        ui.checkMiaobian->setChecked(m_currentMiaobianOrYinying==1 || m_currentMiaobianOrYinying==3);
        ui.checkYinying->setChecked(m_currentMiaobianOrYinying>1);
        ui.btnLeaveBgColor->setEnabled(m_currentKeepBackground);
        ui.labelLeaveBgColorDisplay->setEnabled(m_currentKeepBackground);

    // 更新预览
    //updatePreview();
}

QString SettingWid::getSubtitleFontFamily() const
{
    return m_currentFontFamily;
}

int SettingWid::getSubtitleFontSize() const
{
    return m_currentFontSize;
}
QColor SettingWid::getSubtitleTextColor() const
{
    return m_currentTextColor;
}

QColor SettingWid::getSubtitleStrokeColor() const
{
    return m_currentStrokeColor;
}

QColor SettingWid::getSubtitleHoverBgColor() const
{
    return m_currentHoverBgColor;
}

QColor SettingWid::getSubtitleLeaveBgColor() const
{
    return m_currentLeaveBgColor;
}

bool SettingWid::getSubtitleKeepBackground() const
{
    return m_currentKeepBackground;
}

void SettingWid::setSubtitleFont(const QString &family, int size)
{
    m_currentFontFamily = family;
    m_currentFontSize = size;

    // 更新UI
    int fontIndex = ui.comboFontFamily->findText(family);
    if (fontIndex >= 0) {
        ui.comboFontFamily->setCurrentIndex(fontIndex);
    }

    ui.spinFontSize->setValue(size);
    updatePreview();
}
void SettingWid::setSubtitleColors(const QColor &textColor, const QColor &strokeColor,
                                  const QColor &hoverBgColor, const QColor &leaveBgColor,
                                  bool keepBackground)
{
    m_currentTextColor = textColor;
    m_currentStrokeColor = strokeColor;
    m_currentHoverBgColor = hoverBgColor;
    m_currentLeaveBgColor = leaveBgColor;
    m_currentKeepBackground = keepBackground;

    updateColorButton(ui.btnTextColor, textColor);
    updateColorButton(ui.btnStrokeColor, strokeColor);
    updateColorButton(ui.btnHoverBgColor, hoverBgColor);
    updateColorButton(ui.btnLeaveBgColor, leaveBgColor);

    updateColorLabel(ui.labelTextColorDisplay, textColor);
    updateColorLabel(ui.labelStrokeColorDisplay, strokeColor);
    updateColorLabel(ui.labelHoverBgColorDisplay, hoverBgColor);
    updateColorLabel(ui.labelLeaveBgColorDisplay, leaveBgColor);

    ui.checkKeepBackground->setChecked(keepBackground);
    ui.btnLeaveBgColor->setEnabled(keepBackground);
    ui.labelLeaveBgColorDisplay->setEnabled(keepBackground);

    updatePreview();
}

void SettingWid::updateColorButton(QPushButton *btn, const QColor &color)
{
    QString style = QString("QPushButton { min-width: 60px; max-width: 60px; min-height: 25px; max-height: 25px; border: 1px solid #666; background-color: %1; }")
                    .arg(color.name(QColor::HexArgb));
    btn->setStyleSheet(style);
}

void SettingWid::updateColorLabel(QLabel *label, const QColor &color)
{
    QString colorName;
    if (color == Qt::white) colorName = "白色";
    else if (color == Qt::black) colorName = "黑色";
    else if (color.alpha() < 255) colorName = QString("透明%1").arg(colorToString(color));
    else colorName = colorToString(color);

    label->setText(colorName);
}

QString SettingWid::colorToString(const QColor &color) const
{
    QString colorName;

        // 判断颜色类型
        if (color == Qt::white) colorName = "白色";
        else if (color == Qt::black) colorName = "黑色";
        else if (color == Qt::red) colorName = "红色";
        else if (color == Qt::green) colorName = "绿色";
        else if (color == Qt::blue) colorName = "蓝色";
        else if (color == Qt::yellow) colorName = "黄色";
        else if (color == Qt::cyan) colorName = "青色";
        else if (color == Qt::magenta) colorName = "洋红";
        else if (color == Qt::gray) colorName = "灰色";
        else if (color == Qt::darkRed) colorName = "深红";
        else if (color == Qt::darkGreen) colorName = "深绿";
        else if (color == Qt::darkBlue) colorName = "深蓝";
        else if (color == Qt::darkYellow) colorName = "深黄";
        else if (color == Qt::darkCyan) colorName = "深青";
        else if (color == Qt::darkMagenta) colorName = "深洋红";
        else if (color == Qt::darkGray) colorName = "深灰";
        else if (color == Qt::lightGray) colorName = "浅灰";
        else {
            // 自定义颜色，显示RGB值
            colorName = QString("RGB(%1,%2,%3)")
                        .arg(color.red())
                        .arg(color.green())
                        .arg(color.blue());
        }

        // 添加透明度信息
        if (color.alpha() < 255) {
            int alphaPercent = (color.alpha() * 100) / 255;
            colorName += QString(" [%1%透明]").arg(alphaPercent);
        }
        return colorName;
}

void SettingWid::onFontFamilyChanged(int index)
{
    if (index < 0) return;

    QString fontFamily = ui.comboFontFamily->currentText();

    // 检查是否为分隔线
    if (fontFamily.isEmpty()) {
        // 如果是分隔线，恢复之前的选择
        int fontIndex = ui.comboFontFamily->findText(m_tempFontFamily);
        if (fontIndex >= 0) {
            ui.comboFontFamily->setCurrentIndex(fontIndex);
        }
        return;
    }

    m_tempFontFamily = fontFamily;
    updatePreview();
    ui.btnApply->setEnabled(true);
}

void SettingWid::onFontSizeChanged(int value)
{
    m_tempFontSize = value;
    updatePreview();
    ui.btnApply->setEnabled(true);
}

void SettingWid::onTextColorClicked()
{
    QColor color = QColorDialog::getColor(m_tempTextColor, this, "选择文字颜色");
    if (color.isValid()) {
        m_tempTextColor = color;
        updateColorButton(ui.btnTextColor, color);
        updateColorLabel(ui.labelTextColorDisplay, color);
        updatePreview();
        ui.btnApply->setEnabled(true);
    }
}

void SettingWid::onStrokeColorClicked()
{
    QColor color = QColorDialog::getColor(m_tempStrokeColor, this, "选择描边颜色");
    if (color.isValid()) {
        m_tempStrokeColor = color;
        updateColorButton(ui.btnStrokeColor, color);
        updateColorLabel(ui.labelStrokeColorDisplay, color);
        updatePreview();
        ui.btnApply->setEnabled(true);
    }
}

void SettingWid::onHoverBgColorClicked()
{
    QColorDialog dialog;
        dialog.setOptions(QColorDialog::ShowAlphaChannel);
        dialog.setCurrentColor(m_tempHoverBgColor);

        if (dialog.exec() == QDialog::Accepted) {
            QColor color = dialog.selectedColor();
            if (color.isValid()) {
                m_tempHoverBgColor = color;
                updateColorButton(ui.btnHoverBgColor, color);
                updateColorLabel(ui.labelHoverBgColorDisplay, color);
                updatePreview();
                ui.btnApply->setEnabled(true);
            }
        }
}

void SettingWid::onLeaveBgColorClicked()
{
    QColorDialog dialog;
        dialog.setOptions(QColorDialog::ShowAlphaChannel);
        dialog.setCurrentColor(m_tempLeaveBgColor);

        if (dialog.exec() == QDialog::Accepted) {
            QColor color = dialog.selectedColor();
            if (color.isValid()) {
                m_tempLeaveBgColor = color;
                updateColorButton(ui.btnLeaveBgColor, color);
                updateColorLabel(ui.labelLeaveBgColorDisplay, color);
                updatePreview();
                ui.btnApply->setEnabled(true);
            }
        }
}

void SettingWid::onKeepBackgroundChanged(int state)
{
    m_tempKeepBackground = (state == Qt::Checked);
    ui.btnLeaveBgColor->setEnabled(m_tempKeepBackground);
    ui.labelLeaveBgColorDisplay->setEnabled(m_tempKeepBackground);
    updatePreview();
    ui.btnApply->setEnabled(true);
}
void SettingWid::onMiaobianChanged(int state)
{
    m_tempMiaobianOrYinying = (state == Qt::Checked)?(m_tempMiaobianOrYinying <=1?1:3):(m_tempMiaobianOrYinying ==3?2:0);
    updatePreview();
    ui.btnApply->setEnabled(true);
}
void SettingWid::onYinyingChanged(int state)
{
m_tempMiaobianOrYinying = (state == Qt::Checked)?(m_tempMiaobianOrYinying % 2 == 0?2:3):(m_tempMiaobianOrYinying % 2 == 0?0:1);
    updatePreview();
    ui.btnApply->setEnabled(true);
}
void SettingWid::onSpectrumModeChanged1()
{
    m_tempspectrumMode = 0;
    ui.btnApply->setEnabled(true);
}
void SettingWid::onSpectrumModeChanged2()
{
    m_tempspectrumMode = 1;
    ui.btnApply->setEnabled(true);
}
void SettingWid::onSpectrumModeChanged3()
{
    m_tempspectrumMode = 2;
    ui.btnApply->setEnabled(true);
}
void SettingWid::updatePreview()
{
    // 1. 获取预览控件的逻辑尺寸（固定为 512x154，避免布局影响）
    QSize physicalSize(this->width()-38, 154);
    ui.labelPreview->setFixedSize(physicalSize);          // 固定大小
    ui.labelPreview->setScaledContents(false);        // 禁止缩放
    // 2. 获取设备像素比（例如 125% -> 1.25）
    // 3. 创建高分辨率 Pixmap 并设置设备像素比
    QPixmap pixmap(physicalSize);
    pixmap.setDevicePixelRatio(1);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // 4. 动态调整背景色（保留你的原逻辑）
    QColor bgColor = m_tempLeaveBgColor;
    QColor strokeColor = m_tempStrokeColor;
    int dr = bgColor.red() - strokeColor.red();
    int dg = bgColor.green() - strokeColor.green();
    int db = bgColor.blue() - strokeColor.blue();
    if (qAbs(dr) + qAbs(dg) + qAbs(db) < 60) {
        int r = qMin(255, strokeColor.red() + 70);
        int g = qMin(255, strokeColor.green() + 70);
        int b = qMin(255, strokeColor.blue() + 70);
        bgColor = QColor(r, g, b);
    }

    // 5. 定义绘制区域（距离边缘保留 4 物理像素的安全边距）
    int safeMargin = 4;            // 物理像素边距
    QRectF drawRect = QRectF(QPointF(0,0), physicalSize).adjusted(safeMargin, safeMargin, -safeMargin, -safeMargin);
    qreal radius = 8;              // 圆角半径随 DPI 缩放

    // 6. 绘制背景（仅限圆角矩形内部）
    QPainterPath path;
    path.addRoundedRect(drawRect, radius, radius);
    painter.setClipPath(path);
    painter.fillRect(drawRect, bgColor);

    // 7. 绘制边框（线宽 2 物理像素）
    painter.setClipRect(pixmap.rect());   // 恢复裁剪区域
    painter.setPen(QPen(m_tempStrokeColor, 2));
    painter.drawRoundedRect(drawRect, radius, radius);

    // 8. 绘制文本（在边框内部再留内边距）
    int textPadding = 8;
    QRectF textRect = drawRect.adjusted(textPadding, textPadding, -textPadding, -textPadding);
    painter.setClipRect(textRect);

    // 字体设置
    QString fontFamily = ui.comboFontFamily->currentText();
    int fontSize = ui.spinFontSize->value();
    if (fontSize <= 9) fontSize = 29;
    QFont font;
    font.setFamily(fontFamily);
    font.setPixelSize(fontSize);
    font.setBold(true);
    painter.setFont(font);

    // 预览文本
    QString previewText = QString("字体预览: %1\n").arg(fontFamily);
    previewText += "设置: =8像素，代表单行/自适应\n";
    previewText += "9-72PX: 多行/=9: 自动";

    QStringList lines = previewText.split('\n');
    QFontMetricsF fm(font);
    qreal lineHeight = fm.height();
    qreal totalHeight = lines.size() * lineHeight;
    qreal startY = textRect.top() + (textRect.height() - totalHeight) / 2;

    // 绘制每行文字（带描边/阴影效果）
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        if (line.isEmpty()) continue;
        qreal lineWidth = fm.horizontalAdvance(line);
        qreal x = textRect.left() + (textRect.width() - lineWidth) / 2;
        qreal y = startY + i * lineHeight + fm.ascent();

        int effect = m_tempMiaobianOrYinying;
        if (effect == 0) {
            painter.setPen(m_tempTextColor);
            painter.drawText(QPointF(x, y), line);
        } else
        {if (effect == 1 || effect == 3) {
            // 描边：8方向偏移 1 物理像素
            painter.setPen(Qt::NoPen);
            qreal offset = 1;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    painter.setPen(QPen(m_tempStrokeColor, 2));
                    painter.drawText(QPointF(x + dx * offset, y + dy * offset), line);
                }
            }
            painter.setPen(QPen(m_tempTextColor, 1));
            painter.drawText(QPointF(x, y), line);
        }
        if (effect > 1) {
            // 阴影：偏移 3 物理像素
            qreal shadowOffset = 3;
            painter.setPen(QColor(0, 0, 0, 128));
            painter.drawText(QPointF(x + shadowOffset, y + shadowOffset), line);
            painter.setPen(m_tempTextColor);
            painter.drawText(QPointF(x, y), line);
        }
        }
    }
    painter.end();
    ui.labelPreview->setPixmap(pixmap);
}

void SettingWid::onApplyClicked()
{
    // 应用设置
    m_currentFontFamily = m_tempFontFamily;
    m_currentFontSize = m_tempFontSize;
    m_currentTextColor = m_tempTextColor;
    m_currentStrokeColor = m_tempStrokeColor;
    m_currentHoverBgColor = m_tempHoverBgColor;
    m_currentLeaveBgColor = m_tempLeaveBgColor;
    m_currentKeepBackground = m_tempKeepBackground;
    m_currentMiaobianOrYinying = m_tempMiaobianOrYinying;
    m_spectrumMode = m_tempspectrumMode;
    qDebug() << "m_tempspectrumMode = " << m_tempspectrumMode;
    // 保存到配置文件
    saveSettings();

    // 更新全局变量
    GlobalVars::subtitleFontFamily() = m_currentFontFamily;
    GlobalVars::subtitleFontSize() = m_currentFontSize;
    GlobalVars::spectrumMode() = m_spectrumMode;
    GlobalVars::subtitleTextColor() = m_currentTextColor;
    qDebug() << "保存的颜色是：" << m_currentTextColor;
    GlobalVars::subtitleStrokeColor() = m_currentStrokeColor;
    GlobalVars::subtitleHoverBgColor() = m_currentHoverBgColor;
    GlobalVars::subtitleLeaveBgColor() = m_currentLeaveBgColor;
    GlobalVars::subtitleKeepBackground() = m_currentKeepBackground;
    GlobalVars::miaobianOrYinying() = m_currentMiaobianOrYinying;
    // 发送设置变化信号
    emit subtitleSettingsChanged(m_currentFontFamily, m_currentFontSize);
    emit subtitleColorSettingsChanged(m_currentTextColor, m_currentStrokeColor,
                                     m_currentHoverBgColor, m_currentLeaveBgColor,
                                     m_currentKeepBackground);

    ui.btnApply->setEnabled(false);

    qDebug() << "设置已应用：GlobalVars::miaobianOrYinying()=========================" << GlobalVars::miaobianOrYinying() <<":"<< m_currentMiaobianOrYinying;
    qDebug() << "颜色设置已应用：" << m_currentTextColor << m_currentStrokeColor;
}

void SettingWid::onCancelClicked()
{
    // 恢复临时值
    m_tempFontFamily = m_currentFontFamily;
    m_tempFontSize = m_currentFontSize;
    m_tempTextColor = m_currentTextColor;
    m_tempStrokeColor = m_currentStrokeColor;
    m_tempHoverBgColor = m_currentHoverBgColor;
    m_tempLeaveBgColor = m_currentLeaveBgColor;
    m_tempKeepBackground = m_currentKeepBackground;
    m_tempMiaobianOrYinying = m_currentMiaobianOrYinying;
    // 更新UI
    int fontIndex = ui.comboFontFamily->findText(m_currentFontFamily);
    if (fontIndex >= 0) {
        ui.comboFontFamily->setCurrentIndex(fontIndex);
    }

    ui.spinFontSize->setValue(m_currentFontSize);

    updateColorButton(ui.btnTextColor, m_currentTextColor);
    updateColorButton(ui.btnStrokeColor, m_currentStrokeColor);
    updateColorButton(ui.btnHoverBgColor, m_currentHoverBgColor);
    updateColorButton(ui.btnLeaveBgColor, m_currentLeaveBgColor);

    updateColorLabel(ui.labelTextColorDisplay, m_currentTextColor);
    updateColorLabel(ui.labelStrokeColorDisplay, m_currentStrokeColor);
    updateColorLabel(ui.labelHoverBgColorDisplay, m_currentHoverBgColor);
    updateColorLabel(ui.labelLeaveBgColorDisplay, m_currentLeaveBgColor);

    ui.checkKeepBackground->setChecked(m_currentKeepBackground);
    ui.btnLeaveBgColor->setEnabled(m_currentKeepBackground);
    ui.labelLeaveBgColorDisplay->setEnabled(m_currentKeepBackground);

    updatePreview();

    ui.btnApply->setEnabled(false);

    // 关闭窗口
    accept();
}

void SettingWid::onOkClicked()
{
    // 先应用设置
    onApplyClicked();
    // 关闭窗口
    accept();
}
