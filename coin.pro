#-------------------------------------------------
#
# Project created by QtCreator 2013-05-05T21:53:28
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = coin
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    transactionsmodel.cpp

HEADERS  += mainwindow.h \
    transactionsmodel.h

FORMS    += mainwindow.ui
