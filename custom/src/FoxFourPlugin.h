/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QTranslator>
#include <QtQml/QQmlAbstractUrlInterceptor>

#include "QGCCorePlugin.h"
#include "QGCOptions.h"

#ifdef QGC_GST_STREAMING
#include "VideoReceiver/FoxFourGstVideoReceiver.h"
#endif

#include "ParameterSetter/ParameterSetter.h"
class QQmlApplicationEngine;

Q_DECLARE_LOGGING_CATEGORY(FoxFourLog)

class FoxFourPlugin : public QGCCorePlugin
{
    Q_OBJECT
    Q_PROPERTY(QString version MEMBER _version)
    Q_PROPERTY (ParameterSetter *parameterSetter MEMBER _parameterSetter)
public:
    explicit FoxFourPlugin(QObject *parent = nullptr);

    static QGCCorePlugin *instance();


    void cleanup() final;
    QGCOptions *options() final { return _options; }

    bool overrideSettingsGroupVisibility(const QString &name) final;

    VideoReceiver *createVideoReceiver(QObject *parent);


    QQmlApplicationEngine *createQmlApplicationEngine(QObject *parent) final;

private slots:
    void _advancedChanged(bool advanced);

private:
    ParameterSetter *_parameterSetter = nullptr;
    QString _version= "0.0.0";
    QGCOptions *_options = nullptr;
    QQmlApplicationEngine *_qmlEngine = nullptr;
    class CustomOverrideInterceptor *_selector = nullptr;
    QVariantList _customSettingsList; // Not to be mixed up with QGCCorePlugin implementation

};

/*===========================================================================*/

class CustomOverrideInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    CustomOverrideInterceptor();

    QUrl intercept(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) final;
};
