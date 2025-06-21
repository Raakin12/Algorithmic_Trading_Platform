import QtQuick 6.5
import QtQuick.Controls 6.5

// We omit "import QtQuick.Layouts" entirely.
// We'll use a simple 'Row' element rather than RowLayout.

Window {
    id: tradeHistoryWindow
    width: 1200
    height: 900
    title: "Trade History"
    visible: false
    color: "#1A1A1A"

    // 1) Hook the globalAccount's signal
    Connections {
        target: globalAccount
        function onTradeHistoryUpdated() {
            console.log("[TradeHistoryWidget.qml] onTradeHistoryUpdated => from globalAccount.")
            let data = globalAccount.getTradeHistoryVariant()
            loadTradeHistory(data)
        }
    }

    // 2) On creation => load existing trades
    Component.onCompleted: {
        console.log("[TradeHistoryWidget.qml] onCompleted => load trades from globalAccount.")
        let initialData = globalAccount.getTradeHistoryVariant()
        loadTradeHistory(initialData)
    }

    // 3) Outer Column for title + ListView
    Column {
        anchors.fill: parent
        spacing: 10
        anchors.margins: 10

        Text {
            text: "Trade History"
            color: "#F0B90B"
            font.pointSize: 16
            font.bold: true
            font.family: "Open Sans"
        }

        // Our scrollable ListView of trades
        ListView {
            id: tradeHistoryList
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            spacing: 4
            clip: true

            model: ListModel {}

            delegate: Rectangle {
                id: rowRect
                width: parent.width
                height: 60        // Force each row to be 60px tall
                radius: 4
                color: "#1A1A1A"
                border.color: "#333333"
                border.width: 1
                anchors.margins: 4

                // A Row of text items, spaced out
                Row {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 40    // Big horizontal gap

                    Text {
                        text: "TradeID: " + model.tradeID
                        color: "#E0E0E0"
                        font.family: "Open Sans"
                        font.bold: true
                    }

                    Text {
                        text: "Asset: " + model.asset
                        color: "#E0E0E0"
                        font.family: "Open Sans"
                        font.bold: true
                    }

                    Text {
                        text: "Size: " + model.size
                        color: "#E0E0E0"
                        font.family: "Open Sans"
                    }

                    Text {
                        text: "Open: " + model.openPrice
                        color: "#E0E0E0"
                        font.family: "Open Sans"
                    }

                    Text {
                        text: "Close: " + model.closePrice
                        color: "#E0E0E0"
                        font.family: "Open Sans"
                    }

                    Text {
                        text: "PnL: " + parseFloat(model.pnl).toFixed(2)
                        color: parseFloat(model.pnl) >= 0 ? "#44BB44" : "#FF4444"
                        font.bold: true
                        font.family: "Open Sans"
                    }

                    Text {
                        text: "Date: " + model.date
                        color: "#E0E0E0"
                        font.family: "Open Sans"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: rowRect.color = "#2A2A2A"
                    onExited:  rowRect.color = "#1A1A1A"
                }
            }
        }
    }

    // 4) Called to rebuild the ListView model and show the window
    function loadTradeHistory(history) {
        console.log("[TradeHistoryWidget.qml] loadTradeHistory => length:", history.length)

        tradeHistoryList.model.clear()
        for (let i = 0; i < history.length; i++) {
            tradeHistoryList.model.append({
                tradeID:    history[i].tradeID,
                asset:      history[i].asset,
                size:       history[i].size,
                openPrice:  history[i].openPrice,
                closePrice: history[i].closePrice,
                pnl:        history[i].pnl,
                date:       history[i].date
            })
        }

        // Make the window visible
        tradeHistoryWindow.visible = true
        tradeHistoryWindow.raise()
        tradeHistoryWindow.requestActivate()
    }
}
