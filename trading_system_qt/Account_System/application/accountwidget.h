/* =========================================================================
   AccountWidget.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Qt Quick bridge that embeds the “Account.qml” dashboard inside a QWidget.

     • Listens to Account-level signals (balanceUpdated, alphaUpdated,
       equityUpdated, accountLocked) and forwards values into QML via
       setQmlProperty().
     • Owns a TradeHistoryWidget instance and shows it on demand.
     • Exposes widget() so MainWindow can dock the QQuickWidget wherever it
       likes (tab, splitter, stacked view, etc.).

   Design notes
     • The parent QWidget is passed from MainWindow; the QQuickWidget is
       created with that parent so focus handling is native-style.
     • QML root is cached (qmlRootObject) to avoid repeated
       findChild<>() look-ups on every tick.
     • TODO: throttle equityUpdated() if update frequency exceeds 30 Hz.
   ========================================================================= */

#ifndef ACCOUNTWIDGET_H
#define ACCOUNTWIDGET_H

#include <QObject>
#include <QVariant>

/* Forward declarations to cut compile time */
class QQuickWidget;
class QQuickItem;
class Account;
class TradeHistoryWidget;

class AccountWidget : public QObject
{
    Q_OBJECT
public:
    explicit AccountWidget(QWidget *parentWidget = nullptr,
                           QObject *parent       = nullptr);
    ~AccountWidget();

    /* Returns the embedded QQuickWidget so callers can insert it in layouts */
    QQuickWidget* widget() const;

public slots:
    void showTradeHistory();   // slot called from QML button

private slots:
    void onBalanceUpdated(double newBalance);
    void onAccountLocked();
    void onAlphaUpdated(double newAlpha);
    void onEquityUpdated();

private:
    QQuickWidget*       quickWidget   {nullptr};  // container for QML scene
    QObject*            qmlRootObject {nullptr};  // cached root item
    Account*            account       {nullptr};  // singleton instance

    TradeHistoryWidget* m_historyWidget {nullptr};

    /* Helper: push a property into the QML root in a single line */
    void setQmlProperty(const char* propertyName, const QVariant& value);
};

#endif // ACCOUNTWIDGET_H
