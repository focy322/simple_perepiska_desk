QT       += core gui network
QT += websockets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    authcontroller.cpp \
    authservice.cpp \
    chatscontroller.cpp \
    chatlistitemdelegate.cpp \
    chatlistmodel.cpp \
    chatmessagesitemdelegate.cpp \
    chatmessageslistmodel.cpp \
    chatservice.cpp \
    errortypes.cpp \
    main.cpp \
    mainwindow.cpp \
    searchitemdelegate.cpp \
    searchlistmodel.cpp \
    userinfocontroller.cpp \
    userinfoservice.cpp \
    websocketcontroller.cpp \
    websocketservice.cpp

HEADERS += \
    authcontroller.h \
    authservice.h \
    chatscontroller.h \
    chatlistitemdelegate.h \
    chatlistmodel.h \
    chatmessagesitemdelegate.h \
    chatmessageslistmodel.h \
    chatservice.h \
    endpoints.h \
    errortypes.h \
    mainwindow.h \
    searchitemdelegate.h \
    searchlistmodel.h \
    userinfocontroller.h \
    userinfoservice.h \
    websocketcontroller.h \
    websocketservice.h

FORMS += \
    mainwindow.ui

win32:RC_ICONS += enot_windows.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
