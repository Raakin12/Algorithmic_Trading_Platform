/* =========================================================================
   MainWindow.cpp – implementation of MainWindow.h
   -------------------------------------------------------------------------
   Top-level Qt container that wires every major panel together.

     • Left column ─ ExecutionWidget (flashing gold frame) and ChatAIWidget
       stacked vertically.
     • Right column ─ Account summary, ChartWidget, and TradeWidget stacked
       vertically inside a 2-column QHBoxLayout.
     • Owns all controller classes (DisplayManager, WebSocketClient,
       TradeManager, ChartManager) and connects them so signals propagate
       from GUI → DisplayManager → cloud and back.
     • Boots the chart with historical candles, starts the Binance live
       feed, and sets the main window title.
   ========================================================================= */

#include "mainwindow.h"
#include "tradewidget.h"
#include "executionwidget.h"
#include "displaymanager.h"
#include "websocketclient.h"
#include "chartwidget.h"
#include "chartmanager.h"
#include "trademanager.h"
#include "accountwidget.h"
#include "chataiwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QQuickWidget>
#include <QFrame>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #000000;");
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    executionWidget = new ExecutionWidget(central, this);
    chartWidget       = new ChartWidget(central);
    chartManager      = new ChartManager(chartWidget, this);
    chartWidget->setChartManager(chartManager);

    tradeWidget     = new TradeWidget(central, this);

    displayManager  = new DisplayManager(this);
    webSocketClient = new WebSocketClient(this);
    tradeManager    = new TradeManager(displayManager, this);

    displayManager->setTradeWidget(tradeWidget);
    displayManager->setExecutionWidget(executionWidget);
    displayManager->setWebSocketClient(webSocketClient);

    tradeWidget->setDisplayManager(displayManager);
    executionWidget->setDisplayManager(displayManager);

    webSocketClient->setDisplayManager(displayManager);
    webSocketClient->setTradeManager(tradeManager);

    if (auto execQQuick = qobject_cast<QQuickWidget*>(executionWidget->widget())) {
        execQQuick->setClearColor(QColor("#202020"));
    }
    if (auto tradeQQuick = qobject_cast<QQuickWidget*>(tradeWidget->widget())) {
        tradeQQuick->setClearColor(QColor("#202020"));
    }

    QFrame *execFrame = new QFrame(central);
    execFrame->setObjectName("ExecutionFrame");
    execFrame->setStyleSheet(
        "#ExecutionFrame {"
        "    background-color: #000000;"
        "    border: 2px dashed #DB9A39;"
        "    border-radius: 0px;"
        "}"
        );

    QVBoxLayout *execLayout = new QVBoxLayout(execFrame);
    execLayout->setSpacing(0);
    execLayout->setContentsMargins(0, 0, 0, 0);
    execLayout->addWidget(executionWidget->widget());

    QTimer *flashTimer = new QTimer(this);
    flashTimer->setInterval(1000);
    static bool gold = true;
    connect(flashTimer, &QTimer::timeout, this, [=]() mutable {
        gold = !gold;
        if (gold) {
            execFrame->setStyleSheet(
                "#ExecutionFrame {"
                "    background-color: #000000;"
                "    border: 2px dashed #DB9A39;"
                "    border-radius: 0px;"
                "}"
                );
        } else {
            execFrame->setStyleSheet(
                "#ExecutionFrame {"
                "    background-color: #000000;"
                "    border: 2px dashed #000000;"
                "    border-radius: 0px;"
                "}"
                );
        }
    });
    flashTimer->start();

    ChatAIWidget *chatAiWidget = new ChatAIWidget(central);
    if (!chatAiWidget) {
        qCritical() << "Failed to initialize ChatAIWidget!";
        return;
    }

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->setSpacing(0);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(execFrame,0);
    leftLayout->addWidget(chatAiWidget->widget(),1);

    mainLayout->addLayout(leftLayout, 1);

    AccountWidget *accountWidget = new AccountWidget(central, this);

    QFrame *chartFrame = new QFrame(central);
    chartFrame->setObjectName("ChartFrame");
    chartFrame->setStyleSheet(
        "#ChartFrame {"
        "    background-color: #000000;"
        "    border: 1px solid #DB9A39;"
        "    border-radius: 0px;"
        "}"
        );

    QVBoxLayout *chartLayout = new QVBoxLayout(chartFrame);
    chartLayout->setSpacing(0);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->addWidget(chartWidget);
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(0);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(accountWidget->widget(),0);
    rightLayout->addWidget(chartFrame,3);
    rightLayout->addWidget(tradeWidget->widget(),1);

    mainLayout->addLayout(rightLayout, 2);

    chartWidget->loadHistoricalData();
    chartWidget->startLiveData();

    resize(1300, 900);
    setWindowTitle("RM Capital Markets - Fully Wired");
}

MainWindow::~MainWindow()
{}
