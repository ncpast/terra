cmake_minimum_required(VERSION 3.16)

project(terra VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.8)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appterra
    src/main.cpp
)

set_source_files_properties(
    qml/Theme.qml
    qml/GlobalValues.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

set(QML_IMPORT_PATH "$ENV{HOME}/.local/maplibre/qml" CACHE STRING "" FORCE)

qt_add_qml_module(appterra
    URI terra
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/GlobalValues.qml
        qml/navigation/StatusBar.qml
        qml/navigation/TopNavBar.qml
        qml/templates/TopNavButton.qml
        qml/pages/MapPage.qml
        styles/satellite.json
        qml/pages/Map3D.qml
        qml/pages/RoutesPage.qml
        qml/pages/WeatherPage.qml
        qml/pages/NotesPage.qml
        qml/pages/SystemPage.qml
    SOURCES
        src/systemstatus.h
        src/systemstatus.cpp
        QML_FILES qml/templates/MapToolButton.qml
)

target_include_directories(appterra PRIVATE src)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appterra PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appterra
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appterra
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Network
)

install(TARGETS appterra
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
