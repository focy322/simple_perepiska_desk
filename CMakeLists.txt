cmake_minimum_required(VERSION 3.19)
project(vent VERSION 0.0.1 LANGUAGES CXX)

option(VENT_STATIC_BUILD "Build with static Qt and runtime on Windows with MinGW" ON)
option(VENT_WINDOWS_CONSOLE "Build Windows app with console window" ON)

if(WIN32 AND MINGW AND VENT_STATIC_BUILD)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network WebSockets Multimedia MultimediaWidgets Widgets)
get_target_property(vent_qt_type Qt6::Core TYPE)

if(WIN32 AND MINGW AND VENT_STATIC_BUILD AND NOT vent_qt_type STREQUAL "STATIC_LIBRARY")
    message(FATAL_ERROR
            "VENT_STATIC_BUILD=ON requires a static Qt kit. "
            "Select a static Qt kit or set VENT_STATIC_BUILD=OFF.")
endif()

include(FetchContent)
FetchContent_Declare(
        qtkeychain
        GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
        GIT_TAG        0.14.2
)

function(vent_add_keychain)
    set(BUILD_WITH_QT6 ON)
    set(BUILD_TRANSLATIONS OFF)
    set(BUILD_TEST_APPLICATION OFF)
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(qtkeychain)

    # В установщик приложения не входят архив и заголовки qtkeychain.
    set_property(DIRECTORY "${qtkeychain_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)
    target_include_directories(qt6keychain INTERFACE
            "$<BUILD_INTERFACE:${qtkeychain_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${qtkeychain_BINARY_DIR}>"
    )
endfunction()

vent_add_keychain()

file(GLOB_RECURSE vent_sources CONFIGURE_DEPENDS
        "src/*.cpp"
        "src/*.h"
        "src/*.ui"
        "resources/*.qrc"
)

qt_add_executable(vent ${vent_sources})
target_compile_features(vent PRIVATE cxx_std_20)
set_target_properties(vent PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
        # Предпросмотр в src/widgets использует общую форму главного окна.
        AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(WIN32)
    target_sources(vent PRIVATE resources/enot_windows.rc)
    set_property(SOURCE resources/enot_windows.rc APPEND PROPERTY OBJECT_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/enot_windows.ico")
    if(NOT VENT_WINDOWS_CONSOLE)
        set_target_properties(vent PROPERTIES WIN32_EXECUTABLE ON)
    endif()
endif()

target_include_directories(vent PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
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

if(WIN32 AND MINGW AND VENT_STATIC_BUILD)
    find_package(PkgConfig REQUIRED)

    if(TARGET TIFF::TIFF)
        # Плагин Qt подтягивает TIFF::TIFF, поэтому кодеки должны идти за архивом libtiff.
        pkg_check_modules(VENT_TIFF REQUIRED libtiff-4)
        list(REMOVE_ITEM VENT_TIFF_STATIC_LIBRARIES tiff)
        target_link_directories(TIFF::TIFF INTERFACE ${VENT_TIFF_STATIC_LIBRARY_DIRS})
        target_link_libraries(TIFF::TIFF INTERFACE ${VENT_TIFF_STATIC_LIBRARIES})
        target_link_options(TIFF::TIFF INTERFACE ${VENT_TIFF_STATIC_LDFLAGS_OTHER})
    endif()

    # FreeType и HarfBuzz могут ссылаться друг на друга.
    pkg_check_modules(VENT_RUNTIME REQUIRED freetype2 openssl)
    target_link_libraries(vent PRIVATE
            -Wl,--start-group
            ${VENT_RUNTIME_STATIC_LDFLAGS}
            -Wl,--end-group
            ws2_32
            crypt32
    )
    target_link_options(vent PRIVATE -static)
endif()

install(TARGETS vent RUNTIME DESTINATION .)

if(WIN32 AND vent_qt_type STREQUAL "SHARED_LIBRARY")
    install(CODE "
        execute_process(
            COMMAND \"$<TARGET_FILE:Qt6::windeployqt>\"
                    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:vent>\"
            COMMAND_ERROR_IS_FATAL ANY
        )
    ")
endif()

set(CPACK_PACKAGE_NAME "Vent")
set(CPACK_PACKAGE_VENDOR "Vent")
set(CPACK_PACKAGE_FILE_NAME "Vent-Installer-${PROJECT_VERSION}")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Vent")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/enot_windows.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CPACK_NSIS_MUI_ICON}")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "vent.exe")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_PACKAGE_EXECUTABLES "vent" "Vent")
    if(MINGW AND VENT_STATIC_BUILD AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CPACK_STRIP_FILES ON)
    endif()
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Vent")
    set(CPACK_RPM_PACKAGE_LICENSE "Unknown")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/Vent")
endif()

include(CPack)