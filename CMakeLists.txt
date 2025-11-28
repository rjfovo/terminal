cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cutefish-terminal)
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Quick QuickControls2 Widgets DBus LinguistTools
)

# Qt6 QML install directory
get_target_property(QT_QML_DIR Qt6::Core QT_INSTALL_QML)
message(STATUS "Qt6 QML dir: ${QT_QML_DIR}")

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

# Subdirectory
add_subdirectory(qmltermwidget)

# Main executable -------------------------------------
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

# --------------------- Translations ---------------------
file(GLOB TS_FILES translations/*.ts)

qt_add_translations(${PROJECT_NAME}
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

install(FILES ${QM_FILES}
    DESTINATION /usr/share/cutefish-terminal/translations
)

# --------------------- Install binary ---------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION /usr/bin
)

install(FILES
    cutefish-terminal.desktop
    DESTINATION /usr/share/applications/
    COMPONENT Runtime
)
