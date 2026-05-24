QT += widgets
QT += core gui
CONFIG += c++11

TARGET = FreelanceARM
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/models.cpp \
    src/storage.cpp \
    src/authdialog.cpp \
    src/registerdialog.cpp \
    src/mainwindow.cpp \
    src/buyerwidget.cpp \
    src/freelancerwidget.cpp

HEADERS += \
    src/models.h \
    src/storage.h \
    src/authdialog.h \
    src/registerdialog.h \
    src/mainwindow.h \
    src/buyerwidget.h \
    src/freelancerwidget.h
