import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    width: 600
    height: 300
    radius: 0  // no rounded corners

    color: "#0C0C0C"
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#1A1A1A" }
        GradientStop { position: 1.0; color: "#0C0C0C" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        Text {
            id: title
            text: "Open Trades"
            color: "#F0B90B"
            font.pointSize: 16
            font.bold: true
            font.family: "Open Sans"
        }

        ListModel {
            id: tradesModel
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: tradesModel
            spacing: 2

            delegate: Rectangle {
                width: parent.width
                height: 44
                radius: 4
                color: "#1A1A1A"
                border.color: "#333333"
                border.width: 1
                anchors.margins: 2

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    // Extra spacing to separate fields more
                    spacing: 20

                    // 1) TradeID
                    Text {
                        text: tradeID
                        color: "#FFFFFF"
                        font.bold: true
                        font.family: "Open Sans"
                    }

                    // 2) Asset
                    Text {
                        text: asset
                        color: "#FFFFFF"
                        font.bold: true
                        font.family: "Open Sans"
                    }

                    // 3) Size
                    Text {
                        text: "Size: " + size
                        color: "#E0E0E0"
                        font.family: "Open Sans"
                    }

                    // 4) PnL
                    Text {
                        text: "PnL: " + pnl.toFixed(2)
                        color: (pnl >= 0 ? "#44BB44" : "#FF4444")
                        font.bold: true
                        font.family: "Open Sans"
                    }

                    // 5) Close button
                    Button {
                        text: "Close"
                        font.family: "Open Sans"
                        font.bold: true
                        contentItem: Text {
                            text: "Close"
                            color: "#0C0C0C"
                            font.bold: true
                            font.family: "Open Sans"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        onClicked: {
                            tradeWidgetBackend.onCloseTradeClicked(tradeID)
                        }
                        background: Rectangle {
                            radius: 4
                            color: "#F0B90B"
                            border.color: "#D9A600"
                            border.width: 1

                            Behavior on color {
                                ColorAnimation {
                                    duration: 400
                                    easing.type: Easing.InOutQuad
                                }
                            }
                        }
                        Layout.alignment: Qt.AlignRight

                        onPressed:  background.color = "#C1A300"
                        onReleased: background.color = "#F0B90B"
                    }
                }

                // Hover effect, but no left-click capture
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton

                    onEntered: parent.color = "#2A2A2A"
                    onExited: parent.color = "#1A1A1A"
                }
            }
        }
    }

    // Prepare model for sync
    function prepareSync() {
        for (var i = 0; i < tradesModel.count; i++) {
            tradesModel.setProperty(i, "used", false);
        }
    }

    // upsertTrade => accept 9 arguments including 'asset'
    function upsertTrade(tradeID, asset, stopLoss, takeProfit, size, openPrice,
                         type, position, pnl) {
        for (var i = 0; i < tradesModel.count; i++) {
            if (tradesModel.get(i).tradeID === tradeID) {
                tradesModel.setProperty(i, "asset", asset);
                tradesModel.setProperty(i, "pnl", pnl);
                tradesModel.setProperty(i, "size", size);
                tradesModel.setProperty(i, "used", true);
                return;
            }
        }
        tradesModel.append({
            "tradeID": tradeID,
            "asset": asset,
            "stopLoss": stopLoss,
            "takeProfit": takeProfit,
            "size": size,
            "openPrice": openPrice,
            "type": type,
            "position": position,
            "pnl": pnl,
            "used": true
        });
    }

    function removeUnusedTrades() {
        for (var i = tradesModel.count - 1; i >= 0; i--) {
            if (!tradesModel.get(i).used) {
                tradesModel.remove(i);
            }
        }
    }
}
