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

    void updateDeviceListInfo(void);

protected:
    void showEvent(QShowEvent *event);

private:
    Ui::QInputDeviceListWindow *ui;
};

#endif // QINPUTDEVICELISTWINDOW_H
