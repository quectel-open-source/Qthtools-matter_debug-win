QT += core
QT += gui-private

DEFINES += QSU_INCLUDE_MOC=1

HEADERS += \
    $$PWD/ccrashstack.h \
    $$PWD/logthread.h \
    $$PWD/openlog.h

SOURCES += \
    $$PWD/ccrashstack.cpp \
    $$PWD/logthread.cpp \
    $$PWD/openlog.cpp
