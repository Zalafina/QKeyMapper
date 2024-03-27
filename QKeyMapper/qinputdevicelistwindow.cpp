#include "interception_worker.h"
#include "qinputdevicelistwindow.h"
#include "ui_qinputdevicelistwindow.h"

static const char *FONTNAME_ENGLISH = "Microsoft YaHei UI";
static const char *FONTNAME_CHINESE = "NSimSun";

static const int UI_SCALE_NORMAL = 0;
static const int UI_SCALE_1K_PERCENT_100 = 1;
static const int UI_SCALE_1K_PERCENT_125 = 2;
static const int UI_SCALE_1K_PERCENT_150 = 3;
static const int UI_SCALE_2K_PERCENT_100 = 4;
static const int UI_SCALE_2K_PERCENT_125 = 5;
static const int UI_SCALE_2K_PERCENT_150 = 6;
static const int UI_SCALE_4K_PERCENT_100 = 7;
static const int UI_SCALE_4K_PERCENT_125 = 8;
static const int UI_SCALE_4K_PERCENT_150 = 9;

static const int DEVICE_TABLE_DEVICEDESC_COLUMN     = 0;
static const int DEVICE_TABLE_HARDWAREID_COLUMN     = 1;
static const int DEVICE_TABLE_VENDORID_COLUMN       = 2;
static const int DEVICE_TABLE_PRODUCTID_COLUMN      = 3;
static const int DEVICE_TABLE_VENDORSTR_COLUMN      = 4;
static const int DEVICE_TABLE_PRODUCTSTR_COLUMN     = 5;
static const int DEVICE_TABLE_MANUFACTURER_COLUMN   = 6;
static const int DEVICE_TABLE_DISABLE_COLUMN        = 7;

static const int KEYBOARD_TABLE_COLUMN_COUNT = 8;
static const int MOUSE_TABLE_COLUMN_COUNT = 8;

static const char *DEVICELIST_WINDOWTITLE_ENGLISH = "Input Device List";

static const char *DEVICELIST_WINDOWTITLE_CHINESE = "输入设备列表";

static const char *DEVICE_TABLE_COL1_ENGLISH = "Device Description";
static const char *DEVICE_TABLE_COL2_ENGLISH = "HardwareID";
static const char *DEVICE_TABLE_COL3_ENGLISH = "VendorID";
static const char *DEVICE_TABLE_COL4_ENGLISH = "ProductID";
static const char *DEVICE_TABLE_COL5_ENGLISH = "Vendor";
static const char *DEVICE_TABLE_COL6_ENGLISH = "Product";
static const char *DEVICE_TABLE_COL7_ENGLISH = "Manufacturer";
static const char *DEVICE_TABLE_COL8_ENGLISH = "Disable";

static const char *DEVICE_TABLE_COL1_CHINESE = "设备描述";
static const char *DEVICE_TABLE_COL2_CHINESE = "硬件ID";
static const char *DEVICE_TABLE_COL3_CHINESE = "VendorID";
static const char *DEVICE_TABLE_COL4_CHINESE = "ProductID";
static const char *DEVICE_TABLE_COL5_CHINESE = "厂商";
static const char *DEVICE_TABLE_COL6_CHINESE = "产品名";
static const char *DEVICE_TABLE_COL7_CHINESE = "制造商";
static const char *DEVICE_TABLE_COL8_CHINESE = "禁用";


QInputDeviceListWindow::QInputDeviceListWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QInputDeviceListWindow)
{
    ui->setupUi(this);

    initKeyboardDeviceTable();
    initMouseDeviceTable();
}

QInputDeviceListWindow::~QInputDeviceListWindow()
{
    delete ui;
}

