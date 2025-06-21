/* =========================================================================
   ExecutionWidget.cpp – implementation of ExecutionWidget.h
   -------------------------------------------------------------------------
   GUI order-ticket panel (QML front-end) that lets the trader submit
   market-orders and shows the live bid / ask for the currently-selected
   symbol.

     • onLiveAssetPrice(bid, ask) updates the two price labels in QML every
       tick so the user sees up-to-date quotes while sizing the order.
     • onPlaceMarketTradeRequested(…) is called from QML when the trader
       presses the “Market” button; it simply forwards the request via
       newTradePlaced(…) to DisplayManager, which performs the risk checks
       and cloud I/O.
     • handleOrderResult(success) receives an immediate OK/FAIL boolean
       from DisplayManager and invokes showOrderResultBanner(success) in
       QML so the ticket flashes green (success) or red (rejected).

   The class itself stays deliberately thin; all validation and networking
   live in DisplayManager so the UI thread can never block.
   ========================================================================= */

#include "executionwidget.h"
#include <QDebug>
#include <QQmlContext>
#include <QVariant>
#include <QMetaObject>

/* -------------------------------------------------------------------------
   ctor – build QQuickWidget, expose C++ backend to QML, load the .qml file
   ------------------------------------------------------------------------- */
ExecutionWidget::ExecutionWidget(QWidget *parentWidget,
                                 QObject  *parent)
    : QObject(parent),
    quickWidget(new QQuickWidget(parentWidget)),
    qmlRootObject(nullptr),
    displayManager(nullptr)
{
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickWidget->rootContext()->setContextProperty("executionWidgetBackend", this);
    quickWidget->setSource(QUrl(QStringLiteral("qrc:/ExecutionWidget.qml")));

    qmlRootObject = quickWidget->rootObject();
    if (!qmlRootObject)
        qWarning() << "[ExecutionWidget] Failed to load ExecutionWidget.qml!";
}

/* ------------------------------ dtor ------------------------------------ */
ExecutionWidget::~ExecutionWidget() = default;

/* -------------------- helper: return embedded widget -------------------- */
QQuickWidget *ExecutionWidget::widget() const{
    return quickWidget;
}

/* -------------------------------------------------------------------------
   Wire DisplayManager signals → slots and vice-versa
   ------------------------------------------------------------------------- */
void ExecutionWidget::setDisplayManager(DisplayManager *manager){
    this->displayManager = manager;

    connect(displayManager, &DisplayManager::liveAssetPrice,
            this,           &ExecutionWidget::onLiveAssetPrice);

    connect(displayManager, &DisplayManager::orderSuccesful,
            this,           &ExecutionWidget::handleOrderResult);
}

/* -------------------------------------------------------------------------
   Tick handler – push latest bid / ask into QML properties
   ------------------------------------------------------------------------- */
void ExecutionWidget::onLiveAssetPrice(double bid, double sell){
    if (qmlRootObject) {
        qmlRootObject->setProperty("bidPrice", bid);
        qmlRootObject->setProperty("askPrice", sell);
    }
}

/* -------------------------------------------------------------------------
   Called from QML when the user hits “Market”
   ------------------------------------------------------------------------- */
void ExecutionWidget::onPlaceMarketTradeRequested(double openPrice,
                                                  double stopLoss,
                                                  double takeProfit,
                                                  double size,
                                                  int    assetIndex,
                                                  QString position)
{
    qDebug() << "[ExecutionWidget] Market trade requested. Position:" << position;

    emit newTradePlaced(stopLoss, takeProfit, size,
                        assetIndex, openPrice, "market", position);
}

/* forward toolbar asset picker change ----------------------------------- */
void ExecutionWidget::onAssetChange(int asset){
    if (displayManager)
        displayManager->assetChange(asset);
}

/* -------------------------------------------------------------------------
   Order result (green / red banner) feedback from DisplayManager
   ------------------------------------------------------------------------- */
void ExecutionWidget::handleOrderResult(bool success){
    if (!qmlRootObject) {
        qWarning() << "[ExecutionWidget] QML root object is null; cannot show banner.";
        return;
    }

    QMetaObject::invokeMethod(qmlRootObject,
                              "showOrderResultBanner",
                              Q_ARG(QVariant, success));
}
