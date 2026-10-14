# Qt5 uic Enums and Native UI Evidence

## Designer enum compatibility

Qt 5.15.2 uic recognizes the short `Qt::Vertical` spelling for spacers. In the
observed generator, `Qt::Orientation::Vertical` falls through to horizontal
handling. Check generated QSizePolicy in both kits before accepting a spacer
change. The qualified Horizontal spelling currently gets the desired horizontal
result through the fallback; that is not proof of explicit recognition. Prefer
short Qt::Horizontal / Qt::Vertical when editing a spacer, without mass-changing
unaffected horizontal spacers.

## Style-option versions and live palettes

Qt5 native tab painting can use QStyleOptionTabV4. A scale proxy must normalize
the actual option version, not only the older base option. Keep the Qt5/Qt6 type
choice in qkeymapper_qt_compat.h and test the rectangle delivered to tab-label
painting, including native and Fusion regression paths.

Style repolishing can reset an explicit live QWidget palette. Preserve the
current WA_SetPalette value around scale/theme repolishing for owned widgets;
restoring an authored palette would discard a user-selected color. Check both
palette values and rendered swatch pixels after scale changes.

## Synthetic native callbacks are mechanism tests

Directly calling nativeEventFilter proves the internal branch only. It does not
prove Qt routes the same synchronous Windows message to an application-wide
filter. The observed Qt6 route did not deliver WM_SIZING there. A hit test taken
at WM_ENTERSIZEMOVE can also be too late after a rapid pointer move. Compare slow
and fast actual system-input paths, real GetDpiForWindow values and native
rectangles before claiming a cross-DPI resize repair. Record failed approaches
and withdrawn trials so a later investigation does not repeat them blindly.

## Clipping evidence

A control width greater than sizeHint is insufficient: inspect every relevant
ancestor boundary and adjacent controls. A fixed minimum width can overflow a
constrained parent or squeeze neighboring groups. Keep screenshot evidence and
explicit user decisions for accepted language-specific residuals; do not turn
one accepted label defect into a general language waiver.

Private-desktop PrintWindow images may contain pixels outside the work area.
Only measured visible intersections count as visibility proof. Keep Qt logical
geometry, native physical geometry, system DPI and application R=T/B distinct.
