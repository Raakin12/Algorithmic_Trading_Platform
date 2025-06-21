/* =========================================================================
   AccountWidget.cpp – implementation of AccountWidget.h
   -------------------------------------------------------------------------
   Qt-Quick bridge that embeds Account.qml inside a QQuickWidget.

     • Exposes accountWidgetBackend to QML for imperative calls.
     • Subscribes to Account* signals and pushes live values into QML
       through setQmlProperty().
     • Owns a TradeHistoryWidget instance and shows it when the user
       clicks the “Trade History” button in QML.
   ========================================================================= */

#include "accountwidget.h"
#include "account.h"
#include "tradehistorywidget.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>
#include <QColor>

/* ------------------------------------------------------------------ */
/* ctor – build QQuickWidget, wire context properties & signals       */
/* ------------------------------------------------------------------ */
AccountWidget::AccountWidget(QWidget *parentWidget, QObject *parent)
    : QObject(parent),
    quickWidget(new QQuickWidget(parentWidget)),
    qmlRootObject(nullptr),
    account(nullptr),
    m_historyWidget(nullptr)
{
    /* QQuickWidget visual setup */
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickWidget->setClearColor(QColor("#202020"));

    /* Expose C++ back-end objects to QML context */
    quickWidget->rootContext()->setContextProperty("accountWidgetBackend", this);
    quickWidget->rootContext()->setContextProperty("globalAccount", Account::getInstance());

    quickWidget->setSource(QUrl(QStringLiteral("qrc:/Account_System/Account.qml")));

    /* Cache root item for fast property updates */
    qmlRootObject = quickWidget->rootObject();
    if (!qmlRootObject)
        qWarning() << "[AccountWidget] Failed to load Account.qml!";

    /* Wire live signals from the Account singleton */
    account = Account::getInstance();
    if (account) {
        connect(account, &Account::balanceUpdated,
                this,    &AccountWidget::onBalanceUpdated);
        connect(account, &Account::accountLocked,
                this,    &AccountWidget::onAccountLocked);
        connect(account, &Account::alphaUpdated,
                this,    &AccountWidget::onAlphaUpdated);
        connect(account, &Account::equityUpdated,
                this,    &AccountWidget::onEquityUpdated);

        /* Push initial values into QML */
        setQmlProperty("userID",  account->getUserID());
        setQmlProperty("balance", account->getBalance());
        setQmlProperty("alpha",   account->getAlpha());
        setQmlProperty("equity",  account->getEquity());
    }
}

AccountWidget::~AccountWidget(){
    if (m_historyWidget)
        delete m_historyWidget;
}

/* Return the underlying QQuickWidget so caller can embed in layouts */
QQuickWidget* AccountWidget::widget() const{
    return quickWidget;
}

/* Slot exposed to QML – opens the Trade History window */
void AccountWidget::showTradeHistory(){
    qDebug() << "[AccountWidget] showTradeHistory() called from QML.";

    if (!m_historyWidget)
        m_historyWidget = new TradeHistoryWidget();

    QQuickWidget* qw = m_historyWidget->getQuickWidget();
    if (!qw) {
        qWarning() << "[AccountWidget] TradeHistoryWidget returned null QQuickWidget!";
        return;
    }

    qw->setWindowTitle("Trade History");
    qw->resize(600, 600);
    qw->show();
}

/* ------------------- Account signal handlers ---------------------- */
void AccountWidget::onBalanceUpdated(double newBalance){
    setQmlProperty("balance", newBalance);
}

void AccountWidget::onAccountLocked(){
    qDebug() << "[AccountWidget] Account locked!";
    setQmlProperty("balance", 0.0);
}

void AccountWidget::onAlphaUpdated(double newAlpha){
    setQmlProperty("alpha", newAlpha);
}

void AccountWidget::onEquityUpdated(){
    if (account)
        setQmlProperty("equity", account->getEquity());
}

/* Helper – push a property into the QML root item */
void AccountWidget::setQmlProperty(const char *propertyName, const QVariant &value){
    if (qmlRootObject)
        qmlRootObject->setProperty(propertyName, value);
    else
        qWarning() << "[AccountWidget] Root QML object not found. Cannot set" << propertyName;
}
