#include "interception_worker.h"
#include "qinputdevicelistwindow.h"
#include "ui_qinputdevicelistwindow.h"

static const char *DEVICELIST_WINDOWTITLE_CHINESE = "输入设备列表";

static const char *DEVICELIST_WINDOWTITLE_ENGLISH = "Input Device List";

QInputDeviceListWindow::QInputDeviceListWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QInputDeviceListWindow)
{
    ui->setupUi(this);
}

QInputDeviceListWindow::~QInputDeviceListWindow()
{
    delete ui;
}

void QInputDeviceListWindow::updateDeviceListInfo()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[updateDeviceListInfo] Called.";
#endif

}

void QInputDeviceListWindow::showEvent(QShowEvent *event)
{
    (void)Interception_Worker::getRefreshedKeyboardDeviceList();
    (void)Interception_Worker::getRefreshedMouseDeviceList();
    updateDeviceListInfo();

    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        setWindowTitle(DEVICELIST_WINDOWTITLE_ENGLISH);
    }
    else {
        setWindowTitle(DEVICELIST_WINDOWTITLE_CHINESE);
    }

    QDialog::showEvent(event);
}
