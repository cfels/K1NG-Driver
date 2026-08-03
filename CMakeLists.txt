cmake_minimum_required(VERSION 3.16)
project(k1ngui VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick Svg Core)
qt_standard_project_setup()

add_subdirectory(src/ui/includes/quickshell/m3shapes EXCLUDE_FROM_ALL)
add_subdirectory(src/ui/includes/quickshell/material-components-qml/src/Core EXCLUDE_FROM_ALL)

add_executable(k1ngui
    src/ui/main.cpp
)

target_include_directories(k1ngui PRIVATE src/ui)

target_link_libraries(k1ngui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Svg
    m3shapes
    md3Core
)

qt_add_qml_module(k1ngui
    URI "com.moxiu.k1ng"
    VERSION 1.0
    SOURCES
        src/ui/driverapi.h
        src/ui/driverapi.cpp
        src/ui/presetmanager.h
        src/ui/presetmanager.cpp
    QML_FILES
        src/ui/qml/main_window.qml
        src/ui/qml/Loading.qml
        src/ui/qml/mainpage.qml
        src/ui/qml/about.qml
        src/ui/qml/components/ElapsedTimer.qml
        src/ui/qml/components/CAnim.qml
        src/ui/qml/components/MapleFontLoader.qml
        src/ui/qml/services/Colours.qml
    RESOURCES
        src/ui/qml/services/qmldir
        src/ui/qml/assets/fonts/MapleMono-NF-Regular.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-Italic.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-Thin.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-ThinItalic.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-ExtraLight.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-ExtraLightItalic.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-Light.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-LightItalic.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-Medium.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-MediumItalic.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-SemiBold.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-SemiBoldItalic.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-Bold.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-BoldItalic.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-ExtraBold.ttf
        src/ui/qml/assets/fonts/MapleMono-NF-ExtraBoldItalic.ttf
        src/ui/qml/assets/logo.png
    RESOURCE_PREFIX "/qt/qml"
    NO_RESOURCE_TARGET_PATH
)

qt_import_qml_plugins(k1ngui)
qt_import_qml_plugins(md3Core)

add_custom_command(TARGET k1ngui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:k1ngui>/driver_bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/ui/driver_bin/k1ng_driver"
        "$<TARGET_FILE_DIR:k1ngui>/driver_bin/k1ng_driver"
)

set_target_properties(k1ngui PROPERTIES
    QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml"
    INSTALL_RPATH "$ORIGIN"
)

install(TARGETS k1ngui
    RUNTIME DESTINATION bin/k1ng-driver
)

install(TARGETS m3shapes
    LIBRARY DESTINATION bin/k1ng-driver
)

install(PROGRAMS src/ui/driver_bin/k1ng_driver
    DESTINATION bin/k1ng-driver/driver_bin
)

install(PROGRAMS src/ui/scripts/run.sh
    DESTINATION bin
    RENAME k1ngui
)

install(FILES src/ui/com.moxiu.k1ng.desktop
    DESTINATION share/applications
)

install(FILES src/ui/qml/assets/logo.png
    DESTINATION bin/k1ng-driver
)

install(FILES src/ui/qml/assets/logo.png
    DESTINATION share/icons/hicolor/512x512/apps
    RENAME com.moxiu.k1ng.png
)
