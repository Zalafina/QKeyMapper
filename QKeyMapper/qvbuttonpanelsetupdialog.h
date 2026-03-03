#ifndef QVBUTTONPANELSETUPDIALOG_H
#define QVBUTTONPANELSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class QVButtonPanelSetupDialog;
}

struct VButtonPanelSettings {
    int    columns        = 3;
    int    maxRows        = 4;
    int    btnWidth       = 80;
    int    btnHeight      = 30;
    double opacity        = 1.0;
    bool   alwaysOnTop    = true;
    bool   defaultShow    = false;
    int    margin         = 2;
    int    radius         = 0;
    bool   dragEnabled    = true;
    int    referencePoint = 0;   // FLOATINGWINDOW_REFERENCEPOINT_* value (0=ScreenTopLeft)
    int    offsetX        = 50;
    int    offsetY        = 50;
};

class QVButtonPanelSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVButtonPanelSetupDialog(QWidget *parent = nullptr);
    ~QVButtonPanelSetupDialog() override;

    static QVButtonPanelSetupDialog *getInstance() { return m_instance; }

    void setUILanguage(int languageindex);

    // Load current settings into the UI controls before showing
    void loadSettings(const VButtonPanelSettings &settings);

    // Retrieve settings from the UI controls after dialog is accepted
    VButtonPanelSettings getSettings() const;

private:
    static QVButtonPanelSetupDialog *m_instance;
    Ui::QVButtonPanelSetupDialog    *ui;
};

#endif // QVBUTTONPANELSETUPDIALOG_H
