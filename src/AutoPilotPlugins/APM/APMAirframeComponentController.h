/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef APMAirframeComponentController_H
#define APMAirframeComponentController_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QAbstractListModel>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"
#include "APMAirframeComponentAirframes.h"

class APMAirframeModel;
class APMAirframeType;

/// MVC Controller for APMAirframeComponent.qml.
class APMAirframeComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    enum FrameId{FRAME_TYPE_PLUS = 0,
                 FRAME_TYPE_X = 1,
                 FRAME_TYPE_V = 2,
                 FRAME_TYPE_H = 3,
                 FRAME_TYPE_NEWY6 = 10};
    Q_ENUM(FrameId)

    APMAirframeComponentController(void);
    ~APMAirframeComponentController();
    
    Q_PROPERTY(QmlObjectListModel* airframeTypesModel MEMBER _airframeTypesModel CONSTANT)
    Q_PROPERTY(APMAirframeType* currentAirframeType READ currentAirframeType WRITE setCurrentAirframeType NOTIFY currentAirframeTypeChanged)

    Q_INVOKABLE void loadParameters(const QString& paramFile);

    int currentAirframeIndex(void);
    void setCurrentAirframeIndex(int newIndex);
    
signals:
    void loadAirframesCompleted();
    void currentAirframeTypeChanged(APMAirframeType* airframeType);

public slots:
    APMAirframeType *currentAirframeType() const;
    Q_INVOKABLE QString currentAirframeTypeName() const;
    void setCurrentAirframeType(APMAirframeType *t);

private slots:
    void _fillAirFrames(void);
    void _factFrameChanged(QVariant v);
    void _githubJsonDownloadFinished(QString remoteFile, QString localFile);
    void _githubJsonDownloadError(QString errorMsg);
    void _paramFileDownloadFinished(QString remoteFile, QString localFile);
    void _paramFileDownloadError(QString errorMsg);

private:
    void _loadParametersFromDownloadFile(const QString& downloadedParamFile);

    APMAirframeType *_currentAirframeType;
    QmlObjectListModel *_airframeTypesModel;

    static bool _typesRegistered;
    static const char* _oldFrameParam;
    static const char* _newFrameParam;
};

class APMAirframe : public QObject
{
    Q_OBJECT
    
public:
    APMAirframe(const QString& name, const QString& paramsFile, int type, QObject* parent = NULL);
    ~APMAirframe();
    
    Q_PROPERTY(QString name MEMBER _name CONSTANT)
    Q_PROPERTY(int type MEMBER _type CONSTANT)
    Q_PROPERTY(QString params MEMBER _paramsFile CONSTANT)
    
    QString name() const;
    QString params() const;
    int type() const;

private:
    QString _name;
    QString _paramsFile;
    int _type;
};

class APMAirframeType : public QObject
{
    Q_OBJECT
    
public:
    APMAirframeType(const QString& name, const QString& imageResource, int type, QObject* parent = NULL);
    ~APMAirframeType();
    
    Q_PROPERTY(QString name MEMBER _name CONSTANT)
    Q_PROPERTY(QString imageResource MEMBER _imageResource CONSTANT)
    Q_PROPERTY(QVariantList airframes MEMBER _airframes CONSTANT)
    Q_PROPERTY(int type MEMBER _type CONSTANT)
    Q_PROPERTY(bool dirty MEMBER _dirty CONSTANT)
    void addAirframe(const QString& name, const QString& paramsFile, int type);

    QString name() const;
    QString imageResource() const;
    int type() const;
private:
    QString         _name;
    QString         _imageResource;
    QVariantList    _airframes;
    int _type;
    bool _dirty;
};

#endif
