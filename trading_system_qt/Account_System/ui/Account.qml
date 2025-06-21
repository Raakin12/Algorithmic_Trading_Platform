import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import QtQml 6.5  // Needed for Qt.createComponent()

Rectangle {
    id: root
    width: 300
    height: 100
    radius: 0

    // Black gradient background
    color: "#0C0C0C"
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#1A1A1A" }
        GradientStop { position: 1.0; color: "#0C0C0C" }
    }

    // Exposed properties for C++ to set
    property int userID: 0
    property double alpha: 0.0
    property double balance: 0.0
    property double equity: 0.0

    // Keep a reference to any trade history window we create
    property var tradeHistoryObject: null

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 24

        // Left column: User ID, Alpha
        ColumnLayout {
            spacing: 6

            Label {
                text: "User ID: " + root.userID
                color: "#F0B90B"
                font.family: "Open Sans"
                font.pointSize: 13
                font.bold: true
            }
            Label {
                text: "Alpha: " + root.alpha.toFixed(4)
                color: "#F0B90B"
                font.family: "Open Sans"
                font.pointSize: 13
                font.bold: true
            }
        }

        // Right column: Balance, Equity
        ColumnLayout {
            spacing: 6

            Label {
                text: "Balance: " + root.balance.toFixed(2)
                color: "#F0B90B"
                font.family: "Open Sans"
                font.pointSize: 13
                font.bold: true
            }
            Label {
                text: "Equity: " + root.equity.toFixed(2)
                color: "#F0B90B"
                font.family: "Open Sans"
                font.pointSize: 13
                font.bold: true
            }
        }

        // The "Trade History" button
        Button {
            text: "Trade History"

            Layout.alignment: Qt.AlignRight
            width: 120
            height: 30
            font.family: "Open Sans"
            font.pointSize: 12

            contentItem: Text {
                text: parent.text
                color: "black"
                font.family: "Open Sans"
                font.pointSize: 12
            }

            background: Rectangle {
                color: parent.pressed ? "#C1A300" : "#F0B90B"
                radius: 6
                border.color: "#F0B90B"
                border.width: 1

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            onClicked: {
                console.log("Trade History button clicked in QML!");

                // If we already created it once, just make sure it's visible and on top
                if (root.tradeHistoryObject) {
                    root.tradeHistoryObject.visible = true
                    root.tradeHistoryObject.raise()
                    root.tradeHistoryObject.requestActivate()
                    return
                }

                // Dynamically load "TradeHistoryWidget.qml" as a component
                var component = Qt.createComponent("qrc:/Account_System/TradeHistoryWidget.qml");
                if (component.status === Component.Ready) {
                    // Create it with null parent => separate top-level window
                    root.tradeHistoryObject = component.createObject(null);

                    if (root.tradeHistoryObject) {
                        // Position the new window
                        root.tradeHistoryObject.x = 400
                        root.tradeHistoryObject.y = 200

                        // Show it
                        root.tradeHistoryObject.visible = true
                        root.tradeHistoryObject.raise()
                        root.tradeHistoryObject.requestActivate()
                    } else {
                        console.log("Failed to create TradeHistoryWidget object!")
                    }
                } else {
                    console.log("Failed to load TradeHistoryWidget.qml:", component.errorString())
                }
            }
        }
    }
}
