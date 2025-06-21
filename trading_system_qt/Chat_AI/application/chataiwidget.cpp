/* =========================================================================
   ChatAIWidget.cpp – implementation of ChatAIWidget.h
   -------------------------------------------------------------------------
   QML-embedded chat panel that
     • Streams free news headlines via an external WebSocket and injects
       them into the chat feed.
     • Filters user questions: if Gemini deems the query finance-related,
       it forwards the prompt to Google Gemini and returns a concise answer
       (<150 words); otherwise it politely refuses.
     • Uses QNetworkAccessManager for Gemini REST calls so the UI thread
       never blocks, and reconnects the news socket after drops (5 s delay).

   NOTE – The Google API key is hard-coded for the demo. In production move
   it out of source control (env var, vault, or OAuth flow).
   ========================================================================= */

#include "chataiwidget.h"
#include "qboxlayout.h"
#include "qtimer.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QQuickItem>

static const QString GOOGLE_BEARER_TOKEN = "AIzaSyDmxAeFuTMeISmIIMq3yxZDQIHfT0ZImDM";
static const QString GEMINI_API_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=";

ChatAIWidget::ChatAIWidget(QWidget *parentWidget,
                           QObject *parent)
    : QObject(parent),
    qmlWidget(new QQuickWidget(parentWidget)),
    newsSocket(nullptr)
{
    qmlWidget->engine()->rootContext()->setContextProperty("chatAIWidgetCpp", this);
    qmlWidget->setSource(QUrl(QStringLiteral("qrc:/Chat_AI/ChatAIWidget.qml")));
    qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    auto *layout = new QVBoxLayout(parentWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(qmlWidget);
    parentWidget->setLayout(layout);
}

ChatAIWidget::~ChatAIWidget(){
    if (newsSocket) {
        newsSocket->close();
    }
}

QQuickWidget *ChatAIWidget::widget() const{
    return qmlWidget;
}

void ChatAIWidget::setNewsSocket(QWebSocket *newsSocket){
    newsSocket = newsSocket;

    connect(newsSocket, &QWebSocket::connected, this, &ChatAIWidget::onNewsConnected);
    connect(newsSocket, &QWebSocket::textMessageReceived, this, &ChatAIWidget::onNewsTextReceived);
    connect(newsSocket, &QWebSocket::disconnected, this, &ChatAIWidget::onNewsDisconnected);

    newsSocket->open(QUrl("wss://example.com/free-news-stream"));
}

void ChatAIWidget::onUserSendMessage(const QString &userMessage){
    checkIfFinancial(userMessage);
}

/* ----------------------------------------------------------------------
   1) Ask Gemini if the question is finance-related.
   2) If yes, handleUserQuestion() is invoked; else reply politely.
   ---------------------------------------------------------------------- */
void ChatAIWidget::checkIfFinancial(const QString &question){
    QString isFinancialPrompt = "Is this question about the financial market? Question: " + question;
    QJsonObject data;
    data["model"] = "gemini-2.0-flash";

    QJsonObject part;
    part["text"] = isFinancialPrompt;

    QJsonArray parts;
    parts.append(part);

    QJsonObject content;
    content["parts"] = parts;

    QJsonArray contents;
    contents.append(content);

    data["contents"] = contents;

    QJsonDocument doc(data);
    QByteArray requestData = doc.toJson();

    QUrl url(GEMINI_API_URL);
    url.setQuery("key=" + GOOGLE_BEARER_TOKEN);

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Content-Type", "application/json");

    QNetworkReply *reply = netManager.post(request, requestData);

    connect(reply, &QNetworkReply::finished, [reply, this, question]() {
        if (reply->error() != QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString errMsg = reply->errorString();
            qDebug() << "Gemini error [" << statusCode << "]: " << errMsg;
            emit newChatResponse(QString("Gemini error [%1]: %2").arg(statusCode).arg(errMsg));
            reply->deleteLater();
            return;
        }

        QByteArray raw = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(raw);
        QJsonObject responseObj = responseDoc.object();

        QString extractedText;

        if (responseObj.contains("candidates")) {
            QJsonArray candidatesArray = responseObj["candidates"].toArray();
            if (!candidatesArray.isEmpty()) {
                QJsonObject candidateObj = candidatesArray[0].toObject();
                if (candidateObj.contains("content")) {
                    QJsonObject contentObj = candidateObj["content"].toObject();
                    if (contentObj.contains("parts")) {
                        QJsonArray partsArray = contentObj["parts"].toArray();
                        if (!partsArray.isEmpty()) {
                            extractedText = partsArray[0].toObject()["text"].toString();
                        }
                    }
                }
            }
        }

        if (extractedText.contains("yes", Qt::CaseInsensitive)) {
            handleUserQuestion(question);
        } else {
            emit newChatResponse("I'm only here to answer financial questions about the market.");
        }

        reply->deleteLater();
    });
}

/* Build a concise prompt & send to Gemini */
void ChatAIWidget::handleUserQuestion(const QString &financialQuestion){
    QString prompt =
        "You are an AI assistant for a university project. Provide clear, "
        "actionable market insight. Keep the answer under 150 words. "
        "Question: " + financialQuestion;
    callGoogleGemini(prompt);
}

void ChatAIWidget::callGoogleGemini(const QString &prompt){
    QJsonObject data;
    data["model"] = "gemini-2.0-flash";

    QJsonObject part;
    part["text"] = prompt;
    QJsonArray parts; parts.append(part);

    QJsonObject content; content["parts"] = parts;
    QJsonArray contents; contents.append(content);
    data["contents"] = contents;

    QJsonDocument doc(data);
    QByteArray requestData = doc.toJson();

    QUrl url(GEMINI_API_URL);
    url.setQuery("key=" + GOOGLE_BEARER_TOKEN);

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Content-Type", "application/json");

    QNetworkReply *reply = netManager.post(request, requestData);

    connect(reply, &QNetworkReply::finished, [reply, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString errMsg = reply->errorString();
            emit newChatResponse(QString("Google Gemini error [%1]: %2").arg(statusCode).arg(errMsg));
            reply->deleteLater();
            return;
        }

        QByteArray raw = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(raw);
        QJsonObject responseObj = responseDoc.object();

        QString extractedText;

        if (responseObj.contains("candidates")) {
            QJsonArray candidatesArray = responseObj["candidates"].toArray();
            if (!candidatesArray.isEmpty()) {
                QJsonObject candidateObj = candidatesArray[0].toObject();
                if (candidateObj.contains("content")) {
                    QJsonObject contentObj = candidateObj["content"].toObject();
                    if (contentObj.contains("parts")) {
                        QJsonArray partsArray = contentObj["parts"].toArray();
                        if (!partsArray.isEmpty()) {
                            extractedText = partsArray[0].toObject()["text"].toString();
                        }
                    }
                }
            }
        }

        /* clean up line-breaks and bullet chars */
        extractedText.replace(QRegularExpression("[\\n\\r\\t]+"), " ")
            .replace(QRegularExpression("[^\\x20-\\x7E]"), "")
            .replace(QRegularExpression("[\\s]+"), " ")
            .replace('*', "")
            .replace(QString::fromUtf8("•"), "")
            .trimmed();

        if (extractedText.isEmpty()) {
            emit newChatResponse("Error: empty response from Gemini.");
        } else {
            emit newChatResponse(extractedText);
        }

        reply->deleteLater();
    });
}

/* -------------------- headline WebSocket handlers ------------------ */
void ChatAIWidget::onNewsDisconnected(){
    qDebug() << "[ChatAIWidget] News socket disconnected. Reconnecting in 5 s...";
    QTimer::singleShot(5000, this, [this]() {
        newsSocket->open(QUrl("wss://example.com/free-news-stream"));
    });
}

void ChatAIWidget::onNewsConnected(){
    qDebug() << "[ChatAIWidget] News socket connected.";
}

void ChatAIWidget::onNewsTextReceived(const QString &message){
    emit newChatResponse(message);   // push to chat feed
}

/* Push headline into QML ListView */
void ChatAIWidget::invokeQmlHeadline(const QString &headline){
    if (qmlWidget->rootObject()) {
        QMetaObject::invokeMethod(qmlWidget->rootObject(),
                                  "onNewHeadline",
                                  Q_ARG(QString, headline));
    }
}
