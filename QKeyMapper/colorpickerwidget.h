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

    QColor getColor(void);

    // void setUILanguage(int languageindex);
    void setColor(const QColor &color);
    void setShowAlphaChannel(bool show);
    void setWindowTitle(QString title);
    void setButtonText(QString text);

signals:
    void colorChanged(QColor &color);

private slots:
    void onPickColor();  // Slot to handle color pick button click

private:
    QColor m_color = QColor();              // Variable to store the selected color
    QPushButton *colorButton = Q_NULLPTR;   // Button to trigger color picker dialog
    QLabel *colorLabel = Q_NULLPTR;         // Label to display the selected color
    QString m_buttonText = "Color";         // Text for the button
    QString m_windowTitle = QString();
    bool m_showAlphaChannel = false;
};

#endif // COLORPICKERWIDGET_H
