/* =========================================================================
   TradeHistoryWidget.cpp – implementation of TradeHistoryWidget.h
   -------------------------------------------------------------------------
   Embeds TradeHistoryWidget.qml inside a QQuickWidget and feeds it the
   latest trade history pulled from Account.

     • Publishes this object to QML as tradeHistoryWidgetBackend.
     • Converts QList<TradeHistory*> → QVariantList so ListView/TableView
       delegates can bind without a custom model.
     • Refreshes the list whenever Account::tradeHistoryUpdated(...) fires.
   ========================================================================= */

#include "tradehistorywidget.h"

#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>
#include <QMetaObject>
#include <QVariantMap>

/* ------------------------------------------------------------------ */
/* ctor – build QQuickWidget, wire context & subscribe to Account     */
/* ------------------------------------------------------------------ */
TradeHistoryWidget::TradeHistoryWidget(QWidget* parentWidget, QObject* parent)
    : QObject(parent)
    , quickWidget(nullptr)
    , qmlRootObject(nullptr)
    , account(nullptr)
{
    account = Account::getInstance();

    quickWidget = new QQuickWidget(parentWidget);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    /* Expose C++ backend to QML */
    quickWidget->rootContext()->setContextProperty("tradeHistoryWidgetBackend", this);
    quickWidget->setSource(QUrl(QStringLiteral("qrc:/Account_System/TradeHistoryWidget.qml")));

    qmlRootObject = qobject_cast<QQuickItem*>(quickWidget->rootObject());
    if (!qmlRootObject)
        qWarning() << "[TradeHistoryWidget] Failed to load TradeHistoryWidget.qml!";

    /* Listen for history updates from the Account singleton */
    connect(account, &Account::tradeHistoryUpdated,
            this,    &TradeHistoryWidget::onTradeHistoryUpdated);

    /* Push initial data */
    onTradeHistoryUpdated(account->getTradeHistory());
}

TradeHistoryWidget::~TradeHistoryWidget(){
    delete quickWidget;
}

/* Return underlying QQuickWidget so caller can embed in layouts */
QQuickWidget* TradeHistoryWidget::getQuickWidget() const{
    return quickWidget;
}

QVariantList TradeHistoryWidget::tradeHistory() const{
    return userTradeHistory;
}

/* ------------------------------------------------------------------ */
/* Slot – rebuild QVariantList and notify QML                         */
/* ------------------------------------------------------------------ */
void TradeHistoryWidget::onTradeHistoryUpdated(const QList<TradeHistory*> &history){
    if (!qmlRootObject) {
        qWarning() << "[TradeHistoryWidget] No root QML object to update!";
        return;
    }

    userTradeHistory.clear();
    for (TradeHistory* th : history) {
        QVariantMap map;
        map["tradeID"]    = th->getTradeID();
        map["asset"]      = static_cast<int>(th->getAsset());
        map["size"]       = th->getSize();
        map["openPrice"]  = th->getOpenPrice();
        map["closePrice"] = th->getClosingPrice();
        map["pnl"]        = th->getPnl();
        map["date"]       = th->getDate().toString(Qt::ISODate);

        userTradeHistory.push_back(map);
    }

    emit tradeHistoryUpdated();   // triggers QML bindings to re-evaluate

    /* Call into QML to load the new list (imperative refresh) */
    QMetaObject::invokeMethod(
        qmlRootObject,
        "loadTradeHistory",
        Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(userTradeHistory))
        );
}
