#ifndef MAVLINKXMLPARSER_H
#define MAVLINKXMLPARSER_H

#include <QObject>
#include <QDomDocument>
#include <QString>

class MAVLinkXMLParser : public QObject
{
    Q_OBJECT
public:
    MAVLinkXMLParser(QDomDocument* document, QString outputDirectory, QObject* parent=0);
    MAVLinkXMLParser(QString document, QString outputDirectory, QObject* parent=0);
    ~MAVLinkXMLParser();

public slots:
    /** @brief Parse XML and generate C files */
    bool generate();

signals:
    /** @brief Status message on the parsing */
    void parseState(QString message);

protected:
    QDomDocument* doc;
    QString outputDirName;
    QString fileName;
};

#endif // MAVLINKXMLPARSER_H
