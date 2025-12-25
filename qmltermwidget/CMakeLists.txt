cmake_minimum_required(VERSION 3.16)

project(cutefishqmltermwidget LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------
# Qt6
# -------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    Widgets
)

# -------------------------------------------------
# Sources
# -------------------------------------------------
set(TERMWIDGET_SRCS
    lib/BlockArray.h
    lib/CharacterColor.h
    lib/Character.h
    lib/ColorScheme.h
    lib/ColorTables.h
    lib/DefaultTranslatorText.h
    lib/Emulation.h
    lib/ExtendedDefaultTranslator.h
    lib/Filter.h
    lib/History.h
    lib/HistorySearch.h
    lib/KeyboardTranslator.h
    lib/konsole_wcwidth.h
    lib/kprocess.h
    lib/kptydevice.h
    lib/kpty.h
    lib/kpty_p.h
    lib/kptyprocess.h
    lib/LineFont.h
    lib/Pty.h
    lib/ProcessInfo.h
    lib/Screen.h
    lib/ScreenWindow.h
    lib/Session.h
    lib/ShellCommand.h
    lib/TerminalCharacterDecoder.h
    lib/TerminalDisplay.h
    lib/tools.h
    lib/Vt102Emulation.h
    lib/mac-vkcode.h

    lib/BlockArray.cpp
    lib/ColorScheme.cpp
    lib/Emulation.cpp
    lib/Filter.cpp
    lib/History.cpp
    lib/HistorySearch.cpp
    lib/KeyboardTranslator.cpp
    lib/konsole_wcwidth.cpp
    lib/kprocess.cpp
    lib/kpty.cpp
    lib/kptydevice.cpp
    lib/kptyprocess.cpp
    lib/ProcessInfo.cpp
    lib/Pty.cpp
    lib/Screen.cpp
    lib/ScreenWindow.cpp
    lib/Session.cpp
    lib/ShellCommand.cpp
    lib/TerminalCharacterDecoder.cpp
    lib/TerminalDisplay.cpp
    lib/tools.cpp
    lib/Vt102Emulation.cpp

    src/ksession.h
    src/qmltermwidget_plugin.h
    src/ksession.cpp
    src/qmltermwidget_plugin.cpp
)

# -------------------------------------------------
# QML plugin (MODULE)
# -------------------------------------------------
add_library(cutefishqmltermwidget MODULE
    ${TERMWIDGET_SRCS}
)

target_include_directories(cutefishqmltermwidget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(cutefishqmltermwidget PRIVATE
    HAVE_POSIX_OPENPT
    HAVE_SYS_TIME_H
)

target_link_libraries(cutefishqmltermwidget
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Widgets
)

# -------------------------------------------------
# Qt6 QML install path (正确方式)
# -------------------------------------------------
set(QT_QML_DIR "${CMAKE_INSTALL_LIBDIR}/qt6/qml")

# -------------------------------------------------
# Install
# -------------------------------------------------
install(TARGETS cutefishqmltermwidget
    LIBRARY DESTINATION ${QT_QML_DIR}/Cutefish/TermWidget
)

install(FILES src/qmldir
    DESTINATION ${QT_QML_DIR}/Cutefish/TermWidget
)

install(FILES src/QMLTermScrollbar.qml
    DESTINATION ${QT_QML_DIR}/Cutefish/TermWidget
)

install(DIRECTORY lib/color-schemes
    DESTINATION ${QT_QML_DIR}/Cutefish/TermWidget
)

install(DIRECTORY lib/kb-layouts
    DESTINATION ${QT_QML_DIR}/Cutefish/TermWidget
)
