/*
 * Copyright (c) 2020-2021 Alex Spataru <https://github.com/alex-spataru>
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

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls

Page {
    id: root

    //
    // Background
    //
    background: Item {
        Image {
            id: img
            opacity: 0.72
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: "qrc:/images/background.jpg"
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop {
                    position: 1
                    color: "#80140042"
                }

                GradientStop {
                    position: 0
                    color: "#80000000"
                }
            }
        }
    }

    //
    // Toolbar
    //
    RowLayout {
        id: toolbar

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: app.spacing
        }

        ColumnLayout {
            spacing: app.spacing
            Layout.alignment: Qt.AlignVCenter

            Image {
                opacity: 0.8
                sourceSize.width: 240
                source: "qrc:/images/kaansat.svg"
                Layout.alignment: Qt.AlignVCenter
            }

            Label {
                opacity: 0.6
                font.pixelSize: 10
                text: Cpp_AppName + " v" + Cpp_AppVersion
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            font.bold: true
            font.pixelSize: 22
            font.family: app.monoFont
            Layout.alignment: Qt.AlignVCenter
            text: Cpp_CanSat_ControlPanel.currentTime
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            Layout.alignment: Qt.AlignVCenter
            text: qsTr("Serial Studio Connection")
        }

        Switch {
            Layout.alignment: Qt.AlignVCenter
            checked: Cpp_SerialStudio_Plugin.isConnected

            MouseArea {
                anchors.fill: parent
            }
        }
    }

    //
    // User interface
    //
    ColumnLayout {
        anchors.fill: parent
        spacing: app.spacing
        anchors.margins: 2 * app.spacing
        anchors.topMargin: 3 * app.spacing + toolbar.height

        //
        // CSV controls
        //
        RowLayout {
            spacing: app.spacing
            Layout.fillWidth: true

            Button {
                icon.width: 24
                icon.height: 24
                Layout.fillWidth: true
                icon.source: "qrc:/icons/cog.svg"
                text: qsTr("Open simulation CSV")
                onClicked: Cpp_CanSat_ControlPanel.openCsv()
            }

            Label {
                Layout.fillWidth: true
                verticalAlignment: Label.AlignVCenter
                horizontalAlignment: Label.AlignHCenter
                text: "<" + Cpp_CanSat_ControlPanel.csvFileName + ">"
            }
        }

        //
        // Console display
        //
        ScrollView {
            id: scrollView
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: parent.width
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                id: textArea
                readOnly: true
                color: "#72d5a3"
                font.pixelSize: 12

                font.family: app.monoFont
                textFormat: Text.PlainText
                width: scrollView.contentWidth
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                text: qsTr(" Welcome to the %1 v%2!\n").arg(Cpp_AppName).arg(Cpp_AppVersion) +
                      qsTr(" Copyright (c) 2022 the Ka'an Sat Team. Released under the MIT License.\n\n")

                Connections {
                    target: Cpp_CanSat_ControlPanel
                    function onPrintLn(line) {
                        textArea.text += " [Control Panel] " + line + "\n"
                    }
                }

                Connections {
                    target: Cpp_SerialStudio_Plugin
                    function onPrintLn(line) {
                        textArea.text += " [Serial Studio] " + line + "\n"
                    }
                }

                background: Rectangle {
                    border.width: 1
                    color: "#aa000000"
                    border.color: "#44bebebe"
                }

                onTextChanged: {
                    if (scrollView.contentHeight > scrollView.height)
                        textArea.cursorPosition = textArea.length - 1
                }
            }
        }

        //
        // Buttons
        //
        GridLayout {
            id: grid
            columns: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            rowSpacing: app.spacing
            columnSpacing: app.spacing

            Button {
                id: simModeEnabled
                text: qsTr("Simulation mode")

                enabled: Cpp_SerialStudio_Plugin.isConnected
                checked: Cpp_CanSat_ControlPanel.simulationEnabled
                onClicked: Cpp_CanSat_ControlPanel.simulationEnabled = !Cpp_CanSat_ControlPanel.simulationEnabled

                icon.width: 42
                icon.height: 42
                font.pixelSize: 16
                icon.source: "qrc:/icons/simulation.svg"

                Layout.fillWidth: true
                Layout.minimumHeight: 64
                Layout.maximumHeight: 64
            }

            Button {
                id: activateSimMode
                text: checked ? qsTr("Disable simulation mode") : qsTr("Activate simulation mode")

                checked: Cpp_CanSat_ControlPanel.simulationActivated
                onClicked: Cpp_CanSat_ControlPanel.simulationActivated = !Cpp_CanSat_ControlPanel.simulationActivated
                enabled: simModeEnabled.checked && simModeEnabled.enabled && Cpp_CanSat_ControlPanel.simulationCsvLoaded

                icon.width: 42
                icon.height: 42
                font.pixelSize: 16
                icon.source: "qrc:/icons/start.svg"

                Layout.fillWidth: true
                Layout.minimumHeight: 64
                Layout.maximumHeight: 64
            }

            Button {
                id: telemetryEnabled

                text: qsTr("Container telemetry")

                enabled: Cpp_SerialStudio_Plugin.isConnected
                checked: Cpp_CanSat_ControlPanel.containerTelemetryEnabled
                onClicked: Cpp_CanSat_ControlPanel.containerTelemetryEnabled = !Cpp_CanSat_ControlPanel.containerTelemetryEnabled

                icon.width: 42
                icon.height: 42
                font.pixelSize: 16
                icon.source: checked ? "qrc:/icons/telemetry-on.svg" :
                                       "qrc:/icons/telemetry-off.svg"

                Layout.fillWidth: true
                Layout.minimumHeight: 64
                Layout.maximumHeight: 64
            }

            Button {
                id: updateTime
                text: qsTr("Update container time")

                enabled: Cpp_SerialStudio_Plugin.isConnected
                onClicked: Cpp_CanSat_ControlPanel.updateContainerTime()

                icon.width: 42
                icon.height: 42
                font.pixelSize: 16
                icon.source: "qrc:/icons/time.svg"

                Layout.fillWidth: true
                Layout.minimumHeight: 64
                Layout.maximumHeight: 64
            }
        }

    }
}
