    QGCLabel {
                        id:         videoSectionLabel
                        text:       qsTr("Video")
                        visible:    QGroundControl.settingsManager.videoSettings.visible && !QGroundControl.videoManager.autoStreamConfigured
                    }
                    Rectangle {
                        Layout.preferredWidth:  videoGrid.width + (_margins * 2)
                        Layout.preferredHeight: videoGrid.height + (_margins * 2)
                        Layout.fillWidth:       true
                        color:                  qgcPal.windowShade
                        visible:                videoSectionLabel.visible

                        GridLayout {
                            id:                         videoGrid
                            anchors.margins:            _margins
                            anchors.top:                parent.top
                            anchors.horizontalCenter:   parent.horizontalCenter
                            Layout.fillWidth:           false
                            Layout.fillHeight:          false
                            columns:                    2
                            QGCLabel {
                                text:                   qsTr("Video Source")
                                visible:                QGroundControl.settingsManager.videoSettings.videoSource.visible
                            }
                            FactComboBox {
                                id:                     videoSource
                                Layout.preferredWidth:  _comboFieldWidth
                                indexModel:             false
                                fact:                   QGroundControl.settingsManager.videoSettings.videoSource
                                visible:                QGroundControl.settingsManager.videoSettings.videoSource.visible
                            }

                            QGCLabel {
                                text:                   qsTr("UDP Port")
                                visible:                (_isUDP264 || _isUDP265 || _isMPEGTS)  && QGroundControl.settingsManager.videoSettings.udpPort.visible
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.udpPort
                                visible:                (_isUDP264 || _isUDP265 || _isMPEGTS) && QGroundControl.settingsManager.videoSettings.udpPort.visible
                            }

                            QGCLabel {
                                text:                   qsTr("RTSP URL")
                                visible:                _isRTSP && QGroundControl.settingsManager.videoSettings.rtspUrl.visible
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.rtspUrl
                                visible:                _isRTSP && QGroundControl.settingsManager.videoSettings.rtspUrl.visible
                            }

                            QGCLabel {
                                text:                   qsTr("TCP URL")
                                visible:                _isTCP && QGroundControl.settingsManager.videoSettings.tcpUrl.visible
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.tcpUrl
                                visible:                _isTCP && QGroundControl.settingsManager.videoSettings.tcpUrl.visible
                            }
                            QGCLabel {
                                text:                   qsTr("Aspect Ratio")
                                visible:                _isGst && QGroundControl.settingsManager.videoSettings.aspectRatio.visible
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.aspectRatio
                                visible:                _isGst && QGroundControl.settingsManager.videoSettings.aspectRatio.visible
                            }

                            QGCLabel {
                                text:                   qsTr("Disable When Disarmed")
                                visible:                _isGst && QGroundControl.settingsManager.videoSettings.disableWhenDisarmed.visible
                            }
                            FactCheckBox {
                                text:                   ""
                                fact:                   QGroundControl.settingsManager.videoSettings.disableWhenDisarmed
                                visible:                _isGst && QGroundControl.settingsManager.videoSettings.disableWhenDisarmed.visible
                            }
                        }
                    }

                    Item { width: 1; height: _margins }

                    QGCLabel {
                        id:                             videoRecSectionLabel
                        text:                           qsTr("Video Recording")
                        visible:                        (QGroundControl.settingsManager.videoSettings.visible && _isGst) || QGroundControl.videoManager.autoStreamConfigured
                    }
                    Rectangle {
                        Layout.preferredWidth:          videoRecCol.width  + (_margins * 2)
                        Layout.preferredHeight:         videoRecCol.height + (_margins * 2)
                        Layout.fillWidth:               true
                        color:                          qgcPal.windowShade
                        visible:                        videoRecSectionLabel.visible

                        GridLayout {
                            id:                         videoRecCol
                            anchors.margins:            _margins
                            anchors.top:                parent.top
                            anchors.horizontalCenter:   parent.horizontalCenter
                            Layout.fillWidth:           false
                            columns:                    2

                            QGCLabel {
                                text:                   qsTr("Auto-Delete Files")
                                visible:                QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
                            }
                            FactCheckBox {
                                text:                   ""
                                fact:                   QGroundControl.settingsManager.videoSettings.enableStorageLimit
                                visible:                QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
                            }

                            QGCLabel {
                                text:                   qsTr("Max Storage Usage")
                                visible:                QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
                            }
                            FactTextField {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.maxVideoSize
                                visible:                QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
                            }

                            QGCLabel {
                                text:                   qsTr("Video File Format")
                                visible:                QGroundControl.settingsManager.videoSettings.recordingFormat.visible
                            }
                            FactComboBox {
                                Layout.preferredWidth:  _comboFieldWidth
                                fact:                   QGroundControl.settingsManager.videoSettings.recordingFormat
                                visible:                QGroundControl.settingsManager.videoSettings.recordingFormat.visible
                            }
                        }
                    }

