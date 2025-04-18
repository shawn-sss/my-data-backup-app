cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(PROJECT_NAME MyDataBackupApp)
set(PROJECT_VERSION 0.4)
set(PROJECT_LANGUAGES CXX)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES ${PROJECT_LANGUAGES})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg)

if(NOT DEFINED QT_VERSION_MAJOR)
    message(FATAL_ERROR "QT_VERSION_MAJOR is not defined. Ensure Qt5 or Qt6 is found properly.")
endif()

# App sources
set(APP_SOURCES
    app/main.cpp
    app/app_manifest.rc
)

# Project resources
set(PROJECT_RESOURCES
    resources/resource.qrc
    resources/styles/light.qss
    resources/styles/dark.qss
)

# Project settings
set(PROJECT_SETTINGS
    config/_constants.h
    config/app_settings.h
    config/backup_settings.h
    config/cfgmgr_settings.h
    config/labels_settings.h
    config/messages_settings.h
    config/progress_settings.h
    config/utilities_settings.h
    config/resources_settings.h
    config/visual_settings.h
)

# UI sources
set(UI_SOURCES
    ui/mainwindow/mainWindow.cpp
    ui/mainwindow/mainWindow.h
    ui/mainwindow/mainWindow.ui
    ui/settingsdialog/settingsdialog.cpp
    ui/settingsdialog/settingsdialog.h
    ui/settingsdialog/settingsdialog.ui
    ui/toolbarmanager/toolbarmanager.h
    ui/toolbarmanager/toolbarmanager.cpp
    ui/toolbarmanager/toolbarmanagerstyling.h
    core/backup_module/styles/backup_styling.h
)

# Backup Controller sources
set(BACKUP_CONTROLLER_SOURCES
    core/backup_module/controller/backupcontroller.cpp
    core/backup_module/controller/backupcontroller.h
)

# Backup Service sources
set(BACKUP_SERVICE_SOURCES
    core/backup_module/service/backupservice.cpp
    core/backup_module/service/backupservice.h
)

# Backup Worker sources
set(BACKUP_WORKER_SOURCES
    core/backup_module/worker/transferworker.cpp
    core/backup_module/worker/transferworker.h
)

# Backup Models sources
set(BACKUP_MODELS_SOURCES
    core/backup_module/models/stagingmodel.cpp
    core/backup_module/models/stagingmodel.h
    core/backup_module/models/destinationproxymodel.cpp
    core/backup_module/models/destinationproxymodel.h
)

# Utilities sources
set(UTILS_SOURCES
    core/utils/file_utils/filewatcher.cpp
    core/utils/file_utils/filewatcher.h
    core/utils/file_utils/fileoperations.cpp
    core/utils/file_utils/fileoperations.h
    core/utils/common_utils/utils.cpp
    core/utils/common_utils/utils.h
    config/configmanager/configmanager.cpp
    config/configmanager/configmanager.h
    core/utils/file_utils/jsonmanager.cpp
    core/utils/file_utils/jsonmanager.h
    config/thememanager/thememanager.cpp
    config/thememanager/thememanager.h
)

# Combine all sources
set(PROJECT_SOURCES
    ${APP_SOURCES}
    ${PROJECT_RESOURCES}
    ${PROJECT_SETTINGS}
    ${UI_SOURCES}
    ${BACKUP_CONTROLLER_SOURCES}
    ${BACKUP_SERVICE_SOURCES}
    ${BACKUP_OPERATIONS_SOURCES}
    ${BACKUP_WORKER_SOURCES}
    ${BACKUP_MODELS_SOURCES}
    ${UTILS_SOURCES}
)

# Add executable (define the target before applying properties)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})
    else()
        add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
    endif()
endif()

# Include directories (AFTER defining the target)
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)

# macOS-specific properties
if(APPLE)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
    )
endif()

# Install rules
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Finalize for Qt 6
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
