cmake_minimum_required(VERSION 3.16)
project(vent VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ищем Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network WebSockets Multimedia Widgets)

# --- Управление зависимостями ---
include(FetchContent)
FetchContent_Declare(
    qtkeychain
    GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
    GIT_TAG        0.14.2
)
# Настройка qtkeychain для статической сборки и без лишних компонентов
set(BUILD_WITH_QT6 ON CACHE BOOL "Сборка с Qt 6" FORCE)
set(BUILD_TRANSLATIONS OFF CACHE BOOL "Сборка переводов" FORCE)
set(BUILD_TEST_APPLICATION OFF CACHE BOOL "Сборка тестового приложения" FORCE)
# Собираем как статическую библиотеку, чтобы избежать проблем с DLL на Windows
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Сборка qtkeychain как статической библиотеки" FORCE) 
FetchContent_MakeAvailable(qtkeychain)
# -----------------------------

set(SOURCES
    src/controllers/authcontroller.cpp
    src/services/authservice.cpp
    src/controllers/chatscontroller.cpp
    src/delegates/chatlistitemdelegate.cpp
    src/models/chatlistmodel.cpp
    src/delegates/chatmessagesitemdelegate.cpp
    src/models/chatmessageslistmodel.cpp
    src/services/chatservice.cpp
    src/utils/errortypes.cpp
    src/controllers/filescontroller.cpp
    src/services/fileservice.cpp
    src/widgets/listviewdragndrop.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/delegates/searchitemdelegate.cpp
    src/models/searchlistmodel.cpp
    src/controllers/userinfocontroller.cpp
    src/services/userinfoservice.cpp
    src/controllers/websocketcontroller.cpp
    src/services/websocketservice.cpp
)

set(HEADERS
    src/controllers/authcontroller.h
    src/services/authservice.h
    src/controllers/chatscontroller.h
    src/delegates/chatlistitemdelegate.h
    src/models/chatlistmodel.h
    src/delegates/chatmessagesitemdelegate.h
    src/models/chatmessageslistmodel.h
    src/services/chatservice.h
    src/utils/endpoints.h
    src/utils/errortypes.h
    src/controllers/filescontroller.h
    src/services/fileservice.h
    src/widgets/listviewdragndrop.h
    src/mainwindow.h
    src/utils/paths.h
    src/delegates/searchitemdelegate.h
    src/models/searchlistmodel.h
    src/controllers/userinfocontroller.h
    src/services/userinfoservice.h
    src/controllers/websocketcontroller.h
    src/services/websocketservice.h
)

set(FORMS
    src/mainwindow.ui
)

set(RESOURCES
    resources/resources.qrc
)

# Иконка для Windows
if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "resources/enot_windows.rc")
endif()

qt_add_executable(vent
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${RESOURCES}
    ${APP_ICON_RESOURCE_WINDOWS}
)

# Линковка библиотеки Qt6 и qtkeychain
target_link_libraries(vent PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::WebSockets
    Qt6::Multimedia
    Qt6::Widgets
    qt6keychain
)

# Подключение всех папок
target_include_directories(vent PRIVATE 
    ${qtkeychain_SOURCE_DIR} 
    ${qtkeychain_BINARY_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/delegates
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)
