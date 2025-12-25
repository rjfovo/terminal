cmake_minimum_required(VERSION 3.16)

project(cutefish-terminal LANGUAGES CXX)

# -------------------------------------------------
# CMake / Qt 基础设置
# -------------------------------------------------
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# -------------------------------------------------
# Qt6 modules
# -------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Widgets
    DBus
    LinguistTools
)

# -------------------------------------------------
# Qt6 QML install directory（正确方式）
# -------------------------------------------------
set(QT_QML_DIR "${CMAKE_INSTALL_LIBDIR}/qt6/qml")
message(STATUS "Qt6 QML install dir: ${QT_QML_DIR}")

# -------------------------------------------------
# Sources
# -------------------------------------------------
set(SRCS
    src/main.cpp
    src/processhelper.h
    src/processhelper.cpp
    src/utils.cpp
    src/fonts.cpp
)

set(RESOURCES
    src/resources.qrc
)

# -------------------------------------------------
# Subdirectories (QML modules / plugins)
# -------------------------------------------------
add_subdirectory(qmltermwidget)

# -------------------------------------------------
# Main executable
# -------------------------------------------------
add_executable(${PROJECT_NAME}
    ${SRCS}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::DBus
)

# -------------------------------------------------
# Translations
# -------------------------------------------------
file(GLOB TS_FILES translations/*.ts)

qt_add_translations(${PROJECT_NAME}
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

install(FILES ${QM_FILES}
    DESTINATION /usr/share/cutefish-terminal/translations
)

# -------------------------------------------------
# Install binary & desktop file
# -------------------------------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    cutefish-terminal.desktop
    DESTINATION /usr/share/applications
)
