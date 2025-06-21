QT += widgets           \
       core gui charts  \
       network          \
       websockets       \
       sql              \
       qml              \
       quick            \
       quickwidgets

# Include paths
INCLUDEPATH += $$PWD/Charting_System
INCLUDEPATH += $$PWD/Account_System
INCLUDEPATH += $$PWD/Trading_System
INCLUDEPATH += $$PWD/Chat_AI

SOURCES += \
    Account_System/account.cpp \
    Account_System/accountwidget.cpp \
    Account_System/tradehistorywidget.cpp \
    Charting_System/chartmanager.cpp \
    Charting_System/historicaldatamanager.cpp \
    Charting_System/livedatamanager.cpp \
    Charting_System/chartwidget.cpp \
    Chat_AI/chataiwidget.cpp \
    Trading_System/displaymanager.cpp \
    Trading_System/executionwidget.cpp \
    Trading_System/trade.cpp \
    Trading_System/trademanager.cpp \
    Trading_System/tradewidget.cpp \
    Trading_System/websocketclient.cpp \
    main.cpp \
    mainwindow.cpp

FORMS += \
    Charting_System/chartwidget.ui \
    mainwindow.ui

HEADERS += \
    Account_System/TradeHistory.h \
    Account_System/account.h \
    Account_System/accountwidget.h \
    Account_System/tradehistorywidget.h \
    Charting_System/asset.h \
    Charting_System/chartmanager.h \
    Charting_System/historicaldatamanager.h \
    Charting_System/livedatamanager.h \
    Charting_System/timeframe.h \
    Charting_System/chartwidget.h \
    Chat_AI/chataiwidget.h \
    DatabaseManager.h \
    Trading_System/displaymanager.h \
    Trading_System/executionwidget.h \
    Trading_System/trade.h \
    Trading_System/trademanager.h \
    Trading_System/tradewidget.h \
    Trading_System/websocketclient.h \
    mainwindow.h

# -- Embed the QML in resources.qrc --
RESOURCES += resources.qrc

# Clear DISTFILES or remove references to QML files so they are not duplicated.
DISTFILES +=
