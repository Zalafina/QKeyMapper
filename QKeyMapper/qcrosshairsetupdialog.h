#ifndef QCROSSHAIRSETUPDIALOG_H
#define QCROSSHAIRSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class QCrosshairSetupDialog;
}

class QCrosshairSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QCrosshairSetupDialog(QWidget *parent = nullptr);
    ~QCrosshairSetupDialog();

private:
    Ui::QCrosshairSetupDialog *ui;
};

#endif // QCROSSHAIRSETUPDIALOG_H
