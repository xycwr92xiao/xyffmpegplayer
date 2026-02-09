// File: equalizerdialog.cpp
#include "equalizerdialog.h"
#include <QGridLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QSpinBox>
#include <QDebug>

EqualizerDialog::EqualizerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("均衡器设置"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(800, 500);

    initPresets();
    initUI();
}

EqualizerDialog::~EqualizerDialog()
{
}

void EqualizerDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 启用复选框
    QHBoxLayout *enableLayout = new QHBoxLayout();
    m_enableCheckBox = new QCheckBox(tr("启用均衡器"), this);
    enableLayout->addWidget(m_enableCheckBox);
    enableLayout->addStretch();
    mainLayout->addLayout(enableLayout);

    // 预设选择
    QHBoxLayout *presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel(tr("预置:"), this));
    m_presetComboBox = new QComboBox(this);
    for (const auto &preset : m_presets) {
        m_presetComboBox->addItem(preset.name);
    }
    m_presetComboBox->setCurrentIndex(0); // 默认为"Flat"
    presetLayout->addWidget(m_presetComboBox);
    presetLayout->addStretch();
    mainLayout->addLayout(presetLayout);

    // 10段均衡器
    QGroupBox *eqGroup = new QGroupBox(tr("十段均衡器"), this);
    QGridLayout *eqGrid = new QGridLayout();

    for (int i = 0; i < 10; i++) {
        // 频率标签
        m_freqLabels[i] = new QLabel(QString("%1Hz").arg(m_frequencies[i]), this);
        m_freqLabels[i]->setAlignment(Qt::AlignCenter);
        eqGrid->addWidget(m_freqLabels[i], 0, i + 1);

        // 垂直滑块
        m_eqSliders[i] = new QSlider(Qt::Vertical, this);
        m_eqSliders[i]->setRange(-12, 12); // -12dB 到 +12dB
        m_eqSliders[i]->setValue(0);
        m_eqSliders[i]->setTickPosition(QSlider::TicksBothSides);
        m_eqSliders[i]->setTickInterval(3);
        m_eqSliders[i]->setSingleStep(1);
        m_eqSliders[i]->setPageStep(3);
        eqGrid->addWidget(m_eqSliders[i], 1, i + 1, Qt::AlignHCenter);

        // 数值标签
        m_eqLabels[i] = new QLabel("0dB", this);
        m_eqLabels[i]->setAlignment(Qt::AlignCenter);
        m_eqLabels[i]->setMinimumWidth(40);
        eqGrid->addWidget(m_eqLabels[i], 2, i + 1);

        connect(m_eqSliders[i], &QSlider::valueChanged, this, &EqualizerDialog::onSliderValueChanged);
    }

    // 添加刻度标签
    eqGrid->addWidget(new QLabel("+12dB", this), 1, 0, Qt::AlignRight);
    eqGrid->addWidget(new QLabel("0dB", this), 2, 0, Qt::AlignRight);
    eqGrid->addWidget(new QLabel("-12dB", this), 3, 0, Qt::AlignRight);

    eqGroup->setLayout(eqGrid);
    mainLayout->addWidget(eqGroup);

    // 左右声道平衡
    QGroupBox *balanceGroup = new QGroupBox(tr("左右声道平衡"), this);
    QHBoxLayout *balanceLayout = new QHBoxLayout();

    m_leftLabel = new QLabel(tr("左"), this);
    m_leftLabel->setAlignment(Qt::AlignCenter);

    m_balanceSlider = new QSlider(Qt::Horizontal, this);
    m_balanceSlider->setRange(-100, 100); // -100左全, 0中, 100右全
    m_balanceSlider->setValue(0);
    m_balanceSlider->setTickPosition(QSlider::TicksBelow);
    m_balanceSlider->setTickInterval(50);

    m_balanceLabel = new QLabel(tr("中"), this);
    m_balanceLabel->setAlignment(Qt::AlignCenter);
    m_balanceLabel->setMinimumWidth(30);

    m_rightLabel = new QLabel(tr("右"), this);
    m_rightLabel->setAlignment(Qt::AlignCenter);

    balanceLayout->addWidget(m_leftLabel);
    balanceLayout->addWidget(m_balanceSlider);
    balanceLayout->addWidget(m_balanceLabel);
    balanceLayout->addWidget(m_rightLabel);

    connect(m_balanceSlider, &QSlider::valueChanged, this, [this](int value) {
        if (value < 0) {
            m_balanceLabel->setText(QString("左%1").arg(-value));
        } else if (value > 0) {
            m_balanceLabel->setText(QString("右%1").arg(value));
        } else {
            m_balanceLabel->setText(tr("中"));
        }
        emit equalizerChanged(getEqualizerGains(), value, m_enableCheckBox->isChecked());
    });

    balanceGroup->setLayout(balanceLayout);
    mainLayout->addWidget(balanceGroup);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_resetButton = new QPushButton(tr("重置"), this);
    m_applyButton = new QPushButton(tr("应用"), this);
    m_closeButton = new QPushButton(tr("关闭"), this);

    connect(m_resetButton, &QPushButton::clicked, this, &EqualizerDialog::onResetClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &EqualizerDialog::onApplyClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &EqualizerDialog::close);

    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(m_enableCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit equalizerChanged(getEqualizerGains(), m_balanceSlider->value(), checked);
    });

    connect(m_presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EqualizerDialog::onPresetChanged);

    // 初始更新标签
    updateLabels();
}

