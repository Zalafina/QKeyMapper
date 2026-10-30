#ifndef QKM_IBINPUT_POPUP_H
#define QKM_IBINPUT_POPUP_H
#include <QFrame>
#include <QDateTime>
#include <QTimer>
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QkmUiScale;

class QkmIbInputPopup final : public QFrame
{
    Q_OBJECT
public:
    explicit QkmIbInputPopup(QWidget *parent);
    void showAt(QWidget *anchor,qreal scale);
    void retranslate();
    void refresh();
    void setRunning(bool running);
    void connectionChecked();
    void connectionChecked(bool success);
    bool connectionPending() const { return m_checkPending; }
signals:
    void preferencesChanged();
    void connectionCheckRequested();
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
private:
    QComboBox *m_backend;
    QComboBox *m_policy;
    QLabel *m_backendLabel;
    QLabel *m_policyLabel;
    QLabel *m_help;
    QLabel *m_feedbackLabel;
    QPlainTextEdit *m_details;
    QPushButton *m_refresh;
    QPushButton *m_check;
    QTimer m_timer;
    QkmUiScale *m_scale;
    enum class Feedback { None, Refreshed, Checking, Passed, Failed };
    Feedback m_feedback=Feedback::None;
    QDateTime m_feedbackAt;
    bool m_running=false;
    bool m_checkPending=false;
};
#endif
