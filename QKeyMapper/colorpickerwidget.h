#ifndef COLORPICKERWIDGET_H
#define COLORPICKERWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QColorDialog>
#include <QLabel>
#include <QHBoxLayout>
#include <QApplication>
#include <QMenu>
#include <QEvent>

class ColorPickerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPickerWidget(QWidget *parent = nullptr, QString buttonText = QString(), int buttonWidth = 51);
    ~ColorPickerWidget();

    QColor getColor(void);

    // void setUILanguage(int languageindex);
    void setColor(const QColor &color);
    void setLivePreviewEnabled(bool enabled);
    void setShowAlphaChannel(bool show);
    void setWindowTitle(QString title);
    void setButtonText(QString text);

signals:
    void colorChanged(QColor &color);
    void previewColorChanged(const QColor &color);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onPickColor();  // Slot to handle color pick button click
    void onColorButtonContextMenu(const QPoint &pos);  // Right-click context menu for default color restore

private:
    void updateColorLabel(const QColor &color);

    QColor m_color = QColor();              // Variable to store the selected color
    QPushButton *colorButton = Q_NULLPTR;   // Button to trigger color picker dialog
    QLabel *colorLabel = Q_NULLPTR;         // Label to display the selected color
    QString m_buttonText = "Color";         // Text for the button
    QString m_windowTitle = QString();
    bool m_showAlphaChannel = false;
    bool m_livePreviewEnabled = false;
};

#endif // COLORPICKERWIDGET_H
