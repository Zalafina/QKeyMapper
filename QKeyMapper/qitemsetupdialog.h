#ifndef QITEMSETUPDIALOG_H
#define QITEMSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class QItemSetupDialog;
}

class KeyListComboBox;

class QItemSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QItemSetupDialog(QWidget *parent = nullptr);
    ~QItemSetupDialog();

    static QItemSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguagee(int languageindex);
    void resetFontSize(void);
    void setItemRow(int row);

public:
    static QString getOriginalKeyText(void);
    static void setOriginalKeyText(const QString &new_keytext);
    static QString getMappingKeyText(void);
    static void setMappingKeyText(const QString &new_keytext);
    static QString getCurrentOriKeyListText(void);
    static QString getCurrentMapKeyListText(void);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void initKeyListComboBoxes(void);

private:
    static QItemSetupDialog *m_instance;
    Ui::QItemSetupDialog *ui;
    int m_ItemRow;

public:
    KeyListComboBox *m_OriginalKeyListComboBox;
    KeyListComboBox *m_MappingKeyListComboBox;

public:
    static QStringList s_valiedOriginalKeyList;
    static QStringList s_valiedMappingKeyList;

private slots:
    void on_burstpressSpinBox_editingFinished();
    void on_burstreleaseSpinBox_editingFinished();
    void on_burstCheckBox_stateChanged(int state);
    void on_lockCheckBox_stateChanged(int state);
    void on_keyupActionCheckBox_stateChanged(int state);
    void on_passThroughCheckBox_stateChanged(int state);
    void on_keySeqHoldDownCheckBox_stateChanged(int state);
    void on_originalKeyUpdateButton_clicked();
    void on_mappingKeyUpdateButton_clicked();
};

#endif // QITEMSETUPDIALOG_H
