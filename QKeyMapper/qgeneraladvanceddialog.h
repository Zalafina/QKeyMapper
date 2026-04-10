#ifndef QGENERALADVANCEDDIALOG_H
#define QGENERALADVANCEDDIALOG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class QGeneralAdvancedDialog;
}

class QGeneralAdvancedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QGeneralAdvancedDialog(QWidget *parent = nullptr);
    ~QGeneralAdvancedDialog();

    static QGeneralAdvancedDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

    int getStartupPosition(void);
    QPoint getSpecifyStartupPosition(void);
    int getTableEditModeTrigger(void);
    int getTableInsertMode(void);
    bool getPlaySoundEffect(void);
    unsigned int getGlobalSettingSwitchTimeout(void);
    bool getDisableGlobalMappingInFullscreen(void);
    QString getGlobalMappingFullscreenAllowedProcesses(void);

    void setStartupPosition(int position);
    void setSpecifyStartupPosition(const QPoint &position);
    void setTableEditModeTrigger(int trigger);
    void setTableInsertMode(int mode);
    void setPlaySoundEffect(bool enabled);
    void setGlobalSettingSwitchTimeout(unsigned int timeout);
    void setDisableGlobalMappingInFullscreen(bool enabled);
    void setGlobalMappingFullscreenAllowedProcesses(const QString &processes);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateStartupSpecifyPositionState(void);
    void updateGlobalMappingFullscreenState(void);

    static QGeneralAdvancedDialog *m_instance;
    Ui::QGeneralAdvancedDialog *ui;
};

#endif // QGENERALADVANCEDDIALOG_H
