/****************************************************************************
 *
 * Copyright (c) 2016, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Streaming Widget Controller
 *   @author Ricardo de Almeida Gonzaga <ricardo.gonzaga@intel.com>
 */

#ifndef VideoStreamingWidgetController_H
#define VideoStreamingWidgetController_H

#include "FactPanelController.h"

class VideoStreamingWidgetController : public FactPanelController
{
    Q_OBJECT

public:
    VideoStreamingWidgetController(void);
    ~VideoStreamingWidgetController();

    void getServerInfo(void);
    void getStreamInfo(void);

    Q_INVOKABLE void refresh(void);
    Q_INVOKABLE void setIp(void);
    Q_INVOKABLE void setPort(void);
    Q_INVOKABLE void setActiveStream(void);
    Q_INVOKABLE void setFormat(void);
    Q_INVOKABLE void setFrameSize(void);
    Q_INVOKABLE void setName(void);

    Q_PROPERTY(bool        rtspEnabled          READ rtspEnabled          CONSTANT)
    Q_PROPERTY(QQuickItem* serverLabel          READ serverLabel          WRITE setServerLabel)
    Q_PROPERTY(QQuickItem* ipField              READ ipField              WRITE setIpField)
    Q_PROPERTY(QQuickItem* portField            READ portField            WRITE setPortField)
    Q_PROPERTY(QQuickItem* streamsComboBox      READ streamsComboBox      WRITE setStreamsComboBox)
    Q_PROPERTY(QQuickItem* formatComboBox       READ formatComboBox       WRITE setFormatComboBox)
    Q_PROPERTY(QQuickItem* frameSizeWidthField  READ frameSizeWidthField  WRITE setFrameSizeWidthField)
    Q_PROPERTY(QQuickItem* frameSizeHeightField READ frameSizeHeightField WRITE setFrameSizeHeightField)
    Q_PROPERTY(QQuickItem* nameField            READ nameField            WRITE setNameField)

    bool        rtspEnabled(void) {          return _rtspEnabled; }
    QQuickItem* serverLabel(void) {          return _serverLabel; }
    QQuickItem* ipField(void) {              return _ipField; }
    QQuickItem* portField(void) {            return _portField; }
    QQuickItem* streamsComboBox(void) {      return _streamsComboBox; }
    QQuickItem* formatComboBox(void) {       return _formatComboBox; }
    QQuickItem* frameSizeWidthField(void) {  return _frameSizeWidthField; }
    QQuickItem* frameSizeHeightField(void) { return _frameSizeHeightField; }
    QQuickItem* nameField(void) {            return _nameField; }

    void setServerLabel(QQuickItem* serverLabel)                   { _serverLabel = serverLabel; }
    void setIpField(QQuickItem* ipField)                           { _ipField = ipField; }
    void setPortField(QQuickItem* portField)                       { _portField = portField; }
    void setStreamsComboBox(QQuickItem* streamsComboBox)           { _streamsComboBox = streamsComboBox; }
    void setFormatComboBox(QQuickItem* formatComboBox)             { _formatComboBox = formatComboBox; }
    void setFrameSizeWidthField(QQuickItem* frameSizeWidthField)   { _frameSizeWidthField = frameSizeWidthField; }
    void setFrameSizeHeightField(QQuickItem* frameSizeHeightField) { _frameSizeHeightField = frameSizeHeightField; }
    void setNameField(QQuickItem* nameField)                       { _nameField = nameField; }

private:
    bool        _rtspEnabled;
    QQuickItem* _serverLabel;
    QQuickItem* _ipField;
    QQuickItem* _portField;
    QQuickItem* _streamsComboBox;
    QQuickItem* _formatComboBox;
    QQuickItem* _frameSizeWidthField;
    QQuickItem* _frameSizeHeightField;
    QQuickItem* _nameField;
};

#endif // VideoStreamingWidgetController_H
