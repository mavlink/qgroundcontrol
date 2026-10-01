import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property var parentToolInsets
    property var totalToolInsets: _toolInsets
    property var mapControl

    // ---------------------------------------------------------------------
    // ESTADO
    // ---------------------------------------------------------------------
    property bool gimbalControlEnabled: false
    property bool recording: false

    property real mouseOriginX: 0
    property real mouseOriginY: 0

    property real yawCommand: 0
    property real pitchCommand: 0

    // Zona morta para evitar pequenos movimentos involuntários
    property real deadzonePixels: 5

    // Sensibilidade do movimento do rato
    property real sensitivity: 200

    // ---------------------------------------------------------------------
    // COMANDOS
    //
    // Atualmente são MOCKS.
    // Na próxima fase estas funções chamam AtlasJetsonClient.
    // ---------------------------------------------------------------------
    function sendGimbal(yaw, pitch) {
        atlasJetsonClient.gimbalRate(
            yaw,
            pitch
        )
    }

    function stopGimbal() {
        root.yawCommand = 0
        root.pitchCommand = 0

        gimbalSendTimer.stop()

        atlasJetsonClient.gimbalStop()
    }

    function recenterGimbal() {
        atlasJetsonClient.gimbalCenter()
    }

    function setZoom(value) {
        atlasJetsonClient.zoom(value)
    }

    function takePhoto() {
        atlasJetsonClient.takePhoto()
    }

    function toggleRecording() {
        root.recording = !root.recording

        atlasJetsonClient.setRecording(
            root.recording
        )
    }

    // ---------------------------------------------------------------------
    // ENVIO PERIÓDICO DA GIMBAL
    //
    // O rato pode produzir mais de 100 eventos/s.
    //
    // Não enviamos todos esses eventos pela rede.
    // Guardamos apenas o comando mais recente e enviamos:
    //
    //      50 ms -> 20 Hz
    //
    // Mais tarde:
    //
    // QGC -> HM30 -> Jetson -> SIYI A8
    // ---------------------------------------------------------------------
    Timer {
        id: gimbalSendTimer

        interval: 50
        repeat: true
        running: false

        // Primeiro comando é enviado imediatamente.
        triggeredOnStart: true

        onTriggered: {
            if (!root.gimbalControlEnabled) {
                stop()
                return
            }

            root.sendGimbal(
                root.yawCommand,
                root.pitchCommand
            )
        }
    }

    // ---------------------------------------------------------------------
    // TOOL INSETS
    // ---------------------------------------------------------------------
    QGCToolInsets {
        id: _toolInsets

        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset

        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset

        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset

        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }

    // ---------------------------------------------------------------------
    // CONTROLO DA GIMBAL COM RATO
    // ---------------------------------------------------------------------
    MouseArea {
        id: gimbalMouseArea

        anchors.fill: parent

        enabled: root.gimbalControlEnabled
        hoverEnabled: false

        // -------------------------------------------------------------
        // Começa um novo movimento
        // -------------------------------------------------------------
        onPressed: function(mouse) {
            root.mouseOriginX = mouse.x
            root.mouseOriginY = mouse.y

            root.yawCommand = 0
            root.pitchCommand = 0
        }

        // -------------------------------------------------------------
        // Movimento do rato
        // -------------------------------------------------------------
        onPositionChanged: function(mouse) {
            var dx = mouse.x - root.mouseOriginX
            var dy = mouse.y - root.mouseOriginY

            // ---------------------------------------------------------
            // DEADZONE
            //
            // Se regressarmos à região central enquanto a gimbal estava
            // em movimento, enviamos STOP imediatamente.
            // ---------------------------------------------------------
            if (Math.abs(dx) < root.deadzonePixels &&
                Math.abs(dy) < root.deadzonePixels) {

                root.yawCommand = 0
                root.pitchCommand = 0

                if (gimbalSendTimer.running) {
                    root.stopGimbal()
                }

                return
            }

            // ---------------------------------------------------------
            // COMPORTAMENTO SIYI
            //
            // Mantemos inicialmente a filosofia do SIYI-QGroundControl:
            // é usado apenas o eixo dominante.
            //
            // Movimento predominantemente horizontal:
            //     -> YAW
            //
            // Movimento predominantemente vertical:
            //     -> PITCH
            // ---------------------------------------------------------
            if (Math.abs(dx) > Math.abs(dy)) {

                // YAW
                root.pitchCommand = 0

                root.yawCommand =
                    dx * root.sensitivity / root.width

                // Limitar ao intervalo suportado:
                // -100 ... +100
                root.yawCommand =
                    Math.max(
                        -100,
                        Math.min(
                            100,
                            root.yawCommand
                        )
                    )

            } else {

                // PITCH
                root.yawCommand = 0

                // O sinal negativo mantém a direção intuitiva
                // do movimento vertical.
                root.pitchCommand =
                    -dy * root.sensitivity / root.height

                root.pitchCommand =
                    Math.max(
                        -100,
                        Math.min(
                            100,
                            root.pitchCommand
                        )
                    )
            }

            // ---------------------------------------------------------
            // Não enviamos diretamente daqui.
            //
            // Apenas atualizamos yawCommand/pitchCommand.
            //
            // O Timer é responsável por enviar a 20 Hz.
            // ---------------------------------------------------------
            if (!gimbalSendTimer.running) {
                gimbalSendTimer.start()
            }
        }

        // -------------------------------------------------------------
        // Largar o rato -> STOP imediato
        // -------------------------------------------------------------
        onReleased: {
            root.stopGimbal()
        }

        // -------------------------------------------------------------
        // Segurança caso o MouseArea perca o evento
        // -------------------------------------------------------------
        onCanceled: {
            root.stopGimbal()
        }

        // -------------------------------------------------------------
        // Duplo clique -> recentrar gimbal
        // -------------------------------------------------------------
        onDoubleClicked: {
            root.stopGimbal()
            root.recenterGimbal()
        }
    }

    // ---------------------------------------------------------------------
    // PAINEL DE CONTROLO DA CÂMARA
    // ---------------------------------------------------------------------
    Rectangle {
        id: cameraPanel

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 12

        width: 72
        height: cameraControls.implicitHeight + 20

        radius: 8

        // Fundo parcialmente transparente
        color: "#AA101112"

        ColumnLayout {
            id: cameraControls

            anchors.centerIn: parent

            spacing: 10

            // ---------------------------------------------------------
            // Ativar/desativar controlo da gimbal com rato
            // ---------------------------------------------------------
            QGCButton {
                text: root.gimbalControlEnabled
                      ? "GIMBAL\nON"
                      : "GIMBAL\nOFF"

                onClicked: {
                    root.gimbalControlEnabled =
                        !root.gimbalControlEnabled

                    // Sempre que o controlo é desligado,
                    // garantimos que a gimbal recebe STOP.
                    if (!root.gimbalControlEnabled) {
                        root.stopGimbal()
                    }
                }
            }

            // ---------------------------------------------------------
            // ZOOM IN
            //
            // +1 enquanto pressionado
            //  0 ao largar
            // ---------------------------------------------------------
            QGCButton {
                text: "ZOOM +"

                onPressed: {
                    root.setZoom(1)
                }

                onReleased: {
                    root.setZoom(0)
                }
            }

            // ---------------------------------------------------------
            // ZOOM OUT
            //
            // -1 enquanto pressionado
            //  0 ao largar
            // ---------------------------------------------------------
            QGCButton {
                text: "ZOOM -"

                onPressed: {
                    root.setZoom(-1)
                }

                onReleased: {
                    root.setZoom(0)
                }
            }

            // ---------------------------------------------------------
            // RECENTER
            // ---------------------------------------------------------
            QGCButton {
                text: "CENTER"

                onClicked: {
                    root.stopGimbal()
                    root.recenterGimbal()
                }
            }

            // ---------------------------------------------------------
            // PHOTO
            // ---------------------------------------------------------
            QGCButton {
                text: "PHOTO"

                onClicked: {
                    root.takePhoto()
                }
            }

            // ---------------------------------------------------------
            // RECORD START / STOP
            // ---------------------------------------------------------
            QGCButton {
                text: root.recording
                      ? "■ STOP"
                      : "● REC"

                onClicked: {
                    root.toggleRecording()
                }
            }
        }
    }
}
