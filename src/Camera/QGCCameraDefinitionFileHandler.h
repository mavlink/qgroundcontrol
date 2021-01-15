/*!
 *   @file
 *   @brief Camera Definition File Parser
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#ifndef QGCCAMERADEFINITIONFILEHANDLER_H
#define QGCCAMERADEFINITIONFILEHANDLER_H

#include "QGCApplication.h"
#include "QGCCameraOption.h"
#include <QLoggingCategory>

typedef struct ParsedFact {
    QString name;
    FactMetaData::ValueType_t type;
    FactMetaData* metaData;
} ParsedFactType;

class QGCCameraDefinitionFileHandler : public QObject
{
    Q_OBJECT
public:
    QGCCameraDefinitionFileHandler(QObject *);
    ~QGCCameraDefinitionFileHandler();

    bool parse(QByteArray&);

    // Constants read from the definition file and set on the CameraComponent
    int _version;
    QString _modelName;
    QString _vendor;

    // Everything parsed from the parameters
    QList<QGCCameraOptionRange*> _optionRanges;
    QStringList _settings;
    QMap<QString, QStringList> _requestUpdates;
    QMap<QString, QStringList> _originalOptNames;
    QMap<QString, QVariantList> _originalOptValues;
    QList<QGCCameraOptionExclusion*> _valueExclusions;

    QList<ParsedFact> _parsedFacts;


signals:
    void constantsUpdated();
    void factsUpdated();

protected:
    bool _handleLocalization(QByteArray&);
    bool _replaceLocaleStrings(QDomNode, QByteArray&);
    bool _loadConstants(const QDomNodeList nodeList);
    bool _loadSettings(const QDomNodeList nodeList);
    bool _loadNameValue(QDomNode, const QString factName, FactMetaData* metaData, QString& optName, QString& optValue, QVariant& optVariant);
    QStringList _loadExclusions(QDomNode option);
    QStringList _loadUpdates(QDomNode option);
    bool _loadRanges(QDomNode option, const QString factName, QString paramValue);
};

#endif // QGCCAMERADEFINITIONFILEHANDLER_H
