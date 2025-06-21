import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5

Rectangle {
    id: root
    width: 100
    height: 80
    radius: 0  // already at 0

    color: "#0C0C0C"
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#1A1A1A" }
        GradientStop { position: 1.0; color: "#0C0C0C" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        RowLayout {
            spacing: 8
            anchors.leftMargin: 8
            anchors.rightMargin: 8

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
                Layout.fillWidth: true
                font.family: "Open Sans"
                font.pointSize: 12

                // The main displayed text (currently selected item) in yellow
                contentItem: Text {
                    text: assetCombo.displayText
                    color: "#F0B90B"
                    font.family: "Open Sans"
                    font.pointSize: 12
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 6
                    rightPadding: 8
                }
                // Dark background + yellow border
                background: Rectangle {
                    radius: 5
                    color: "#1A1A1A"
                    border.color: "#F0B90B"
                    border.width: 1
                }
                // Dropdown indicator (arrow)
                indicator: Text {
                    text: "\u25BE"
                    color: "#F0B90B"
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                }

                // Style the popup list
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

                onCurrentIndexChanged: {
                    if (chartWidgetCpp) {
                        chartWidgetCpp.onAssetButtonClicked(currentIndex);
                    }
                }
            }

            Label {
                text: " Select Timeframe:"
                color: "#F0B90B"
                font.pointSize: 13
                font.bold: true
                font.family: "Open Sans"
            }

            ComboBox {
                id: timeframeCombo
                model: ["1m", "5m", "15m", "1h", "4h", "1d"]
                currentIndex: 0

                Layout.preferredWidth: 180
                Layout.fillWidth: true
                font.family: "Open Sans"
                font.pointSize: 12

                // The main displayed text in yellow
                contentItem: Text {
                    text: timeframeCombo.displayText
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

                // Style the popup list
                delegate: ItemDelegate {
                    width: timeframeCombo.width
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

                onCurrentIndexChanged: {
                    if (chartWidgetCpp) {
                        chartWidgetCpp.onTimeframeButtonClicked(currentIndex);
                    }
                }
            }
        }
    }
}
