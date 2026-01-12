/*
 *   Copyright 2021 Reion Wong <reionwong@gmail.com>
 *   Copyright 2018 Camilo Higuita <milo.h@aol.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it  be useful,
 *   but WITHOUT ANY WARRANTY;  even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Window 6.0

import FishUI 1.0 as FishUI
import Cutefish.TermWidget 1.0

Page {
    id: control

    height: _tabView.height
    width: _tabView.width
    focus: true

    background: Rectangle {
        color: "transparent"
    }

    signal urlsDropped(var urls)
    signal keyPressed(var event)
    signal terminalClosed()

    property string path: "$PWD"
    property alias terminal: _terminal
    readonly property QMLTermSession session: _session

    title: _session.title

    onUrlsDropped: {
        for (var i in urls)
            _session.sendText(urls[i].replace("file://", "") + " ")
    }

    onKeyPressed: {
        console.warn("Terminal.qml:onKeyPressed key", event.key, "modifiers", event.modifiers, "accepted", event.accepted)
        if ((event.key === Qt.Key_A)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            _terminal.selectAll()
            event.accepted = true
        }

        if ((event.key === Qt.Key_C)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            _copyAction.trigger()
            event.accepted = true
        }

        if ((event.key === Qt.Key_V)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            _pasteAction.trigger()
            event.accepted = true
        }

        if ((event.key === Qt.Key_Q)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            Qt.quit()
        }

        if ((event.key === Qt.Key_T)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            root.openNewTab()
            event.accepted = true
        }

        if ((event.key === Qt.Key_W)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            root.closeCurrentTab()
            event.accepted = true
        }

        if (event.key === Qt.Key_Tab && event.modifiers & Qt.ControlModifier) {
            root.toggleTab()
            event.accepted = true
        }

        if (event.key === Qt.Key_F11) {
            root.visibility = root.isFullScreen ? Window.Windowed : Window.FullScreen
            event.accepted = true
        }

        // 未被特殊处理的按键，转发到终端会话（确保命令输入传递给子进程）
        if (!event.accepted) {
            _session.sendKey(1, event.key, event.modifiers)
            event.accepted = true
        }
    }

    QMLTermWidget {
        id: _terminal
        anchors.fill: parent
        colorScheme: FishUI.Theme.darkMode ? "Cutefish-Dark" : "Cutefish-Light"
        font.family: settings.fontName
        font.pointSize: settings.fontPointSize
        blinkingCursor: settings.blinkingCursor
        fullCursorHeight: true
        backgroundOpacity: 0

        Keys.enabled: true
        Keys.onPressed: control.keyPressed(event)

        Timer {
            id: reportTimer
            interval: 700
            running: false
            repeat: true
            property int attempts: 0
            property int maxAttempts: 5
            parent: _terminal
            onTriggered: {
                attempts++
                if (_terminal.reportQmlState) _terminal.reportQmlState()
                if (attempts >= maxAttempts) reportTimer.stop()
            }
        }

        session: QMLTermSession {
            id: _session
            onFinished: control.terminalClosed()
            initialWorkingDirectory: control.path
        }

        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            cursorShape: _terminal.terminalUsesMouse ? Qt.ArrowCursor : Qt.IBeamCursor
            acceptedButtons:  Qt.RightButton | Qt.LeftButton

            onDoubleClicked: {
                 var coord = correctDistortion(mouse.x, mouse.y)
                 _terminal.simulateMouseDoubleClick(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
            }

            onPressed: {
                if ((!_terminal.terminalUsesMouse || mouse.modifiers & Qt.ShiftModifier)
                        && mouse.button == Qt.RightButton) {
                    updateMenu()
                    terminalMenu.open()
                } else {
                    var coord = correctDistortion(mouse.x, mouse.y)
                    _terminal.simulateMousePress(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
                }
            }

            onReleased: {
                var coord = correctDistortion(mouse.x, mouse.y)
                _terminal.simulateMouseRelease(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
            }

            onPositionChanged: {
                var coord = correctDistortion(mouse.x, mouse.y)
                _terminal.simulateMouseMove(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
            }

            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    updateMenu()
                    terminalMenu.open()
                } else if(mouse.button === Qt.LeftButton) {
                    _terminal.forceActiveFocus()
                }

                // control.clicked()
            }
        }

        Component.onCompleted: {
            console.warn("Terminal.qml:Component.onCompleted - starting shell. terminal.lines=", _terminal.lines, "columns=", _terminal.columns)
            _session.startShellProgram()
            _terminal.forceActiveFocus()
            reportTimer.start()
        }
    }

    Connections {
        target: _terminal
        onLinesChanged: {
            console.warn("Terminal.qml:_terminal.lines changed", _terminal.lines)
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        onColumnsChanged: {
            console.warn("Terminal.qml:_terminal.columns changed", _terminal.columns)
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        onWindowLinesChanged: {
            console.warn("Terminal.qml:_terminal.windowLines changed", _terminal.windowLines)
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        onSelectedTextChanged: {
            console.warn("Terminal.qml:_terminal.selectedText changed", _terminal.selectedText)
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        onScrollbarCurrentValueChanged: {
            console.warn("Terminal.qml:_terminal.scrollbarCurrentValue changed", _terminal.scrollbarCurrentValue)
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
    }

    Connections {
        target: _session
        onTitleChanged: {
            console.warn("Terminal.qml:_session.title changed", _session.title)
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        onCurrentDirChanged: {
            console.warn("Terminal.qml:_session.currentDir changed", _session.currentDir)
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        onFinished: {
            console.warn("Terminal.qml:_session finished")
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
    }

    Action {
        id: _copyAction
        text: qsTr("Copy")
        onTriggered: _terminal.copyClipboard()
    }

    Action {
        id: _pasteAction
        text: qsTr("Paste")
        onTriggered: _terminal.pasteClipboard()
    }

    FishUI.DesktopMenu {
        id: terminalMenu

        MenuItem {
            id: copyMenuItem
            action: _copyAction
        }

        MenuItem {
            id: pasteMenuItem
            text: qsTr("Paste")
            action: _pasteAction
        }

        MenuItem {
            id: selectAllItem
            text: qsTr("Select All")
            onTriggered: _terminal.selectAll()
        }

        MenuItem {
            text: qsTr("Open File Manager")
            onTriggered: Process.openFileManager(_session.currentDir)
        }

        MenuItem {
            text: root.isFullScreen ? qsTr("Exit full screen") : qsTr("Full screen")
            onTriggered: {
                root.visibility = root.isFullScreen ? Window.Windowed : Window.FullScreen
            }
        }

        MenuItem {
            text: qsTr("Settings")
            onTriggered: {
                settingsDialog.show()
                settingsDialog.raise()
            }
        }
    }

    ScrollBar {
        id: _scrollbar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: FishUI.Units.smallSpacing * 1.5
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Vertical
        size: (_terminal.lines / (_terminal.lines + _terminal.scrollbarMaximum - _terminal.scrollbarMinimum))
        position: _terminal.scrollbarCurrentValue / (_terminal.lines + _terminal.scrollbarMaximum)
    }

    DropArea {
        id: _dropArea
        anchors.fill: parent
        onDropped: {
            if (drop.hasUrls) {
                control.urlsDropped(drop.urls)
            } else if (drop.hasText) {
                _session.sendText(drop.text)
            }
        }
    }

    function forceActiveFocus() {
        _terminal.forceActiveFocus()
    }

    function correctDistortion(x, y) {
        x = x / width
        y = y / height

        var cc = Qt.size(0.5 - x, 0.5 - y)
        var distortion = 0

        return Qt.point((x - cc.width  * (1 + distortion) * distortion) * _terminal.width,
                        (y - cc.height * (1 + distortion) * distortion) * _terminal.height)
    }

    function updateMenu() {
        copyMenuItem.visible = _terminal.selectedText
        pasteMenuItem.visible = Utils.text()
    }
}