void EqualizerDialog::initPresets()
{
    // 预定义均衡器预设
    m_presets = {
        {"Flat", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0},           // 平直
        {"Classical", {4, 3, 2, 0, -1, -2, -1, 0, 2, 3}, 0},   // 古典
        {"Rock", {6, 4, 2, 1, 0, -1, 1, 3, 4, 5}, 0},          // 摇滚
        {"Pop", {-1, 1, 3, 4, 4, 3, 1, 0, -1, -1}, 0},         // 流行
        {"Jazz", {3, 2, 1, 0, -1, -1, 0, 1, 2, 3}, 0},         // 爵士
        {"Bass", {8, 6, 4, 2, 0, -2, -4, -6, -8, -8}, 0},      // 重低音
        {"Treble", {-8, -6, -4, -2, 0, 2, 4, 6, 8, 8}, 0},     // 高音增强
        {"Vocal", {-3, -2, -1, 0, 2, 3, 2, 0, -1, -2}, 0},     // 人声增强
        {"Custom", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0}         // 自定义
    };
}

void EqualizerDialog::updateLabels()
{
    for (int i = 0; i < 10; i++) {
        int value = m_eqSliders[i]->value();
        m_eqLabels[i]->setText(QString("%1dB").arg(value));
    }
}

void EqualizerDialog::onSliderValueChanged()
{
    updateLabels();

    // 切换到自定义预设
    if (m_presetComboBox->currentIndex() != 8) { // 8是Custom
        m_presetComboBox->setCurrentIndex(8);
    }

    emit equalizerChanged(getEqualizerGains(), m_balanceSlider->value(),
                         m_enableCheckBox->isChecked());
}

void EqualizerDialog::onPresetChanged(int index)
{
    if (index >= 0 && index < m_presets.size()) {
        const auto &preset = m_presets[index];

        // 应用预设值
        for (int i = 0; i < 10; i++) {
            m_eqSliders[i]->setValue(preset.gains[i]);
        }
        m_balanceSlider->setValue(preset.balance);

        updateLabels();
        emit presetChanged(preset.name);
        emit equalizerChanged(preset.gains, preset.balance, m_enableCheckBox->isChecked());
    }
}

void EqualizerDialog::onResetClicked()
{
    // 重置为Flat预设
    m_presetComboBox->setCurrentIndex(0);
    onPresetChanged(0);
}

void EqualizerDialog::onApplyClicked()
{
    emit equalizerChanged(getEqualizerGains(), m_balanceSlider->value(),
                         m_enableCheckBox->isChecked());
    QMessageBox::information(this, tr("提示"), tr("均衡器设置已应用"));
}

QList<int> EqualizerDialog::getEqualizerGains() const
{
    QList<int> gains;
    for (int i = 0; i < 10; i++) {
        gains.append(m_eqSliders[i]->value());
    }
    return gains;
}

int EqualizerDialog::getBalance() const
{
    return m_balanceSlider->value();
}

bool EqualizerDialog::isEqualizerEnabled() const
{
    return m_enableCheckBox->isChecked();
}

void EqualizerDialog::setEqualizerEnabled(bool enabled)
{
    m_enableCheckBox->setChecked(enabled);
}

void EqualizerDialog::setBalance(int balance)
{
    m_balanceSlider->setValue(balance);
}

void EqualizerDialog::setEqualizerGains(const QList<int>& gains)
{
    if (gains.size() == 10) {
        for (int i = 0; i < 10; i++) {
            m_eqSliders[i]->setValue(gains[i]);
        }
        updateLabels();
    }
}

void EqualizerDialog::applyPreset(int presetIndex)
{
    if (presetIndex >= 0 && presetIndex < m_presets.size()) {
        m_presetComboBox->setCurrentIndex(presetIndex);
    }
}
