#ifndef QTABLESETUPDIALOG_H
#define QTABLESETUPDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include "colorpickerwidget.h"

namespace Ui {
class QTableSetupDialog;
}

class QTableSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QTableSetupDialog(QWidget *parent = nullptr);
    ~QTableSetupDialog();

    static QTableSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);
    void resetFontSize(void);
    void setTabIndex(int tabindex);
    void setSettingSelectIndex(int index);
    void initSelectImageFileDialog(void);
    void updateTrayIconPixelComboBoxIcons(const QIcon &icon);

    int getSettingSelectIndex(void);
    bool isSelectImageFileDialogVisible(void);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

public slots:
    void onTabFontColorChanged(QColor &color);
    void onTabBackgroundColorChanged(QColor &color);

private slots:
    void on_tabNameUpdateButton_clicked();

    void on_tabHotkeyUpdateButton_clicked();

    void on_exportTableButton_clicked();

    void on_importTableButton_clicked();

    void on_removeTableButton_clicked();

    void on_selectCustomImageButton_clicked();

    void on_customImageShowPositionComboBox_currentIndexChanged(int index);

    void on_customImagePaddingSpinBox_valueChanged(int value);

    void on_customImageShowAsTrayIconCheckBox_stateChanged(int state);

    void on_customImageTrayIconPixelComboBox_currentIndexChanged(int index);

private:
    static QTableSetupDialog *m_instance;
    Ui::QTableSetupDialog *ui;
    int m_TabIndex;
    int m_SettingSelectIndex;
    ColorPickerWidget *m_NotificationFontColorPicker;
    ColorPickerWidget *m_NotificationBackgroundColorPicker;
    QFileDialog *m_SelectImageFileDialog = Q_NULLPTR;
};

#endif // QTABLESETUPDIALOG_H
