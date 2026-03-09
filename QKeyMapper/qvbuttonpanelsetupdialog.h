#ifndef QVBUTTONPANELSETUPDIALOG_H
#define QVBUTTONPANELSETUPDIALOG_H

#include <QDialog>
#include <QColor>
#include "qkeymapper_constants.h"
#include "colorpickerwidget.h"

namespace Ui {
class QVButtonPanelSetupDialog;
}

struct VButtonPanelSettings {
    int    columns        = QKeyMapperConstants::VBTNPANEL_DEFAULT_COLUMNS;
    int    maxRows        = QKeyMapperConstants::VBTNPANEL_DEFAULT_MAXROWS;
    int    btnWidth       = QKeyMapperConstants::VBTNPANEL_DEFAULT_BTNWIDTH;
    int    btnHeight      = QKeyMapperConstants::VBTNPANEL_DEFAULT_BTNHEIGHT;
    double opacity        = QKeyMapperConstants::VBTNPANEL_DEFAULT_OPACITY;
    bool   alwaysOnTop    = QKeyMapperConstants::VBTNPANEL_DEFAULT_ALWAYSONTOP;
    bool   defaultShow    = QKeyMapperConstants::VBTNPANEL_DEFAULT_DEFAULTSHOW;
    int    margin         = QKeyMapperConstants::VBTNPANEL_DEFAULT_MARGIN;
    int    radius         = QKeyMapperConstants::VBTNPANEL_DEFAULT_RADIUS;
    bool   dragEnabled    = QKeyMapperConstants::VBTNPANEL_DEFAULT_DRAGENABLED;
    int    referencePoint = QKeyMapperConstants::VBTNPANEL_DEFAULT_REFERENCEPOINT;
    int    offsetX        = QKeyMapperConstants::VBTNPANEL_DEFAULT_OFFSETX;
    int    offsetY        = QKeyMapperConstants::VBTNPANEL_DEFAULT_OFFSETY;
    QColor bgColor        = QKeyMapperConstants::VBTNPANEL_BACKGROUND_COLOR_DEFAULT;
    QColor btnColor       = QKeyMapperConstants::VBTNPANEL_BUTTON_COLOR_DEFAULT;
    QColor textColor      = QKeyMapperConstants::VBTNPANEL_TEXT_COLOR_DEFAULT;
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

signals:
    // Emitted when the Apply button is clicked; does not close the dialog
    void settingsApplied();

protected:
    bool event(QEvent *event) override;

private slots:
    void on_okButton_clicked();

private:
    static QVButtonPanelSetupDialog *m_instance;
    Ui::QVButtonPanelSetupDialog    *ui;
    ColorPickerWidget               *m_BGColorPicker;
    ColorPickerWidget               *m_BtnColorPicker;
    ColorPickerWidget               *m_TextColorPicker;
};

#endif // QVBUTTONPANELSETUPDIALOG_H
