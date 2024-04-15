#ifndef QINPUTDEVICELISTWINDOW_H
#define QINPUTDEVICELISTWINDOW_H

#include <QDialog>

namespace Ui {
class QInputDeviceListWindow;
}

class QInputDeviceListWindow : public QDialog
{
    Q_OBJECT

public:
    explicit QInputDeviceListWindow(QWidget *parent = nullptr);
    ~QInputDeviceListWindow();

    void setUILanguagee(int languageindex);
    void resetFontSize(void);
    void updateDeviceListInfo(void);
    void updateKeyboardDeviceListInfo(void);
    void updateMouseDeviceListInfo(void);
    void selectKeyboardDeviceListRow(int select_rowindex);
    void selectMouseDeviceListRow(int select_rowindex);
    void writeDeviceList(void);
    void writeKeyboardDeviceList(void);
    void writeMouseDeviceList(void);

protected:
    void showEvent(QShowEvent *event);

private:
    void initKeyboardDeviceTable(void);
    void initMouseDeviceTable(void);

private slots:
    void onKeyboardListCellChanged(int row, int col);

    void onMouseListCellChanged(int row, int col);

    void on_confirmButton_clicked();

    void on_cancelButton_clicked();

private:
    Ui::QInputDeviceListWindow *ui;
};

#endif // QINPUTDEVICELISTWINDOW_H
