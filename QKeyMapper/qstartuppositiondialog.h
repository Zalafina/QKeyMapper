#ifndef QSTARTUPPOSITIONDIALOG_H
#define QSTARTUPPOSITIONDIALOG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class QStartupPositionDialog;
}

class QStartupPositionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QStartupPositionDialog(QWidget *parent = nullptr);
    ~QStartupPositionDialog();

    static QStartupPositionDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

    int getStartupPosition(void);
    QPoint getSpecifyStartupPosition(void);

    void setStartupPosition(int positon);
    void setSpecifyStartupPosition(const QPoint &position);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    static QStartupPositionDialog *m_instance;
    Ui::QStartupPositionDialog *ui;
};

#endif // QSTARTUPPOSITIONDIALOG_H