void QInputDeviceListWindow::setUILanguagee(int languageindex)
{
    if (LANGUAGE_ENGLISH == languageindex) {
        setWindowTitle(DEVICELIST_WINDOWTITLE_ENGLISH);

        ui->keyboardDeviceTable->setHorizontalHeaderLabels(QStringList() << DEVICE_TABLE_COL1_ENGLISH
                                                                         << DEVICE_TABLE_COL2_ENGLISH
                                                                         << DEVICE_TABLE_COL3_ENGLISH
                                                                         << DEVICE_TABLE_COL4_ENGLISH
                                                                         << DEVICE_TABLE_COL5_ENGLISH
                                                                         << DEVICE_TABLE_COL6_ENGLISH
                                                                         << DEVICE_TABLE_COL7_ENGLISH
                                                                         << DEVICE_TABLE_COL8_ENGLISH );

        ui->mouseDeviceTable->setHorizontalHeaderLabels(QStringList()    << DEVICE_TABLE_COL1_ENGLISH
                                                                         << DEVICE_TABLE_COL2_ENGLISH
                                                                         << DEVICE_TABLE_COL3_ENGLISH
                                                                         << DEVICE_TABLE_COL4_ENGLISH
                                                                         << DEVICE_TABLE_COL5_ENGLISH
                                                                         << DEVICE_TABLE_COL6_ENGLISH
                                                                         << DEVICE_TABLE_COL7_ENGLISH
                                                                         << DEVICE_TABLE_COL8_ENGLISH );
    }
    else {
        setWindowTitle(DEVICELIST_WINDOWTITLE_CHINESE);

        ui->keyboardDeviceTable->setHorizontalHeaderLabels(QStringList() << DEVICE_TABLE_COL1_CHINESE
                                                                         << DEVICE_TABLE_COL2_CHINESE
                                                                         << DEVICE_TABLE_COL3_CHINESE
                                                                         << DEVICE_TABLE_COL4_CHINESE
                                                                         << DEVICE_TABLE_COL5_CHINESE
                                                                         << DEVICE_TABLE_COL6_CHINESE
                                                                         << DEVICE_TABLE_COL7_CHINESE
                                                                         << DEVICE_TABLE_COL8_CHINESE );

        ui->mouseDeviceTable->setHorizontalHeaderLabels(QStringList()    << DEVICE_TABLE_COL1_CHINESE
                                                                         << DEVICE_TABLE_COL2_CHINESE
                                                                         << DEVICE_TABLE_COL3_CHINESE
                                                                         << DEVICE_TABLE_COL4_CHINESE
                                                                         << DEVICE_TABLE_COL5_CHINESE
                                                                         << DEVICE_TABLE_COL6_CHINESE
                                                                         << DEVICE_TABLE_COL7_CHINESE
                                                                         << DEVICE_TABLE_COL8_CHINESE );
    }
}

void QInputDeviceListWindow::resetFontSize()
{
    QFont customFont;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        customFont = QFont(FONTNAME_ENGLISH, 9);
    }
    else {
        customFont = QFont(FONTNAME_CHINESE, 9, QFont::Bold);
    }

    int scale = QKeyMapper::getInstance()->m_UI_Scale;
    if (UI_SCALE_4K_PERCENT_150 == scale) {
        customFont.setPointSize(11);
    }
    else {
        customFont.setPointSize(9);
    }

    ui->keyboardDeviceTable->horizontalHeader()->setFont(customFont);
    ui->mouseDeviceTable->horizontalHeader()->setFont(customFont);

    ui->keyboardDeviceTable->setFont(QFont("Microsoft YaHei", 9));
    ui->mouseDeviceTable->setFont(QFont("Microsoft YaHei", 9));
}

void QInputDeviceListWindow::updateDeviceListInfo()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[updateDeviceListInfo] Called.";
#endif

}

void QInputDeviceListWindow::writeDeviceListInfo()
{

}

void QInputDeviceListWindow::showEvent(QShowEvent *event)
{
    (void)Interception_Worker::getRefreshedKeyboardDeviceList();
    (void)Interception_Worker::getRefreshedMouseDeviceList();
    updateDeviceListInfo();

    QDialog::showEvent(event);
}

