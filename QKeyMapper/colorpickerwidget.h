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
    explicit ColorPickerWidget(QWidget *parent = nullptr);
    ~ColorPickerWidget();

private slots:
    void onPickColor();  // Slot to handle color pick button click

private:
    QPushButton *colorButton;  // Button to trigger color picker dialog
    QLabel *colorLabel;        // Label to display the selected color
};

#endif // COLORPICKERWIDGET_H
