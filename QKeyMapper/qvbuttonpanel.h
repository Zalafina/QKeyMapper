#ifndef QVBUTTONPANEL_H
#define QVBUTTONPANEL_H

#include <QGridLayout>
#include <QScrollArea>
#include <QToolButton>

#include "qkeymapper_worker.h"

// QVButtonPanel: Frameless floating panel showing VButton mapped keys.
//   - WS_EX_NOACTIVATE + Qt::WA_ShowWithoutActivating: no focus stealing.
//   - Drag-to-move (toggleable), configurable layout, opacity, always-on-top.
//   - Corner radius via paintEvent + WA_TranslucentBackground.
//   - Position anchored to screen/window reference point; Window mode tracks dynamically.
//   - Lock mode: first press KEY_DOWN (enter lock), second press KEY_UP (exit lock).
//   - Normal/Burst mode: pressed->KEY_DOWN, released->KEY_UP.
class QVButtonPanel : public QWidget
{
    Q_OBJECT

public:
    explicit QVButtonPanel(QWidget *parent = nullptr);
    ~QVButtonPanel() override;

    // Reapply scroll area transparency rules after theme updates when needed.
    void applyScrollAreaTransparencyStyle();

    void refreshPanel(const QList<MAP_KEYDATA> &dataList);

    // Apply layout/appearance settings (does NOT reposition the panel).
    void applySettings(int columns, int maxRows, int btnWidth, int btnHeight,
                       double opacity, bool alwaysOnTop, int margin, int radius, bool dragEnabled);

    // Apply background, button, and text colors to the panel.
    void applyColors(const QColor &bgColor, const QColor &btnColor, const QColor &txtColor);

    // Compute and move to referencePoint + (offsetX, offsetY).
    void applyPosition(int referencePoint, int offsetX, int offsetY);

    // Called from matchForegroundWindow: repositions if using a Window reference point
    // and the target window has actually moved (debounced via cached RECT).
    void updatePositionIfWindowRef();

    // Current offset relative to the active reference-point origin (updated after drag).
    QPoint panelOffsets() const { return QPoint(m_offsetX, m_offsetY); }

    // Backward-compatible absolute position helpers.
    void   setPanelPosition(const QPoint &pt) { move(pt); }
    QPoint panelPosition()  const { return pos(); }

signals:
    // isKeyDown=true -> KEY_DOWN; false -> KEY_UP
    void triggerVButtonKey_Signal(const QString &keyName, bool isKeyDown);

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onVButtonPressed(int index);
    void onVButtonReleased(int index);

public slots:
    // Slot connected to QKeyMapper_Worker::vbuttonLockStateChanged_Signal.
    // Updates button visual highlight to reflect Worker-side lock state.
    void onVButtonLockStateChanged(const QString &keyName, bool isLocked);

    // Slot connected to QKeyMapper_Worker::vbuttonClearAllLockStates_Signal.
    // Clears all lock highlights when the mapping engine resets.
    void onVButtonClearAllLockStates();

private:
    void    buildGrid();
    QString extractButtonLabel(const QString &vbuttonKey) const;
    QPoint  calculateReferenceOrigin(int referencePoint) const;
    void    recalcOffsets();  // update m_offsetX/Y from current pos() after drag

    QWidget        *m_gridContainer = nullptr;
    QGridLayout    *m_gridLayout    = nullptr;
    QScrollArea    *m_scrollArea    = nullptr;
    QVBoxLayout    *m_mainLayout    = nullptr;

    QList<QToolButton *> m_buttons;
    QStringList          m_keyNames;
    QSet<QString>        m_lockedKeyNames;  // key names currently locked (synced via vbuttonLockStateChanged_Signal)

    // Layout/appearance
    int    m_columns     = 3;
    int    m_maxRows     = 4;
    int    m_btnWidth    = 80;
    int    m_btnHeight   = 30;
    int    m_margin      = 2;
    int    m_radius      = 0;
    bool   m_dragEnabled = true;
    QColor m_bgColor;
    QColor m_btnColor;
    QColor m_txtColor;

    // Position tracking
    int    m_referencePoint  = 0;
    int    m_offsetX         = 50;
    int    m_offsetY         = 50;
    HWND   m_lastTrackHWND   = nullptr;  // debounce: last tracked window handle
    RECT   m_lastTrackRect   = {};       // debounce: last tracked window rect

    // Drag-to-move
    bool   m_dragging   = false;
    QPoint m_dragOffset;
};

#endif // QVBUTTONPANEL_H
