/* =========================================================================
   TradeWidget.cpp – implementation of TradeWidget.h
   -------------------------------------------------------------------------
   QML-driven “Open Positions” panel.

     • Receives tradeMapUpdated() from DisplayManager, converts the
       QMap<Trade*, P&L> into upsert/remove calls so the QML ListView shows
       a live table of each open position and its running P & L.
     • Emits closeTradePressed(id) when the user clicks the ⓧ button in a
       row; DisplayManager forwards this upstream to the cloud.
     • assetToString() maps the Asset enum to a printable symbol.

   All heavy-lifting (risk checks, networking) lives in DisplayManager, so
   this class is a thin UI adapter and never blocks the GUI thread.
   ========================================================================= */

#include "tradewidget.h"
#include <QDebug>
#include <QMap>
#include <QQmlContext>
#include <QMetaObject>
#include <QVariant>
#include <QUrl>
#include "trade.h"

/* ------------------------- ctor ----------------------------------- */
TradeWidget::TradeWidget(QWidget *parentWidget,
                         QObject  *parent)
    : QObject(parent),
    displayManager(nullptr),
    quickWidget(new QQuickWidget(parentWidget)),
    qmlRootObject(nullptr)
{
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickWidget->rootContext()->setContextProperty("tradeWidgetBackend", this);
    quickWidget->setSource(QUrl(QStringLiteral("qrc:/TradeWidget.qml")));

    qmlRootObject = quickWidget->rootObject();
    if (!qmlRootObject)
        qWarning() << "[TradeWidget] Failed to load TradeWidget.qml!";
}

/* ------------------------- dtor ----------------------------------- */
TradeWidget::~TradeWidget() = default;

/* ------------------------- helper --------------------------------- */
QQuickWidget *TradeWidget::widget() const{
    return quickWidget;
}

/* Wire DisplayManager signals → slots ------------------------------ */
void TradeWidget::setDisplayManager(DisplayManager *manager){
    this->displayManager = manager;
    connect(displayManager, &DisplayManager::tradeMapUpdated,
            this,           &TradeWidget::onTradeMapUpdated);
}

/* tradeMap changed: sync into QML ---------------------------------- */
void TradeWidget::onTradeMapUpdated(){
    if (!displayManager) return;
    syncTrades(displayManager->getTradeMap());
}

/* close-button pressed in QML -------------------------------------- */
void TradeWidget::onCloseTradeClicked(QString tradeID){
    emit closeTradePressed(tradeID);
}

/* Push full trade list into QML ListView --------------------------- */
void TradeWidget::syncTrades(const QMap<Trade*, double> &trades){
    if (!qmlRootObject) return;

    QMetaObject::invokeMethod(qmlRootObject,
                              "prepareSync",
                              Qt::DirectConnection);

    for (auto it = trades.begin(); it != trades.end(); ++it) {
        Trade  *t   = it.key();
        double  pnl = it.value();

        QMetaObject::invokeMethod(
            qmlRootObject,
            "upsertTrade",
            Qt::DirectConnection,
            Q_ARG(QVariant, t->getTradeID()),
            Q_ARG(QVariant, assetToString(static_cast<Asset>(t->getAsset()))),
            Q_ARG(QVariant, t->getStopLoss()),
            Q_ARG(QVariant, t->getTakeProfit()),
            Q_ARG(QVariant, t->getSize()),
            Q_ARG(QVariant, t->getOpenPrice()),
            Q_ARG(QVariant, t->getType()),
            Q_ARG(QVariant, t->getPosition()),
            Q_ARG(QVariant, pnl));
    }

    QMetaObject::invokeMethod(qmlRootObject,
                              "removeUnusedTrades",
                              Qt::DirectConnection);
}

/* enum → printable symbol ------------------------------------------ */
QString TradeWidget::assetToString(Asset a){
    switch (a) {
    case BTCUSDT: return "BTCUSDT";
    case ETHUSDT: return "ETHUSDT";
    case SOLUSDT: return "SOLUSDT";
    case XRPUSDT: return "XRPUSDT";
    }
    return "UNKNOWN";
}
