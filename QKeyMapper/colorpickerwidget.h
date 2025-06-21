#ifndef COLORPICKERWIDGET_H
#define COLORPICKERWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QColorDialog>
#include <QLabel>
#include <QHBoxLayout>

class ColorPickerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPickerWidget(QWidget *parent = nullptr, QString buttonText = QString(), int buttonWidth = 51);
    ~ColorPickerWidget();

    void setUILanguage(int languageindex);
    void setColor(QColor &color);

signals:
    void colorChanged(QColor &color);

private slots:
    void onPickColor();  // Slot to handle color pick button click

public:
    static bool s_isColorSelecting;

private:
    QColor m_color;             // Variable to store the selected color
    QPushButton *colorButton;   // Button to trigger color picker dialog
    QLabel *colorLabel;         // Label to display the selected color
    QString m_buttonText;       // Text for the button
};

#endif // COLORPICKERWIDGET_H
