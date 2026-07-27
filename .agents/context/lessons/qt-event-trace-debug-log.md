---
name: qt-event-trace-debug-log
description: When Qt's internal event mechanism fails silently, add DEBUG_LOGOUT_ON logs in event() to trace which events arrive and which don't.
metadata:
  type: reference
---

# Qt Event Trace Debug Logging Pattern

## When To Use

When a Qt widget's built-in behavior (tooltip, hover, focus, etc.) silently fails — the widget receives some events but not others, and no error is surfaced.

## Pattern

Instrument the widget's `event()` override to log specific event types, revealing what Qt does (or doesn't) deliver:

```cpp
bool MyWidget::event(QEvent *event)
{
    const bool handled = BaseClass::event(event);

#if defined(DEBUG_LOGOUT_ON)
    if (event->type() == QEvent::Enter || event->type() == QEvent::ToolTip) {
        qDebug().nospace().noquote()
            << "[MyWidget::event] type=" << event->type()
            << " toolTip=\"" << toolTip() << "\"";
    }
#endif

    return handled;
}
```

Also log in specific event handlers (`enterEvent`, `leaveEvent`) to confirm they fire:

```cpp
void MyWidget::enterEvent(QEnterEvent *event)
{
#if defined(DEBUG_LOGOUT_ON)
    qDebug() << "[MyWidget::enterEvent] toolTip=\"" << toolTip() << "\"";
#endif
    BaseClass::enterEvent(event);
}
```

## QKeyMapper Instance — Tooltip Debugging

Used to diagnose FloatingButtonWidget tooltip not showing. The debug log revealed:
- `QEvent::Enter` fired ✅ (toolTip text correct)
- `QEvent::ToolTip` NEVER fired ❌ (Qt's tooltip timer not starting)

This ruled out:
- Tooltip text issues (text was correctly set)
- Event filter blocking (events reached `event()`)
- enterEvent/leaveEvent override bugs (base class was called)

And pointed toward:
- Qt internal tooltip timer not working for this widget type
- Led to [[floatingbutton-tooltip-manual-qttooltip]] workaround

**Why:** Qt internal mechanisms can fail silently on non-standard widget configurations. Add when: diagnosing any Qt widget behavior where the widget receives some events but a specific event never arrives.
