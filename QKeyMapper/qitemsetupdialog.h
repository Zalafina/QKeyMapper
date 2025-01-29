#ifndef QITEMSETUPDIALOG_H
#define QITEMSETUPDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "qkeyrecord.h"
#include "qcrosshairsetupdialog.h"

namespace Ui {
class QItemSetupDialog;
}

class KeyListComboBox;
class KeyStringLineEdit;

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

    void setUILanguage(int languageindex);
    void resetFontSize(void);
    void setTabIndex(int tabindex);
    void setItemRow(int row);

public:
    static QString getOriginalKeyText(void);
    static int getOriginalKeyCursorPosition(void);
    static void setOriginalKeyText(const QString &new_keytext);
    static QString getMappingKeyText(void);
    static int getMappingKeyCursorPosition(void);
    static void setMappingKeyText(const QString &new_keytext);
    static QString getCurrentOriKeyListText(void);
    static QString getCurrentMapKeyListText(void);
    static void setEditingMappingKeyLineEdit(int editing_lineedit);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void initKeyListComboBoxes(void);
    void initKeyStringLineEdit(void);
    void refreshOriginalKeyRelatedUI(void);
    bool refreshMappingKeyRelatedUI(void);

private:
    static QItemSetupDialog *m_instance;
    Ui::QItemSetupDialog *ui;
    int m_TabIndex;
    int m_ItemRow;
    QKeyRecord *m_KeyRecordDialog;

public:
    QCrosshairSetupDialog *m_CrosshairSetupDialog;
    KeyListComboBox *m_OriginalKeyListComboBox;
    KeyListComboBox *m_MappingKeyListComboBox;
    KeyStringLineEdit *m_MappingKeyLineEdit;
    KeyStringLineEdit *m_MappingKey_KeyUpLineEdit;

public:
    static QStringList s_valiedOriginalKeyList;
    static QStringList s_valiedMappingKeyList;
    static int s_editingMappingKeyLineEdit;

private slots:
    void on_burstpressSpinBox_editingFinished();
    void on_burstreleaseSpinBox_editingFinished();
    void on_burstCheckBox_stateChanged(int state);
    void on_lockCheckBox_stateChanged(int state);
    void on_sendTimingComboBox_currentIndexChanged(int index);
    void on_checkCombKeyOrderCheckBox_stateChanged(int state);
    void on_passThroughCheckBox_stateChanged(int state);
    void on_keySeqHoldDownCheckBox_stateChanged(int state);
    void on_originalKeyUpdateButton_clicked();
    void on_mappingKeyUpdateButton_clicked();
    void on_mappingKey_KeyUpUpdateButton_clicked();
    void on_recordKeysButton_clicked();
    void on_repeatByKeyCheckBox_stateChanged(int state);
    void on_repeatByTimesCheckBox_stateChanged(int state);
    void on_repeatTimesSpinBox_editingFinished();
    void on_itemNoteUpdateButton_clicked();
    void on_mappingKeyUnlockCheckBox_stateChanged(int state);
    void on_postMappingKeyCheckBox_stateChanged(int state);
    void on_unbreakableCheckBox_stateChanged(int state);
    void on_crosshairSetupButton_clicked();
};

class KeyStringLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit KeyStringLineEdit(QWidget *parent = Q_NULLPTR) : QLineEdit(parent)
    {
        if (parent != Q_NULLPTR){
            m_parentWidget_ptr = parent;
        }
    }

protected:
    void focusInEvent(QFocusEvent *event) override;

private:
    QWidget *m_parentWidget_ptr;
};

#endif // QITEMSETUPDIALOG_H
