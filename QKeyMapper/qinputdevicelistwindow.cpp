#include "interception_worker.h"
#include "qinputdevicelistwindow.h"
#include "ui_qinputdevicelistwindow.h"

static const char *DEVICELIST_WINDOWTITLE_CHINESE = "设备列表";

static const char *DEVICELIST_WINDOWTITLE_ENGLISH = "DeviceList";

QInputDeviceListWindow::QInputDeviceListWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QInputDeviceListWindow)
{
    ui->setupUi(this);

    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        setWindowTitle(DEVICELIST_WINDOWTITLE_ENGLISH);
    }
    else {
        setWindowTitle(DEVICELIST_WINDOWTITLE_CHINESE);
    }
}

QInputDeviceListWindow::~QInputDeviceListWindow()
{
    delete ui;
}