void QInputDeviceListWindow::initKeyboardDeviceTable()
{
    ui->keyboardDeviceTable->setFocusPolicy(Qt::NoFocus);
    ui->keyboardDeviceTable->setColumnCount(KEYBOARD_TABLE_COLUMN_COUNT);
    ui->keyboardDeviceTable->horizontalHeader()->setStretchLastSection(true);
    ui->keyboardDeviceTable->horizontalHeader()->setHighlightSections(false);
    ui->keyboardDeviceTable->verticalHeader()->setVisible(false);
    ui->keyboardDeviceTable->verticalHeader()->setDefaultSectionSize(25);
    ui->keyboardDeviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->keyboardDeviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->keyboardDeviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    int devicedesc_width = ui->keyboardDeviceTable->width()/6 + 30;
    int vendorid_width = 70;
    int productid_width = 70;
    int vendorstr_width = ui->keyboardDeviceTable->width()/7 - 40;
    int productstr_width = ui->keyboardDeviceTable->width()/5 - 30;
    int manufacturer_width = vendorstr_width;
    int disable_width = 30;
    int hardwareid_width = ui->keyboardDeviceTable->width() - devicedesc_width - vendorid_width - productid_width - vendorstr_width - productstr_width - manufacturer_width - disable_width - 12;

    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_HARDWAREID_COLUMN, hardwareid_width);
    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_DEVICEDESC_COLUMN, devicedesc_width);
    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_VENDORID_COLUMN, vendorid_width);
    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_PRODUCTID_COLUMN, productid_width);
    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_VENDORSTR_COLUMN, vendorstr_width);
    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_PRODUCTSTR_COLUMN, productstr_width);
    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_MANUFACTURER_COLUMN, manufacturer_width);
    ui->keyboardDeviceTable->setColumnWidth(DEVICE_TABLE_DISABLE_COLUMN, disable_width);
}

void QInputDeviceListWindow::initMouseDeviceTable()
{
    ui->mouseDeviceTable->setFocusPolicy(Qt::NoFocus);
    ui->mouseDeviceTable->setColumnCount(MOUSE_TABLE_COLUMN_COUNT);
    ui->mouseDeviceTable->horizontalHeader()->setStretchLastSection(true);
    ui->mouseDeviceTable->horizontalHeader()->setHighlightSections(false);
    ui->mouseDeviceTable->verticalHeader()->setVisible(false);
    ui->mouseDeviceTable->verticalHeader()->setDefaultSectionSize(30);
    ui->mouseDeviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->mouseDeviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->mouseDeviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    int devicedesc_width = ui->keyboardDeviceTable->width()/6 + 30;
    int vendorid_width = 70;
    int productid_width = 70;
    int vendorstr_width = ui->keyboardDeviceTable->width()/7 - 40;
    int productstr_width = ui->keyboardDeviceTable->width()/5 - 30;
    int manufacturer_width = vendorstr_width;
    int disable_width = 30;
    int hardwareid_width = ui->keyboardDeviceTable->width() - devicedesc_width - vendorid_width - productid_width - vendorstr_width - productstr_width - manufacturer_width - disable_width - 12;

    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_HARDWAREID_COLUMN, hardwareid_width);
    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_DEVICEDESC_COLUMN, devicedesc_width);
    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_VENDORID_COLUMN, vendorid_width);
    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_PRODUCTID_COLUMN, productid_width);
    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_VENDORSTR_COLUMN, vendorstr_width);
    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_PRODUCTSTR_COLUMN, productstr_width);
    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_MANUFACTURER_COLUMN, manufacturer_width);
    ui->mouseDeviceTable->setColumnWidth(DEVICE_TABLE_DISABLE_COLUMN, disable_width);
}

void QInputDeviceListWindow::on_confirmButton_clicked()
{
    writeDeviceListInfo();
    hide();
}


void QInputDeviceListWindow::on_cancelButton_clicked()
{
    hide();
}
