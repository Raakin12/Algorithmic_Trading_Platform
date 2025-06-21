/* =========================================================================
   MainWindow.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   Top-level Qt window that hosts all major panels:

     • Tabs:       “Chart”, “Execution”, “Open Trades”, “Account”, “AI Chat”.
     • Aggregates: TradeWidget, ExecutionWidget, ChartWidget, AccountWidget,
                   ChatAIWidget, plus their controller classes
                   (DisplayManager, WebSocketClient, TradeManager, etc.).

   Nothing happens here except construction and lifetime ownership; all
   business logic lives in the individual widgets and managers.
   ========================================================================= */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>

class TradeWidget;
class ExecutionWidget;
class DisplayManager;
class WebSocketClient;
class TradeManager;
class ChartManager;
class ChartWidget;
class AccountWidget;
class ChatAIWidget;
class TradeHistoryWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    TradeWidget        *tradeWidget;
    ExecutionWidget    *executionWidget;
    DisplayManager     *displayManager;
    WebSocketClient    *webSocketClient;
    TradeManager       *tradeManager;
    ChartManager       *chartManager;
    ChartWidget        *chartWidget;
    AccountWidget      *accountWidget;
    ChatAIWidget       *chatAiWidget;
    TradeHistoryWidget *tradeHistoryWidget;
    QTabWidget         *tabWidget;
};

#endif // MAINWINDOW_H
