import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5

Rectangle {
    id: root
    width: 600
    height: 400
    color: "#0C0C0C"
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#1A1A1A" }
        GradientStop { position: 1.0; color: "#0C0C0C" }
    }

    /* ───── chat data model ───── */
    ListModel { id: chatListModel }

    /* ───── helper for auto-scroll ───── */
    function scrollToBottom() {
        // delay 0 lets layout finish before scrolling
        Qt.callLater(function () {
            chatScroll.flickableItem.contentY =
                    chatScroll.flickableItem.contentHeight - chatScroll.height
        })
    }

    Connections {
        target: chatAIWidgetCpp
        function onNewChatResponse(msg) {
            chatListModel.append({ role: "bot", message: msg })
            scrollToBottom()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 6

        /* title */
        Text {
            text: "Market Sentiment AI"
            color: "#F0B90B"
            font.bold: true
            font.pointSize: 18
            Layout.alignment: Qt.AlignHCenter
        }

        /* scroll-area with messages */
        ScrollView {
            id: chatScroll
            Layout.fillWidth: true
            Layout.fillHeight: true

            Column {
                id: msgColumn
                anchors.left: parent.left
                anchors.right: parent.right
                width: chatScroll.width      /* full width */
                spacing: 4

                Repeater {
                    model: chatListModel
                    delegate: Rectangle {
                        width: msgColumn.width
                        color: role === "bot" ? "#2A2A2A" : "#333333"
                        radius: 4
                        anchors.margins: 6
                        height: msgText.implicitHeight + 12

                        Text {
                            id: msgText
                            anchors.fill: parent
                            anchors.margins: 6
                            text: message
                            wrapMode: Text.Wrap
                            color: "#F0B90B"
                            font.pointSize: 12
                            font.family: "Open Sans"
                        }
                    }
                }
            }
        }

        /* input row */
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            TextField {
                id: userField
                Layout.fillWidth: true
                placeholderText: "Ask your market question..."
                placeholderTextColor: "#F0B90B"
                color: "#F0B90B"
                font.pointSize: 12
                font.family: "Open Sans"
                background: Rectangle {
                    radius: 5
                    color: "#1A1A1A"
                    border.color: "#F0B90B"
                    border.width: 1
                }
                Keys.onReturnPressed: sendButton.clicked()
            }

            Button {
                id: sendButton
                text: "Send"
                font.pointSize: 12
                font.family: "Open Sans"
                background: Rectangle { color: "#F0B90B"; radius: 5 }

                onClicked: {
                    if (userField.text.trim().length > 0) {
                        chatListModel.append({ role: "user", message: userField.text })
                        chatAIWidgetCpp.onUserSendMessage(userField.text)
                        userField.text = ""
                        scrollToBottom()
                    }
                }
            }
        }
    }
}
