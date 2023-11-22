/*
 * Copyright (c) 2015-2016 Alex Spataru <alex_spataru@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0

ApplicationWindow {
    //
    // Holds the current joystick selected by the combobox
    //
    property int currentJoystick: 0

    //
    // Window geometry
    //
    width: 640
    height: 480

    //
    // Other window properties
    //
    visible: true
    title: qsTr ("QJoysticks Example")

    //
    // Generates the axes, button and POV indicators when the user selects
    // another joystick from the combobox
    //
    function generateJoystickWidgets (id) {
        /* Clear the joystick indicators */
        axes.model = 0
        povs.model = 0
        buttons.model = 0

        /* Change the current joystick id */
        currentJoystick = id

        /* Get current joystick information & generate indicators */
        if (QJoysticks.joystickExists (id)) {
            axes.model = QJoysticks.getNumAxes (id)
            povs.model = QJoysticks.getNumPOVs (id)
            buttons.model = QJoysticks.getNumButtons (id)
        }

        /* Resize window to minimum size */
        width = minimumWidth
        height = minimumHeight
    }

    //
    // Display all the widgets in a vertical layout
    //
    ColumnLayout {
        spacing: 5
        anchors.margins: 10
        anchors.fill: parent

        //
        // Joystick selector combobox
        //
        ComboBox {
            id: joysticks
            Layout.fillWidth: true
            model: QJoysticks.deviceNames
            onCurrentIndexChanged: generateJoystickWidgets (currentIndex)
            onCurrentTextChanged: generateJoystickWidgets (currentIndex)
        }

        //
        // Axes indicator
        //
        GroupBox {
            title: qsTr ("Axes")
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Layout.fillHeight: true

                //
                // Generate a progressbar for each joystick axis
                //
                Repeater {
                    id: axes
                    delegate: ProgressBar {
                        id: progressbar
                        minimumValue: -100
                        maximumValue: 100
                        Layout.fillWidth: true

                        value: 0
                        Behavior on value {NumberAnimation{}}

                        Connections {
                            target: QJoysticks
                            onAxisChanged: {
                                if (currentJoystick === js && index === axis)
                                    progressbar.value = QJoysticks.getAxis (js, index) * 100
                            }
                        }
                    }
                }
            }
        }

        //
        // Buttons indicator
        //
        GroupBox {
            title: qsTr ("Buttons")
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                rows: 6
                rowSpacing: 5
                columnSpacing: 5
                Layout.fillWidth: true
                Layout.fillHeight: true
                flow: GridLayout.TopToBottom

                //
                // Generate a checkbox for each joystick button
                //
                Repeater {
                    id: buttons
                    delegate: CheckBox {
                        enabled: false
                        Layout.fillWidth: true
                        text: qsTr ("Button %1").arg (index)

                        //
                        // React to QJoystick signals
                        //
                        Connections {
                            target: QJoysticks
                            onButtonChanged: {
                                if (currentJoystick === js && button === index)
                                    checked = QJoysticks.getButton (js, index)
                            }
                        }
                    }
                }
            }
        }

        //
        // POVs indicator
        //
        GroupBox {
            title: qsTr ("POVs")
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Layout.fillHeight: true

                //
                // Generate a spinbox for each joystick POV
                //
                Repeater {
                    id: povs
                    delegate: SpinBox {
                        enabled: false
                        minimumValue: 0
                        maximumValue: 360
                        Layout.fillWidth: true

                        //
                        // React to QJoystick signals
                        //
                        Connections {
                            target: QJoysticks
                            onPovChanged: {
                                if (currentJoystick === js && pov === index)
                                    value = QJoysticks.getPOV (js, index)
                            }
                        }
                    }
                }
            }
        }
    }
}
