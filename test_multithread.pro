QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 测试程序目标名称
TARGET = test_multithread

# 测试程序源文件
SOURCES += \
    test_multithread_example.cpp \
    database.cpp \
    conflictchecker.cpp \
    exportthread.cpp \
    csvexporter.cpp

# 测试程序头文件
HEADERS += \
    database.h \
    conflictchecker.h \
    exportthread.h \
    csvexporter.h

# 不需要UI文件，因为测试程序是纯代码实现的

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target





