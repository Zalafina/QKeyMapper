#ifndef QPOINTPICKERDIALOG_H
#define QPOINTPICKERDIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>

class PointPickerDragTool : public QFrame
{
    Q_OBJECT
public:
    explicit PointPickerDragTool(QWidget *parent = nullptr);
    ~PointPickerDragTool() override;
    void cancelDrag();
    void finishDrag(bool commit = true);
    bool isDragging() const { return m_isDragging; }
    void setTheme(bool isDark);

signals:
    void dragStarted();
    void dragMoved(const QPoint &screenPt);
    void dragFinished(bool commit, const QPoint &screenPt);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void hideEvent(QHideEvent *event) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private:
    bool m_isDragging = false;
    bool m_isDark = false;
    HCURSOR m_hNativeCursor = NULL;
    qreal m_currentDpr = 1.0;
};

class QPointPickerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QPointPickerDialog(QWidget *parent = nullptr);
    ~QPointPickerDialog() override;

    void setVisible(bool visible) override;
    void retranslateUi();
    void syncPickedPoint(const QPoint &point);
    void applyTheme();
    void applyTheme(bool isDark);

signals:
    void pointPicked(const QPoint &point, bool isWindowMode);
    void visibilityChanged(bool visible);

protected:
    bool event(QEvent *e) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private slots:
    void onUpdateCurrentCoord();
    void onModeChanged();
    void onDragStarted();
    void onDragMoved(const QPoint &screenPt);
    void onDragFinished(bool commit, const QPoint &screenPt);

private:
    void setupUi();
    void updateTargetWindowInfo();
    bool isPositionValidOnScreens(const QPoint &pos, const QSize &size) const;

    QRadioButton *m_screenRadio = nullptr;
    QRadioButton *m_windowRadio = nullptr;
    QLabel *m_targetInfoLabel = nullptr;
    QLabel *m_currentCoordLabel = nullptr;
    QLabel *m_currentCoordValueLabel = nullptr;
    PointPickerDragTool *m_dragTool = nullptr;
    QLabel *m_pickedCoordLabel = nullptr;
    QLineEdit *m_pickedCoordEdit = nullptr;
    QTimer *m_coordTimer = nullptr;

    QPoint m_lastCoord = QPoint(-99999, -99999);
    bool m_lastModeWasWindow = false;
    HWND m_lastTargetHWND = NULL;

    bool m_hasUserMoved = false;
    QPoint m_lastUserPos;
    bool m_initialShowCompleted = false;
    bool m_isRestoringPos = false;
    bool m_isUserMoving = false;
    QPoint m_posBeforeMove;
};

#endif // QPOINTPICKERDIALOG_H
