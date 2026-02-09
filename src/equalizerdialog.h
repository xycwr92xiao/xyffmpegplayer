// File: equalizerdialog.h
#ifndef EQUALIZERDIALOG_H
#define EQUALIZERDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>

class EqualizerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EqualizerDialog(QWidget *parent = nullptr);
    ~EqualizerDialog();

    // 获取均衡器增益值
    QList<int> getEqualizerGains() const;

    // 获取平衡值 (-100左全, 0中, 100右全)
    int getBalance() const;

    // 是否启用均衡器
    bool isEqualizerEnabled() const;

    // 应用预设
    void applyPreset(int presetIndex);

signals:
    void equalizerChanged(const QList<int>& gains, int balance, bool enabled);
    void presetChanged(const QString& presetName);

public slots:
    void setEqualizerEnabled(bool enabled);
    void setBalance(int balance);
    void setEqualizerGains(const QList<int>& gains);

private slots:
    void onPresetChanged(int index);
    void onResetClicked();
    void onApplyClicked();
    void onSliderValueChanged();

private:
    void initUI();
    void initPresets();
    void updateLabels();

    // 预设配置
    struct EqualizerPreset {
        QString name;
        QList<int> gains; // 10个频段的增益值
        int balance;      // 平衡值
    };

    QList<EqualizerPreset> m_presets;

    // UI 组件
    QCheckBox *m_enableCheckBox;
    QComboBox *m_presetComboBox;

    // 10段均衡器滑块
    QSlider *m_eqSliders[10];
    QLabel *m_eqLabels[10];
    QLabel *m_freqLabels[10];

    // 左右声道平衡滑块
    QSlider *m_balanceSlider;
    QLabel *m_balanceLabel;
    QLabel *m_leftLabel;
    QLabel *m_rightLabel;

    // 按钮
    QPushButton *m_resetButton;
    QPushButton *m_applyButton;
    QPushButton *m_closeButton;

    // 频段中心频率
    const int m_frequencies[10] = {60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000};
};

#endif // EQUALIZERDIALOG_H
