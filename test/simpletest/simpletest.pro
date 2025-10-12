QT       += core widgets testlib
CONFIG   += c++17 console

INCLUDEPATH += ../../src

SOURCES += \
    main.cpp \
    simpletest.cpp \
    ../../src/collision.cpp \
    ../../src/box.cpp \
    ../../src/character.cpp \
    ../../src/map.cpp \
    ../../src/powerupmanager.cpp \
    ../../src/savegamemanager.cpp \
    ../../src/score.cpp

HEADERS += \
    simpletest.h \
    ../../src/collision.h \
    ../../src/box.h \
    ../../src/character.h \
    ../../src/map.h \
    ../../src/powerupmanager.h \
    ../../src/savegamemanager.h \
    ../../src/score.h

RESOURCES += ../../resources/resources.qrc
