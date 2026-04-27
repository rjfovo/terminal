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

    onKeyPressed: function(event) {
        // 日志：记录所有按键事件
        console.log("Terminal.qml: onKeyPressed key=" + event.key + " text='" + event.text + "' modifiers=" + event.modifiers + " keyName=" + Qt.platform.os)

        // 处理所有按键事件
        var handled = false

        // 快捷键处理（优先于普通按键）
        if ((event.key === Qt.Key_A)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            console.log("Terminal.qml: Ctrl+Shift+A -> selectAll")
            _terminal.selectAll()
            handled = true
        }

        if ((event.key === Qt.Key_C)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            console.log("Terminal.qml: Ctrl+Shift+C -> copy")
            _copyAction.trigger()
            handled = true
        }

        if ((event.key === Qt.Key_V)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            console.log("Terminal.qml: Ctrl+Shift+V -> paste")
            _pasteAction.trigger()
            handled = true
        }

        if ((event.key === Qt.Key_Q)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            console.log("Terminal.qml: Ctrl+Shift+Q -> quit")
            Qt.quit()
            handled = true
        }

        if ((event.key === Qt.Key_T)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            console.log("Terminal.qml: Ctrl+Shift+T -> newTab")
            root.openNewTab()
            handled = true
        }

        if ((event.key === Qt.Key_W)
                && (event.modifiers & Qt.ControlModifier)
                && (event.modifiers & Qt.ShiftModifier)) {
            console.log("Terminal.qml: Ctrl+Shift+W -> closeTab")
            root.closeCurrentTab()
            handled = true
        }

        if (event.key === Qt.Key_Tab && event.modifiers & Qt.ControlModifier) {
            console.log("Terminal.qml: Ctrl+Tab -> toggleTab")
            root.toggleTab()
            handled = true
        }

        if (event.key === Qt.Key_F11) {
            console.log("Terminal.qml: F11 -> toggleFullScreen")
            root.visibility = root.isFullScreen ? Window.Windowed : Window.FullScreen
            handled = true
        }

        // 如果事件已被快捷键处理，阻止继续传播
        if (handled) {
            event.accepted = true
            return
        }

        // 普通按键处理 - 发送到终端
        // 注意：所有按键事件都通过 sendText/sendKey 发送到终端
        // 因为 QML 的 Keys.onPressed 会拦截事件，不会传递到 C++ 层

        // 回车键：发送 \n（换行符），而不是 \r（回车符）
        // PTY 的 ONLCR 标志会将 \n 转换为 \r\n
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            console.log("Terminal.qml: Enter -> sendText('\\n')")
            _session.sendText("\n")
            event.accepted = true
            return
        }

        // 退格键
        if (event.key === Qt.Key_Backspace) {
            console.log("Terminal.qml: Backspace -> sendText('\\b')")
            _session.sendText("\b")
            event.accepted = true
            return
        }

        // Tab 键
        if (event.key === Qt.Key_Tab) {
            console.log("Terminal.qml: Tab -> sendText('\\t')")
            _session.sendText("\t")
            event.accepted = true
            return
        }

        // Escape 键
        if (event.key === Qt.Key_Escape) {
            console.log("Terminal.qml: Escape -> sendText('\\x1b')")
            _session.sendText("\x1b")
            event.accepted = true
            return
        }

        // 方向键
        if (event.key === Qt.Key_Up) {
            console.log("Terminal.qml: Up -> sendText('\\x1b[A')")
            _session.sendText("\x1b[A")
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Down) {
            console.log("Terminal.qml: Down -> sendText('\\x1b[B')")
            _session.sendText("\x1b[B")
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right) {
            console.log("Terminal.qml: Right -> sendText('\\x1b[C')")
            _session.sendText("\x1b[C")
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Left) {
            console.log("Terminal.qml: Left -> sendText('\\x1b[D')")
            _session.sendText("\x1b[D")
            event.accepted = true
            return
        }

        // Home 键
        if (event.key === Qt.Key_Home) {
            console.log("Terminal.qml: Home -> sendText('\\x1b[H')")
            _session.sendText("\x1b[H")
            event.accepted = true
            return
        }

        // End 键
        if (event.key === Qt.Key_End) {
            console.log("Terminal.qml: End -> sendText('\\x1b[F')")
            _session.sendText("\x1b[F")
            event.accepted = true
            return
        }

        // Delete 键
        if (event.key === Qt.Key_Delete) {
            console.log("Terminal.qml: Delete -> sendText('\\x1b[3~')")
            _session.sendText("\x1b[3~")
            event.accepted = true
            return
        }

        // PageUp / PageDown
        if (event.key === Qt.Key_PageUp) {
            console.log("Terminal.qml: PageUp -> sendText('\\x1b[5~')")
            _session.sendText("\x1b[5~")
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_PageDown) {
            console.log("Terminal.qml: PageDown -> sendText('\\x1b[6~')")
            _session.sendText("\x1b[6~")
            event.accepted = true
            return
        }

        // Insert 键
        if (event.key === Qt.Key_Insert) {
            console.log("Terminal.qml: Insert -> sendText('\\x1b[2~')")
            _session.sendText("\x1b[2~")
            event.accepted = true
            return
        }

        // F1-F12 功能键
        if (event.key >= Qt.Key_F1 && event.key <= Qt.Key_F12) {
            var fKeys = ["\x1bOP", "\x1bOQ", "\x1bOR", "\x1bOS",
                         "\x1b[15~", "\x1b[17~", "\x1b[18~", "\x1b[19~",
                         "\x1b[20~", "\x1b[21~", "\x1b[23~", "\x1b[24~"]
            var index = event.key - Qt.Key_F1
            if (index >= 0 && index < fKeys.length) {
                console.log("Terminal.qml: F" + (index+1) + " -> sendText('" + fKeys[index] + "')")
                _session.sendText(fKeys[index])
            }
            event.accepted = true
            return
        }

        // Ctrl+字母键（发送控制字符）
        if (event.modifiers & Qt.ControlModifier && !(event.modifiers & Qt.AltModifier) && !(event.modifiers & Qt.ShiftModifier)) {
            var ctrlKey = event.key
            // 处理 Ctrl+A 到 Ctrl+Z
            if (ctrlKey >= Qt.Key_A && ctrlKey <= Qt.Key_Z) {
                var ctrlChar = String.fromCharCode(ctrlKey - Qt.Key_A + 1)
                console.log("Terminal.qml: Ctrl+" + String.fromCharCode(ctrlKey) + " -> sendText(ctrlChar=" + (ctrlKey - Qt.Key_A + 1) + ")")
                _session.sendText(ctrlChar)
                event.accepted = true
                return
            }
        }

        // 普通字符键：如果有文本，发送文本
        if (event.text && event.text.length > 0) {
            console.log("Terminal.qml: char text='" + event.text + "' -> sendText")
            _session.sendText(event.text)
            event.accepted = true
            return
        }

        // 其他未处理的按键，通过 sendKey 发送
        console.log("Terminal.qml: unhandled key=" + event.key + " -> sendKey")
        _session.sendKey(1, event.key, event.modifiers)
        event.accepted = true
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
        Keys.onPressed: function(event) {
            control.keyPressed(event)
        }

        Timer {
            id: reportTimer
            interval: 700
            running: false
            repeat: true
            property int attempts: 0
            property int maxAttempts: 5
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

            onDoubleClicked: function(mouse) {
                 var coord = correctDistortion(mouse.x, mouse.y)
                 _terminal.simulateMouseDoubleClick(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
            }

            onPressed: function(mouse) {
                if ((!_terminal.terminalUsesMouse || mouse.modifiers & Qt.ShiftModifier)
                        && mouse.button == Qt.RightButton) {
                    updateMenu()
                    terminalMenu.open()
                } else {
                    var coord = correctDistortion(mouse.x, mouse.y)
                    _terminal.simulateMousePress(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
                }
            }

            onReleased: function(mouse) {
                var coord = correctDistortion(mouse.x, mouse.y)
                _terminal.simulateMouseRelease(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
            }

            onPositionChanged: function(mouse) {
                var coord = correctDistortion(mouse.x, mouse.y)
                _terminal.simulateMouseMove(coord.x, coord.y, mouse.button, mouse.buttons, mouse.modifiers)
            }

            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    updateMenu()
                    terminalMenu.open()
                } else if(mouse.button === Qt.LeftButton) {
                    _terminal.forceActiveFocus()
                }
            }
        }
    }

    Timer {
        id: _startShellTimer
        interval: 100
        running: false
        repeat: true
        property int attempts: 0
        property int maxAttempts: 50
        onTriggered: {
            attempts++
            if (_terminal && _terminal.lines && _terminal.columns && _terminal.lines > 1 && _terminal.columns > 1) {
                console.log("Terminal.qml: Terminal ready, starting shell")
                _session.startShellProgram()
                _terminal.forceActiveFocus()
                reportTimer.start()
                stop()
            } else if (attempts >= maxAttempts) {
                console.log("Terminal.qml: Terminal not ready after attempts, starting shell fallback")
                _session.startShellProgram()
                _terminal.forceActiveFocus()
                reportTimer.start()
                stop()
            }
        }
    }

    Component.onCompleted: {
        console.log("Terminal.qml: Component.onCompleted, path=", control.path)
        console.log("Terminal.qml: Scheduling shell start when terminal is ready")
        _startShellTimer.start()
    }

    Connections {
        target: _terminal
        function onLinesChanged() {
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        function onColumnsChanged() {
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        function onScrollbarCurrentValueChanged() {
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
    }

    Connections {
        target: _session
        function onTitleChanged() {
            if (_terminal.reportQmlState) _terminal.reportQmlState()
        }
        function onFinished() {
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
