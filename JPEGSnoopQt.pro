#-------------------------------------------------
#
# Project created by QtCreator 2018-06-07T16:53:35
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = JPEGSnoopQt
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

macx{
QMAKE_CXXFLAGS += -isystem /Users/bob/Qt/5.11.0/clang_64/lib/QtCore.framework/Versions/5/Headers
QMAKE_CXXFLAGS += -isystem /Users/bob/Qt/5.11.0/clang_64/lib/QtGui.framework/Versions/5/Headers
QMAKE_CXXFLAGS += -isystem /Users/bob/Qt/5.11.0/clang_64/lib/QtWidgets.framework/Versions/5/Headers
}

linux:!macx{
QMAKE_CXXFLAGS += -isystem /usr/include/qt5/QtCore
QMAKE_CXXFLAGS += -isystem /usr/include/qt5/QtGui
QMAKE_CXXFLAGS += -isystem /usr/include/qt5/QtWidgets
}

QMAKE_CXXFLAGS += -fstrict-aliasing -Wextra -pedantic -Weffc++ -Wfloat-equal -Wswitch-default -Wcast-align -Wcast-qual -Wchar-subscripts -Wcomment -Wdisabled-optimization -Wformat-nonliteral -Wformat-security
QMAKE_CXXFLAGS += -Wconversion -Wformat-nonliteral -Wformat-y2k -Wformat=2 -Wimport -Winit-self -Winline -Winvalid-pch -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn -Wold-style-cast
QMAKE_CXXFLAGS += -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wstack-protector -Wstrict-aliasing=2 -Wswitch-enum -Wunreachable-code -Wunused -Wvariadic-macros -Wwrite-strings

#CAL! Added source directory
INCLUDEPATH += source

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES =  \
    source/JfifDecode.cpp \
    source/DbSigs.cpp \
    source/DecodePs.cpp \
    source/DocLog.cpp \
    source/General.cpp \
    source/ImgDecode.cpp \
    source/JPEGsnoopCore.cpp \
    source/main.cpp \
    source/mainwindow.cpp \
    source/Md5.cpp \
    source/q_decodedetaildlg.cpp \
    source/SnoopConfig.cpp \
    source/snoopconfigdialog.cpp \
    source/WindowBuf.cpp
SOURCES +=
SOURCES +=


HEADERS =  \
    source/JfifDecode.h \
    source/DbSigs.h \
    source/WindowBuf.h \
    source/snoopconfigdialog.h \
    source/SnoopConfig.h \
    source/snoop.h \
    source/q_decodedetaildlg.h \
    source/Md5.h \
    source/mainwindow.h \
    source/JPEGsnoopCore.h \
    source/JPEGsnoop.h \
    source/ImgDecode.h \
    source/General.h \
    source/DocLog.h \
    source/DecodePs.h \
    source/DecodeDicom.h
HEADERS +=
HEADERS +=


FORMS += \
    source/q_decodedetaildlg.ui \
    source/snoopconfigdialog.ui
