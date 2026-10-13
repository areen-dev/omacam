cmake_minimum_required(VERSION 3.21)

project(omacam
    VERSION 0.1.0
    DESCRIPTION "Omacam camera app for Omarchy"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia Concurrent Test)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

qt_add_library(camera-core STATIC
    src/camera/CatalogFormat.cpp
    src/config/SettingsStore.cpp
    src/config/OutputPaths.cpp
    src/theme/ThemeParser.cpp
)

target_include_directories(camera-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(camera-core PUBLIC Qt6::Core)

qt_add_executable(dev.areen.omacam
    src/main.cpp
    src/camera/CameraModel.cpp
    src/theme/ThemePalette.cpp
)

qt_add_qml_module(dev.areen.omacam
    URI dev.areen.omacam
    VERSION ${PROJECT_VERSION}
    QML_FILES
        src/qml/Main.qml
        src/qml/PreviewPane.qml
        src/qml/SettingsPanel.qml
        src/qml/AppButton.qml
        src/qml/SettingCombo.qml
)

target_link_libraries(dev.areen.omacam PRIVATE
    camera-core
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Concurrent
)

install(TARGETS dev.areen.omacam RUNTIME DESTINATION bin)
install(FILES packaging/dev.areen.omacam.desktop DESTINATION share/applications)
install(FILES packaging/dev.areen.omacam.svg DESTINATION share/icons/hicolor/scalable/apps)

enable_testing()

function(camera_test name)
    qt_add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE camera-core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

camera_test(tst_catalog)
camera_test(tst_settings)
camera_test(tst_output_paths)
camera_test(tst_theme_parser)
