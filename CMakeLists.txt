cmake_minimum_required(VERSION 3.16)
project(vent VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Настройка Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network WebSockets Multimedia MultimediaWidgets Widgets)


# --- Управление зависимостями ---
include(FetchContent)
FetchContent_Declare(
    qtkeychain
    GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
    GIT_TAG        0.14.2
)
# Настройка qtkeychain для динамической сборки и без лишних компонентов
set(BUILD_WITH_QT6 ON CACHE BOOL "Сборка с Qt 6" FORCE)
set(BUILD_TRANSLATIONS OFF CACHE BOOL "Сборка переводов" FORCE)
set(BUILD_TEST_APPLICATION OFF CACHE BOOL "Сборка тестового приложения" FORCE)

# Собираем как динамическую библиотеку
set(BUILD_SHARED_LIBS ON CACHE BOOL "Сборка qtkeychain как динамической библиотеки" FORCE)

FetchContent_MakeAvailable(qtkeychain)

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
    Qt6::MultimediaWidgets
    Qt6::Widgets
    qt6keychain
)

if(WIN32)
    target_link_libraries(vent PRIVATE dwmapi)
endif()

# Подключение всех папок
target_include_directories(vent PRIVATE 
    ${qtkeychain_SOURCE_DIR} 
    ${qtkeychain_BINARY_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


# --- Установка и генерация инсталлера ---
# Установка самого .exe файла в корень директории установки
install(TARGETS vent DESTINATION .)

# Подтягивание .dll библиотек Qt
if(WIN32)
    # Путь к windeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # Запуск windeployqt
    install(CODE "
        message(STATUS \"Running windeployqt to resolve dependencies...\")
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \"\${CMAKE_INSTALL_PREFIX}/vent.exe\"
        )
    ")
endif()

# Настройка CPack для кроссплатформенного создания инсталлеров
set(CPACK_PACKAGE_NAME "Vent")
set(CPACK_PACKAGE_VENDOR "Vent")
set(CPACK_PACKAGE_VERSION "0.0.1")
set(CPACK_PACKAGE_FILE_NAME "Vent-Installer-${CPACK_PACKAGE_VERSION}")

if(WIN32)
    # Специфичные настройки NSIS для Windows
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Vent")        # Имя папки в Program Files
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "vent.exe")      # Файл для запуска по завершении
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON) # Удалять старую версию перед новой
elseif(APPLE)
    # Настройки для macOS (дай бог когда-нибудь протестирую)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX AND NOT APPLE)
    # Настройки для Linux (DEB, RPM и .tar.gz)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Vent") # Обязательно для DEB
    set(CPACK_RPM_PACKAGE_LICENSE "Unknown")    # Желательно для RPM (может ругаться в некоторых ситуациях)
    
    # Установка в /opt/
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/Vent")
endif()

include(CPack)
