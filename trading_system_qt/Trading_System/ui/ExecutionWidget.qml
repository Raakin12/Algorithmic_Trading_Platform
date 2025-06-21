import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: executionRoot
    width: 380
    height: 350
    radius: 0

    // Black & gray gradient background
    gradient: Gradient {
        GradientStop { position: 0; color: "#1A1A1A" }
        GradientStop { position: 1; color: "#0C0C0C" }
    }

    // "buy" or "sell"
    property string selectedPosition: "buy"

    // Negative means "no valid price" => placeholders
    property double bidPrice: 0.0
    property double askPrice: 0.0

    // Whether trading is allowed or not
    property bool allowTrading: true

    // 5-second Timer to keep trading disabled after an asset change
    Timer {
        id: disableTradeTimer
        interval: 5000
        repeat: false
        onTriggered: {
            executionRoot.allowTrading = true
        }
    }

    // +-----------------------------------------------+
    // |  SUCCESS BANNER (Green at top)                |
    // +-----------------------------------------------+
    Rectangle {
        id: successBanner
        visible: false
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: "green"
        z: 999

        Text {
            anchors.centerIn: parent
            text: "Order Filled!"
            color: "white"
            font.bold: true
            font.pointSize: 14
        }
    }

    // +-----------------------------------------------+
    // |  FAIL BANNER (Red at bottom)                  |
    // +-----------------------------------------------+
    Rectangle {
        id: failBanner
        visible: false
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: "red"
        z: 999

        Text {
            anchors.centerIn: parent
            text: "Order Denied!"
            color: "white"
            font.bold: true
            font.pointSize: 14
        }
    }

    // Timer to hide banners after 3 seconds
    Timer {
        id: bannerTimer
        interval: 3000
        repeat: false
        onTriggered: {
            successBanner.visible = false
            failBanner.visible = false
        }
    }

    // This function is called from C++ via invokeMethod(...,"showOrderResultBanner",...)
    function showOrderResultBanner(success) {
        if (success) {
            successBanner.visible = true
            failBanner.visible = false
            bannerTimer.restart()
        } else {
            successBanner.visible = false
            failBanner.visible = true
            bannerTimer.restart()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        Text {
            text: "Market Execution"
            color: "#F0B90B"
            font.bold: true
            font.pointSize: 18
            Layout.alignment: Qt.AlignHCenter
        }

        // Row for "Select Asset"
        RowLayout {
            Layout.alignment: Qt.AlignLeft
            spacing: 8

            Label {
                text: "Select Asset:"
                color: "#F0B90B"
                font.pointSize: 13
                font.bold: true
                font.family: "Open Sans"
            }

            ComboBox {
                id: assetCombo
                model: ["BTCUSDT", "ETHUSDT", "SOLUSDT", "XRPUSDT"]
                currentIndex: 0
                Layout.preferredWidth: 200
                font.family: "Open Sans"
                font.pointSize: 12

                contentItem: Text {
                    text: assetCombo.displayText
                    color: "#F0B90B"
                    font.family: "Open Sans"
                    font.pointSize: 12
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 6
                    rightPadding: 8
                }
                background: Rectangle {
                    radius: 5
                    color: "#1A1A1A"
                    border.color: "#F0B90B"
                    border.width: 1
                }
                indicator: Text {
                    text: "\u25BE"
                    color: "#F0B90B"
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                }
                delegate: ItemDelegate {
                    width: assetCombo.width
                    contentItem: Text {
                        text: modelData
                        font.family: "Open Sans"
                        font.pointSize: 12
                        color: "#F0B90B"
                    }
                    background: Rectangle {
                        color: highlighted ? "#555555" : "#000000"
                    }
                }

                // When user changes asset:
                // 1) Reset prices to negative => placeholders
                // 2) Temporarily disable trading
                // 3) Start 5s timer
                // 4) Call onAssetChange(...) in C++ to switch streams
                onCurrentIndexChanged: {
                    executionRoot.bidPrice = -1
                    executionRoot.askPrice = -1
                    executionRoot.allowTrading = false
                    disableTradeTimer.restart()

                    executionWidgetBackend.onAssetChange(currentIndex)
                }
            }
        }

        // SELL/BUY row (centered)
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            // SELL box
            Rectangle {
                Layout.preferredWidth: 150
                height: 60
                radius: 6
                opacity: (executionRoot.allowTrading ? 1.0 : 0.6)

                color: (executionRoot.selectedPosition === "sell") ? "#FF4444" : "#AA3333"
                border.width: (executionRoot.selectedPosition === "sell") ? 3 : 0
                border.color: "#F0B90B"

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "SELL"
                        color: "#FFFFFF"
                        font.bold: true
                        font.pointSize: 14
                    }
                    Text {
                        text: (executionRoot.bidPrice <= 0)
                              ? "--"
                              : executionRoot.bidPrice.toFixed(2)
                        color: "#FFFFFF"
                        font.bold: true
                        font.pointSize: 16
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: executionRoot.allowTrading
                    onClicked: {
                        executionRoot.selectedPosition = "sell"
                    }
                }
            }

            // BUY box
            Rectangle {
                Layout.preferredWidth: 150
                height: 60
                radius: 6
                opacity: (executionRoot.allowTrading ? 1.0 : 0.6)

                color: (executionRoot.selectedPosition === "buy") ? "#44BB44" : "#33AA33"
                border.width: (executionRoot.selectedPosition === "buy") ? 3 : 0
                border.color: "#F0B90B"

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "BUY"
                        color: "#FFFFFF"
                        font.bold: true
                        font.pointSize: 14
                    }
                    Text {
                        text: (executionRoot.askPrice <= 0)
                              ? "--"
                              : executionRoot.askPrice.toFixed(2)
                        color: "#FFFFFF"
                        font.bold: true
                        font.pointSize: 16
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: executionRoot.allowTrading
                    onClicked: {
                        executionRoot.selectedPosition = "buy"
                    }
                }
            }
        }

        // Stop Loss / Take Profit / Size fields
        ColumnLayout {
            spacing: 14
            Layout.alignment: Qt.AlignLeft

            // STOP LOSS
            RowLayout {
                spacing: 16
                Layout.alignment: Qt.AlignLeft

                Label {
                    text: "Stop Loss:"
                    color: "#F0B90B"
                    font.pointSize: 13
                    font.bold: true
                    font.family: "Open Sans"
                    Layout.preferredWidth: 100
                }

                TextField {
                    id: slField
                    text: ""
                    font.family: "Open Sans"
                    font.pointSize: 12
                    color: "#F0B90B"
                    inputMethodHints: Qt.ImhDigitsOnly

                    Layout.preferredWidth: 100
                    background: Rectangle {
                        radius: 5
                        color: "#1A1A1A"
                        border.color: "#F0B90B"
                        border.width: 1
                    }
                }
            }

            // TAKE PROFIT
            RowLayout {
                spacing: 16
                Layout.alignment: Qt.AlignLeft

                Label {
                    text: "Take Profit:"
                    color: "#F0B90B"
                    font.pointSize: 13
                    font.bold: true
                    font.family: "Open Sans"
                    Layout.preferredWidth: 100
                }

                TextField {
                    id: tpField
                    text: ""
                    font.family: "Open Sans"
                    font.pointSize: 12
                    color: "#F0B90B"
                    inputMethodHints: Qt.ImhDigitsOnly

                    Layout.preferredWidth: 100
                    background: Rectangle {
                        radius: 5
                        color: "#1A1A1A"
                        border.color: "#F0B90B"
                        border.width: 1
                    }
                }
            }

            // SIZE
            RowLayout {
                spacing: 16
                Layout.alignment: Qt.AlignLeft

                Label {
                    text: "Size:"
                    color: "#F0B90B"
                    font.pointSize: 13
                    font.bold: true
                    font.family: "Open Sans"
                    Layout.preferredWidth: 100
                }

                TextField {
                    id: sizeField
                    text: ""
                    font.family: "Open Sans"
                    font.pointSize: 12
                    color: "#F0B90B"
                    inputMethodHints: Qt.ImhDigitsOnly

                    Layout.preferredWidth: 100
                    background: Rectangle {
                        radius: 5
                        color: "#1A1A1A"
                        border.color: "#F0B90B"
                        border.width: 1
                    }
                }
            }
        }

        // Place Trade Button
        Button {
            id: placeTradeButton
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            height: 38
            font.bold: true

            enabled: executionRoot.allowTrading

            background: Rectangle {
                radius: 6
                color: placeTradeButton.pressed
                       ? (executionRoot.selectedPosition === "buy" ? "#113b11" : "#691717")
                       : (executionRoot.selectedPosition === "buy" ? "#228822" : "#AA3333")
                // Short fade animation
                Behavior on color {
                    ColorAnimation { duration: 120 }
                }
            }

            text: (executionRoot.selectedPosition === "buy")
                  ? "Place BUY Trade"
                  : "Place SELL Trade"

            onClicked: {
                var slVal   = parseFloat(slField.text);
                var tpVal   = parseFloat(tpField.text);
                var sizeVal = parseFloat(sizeField.text);

                // Basic checks
                if (slField.text === "" || isNaN(slVal) ||
                    tpField.text === "" || isNaN(tpVal) ||
                    sizeField.text === "" || isNaN(sizeVal)) {
                    console.log("Stop Loss, Take Profit, and Size must not be blank or invalid.");
                    return;
                }

                var posVal  = (executionRoot.selectedPosition === "buy") ? "long" : "short";
                var assetVal = assetCombo.currentIndex;
                var openPx  = (executionRoot.selectedPosition === "buy")
                              ? executionRoot.askPrice
                              : executionRoot.bidPrice;

                executionWidgetBackend.onPlaceMarketTradeRequested(
                    openPx,
                    slVal,
                    tpVal,
                    sizeVal,
                    assetVal,
                    posVal
                );
            }
        }
    }
}
