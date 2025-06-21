/* =========================================================================
   ChatAIWidget.h – Raakin Bhatti (M.Eng. capstone)
   -------------------------------------------------------------------------
   In-app chat panel that pipes user questions to Google Gemini and streams
   live news headlines into the same view.

     • onUserSendMessage() is called from QML when the user hits Send; the
       text is first checked for finance-related keywords, then forwarded to
       callGoogleGemini() for an LLM response.
     • setNewsSocket() plugs in an already-connected QWebSocket that pushes
       headline JSON; onNewsTextReceived() injects each headline into QML
       via invokeQmlHeadline().
     • Emits newChatResponse(message) so QML can append bot bubbles.

   Design notes
     • All network traffic (Gemini REST + headline WebSocket) is asynchronous
       through QNetworkAccessManager/QWebSocket, so the GUI never blocks.
     • invokeQmlHeadline() uses QMetaObject::invokeMethod() to ensure updates
       land on the QML thread safely.
     • TODO – add a simple rate-limit (e.g., max 3 requests per second) to
       prevent users from exhausting the Gemini quota by spamming messages.
   ========================================================================= */

#ifndef CHATAIWIDGET_H
#define CHATAIWIDGET_H

#include <QObject>
#include <QQuickWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebSocket>

class ChatAIWidget : public QObject
{
    Q_OBJECT
public:
    explicit ChatAIWidget(QWidget *parentWidget = nullptr,
                          QObject *parent = nullptr);
    ~ChatAIWidget();

    QQuickWidget *widget() const;

    void setNewsSocket(QWebSocket *newsSocket);

    Q_INVOKABLE void onUserSendMessage(const QString &userMessage);

signals:
    void newChatResponse(const QString &message);

private slots:
    void onNewsTextReceived(const QString &message);
    void onNewsConnected();
    void onNewsDisconnected();
    void callGoogleGemini(const QString &prompt);
    void checkIfFinancial(const QString &question);
    void handleUserQuestion(const QString &financialQuestion);

private:
    QQuickWidget *qmlWidget            {nullptr};
    QQuickItem   *qmlRootObject        {nullptr};
    QNetworkAccessManager netManager;
    QWebSocket  *newsSocket            {nullptr};

    void invokeQmlHeadline(const QString &headline);
};

#endif // CHATAIWIDGET_H
