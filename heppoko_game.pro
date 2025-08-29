QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# SOURCES += \
#     main.cpp \
#     mainwindow.cpp \
#     src/Player.cpp \
#     src/Box.cpp \
#     src/Map.cpp

# HEADERS += \
#     mainwindow.h \
#     src/Player.h \
#     src/Box.h \
#     src/Map.h


# FORMS += \
#     mainwindow.ui

# RESOURCES += \
#     resources/resources.qrc

SOURCES += main.cpp \
           mainwindow.cpp \
           src/box.cpp \
           src/map.cpp

HEADERS += mainwindow.h \
           src/box.h \
           src/map.h


RESOURCES += resources/resources.qrc

# 添加 include 路径，让编译器能找到 src 下的头文件
INCLUDEPATH += src



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
