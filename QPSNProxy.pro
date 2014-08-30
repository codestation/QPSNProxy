#-------------------------------------------------
#
# Project created by QtCreator 2014-06-08T14:47:51
#
#-------------------------------------------------

QT += core gui widgets network

lessThan(QT_MAJOR_VERSION, 5) {
    error("QPSNProxy is only compatible with Qt5")
}

lessThan(QT_MINOR_VERSION, 3) {
    QT += script
}

TARGET = QPSNProxy
TEMPLATE = app
VERSION = 0.1

SOURCES += main.cpp\
    mainwindow.cpp \
    downloaditem.cpp \
    psnrequest.cpp \
    json.cpp \
    authcookiejar.cpp \
    filterlineedit.cpp \
    psnparser.cpp \
    utils.cpp \
    authdialog.cpp \
    proxyserver.cpp \
    proxyconnection.cpp \
    configdialog.cpp

HEADERS  += mainwindow.h \
    downloaditem.h \
    psnrequest.h \
    json.h \
    authcookiejar.h \
    filterlineedit.h \
    psnparser.h \
    utils.h \
    authdialog.h \
    proxyserver.h \
    proxyconnection.h \
    configdialog.h

FORMS    += mainwindow.ui downloaditem.ui \
    authdialog.ui \
    configdialog.ui

RESOURCES += \
    qpsnres.qrc

OTHER_FILES += qpsnproxy.rc

QPSNPROXY_GIT_VERSION=$$system(git describe --tags --always)
isEmpty(QPSNPROXY_GIT_VERSION) {
    DEFINES += QPSNPROXY_VER=\\\"$$VERSION\\\"
} else {
    DEFINES += QPSNPROXY_VER=\\\"$$QPSNPROXY_GIT_VERSION\\\"
}

# Windows config
win32 {
    # Windows icon
    RC_FILE = qpsnproxy.rc
    # avoid alignment issues with newer mingw compiler
    QMAKE_CXXFLAGS += -mno-ms-bitfields
}

macx {
    # OS X icon
    ICON = resources/images/qpsnproxy.icns
}
