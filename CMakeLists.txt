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

# Автоматически находим все файлы в нужных папках                                                                                                                                                    
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")                                                                                                                                             
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.h")                                                                                                                                               
file(GLOB_RECURSE FORMS CONFIGURE_DEPENDS "src/*.ui")                                                                                                                                                
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS "resources/*.qrc") 

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
)
