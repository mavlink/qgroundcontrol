/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of class MAVLinkXMLParserV10
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include <QFile>
#include <QDir>
#include <QPair>
#include <QList>
#include <QMap>
#include <QDateTime>
#include <QLocale>
#include <QApplication>

#include "MAVLinkXMLParserV10.h"

#include <QDebug>

MAVLinkXMLParserV10::MAVLinkXMLParserV10(QDomDocument* document, QString outputDirectory, QObject* parent) : QObject(parent),
doc(document),
outputDirName(outputDirectory),
fileName("")
{
}

MAVLinkXMLParserV10::MAVLinkXMLParserV10(QString document, QString outputDirectory, QObject* parent) : QObject(parent)
{
    doc = new QDomDocument();
    QFile file(document);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString instanceText(QString::fromUtf8(file.readAll()));
        doc->setContent(instanceText);
    }
    fileName = document;
    outputDirName = outputDirectory;
}

MAVLinkXMLParserV10::~MAVLinkXMLParserV10()
{
}

void MAVLinkXMLParserV10::processError(QProcess::ProcessError err)
{
    switch(err)
    {
    case QProcess::FailedToStart:
        emit parseState(tr("Generator failed to start. Please check if the path and command is correct."));
        break;
    case QProcess::Crashed:
        emit parseState("Generator crashed, This is a generator-related problem. Please upgrade MAVLink generator.");
        break;
    case QProcess::Timedout:
        emit parseState(tr("Generator start timed out, please check if the path and command are correct"));
        break;
    case QProcess::WriteError:
        emit parseState(tr("Could not communicate with generator. Please check if the path and command are correct"));
        break;
    case QProcess::ReadError:
        emit parseState(tr("Could not communicate with generator. Please check if the path and command are correct"));
        break;
    case QProcess::UnknownError:
    default:
        emit parseState(tr("Generator error. Please check if the path and command is correct."));
        break;
    }
}

/**
 * Generate C-code (C-89 compliant) out of the XML protocol specs.
 */
bool MAVLinkXMLParserV10::generate()
{
#ifdef Q_OS_WIN
    QString generatorCall("%1/files/mavlink_generator/generator/mavgen.exe");
#endif
#if (defined Q_OS_MAC) || (defined Q_OS_LINUX)
    QString generatorCall("python");
#endif
    QString lang("C");
    QString version("1.0");

    QStringList arguments;
#if (defined Q_OS_MAC) || (defined Q_OS_LINUX)
    // Script is only needed as argument if Python is used, the Py2Exe implicitely knows the script
    arguments << QString("%1/files/mavlink_generator/generator/mavgen.py").arg(QApplication::applicationDirPath());
#endif
    arguments << QString("--lang=%1").arg(lang);
    arguments << QString("--output=%2").arg(outputDirName);
    arguments << QString("%3").arg(fileName);
    arguments << QString("--wire-protocol=%4").arg(version);

    qDebug() << "Attempted to start" << generatorCall << arguments;
    process = new QProcess(this);
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
    bool result = (process->execute(generatorCall, arguments) == 0);
    // Print process status
    emit parseState(QString("<font color=\"red\">%1</font>").arg(QString(process->readAllStandardError())));
    emit parseState(QString(process->readAllStandardOutput()));
    return result;
}

///**
// * Generate C-code (C-89 compliant) out of the XML protocol specs.
// */
//bool MAVLinkXMLParserV10::generate()
//{
//    uint16_t crc_key = X25_INIT_CRC;
//    // Process result
//    bool success = true;

//    // Only generate if output dir is correctly set
//    if (outputDirName == "")
//    {
//        emit parseState(tr("<font color=\"red\">ERROR: No output directory given.\nAbort.</font>"));
//        return false;
//    }

//    QString topLevelOutputDirName = outputDirName;

//    // print out the element names of all elements that are direct children
//    // of the outermost element.
//    QDomElement docElem = doc->documentElement();
//    QDomNode n = docElem;//.firstChild();
//    QDomNode p = docElem;

//    // Sanity check variables
//    QList<int>* usedMessageIDs = new QList<int>();
//    QMap<QString, QString>* usedMessageNames = new QMap<QString, QString>();
//    QMap<QString, QString>* usedEnumNames = new QMap<QString, QString>();

//    QList< QPair<QString, QString> > cFiles;
//    QString lcmStructDefs = "";

//    QString pureFileName;
//    QString pureIncludeFileName;

//    QFileInfo fInfo(this->fileName);
//    pureFileName = fInfo.baseName().split(".", QString::SkipEmptyParts).first();

//    // XML parsed and converted to C code. Now generating the files
//    outputDirName += QDir::separator() + pureFileName;
//    QDateTime now = QDateTime::currentDateTime().toUTC();
//    QLocale loc(QLocale::English);
//    QString dateFormat = "dddd, MMMM d yyyy, hh:mm UTC";
//    QString date = loc.toString(now, dateFormat);
//    QString includeLine = "#include \"%1\"\n";
//    QString mainHeaderName = pureFileName + ".h";
//    QString messagesDirName = ".";//"generated";
//    QDir dir(outputDirName + "/" + messagesDirName);

//    int mavlinkVersion = 0;
//    static unsigned message_lengths[256];
//    static unsigned message_key[256];
//    static int highest_message_id;
//    static int recursion_level;

//    if (recursion_level == 0) {
//        highest_message_id = 0;
//        memset(message_lengths, 0, sizeof(message_lengths));
//    }


//    // Start main header
//    QString mainHeader = QString("/** @file\n *\t@brief MAVLink comm protocol.\n *\t@see http://qgroundcontrol.org/mavlink/\n *\t Generated on %1\n */\n#ifndef " + pureFileName.toUpper() + "_H\n#define " + pureFileName.toUpper() + "_H\n\n").arg(date); // The main header includes all messages
//    // Mark all code as C code
//    mainHeader += "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";
//    mainHeader += "\n#include \"../mavlink_protocol.h\"\n";
//    mainHeader += "\n#define MAVLINK_ENABLED_" + pureFileName.toUpper() + "\n\n";

//    QString enums;


//    // Run through root children
//    while(!n.isNull())
//    {
//        // Each child is a message
//        QDomElement e = n.toElement(); // try to convert the node to an element.
//        if(!e.isNull())
//        {
//            if (e.tagName() == "mavlink")
//            {
//                p = n;
//                n = n.firstChild();
//                while (!n.isNull())
//                {
//                    e = n.toElement();
//                    if (!e.isNull())
//                    {
//                        // Handle all include tags
//                        if (e.tagName() == "include")
//                        {
//                            QString incFileName = e.text();
//                            // Load file
//                            //QDomDocument includeDoc = QDomDocument();

//                            // Prepend file path if it is a relative path and
//                            // make it relative to opened file
//                            QFileInfo fInfo(incFileName);

//                            QString incFilePath;
//                            if (fInfo.isRelative())
//                            {
//                                QFileInfo rInfo(this->fileName);
//                                incFilePath = rInfo.absoluteDir().canonicalPath() + "/" + incFileName;
//                                pureIncludeFileName = fInfo.baseName().split(".", QString::SkipEmptyParts).first();
//                            }

//                            QFile file(incFilePath);
//                            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
//                            {
//                                emit parseState(QString("<font color=\"green\">Included messages from file: %1</font>").arg(incFileName));
//                                // NEW MODE: CREATE INDIVIDUAL FOLDERS
//                                // Create new output directory, parse included XML and generate C-code
//                                MAVLinkXMLParserV10 includeParser(incFilePath, topLevelOutputDirName, this);
//                                connect(&includeParser, SIGNAL(parseState(QString)), this, SIGNAL(parseState(QString)));
//                                // Generate and write
//                                includeParser.generate();
//                                mainHeader += "\n#include \"../" + pureIncludeFileName + "/" + pureIncludeFileName + ".h\"\n";

//                                emit parseState(QString("<font color=\"green\">End of inclusion from file: %1</font>").arg(incFileName));
//                            }
//                            else
//                            {
//                                // Include file could not be opened
//                                emit parseState(QString("<font color=\"red\">ERROR: Failed including file: %1, file is not readable. Wrong/misspelled filename?\nAbort.</font>").arg(fileName));
//                                return false;
//                            }

//                        }
//                        // Handle all enum tags
//                        else if (e.tagName() == "version")
//                        {
//                            //QString fieldType = e.attribute("type", "");
//                            //QString fieldName = e.attribute("name", "");
//                            QString fieldText = e.text();

//                            // Check if version has been previously set
//                            if (mavlinkVersion != 0)
//                            {
//                                emit parseState(QString("<font color=\"red\">ERROR: Protocol version tag set twice, please use it only once. First version was %1, second version is %2.\nAbort.</font>").arg(mavlinkVersion).arg(fieldText));
//                                return false;
//                            }

//                            bool ok;
//                            int version = fieldText.toInt(&ok);
//                            if (ok && (version > 0) && (version < 256))
//                            {
//                                // Set MAVLink version
//                                mavlinkVersion = version;
//                            }
//                            else
//                            {
//                                emit parseState(QString("<font color=\"red\">ERROR: Reading version string failed: %1, string is not an integer number between 1 and 255.\nAbort.</font>").arg(fieldText));
//                                return false;
//                            }
//                        }
//                        // Handle all enum tags
//                        else if (e.tagName() == "enums")
//                        {
//                            // One down into the enums list
//                            p = n;
//                            n = n.firstChild();
//                            while (!n.isNull())
//                            {
//                                e = n.toElement();

//                                QString currEnum;
//                                QString currEnumEnd;
//                                // Comment
//                                QString comment;

//                                if(!e.isNull() && e.tagName() == "enum")
//                                {
//                                    // Get enum name
//                                    QString enumName = e.attribute("name", "").toLower();
//                                    if (enumName.size() == 0)
//                                    {
//                                        emit parseState(tr("<font color=\"red\">ERROR: Missing required name=\"\" attribute for tag %2 near line %1\nAbort.</font>").arg(QString::number(e.lineNumber()), e.tagName()));
//                                        return false;
//                                    }
//                                    else
//                                    {
//                                        // Sanity check: Accept only enum names not used previously
//                                        if (usedEnumNames->contains(enumName))
//                                        {
//                                            emit parseState(tr("<font color=\"red\">ERROR: Enum name %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(enumName, QString::number(e.lineNumber()), fileName));
//                                            return false;
//                                        }
//                                        else
//                                        {
//                                            usedEnumNames->insert(enumName, QString::number(e.lineNumber()));
//                                        }

//                                        // Everything sane, starting with enum content
//                                        currEnum = "enum " + enumName.toUpper() + "\n{\n";
//                                        currEnumEnd = QString("\t%1_ENUM_END\n};\n\n").arg(enumName.toUpper());

//                                        int nextEnumValue = 0;

//                                        // Get the enum fields
//                                        QDomNode f = e.firstChild();
//                                        while (!f.isNull())
//                                        {
//                                            QDomElement e2 = f.toElement();
//                                            if (!e2.isNull() && e2.tagName() == "entry")
//                                            {
//                                                QString fieldValue = e2.attribute("value", "");

//                                                // If value was given, use it, if not, use the enum iterator
//                                                // value. The iterator value gets reset by manual values

//                                                QString fieldName = e2.attribute("name", "");
//                                                if (fieldValue.length() == 0)
//                                                {
//                                                    fieldValue = QString::number(nextEnumValue);
//                                                    nextEnumValue++;
//                                                }
//                                                else
//                                                {
//                                                    bool ok;
//                                                    nextEnumValue = fieldValue.toInt(&ok) + 1;
//                                                    if (!ok)
//                                                    {
//                                                        emit parseState(tr("<font color=\"red\">ERROR: Enum entry %1 has not a valid number (%2) in the value field.\nAbort.</font>").arg(fieldName, fieldValue));
//                                                        return false;
//                                                    }
//                                                }

//                                                // Add comment of field if there is one
//                                                QString fieldComment;
//                                                if (e2.text().length() > 0)
//                                                {
//                                                    QString sep(" | ");
//                                                    QDomNode pp = e2.firstChild();
//                                                    while (!pp.isNull())
//                                                    {
//                                                        QDomElement pp2 = pp.toElement();
//                                                        if (pp2.isText() || pp2.isCDATASection())
//                                                        {
//                                                            // If this is the only field, don't add the separator
//                                                            if (pp.nextSibling().isNull())
//                                                            {
//                                                                fieldComment +=  pp2.nodeValue();
//                                                            }
//                                                            else
//                                                            {
//                                                                fieldComment +=  pp2.nodeValue() + sep;
//                                                            }
//                                                        }
//                                                        else if (pp2.isElement())
//                                                        {
//                                                            // If this is the only field, don't add the separator
//                                                            if (pp.nextSibling().isNull())
//                                                            {
//                                                                fieldComment += pp2.text();
//                                                            }
//                                                            else
//                                                            {
//                                                                fieldComment += pp2.text() + sep;
//                                                            }
//                                                        }
//                                                        pp = pp.nextSibling();
//                                                    }
//                                                    fieldComment = fieldComment.replace("\n", " ");
//                                                    fieldComment = " /* " + fieldComment.simplified() + " */";
//                                                }
//                                                currEnum += "\t" + fieldName.toUpper() + "=" + fieldValue + "," + fieldComment + "\n";
//                                            }
//                                            else if(!e2.isNull() && e2.tagName() == "description")
//                                            {
//                                                comment = " " + e2.text().replace("\n", " ") + comment;
//                                            }
//                                            f = f.nextSibling();
//                                        }
//                                    }
//                                    // Add the last parsed enum
//                                    // Remove the last comma, as the last value has none
//                                    // ENUM END MARKER IS LAST ENTRY, COMMA REMOVAL NOT NEEDED
//                                    //int commaPosition = currEnum.lastIndexOf(",");
//                                    //currEnum.remove(commaPosition, 1);

//                                    enums += "/** @brief " + comment.simplified() + " */\n" + currEnum + currEnumEnd;
//                                } // Element is non-zero and element name is <enum>
//                                n = n.nextSibling();
//                            } // While through <enums>
//                            // One up, back into the <mavlink> structure
//                            n = p;
//                        }

//                        // Handle all message tags
//                        else if (e.tagName() == "messages")
//                        {
//                            p = n;
//                            n = n.firstChild();
//                            while (!n.isNull())
//                            {
//                                e = n.toElement();
//                                if(!e.isNull())
//                                {
//                                    //if (e.isNull()) continue;
//                                    // Get message name
//                                    QString messageName = e.attribute("name", "").toLower();
//                                    if (messageName.size() == 0)
//                                    {
//                                        emit parseState(tr("<font color=\"red\">ERROR: Missing required name=\"\" attribute for tag %2 near line %1\nAbort.</font>").arg(QString::number(e.lineNumber()), e.tagName()));
//                                        return false;
//                                    }
//                                    else
//                                    {
//                                        // Get message id
//                                        bool ok;
//                                        int messageId = e.attribute("id", "-1").toInt(&ok, 10);
////                                        emit parseState(tr("Compiling message <strong>%1 \t(#%3)</strong> \tnear line %2").arg(messageName, QString::number(n.lineNumber()), QString::number(messageId)));

//                                        // Sanity check: Accept only message IDs not used previously
//                                        if (usedMessageIDs->contains(messageId))
//                                        {
//                                            emit parseState(tr("<font color=\"red\">ERROR: Message ID %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(QString::number(messageId), QString::number(e.lineNumber()), fileName));
//                                            return false;
//                                        }
//                                        else
//                                        {
//                                            usedMessageIDs->append(messageId);
//                                        }

//                                        // Sanity check: Accept only message names not used previously
//                                        if (usedMessageNames->contains(messageName))
//                                        {
//                                            emit parseState(tr("<font color=\"red\">ERROR: Message name %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(messageName, QString::number(e.lineNumber()), fileName));
//                                            return false;
//                                        }
//                                        else
//                                        {
//                                            usedMessageNames->insert(messageName, QString::number(e.lineNumber()));
//                                        }

//                                        QString channelType("mavlink_channel_t");
//                                        QString messageType("mavlink_message_t");
//                                        QString headerType("mavlink_header_t");

//                                        // Build up function call
//                                        QString commentContainer("/**\n * @brief Pack a %1 message\n * @param system_id ID of this system\n * @param component_id ID of this component (e.g. 200 for IMU)\n * @param msg The MAVLink message to compress the data into\n *\n%2 * @return length of the message in bytes (excluding serial stream start sign)\n */\n");
//                                        QString commentPackChanContainer("/**\n * @brief Pack a %1 message\n * @param system_id ID of this system\n * @param component_id ID of this component (e.g. 200 for IMU)\n * @param chan The MAVLink channel this message was sent over\n * @param msg The MAVLink message to compress the data into\n%2 * @return length of the message in bytes (excluding serial stream start sign)\n */\n");
//                                        QString commentSendContainer("/**\n * @brief Send a %1 message\n * @param chan MAVLink channel to send the message\n *\n%2 */\n");
//                                        QString commentEncodeContainer("/**\n * @brief Encode a %1 struct into a message\n *\n * @param system_id ID of this system\n * @param component_id ID of this component (e.g. 200 for IMU)\n * @param msg The MAVLink message to compress the data into\n * @param %1 C-struct to read the message contents from\n */\n");
//                                        QString commentDecodeContainer("/**\n * @brief Decode a %1 message into a struct\n *\n * @param msg The message to decode\n * @param %1 C-struct to decode the message contents into\n */\n");
//                                        QString commentEntry(" * @param %1 %2\n");
//                                        QString idDefine = QString("#define MAVLINK_MSG_ID_%1 %2\n#define MAVLINK_MSG_ID_%1_LEN %3\n#define MAVLINK_MSG_%2_LEN %3\n#define MAVLINK_MSG_ID_%1_KEY 0x%4\n#define MAVLINK_MSG_%2_KEY 0x%4");
//                                        QString arrayDefines;
//                                        QString cStructName = QString("mavlink_%1_t").arg(messageName);
//                                        QString cStruct("typedef struct __%1 \n{\n%2\n} %1;");
//                                        QString cStructLines;
//                                        QString encode("static inline uint16_t mavlink_msg_%1_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const %2* %1)\n{\n\treturn mavlink_msg_%1_pack(%3);\n}\n");

//                                                                                //                                        QString decode("static inline void mavlink_msg_%1_decode(const mavlink_message_t* msg, %2* %1)\n{\n%3}\n");
//                                        QString decode("static inline void mavlink_msg_%1_decode(const mavlink_message_t* msg, %2* %1)\n{\n\tmemcpy( %1, msg->payload, sizeof(%2));\n}\n");
//                                                                                //                                        QString pack("static inline uint16_t mavlink_msg_%1_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg%2)\n{\n\tuint16_t i = 0;\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\n%4\n\treturn mavlink_finalize_message(msg, system_id, component_id, i);\n}\n\n");
//                                        QString pack("static inline uint16_t mavlink_msg_%1_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg%2)\n{\n\tmavlink_%1_t *p = (mavlink_%1_t *)&msg->payload[0];\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\n%4\n\treturn mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_%3_LEN);\n}\n\n");
//                                                                                //                                        QString packChan("static inline uint16_t mavlink_msg_%1_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg%2)\n{\n\tuint16_t i = 0;\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\n%4\n\treturn mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);\n}\n\n");
//                                        QString packChan("static inline uint16_t mavlink_msg_%1_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg%2)\n{\n\tmavlink_%1_t *p = (mavlink_%1_t *)&msg->payload[0];\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\n%4\n\treturn mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_%3_LEN);\n}\n\n");
//                                        //QString compactStructSend = "#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\n\nstatic inline void mavlink_msg_%3_struct_send(%1 chan%5)\n{\n\t%2 msg;\n\tmavlink_msg_%3_encode(mavlink_system.sysid, mavlink_system.compid, &msg%4);\n\tmavlink_send_uart(chan, &msg);\n}\n\n#endif";
//                                        QString unpacking;
//                                        QString prepends;
//                                        QString packParameters;
//                                        QString packArguments("system_id, component_id, msg");
//                                        QString packLines;
//                                        QString decodeLines;
//                                        QString sendArguments;
//                                        QString commentLines;
//                                        int calculatedLength = 0;
//                                        unsigned message_length = 0;


//                                        // Get the message fields
//                                        QDomNode f = e.firstChild();

//                                        // The field types and order are hashed with a checksum

//                                        // Initialize CRC
//                                        uint16_t fieldHash;
//                                        crcInit(&fieldHash);

//                                        while (!f.isNull())
//                                        {
//                                            QDomElement e2 = f.toElement();
//                                            if (!e2.isNull() && e2.tagName() == "field")
//                                            {
//                                                QString fieldType = e2.attribute("type", "");
//                                                QString fieldName = e2.attribute("name", "");
//                                                QString fieldOffset = e2.attribute("offset", "");
//                                                QString fieldText = e2.text();

//                                                QString unpackingCode;
//                                                QString unpackingComment = QString("/**\n * @brief Get field %1 from %2 message\n *\n * @return %3\n */\n").arg(fieldName, messageName, fieldText);

//                                                // Send arguments do not work for the version field
//                                                if (!fieldType.contains("uint8_t_mavlink_version"))
//                                                {
//                                                    // Send arguments are the same for integral types and arrays
//                                                    sendArguments += ", " + fieldName;
//                                                    commentLines += commentEntry.arg(fieldName, fieldText.replace("\n", " "));
//                                                }

//                                                // MAVLink version field
//                                                // this is a special field always containing the version define
//                                                if (fieldType.contains("uint8_t_mavlink_version"))
//                                                {
//                                                    // Add field to C structure
//                                                    cStructLines += QString("\t%1 %2;\t///< %3\n").arg("uint8_t", fieldName, fieldText);
//                                                    calculatedLength += 1;
//                                                    // Add pack line to message_xx_pack function
//                                                                                                        //                                                    packLines += QString("\ti += put_uint8_t_by_index(%1, i, msg->payload); // %2\n").arg(mavlinkVersion).arg(fieldText);
//                                                    packLines += QString("\n\tp->%2 = MAVLINK_VERSION;\t// %1:%3").arg(fieldType, fieldName, e2.text());
//                                                    // Add decode function for this type
//                                                    decodeLines += QString("\n\t%1->%2 = mavlink_msg_%1_get_%2(msg);").arg(messageName, fieldName);

//                                                    if (fieldOffset != "") { // does not use the number - always moves up one slot
//                                                        QStringList itemList;
//                                                        // Swap field in C structure
//                                                        itemList = cStructLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        cStructLines = itemList.join("\n") + "\n";

//                                                        // Swap line in message_xx_pack function
//                                                        itemList = packLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        packLines = itemList.join("\n") + "\n";

//                                                        // Swap line in decode function for this type
//                                                        itemList = decodeLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        decodeLines = itemList.join("\n") + "\n";
//                                                    }
//                                                }

//                                                // ARRAYS are not longer supported - leave error message in here to inform users!
//                                                else if (fieldType.startsWith("array"))
//                                                {
//                                                    emit parseState(tr("<font color=\"red\">ERROR: In message %1 deprecated type <array> used near line %2 of file %3. Please change from array[size] to uint8_t[size] to get the same behaviour.\nAbort.</font>").arg(messageName, QString::number(e.lineNumber()), fileName));
//                                                    return false;
//                                                }
//                                                else if (fieldType.startsWith("string"))
//                                                {
//                                                    int arrayLength = QString(fieldType.split("[").at(1).split("]").first()).toInt();
//                                                    // String array is always unsigned char, so bytes
//                                                    calculatedLength += arrayLength;
//                                                    packParameters += QString(", const ") + QString("char*") + " " + fieldName;
//                                                    packArguments += ", " + messageName + "->" + fieldName;

//                                                    // Add field to C structure
//                                                    cStructLines += QString("\t%1 %2[%3];\t///< %4\n").arg("char", fieldName, QString::number(arrayLength), fieldText);
//                                                    // Add pack line to message_xx_pack function
//                                                                                                        //                                                    packLines += QString("\ti += put_%1_by_index(%2, %3, i, msg->payload); // %4\n").arg(arrayType, fieldName, QString::number(arrayLength), e2.text());
//                                                    packLines += QString("\tstrncpy( p->%2, %2, sizeof(p->%2));\t// %1[%3]:%4\n").arg("char", fieldName, QString::number(arrayLength), fieldText);
//                                                    // Add decode function for this type
//                                                    decodeLines += QString("\n\tmavlink_msg_%1_get_%2(msg, %1->%2);\n").arg(messageName, fieldName);
//                                                    arrayDefines += QString("#define MAVLINK_MSG_%1_FIELD_%2_LEN %3\n").arg(messageName.toUpper(), fieldName.toUpper(), QString::number(arrayLength));

//                                                    if (fieldOffset != "")
//                                                    { // does not use the number - always moves up one slot
//                                                        QStringList itemList;
//                                                        // Swap field in C structure
//                                                        itemList = cStructLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        cStructLines = itemList.join("\n") + "\n";

//                                                        // Swap line in message_xx_pack function
//                                                        itemList = packLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        packLines = itemList.join("\n") + "\n";

//                                                        // Swap line in decode function for this type
//                                                        itemList = decodeLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        decodeLines = itemList.join("\n") + "\n";
//                                                    }
//                                                }
//                                                // Expand array handling to all valid mavlink data types
//                                                else if(fieldType.contains('[') && fieldType.contains(']'))
//                                                {
//                                                    int arrayLength = QString(fieldType.split("[").at(1).split("]").first()).toInt();
//                                                    QString arrayType = fieldType.split("[").first();
//                                                    if (arrayType.contains("array"))
//                                                    {
//                                                        calculatedLength += arrayLength;
//                                                    }
//                                                    else if (arrayType.contains("char"))
//                                                    {
//                                                        calculatedLength += arrayLength;
//                                                    }
//                                                    else if (arrayType.contains("int8"))
//                                                    {
//                                                        calculatedLength += arrayLength;
//                                                    }
//                                                    else if (arrayType.contains("int16"))
//                                                    {
//                                                        calculatedLength += arrayLength*2;
//                                                    }
//                                                    else if (arrayType.contains("int32"))
//                                                    {
//                                                        calculatedLength += arrayLength*4;
//                                                    }
//                                                    else if (arrayType.contains("int64"))
//                                                    {
//                                                        calculatedLength += arrayLength*8;
//                                                    }
//                                                    else if (arrayType == "float")
//                                                    {
//                                                        calculatedLength += arrayLength*4;
//                                                    }
//                                                    else
//                                                    {
//                                                        emit parseState(tr("<font color=\"red\">ERROR: In message %1 invalid array type %4 used near line %2 of file %3\nAbort.</font>").arg(messageName, QString::number(e.lineNumber()), fileName, arrayType));
//                                                        return false;
//                                                    }
//                                                    packParameters += QString(", const ") + arrayType + "* " + fieldName;
//                                                    packArguments += ", " + messageName + "->" + fieldName;

//                                                    // Add field to C structure
//                                                    cStructLines += QString("\t%1 %2[%3];\t///< %4\n").arg(arrayType, fieldName, QString::number(arrayLength), fieldText);
//                                                    // Add pack line to message_xx_pack function
//                                                                                                        //                                                    packLines += QString("\ti += put_array_by_index((const int8_t*)%1, sizeof(%2)*%3, i, msg->payload); // %4\n").arg(fieldName, arrayType, QString::number(arrayLength), fieldText);
//                                                    packLines += QString("\tmemcpy(p->%2, %2, sizeof(p->%2));\t// %1[%3]:%4\n").arg(arrayType, fieldName, QString::number(arrayLength), fieldText);
//                                                    // Add decode function for this type
//                                                    decodeLines += QString("\n\tmavlink_msg_%1_get_%2(msg, %1->%2);").arg(messageName, fieldName);

//                                                    if (fieldOffset != "")
//                                                    { // does not use the number - always moves up one slot
//                                                        QStringList itemList;
//                                                        // Swap field in C structure
//                                                        itemList = cStructLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        cStructLines = itemList.join("\n") + "\n";

//                                                        // Swap line in message_xx_pack function
//                                                        itemList = packLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        packLines = itemList.join("\n") + "\n";

//                                                        // Swap line in decode function for this type
//                                                        itemList = decodeLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        decodeLines = itemList.join("\n") + "\n";
//                                                    }

//                                                    arrayDefines += QString("#define MAVLINK_MSG_%1_FIELD_%2_LEN %3\n").arg(messageName.toUpper(), fieldName.toUpper(), QString::number(arrayLength));

//                                                                                                        //                                                    unpackingCode = QString("\n\tmemcpy(r_data, msg->payload%1, sizeof(%2)*%3);\n\treturn sizeof(%2)*%3;").arg(prepends, arrayType, QString::number(arrayLength));
//                                                    unpackingCode = QString("\n\tmemcpy(%1, p->%1, sizeof(p->%1));\n\treturn sizeof(p->%1);").arg(fieldName);

//                                                                                                        //                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, %3* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, arrayType, unpackingCode);
//                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, %3* %2)\n{\n\tmavlink_%1_t *p = (mavlink_%1_t *)&msg->payload[0];\n%4\n}\n\n").arg(messageName, fieldName, arrayType, unpackingCode);
//                                                    //                                                    decodeLines += "";
//                                                    prepends += QString("+sizeof(%1)*%2").arg(arrayType, QString::number(arrayLength));

//                                                }
//                                                else
//                                                    // Handle simple types like integers and floats
//                                                {
//                                                    packParameters += ", " + fieldType + " " + fieldName;
//                                                    packArguments += ", " + messageName + "->" + fieldName;

//                                                    // Add field to C structure
//                                                    cStructLines += QString("\t%1 %2;\t///< %3\n").arg(fieldType, fieldName, fieldText);
//                                                    // Add pack line to message_xx_pack function
//                                                                                                        //                                                     packLines += QString("\ti += put_%1_by_index(%2, i, msg->payload); // %3\n").arg(fieldType, fieldName, e2.text());
//                                                    packLines += QString("\tp->%2 = %2;\t// %1:%3\n").arg(fieldType, fieldName, e2.text());
//                                                    // Add decode function for this type
//                                                                                                        //                                                     decodeLines += QString("\t%1->%2 = mavlink_msg_%1_get_%2(msg);\n").arg(messageName, fieldName);
//                                                    decodeLines += QString("\n\t%1 = p->%1;").arg(fieldName);

//                                                    if (fieldOffset != "") { // does not use the number - always moves up one slot
//                                                        QStringList itemList;
//                                                        // Swap field in C structure
//                                                        itemList = cStructLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        cStructLines = itemList.join("\n") + "\n";

//                                                        // Swap line in message_xx_pack function
//                                                        itemList = packLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        packLines = itemList.join("\n") + "\n";

//                                                        // Swap line in decode function for this type
//                                                        itemList = decodeLines.split("\n", QString::SkipEmptyParts);
//                                                        if (itemList.size() > 1) itemList.swap(itemList.size() - 1, itemList.size() - 2);
//                                                        decodeLines = itemList.join("\n") + "\n";
//                                                    }
//                                                    if (fieldType.contains("char"))
//                                                    {
//                                                        calculatedLength += 1;
//                                                    }
//                                                    else if (fieldType.contains("int8"))
//                                                    {
//                                                        calculatedLength += 1;
//                                                    }
//                                                    else if (fieldType.contains("int16"))
//                                                    {
//                                                        calculatedLength += 2;
//                                                    }
//                                                    else if (fieldType.contains("int32"))
//                                                    {
//                                                        calculatedLength += 4;
//                                                    }
//                                                    else if (fieldType.contains("int64"))
//                                                    {
//                                                        calculatedLength += 8;
//                                                    }
//                                                    else if (fieldType == "float")
//                                                    {
//                                                        calculatedLength += 4;
//                                                    }
//                                                    else
//                                                    {
//                                                        emit parseState(tr("<font color=\"red\">ERROR: In message %1 inavlid type %4 used near line %2 of file %3\nAbort.</font>").arg(messageName, QString::number(e.lineNumber()), fileName, fieldType));
//                                                        return false;
//                                                    }
//                                                }

//                                                // message length calculation
//                                                unsigned element_multiplier = 1;
//                                                unsigned element_length = 0;

//                                                if (fieldType.contains("[")) {
//                                                    element_multiplier = fieldType.split("[").at(1).split("]").first().toInt();
//                                                }
//                                                for (unsigned i=0; i<sizeof(length_map)/sizeof(length_map[0]); i++)
//                                                {
//                                                    if (fieldType.startsWith(length_map[i].prefix)) {
//                                                        element_length = length_map[i].length * element_multiplier;
//                                                        break;
//                                                    }
//                                                }
//                                                if (element_length == 0) {
//                                                    emit parseState(tr("<font color=\"red\">ERROR: Unable to calculate length for %2 near line %1\nAbort.</font>").arg(QString::number(e.lineNumber()), fieldType));
//                                                    return false;
//                                                }
//                                                message_length += element_length;


//                                                //
//                                                //                                                QString unpackingCode;

//                                                if (fieldType == "uint8_t_mavlink_version")
//                                                {
//                                                                                                        //                                                    unpackingCode = QString("\treturn (%1)(msg->payload%2)[0];").arg("uint8_t", prepends);
//                                                    unpackingCode = QString("\treturn (%1)(p->%2);").arg("uint8_t", fieldName);
//                                                }
//                                                else if (fieldType == "uint8_t" || fieldType == "int8_t")
//                                                {
//                                                                                                        //                                                    unpackingCode = QString("\treturn (%1)(msg->payload%2)[0];").arg(fieldType, prepends);
//                                                    unpackingCode = QString("\treturn (%1)(p->%2);").arg(fieldType, fieldName);
//                                                }
//                                                else if (fieldType == "uint16_t" || fieldType == "int16_t")
//                                                {
//                                                                                                        //                                                    unpackingCode = QString("\tgeneric_16bit r;\n\tr.b[1] = (msg->payload%1)[0];\n\tr.b[0] = (msg->payload%1)[1];\n\treturn (%2)r.s;").arg(prepends).arg(fieldType);
//                                                    unpackingCode = QString("\treturn (%1)(p->%2);").arg(fieldType, fieldName);
//                                                }
//                                                else if (fieldType == "uint32_t" || fieldType == "int32_t")
//                                                {
//                                                                                                        //                                                    unpackingCode = QString("\tgeneric_32bit r;\n\tr.b[3] = (msg->payload%1)[0];\n\tr.b[2] = (msg->payload%1)[1];\n\tr.b[1] = (msg->payload%1)[2];\n\tr.b[0] = (msg->payload%1)[3];\n\treturn (%2)r.i;").arg(prepends).arg(fieldType);
//                                                    unpackingCode = QString("\treturn (%1)(p->%2);").arg(fieldType, fieldName);
//                                                }
//                                                else if (fieldType == "float")
//                                                {
//                                                                                                        //                                                    unpackingCode = QString("\tgeneric_32bit r;\n\tr.b[3] = (msg->payload%1)[0];\n\tr.b[2] = (msg->payload%1)[1];\n\tr.b[1] = (msg->payload%1)[2];\n\tr.b[0] = (msg->payload%1)[3];\n\treturn (%2)r.f;").arg(prepends).arg(fieldType);
//                                                    unpackingCode = QString("\treturn (%1)(p->%2);").arg(fieldType, fieldName);
//                                                }
//                                                else if (fieldType == "uint64_t" || fieldType == "int64_t")
//                                                {
//                                                                                                        //                                                    unpackingCode = QString("\tgeneric_64bit r;\n\tr.b[7] = (msg->payload%1)[0];\n\tr.b[6] = (msg->payload%1)[1];\n\tr.b[5] = (msg->payload%1)[2];\n\tr.b[4] = (msg->payload%1)[3];\n\tr.b[3] = (msg->payload%1)[4];\n\tr.b[2] = (msg->payload%1)[5];\n\tr.b[1] = (msg->payload%1)[6];\n\tr.b[0] = (msg->payload%1)[7];\n\treturn (%2)r.ll;").arg(prepends).arg(fieldType);
//                                                    unpackingCode = QString("\treturn (%1)(p->%2);").arg(fieldType, fieldName);
//                                                }
//                                                else if (fieldType.startsWith("array"))
//                                                {  // fieldtype formatis string[n] where n is the number of bytes, extract n from field type string
//                                                                                                        //                                                    unpackingCode = QString("\n\tmemcpy(r_data, msg->payload%1, %2);\n\treturn %2;").arg(filedName, prepends, fieldType.split("[").at(1).split("]").first());
//                                                    unpackingCode = QString("\n\tmemcpy(%1, p->%1, sizeof(p->%1));\n\treturn sizeof(p->%1);").arg(fieldName);
//                                                }
//                                                else if (fieldType.startsWith("string"))
//                                                {  // fieldtype formatis string[n] where n is the number of bytes, extract n from field type string
//                                                                                                        //                                                    unpackingCode = QString("\n\tstrcpy(r_data, msg->payload%1, %2);\n\treturn %2;").arg(prepends, fieldType.split("[").at(1).split("]").first());
//                                                    unpackingCode = QString("\n\tstrncpy(%1, p->%1, sizeof(p->%1));\n\treturn sizeof(p->%1);").arg(fieldName);
//                                                }


//                                                // Generate the message decoding function
//                                                if (fieldType.contains("uint8_t_mavlink_version"))
//                                                {
//                                                                                                        //                                                    unpacking += unpackingComment + QString("static inline %1 mavlink_msg_%2_get_%3(const mavlink_message_t* msg)\n{\n%4\n}\n\n").arg("uint8_t", messageName, fieldName, unpackingCode);
//                                                    unpacking += unpackingComment + QString("static inline %1 mavlink_msg_%2_get_%3(const mavlink_message_t* msg)\n{\n\tmavlink_%2_t *p = (mavlink_%2_t *)&msg->payload[0];\n%4\n}\n\n").arg("uint8_t", messageName, fieldName, unpackingCode);
//                                                    decodeLines += "";
//                                                    prepends += "+sizeof(uint8_t)";
//                                                }
//                                                // Array handling is different from simple types
//                                                else if (fieldType.startsWith("array"))
//                                                {
//                                                                                                        //                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, int8_t* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, unpackingCode);
//                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, int8_t* %2)\n{\n\tmavlink_%1_t *p = (mavlink_%1_t *)&msg->payload[0];\n%3\n}\n\n").arg(messageName, fieldName, unpackingCode);
//                                                    decodeLines += "";
//                                                    QString arrayLength = QString(fieldType.split("[").at(1).split("]").first());
//                                                    prepends += "+" + arrayLength;
//                                                }
//                                                else if (fieldType.startsWith("string"))
//                                                {
//                                                                                                        //                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, char* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, unpackingCode);
//                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, char* %2)\n{\n\tmavlink_%1 *p = (mavlink_%1 *)&msg->payload[0];\n%3\n}\n\n").arg(messageName, fieldName, unpackingCode);
//                                                    decodeLines += "";
//                                                    QString arrayLength = QString(fieldType.split("[").at(1).split("]").first());
//                                                    prepends += "+" + arrayLength;
//                                                }
//                                                else if(fieldType.contains('[') && fieldType.contains(']'))
//                                                {
//                                                    // prevent this case from being caught in the following else
//                                                }
//                                                else
//                                                {
//                                                                                                        //                                                    unpacking += unpackingComment + QString("static inline %1 mavlink_msg_%2_get_%3(const mavlink_message_t* msg)\n{\n%4\n}\n\n").arg(fieldType, messageName, fieldName, unpackingCode);
//                                                    unpacking += unpackingComment + QString("static inline %1 mavlink_msg_%2_get_%3(const mavlink_message_t* msg)\n{\n\tmavlink_%2_t *p = (mavlink_%2_t *)&msg->payload[0];\n%4\n}\n\n").arg(fieldType, messageName, fieldName, unpackingCode);
//                                                    decodeLines += "";
//                                                    prepends += "+sizeof(" + e2.attribute("type", "void") + ")";
//                                                }
//                                            }
//                                            f = f.nextSibling();
//                                        }

//                                        if (messageId > highest_message_id) {
//                                            highest_message_id = messageId;
//                                        }
//                                        message_lengths[messageId] = message_length;


//                                        // Sort fields to ensure 16bit-boundary aligned data
//                                        QStringList fieldList;
//                                        // Stable sort fields in C structure
//                                        fieldList = cStructLines.split("\n", QString::SkipEmptyParts);
//                                        if (fieldList.size() > 1)
//                                        {
//                                            qStableSort(fieldList.begin(), fieldList.end(), structSort);
//                                        }

//                                        // struct now sorted, do crc calc for each field
//                                        QString fieldCRCstring;
//                                        QByteArray fieldText;
//                                        crc_key = X25_INIT_CRC;

//                                        for (int i =0; i < fieldList.size(); i++)
//                                        {
//                                            fieldCRCstring = fieldList.at(i).simplified();
//                                            fieldCRCstring = fieldCRCstring.section(" ",0,1); // note: this has one space bewteen type and name
//                                            fieldText = fieldCRCstring.toAscii();
//                                            for (int i = 0; i < fieldText.size(); ++i)
//                                            {
//                                                crcAccumulate((uint8_t) fieldText.at(i), &crc_key);
//                                            }
//                                        }

//                                        // generate the key byte value
//                                        QString stringCRC;
//                                        message_key[messageId] = (crc_key&0xff)^((crc_key>>8)&0xff);
//                                        stringCRC = stringCRC.number( message_key[messageId], 16);

//                                        // create structure
//                                        cStructLines = fieldList.join("\n") + "\n";

//                                           //                                        cStruct = cStruct.arg(cStructName, cStructLines, QString::number(calculatedLength) );
//                                        cStruct = cStruct.arg(cStructName, cStructLines );
//                                        lcmStructDefs.append("\n").append(cStruct).append("\n");
//                                        pack = pack.arg(messageName, packParameters, messageName.toUpper(), packLines);
//                                        packChan = packChan.arg(messageName, packParameters, messageName.toUpper(), packLines);
//                                        encode = encode.arg(messageName).arg(cStructName).arg(packArguments);
////                                        decode = decode.arg(messageName).arg(cStructName).arg(decodeLines);
//                                        decode = decode.arg(messageName).arg(cStructName);
////                                        QString compactSend("#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\n\nstatic inline void mavlink_msg_%3_send(%1 chan%5)\n{\n\t%2 msg;\n\tmavlink_msg_%3_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg%4);\n\tmavlink_send_uart(chan, &msg);\n}\n\n#endif");
////                                        QString compactSend("#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\nstatic inline void mavlink_msg_%3_send(%1 chan%5)\n{\n\t%2 msg;\n\tuint16_t checksum;\n\tmavlink_%3_t *p = (mavlink_%3_t *)&msg.payload[0];\n\n%6\n\tmsg.STX = MAVLINK_STX;\n\tmsg.len = MAVLINK_MSG_ID_%4_LEN;\n\tmsg.msgid = MAVLINK_MSG_ID_%4;\n");
////                                        QString compactSend2("\tmsg.sysid = mavlink_system.sysid;\n\tmsg.compid = mavlink_system.compid;\n\tmsg.seq = mavlink_get_channel_status(chan)->current_tx_seq;\n\tmavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;\n");
////                                        QString compactSend3("\tchecksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);\n\tmsg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte\n\tmsg.ck_b = (uint8_t)(checksum >> 8); ///< High byte\n\n\tmavlink_send_msg(chan, &msg);\n}\n\n#endif");
////                                        compactSend = compactSend.arg(channelType, messageType, messageName, sendArguments, packParameters );
////                                        compactSend = compactSend.arg(channelType, messageType, messageName, messageName.toUpper(), packParameters, packLines ) + compactSend2 + compactSend3;
////                                        QString compact2Send("\n\n#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL\nstatic inline void mavlink_msg_%3_send(%1 chan%5)\n{\n\t%2 hdr;\n\tmavlink_%3_t payload;\n\tuint16_t checksum;\n\tmavlink_%3_t *p = &payload;\n\n%6\n\thdr.STX = MAVLINK_STX;\n\thdr.len = MAVLINK_MSG_ID_%4_LEN;\n\thdr.msgid = MAVLINK_MSG_ID_%4;\n");
//                                        QString compact2Send0( "\n#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\n" );
//                                        QString compact2Send1("static inline void mavlink_msg_%3_send(%1 chan%5)\n{\n\t%2 hdr;\n\tmavlink_%3_t payload;\n\n\tMAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_%4_LEN )\n%6\n\thdr.STX = MAVLINK_STX;\n\thdr.len = MAVLINK_MSG_ID_%4_LEN;\n\thdr.msgid = MAVLINK_MSG_ID_%4;\n");
//                                        QString compact2Send2("\thdr.sysid = mavlink_system.sysid;\n\thdr.compid = mavlink_system.compid;\n\thdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;\n\tmavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;\n\tmavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );\n\tmavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );\n");
//                                        QString compact2Send3("\n\tcrc_init(&hdr.ck);\n\tcrc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);\n\tcrc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );\n\tcrc_accumulate( 0x%1, &hdr.ck); /// include key in X25 checksum\n\tmavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);\n\tMAVLINK_BUFFER_CHECK_END\n}\n\n#endif");
//                                        QString compact2Send = compact2Send0 + commentSendContainer.arg(messageName.toLower(), commentLines) + compact2Send1.arg(channelType, headerType, messageName, messageName.toUpper(), packParameters, packLines.replace(QString("p->"),QString("payload.")) ) + compact2Send2 + compact2Send3.arg(stringCRC.toUpper());
////                                        QString cFile = "// MESSAGE " + messageName.toUpper() + " PACKING\n\n" + idDefine.arg(messageName.toUpper(), QString::number(messageId), QString::number(calculatedLength), stringCRC.toUpper() ) + "\n\n" + cStruct + "\n" + arrayDefines + "\n" + commentContainer.arg(messageName.toLower(), commentLines) + pack + commentPackChanContainer.arg(messageName.toLower(), commentLines) + packChan + commentEncodeContainer.arg(messageName.toLower()) + encode + "\n" + commentSendContainer.arg(messageName.toLower(), commentLines) + compactSend + compact2Send + "\n" + "// MESSAGE " + messageName.toUpper() + " UNPACKING\n\n" + unpacking + commentDecodeContainer.arg(messageName.toLower()) + decode;
//                                        QString cFile = "// MESSAGE " + messageName.toUpper() + " PACKING\n\n" + idDefine.arg(messageName.toUpper(), QString::number(messageId), QString::number(calculatedLength), stringCRC.toUpper() ) + "\n\n" + cStruct + "\n" + arrayDefines + "\n" + commentContainer.arg(messageName.toLower(), commentLines) + pack + commentPackChanContainer.arg(messageName.toLower(), commentLines) + packChan + commentEncodeContainer.arg(messageName.toLower()) + encode + "\n" + compact2Send + "\n" + "// MESSAGE " + messageName.toUpper() + " UNPACKING\n\n" + unpacking + commentDecodeContainer.arg(messageName.toLower()) + decode;
//                                        cFiles.append(qMakePair(QString("mavlink_msg_%1.h").arg(messageName), cFile));

//                                        emit parseState(tr("Compiled message <strong>%1 \t(#%3)</strong> \tend near line %2, length %4, crc key 0x%5(%6)").arg(messageName, QString::number(n.lineNumber()), QString::number(messageId), QString::number(message_lengths[messageId]), stringCRC.toUpper(), QString::number(message_key[messageId])));
//                                    } // Check if tag = message
//                                } // Check if e = NULL
//                                n = n.nextSibling();
//                            } // While through <message>
//                            n = p;

//                        } // Check if tag = messages
//                    } // Check if e = NULL
//                    n = n.nextSibling();
//                } // While through include and messages
//                // One up - current node = parent
//                n = p;

//            } // Check if tag = mavlink
//        } // Check if e = NULL
//        n = n.nextSibling();
//    } // While through root children

//    // Add version to main header

//    mainHeader += "// MAVLINK VERSION\n\n";
//    mainHeader += QString("#ifndef MAVLINK_VERSION\n#define MAVLINK_VERSION %1\n#endif\n\n").arg(mavlinkVersion);
//    mainHeader += QString("#if (MAVLINK_VERSION == 0)\n#undef MAVLINK_VERSION\n#define MAVLINK_VERSION %1\n#endif\n\n").arg(mavlinkVersion);

//    // Add enums to main header

//    mainHeader += "// ENUM DEFINITIONS\n\n";
//    mainHeader += enums;
//    mainHeader += "\n";

//    mainHeader += "// MESSAGE DEFINITIONS\n\n";
//    // Create directory if it doesn't exist, report result in success
//    if (!dir.exists()) success = success && dir.mkpath(outputDirName + "/" + messagesDirName);
//    for (int i = 0; i < cFiles.size(); i++)
//    {
//        QFile rawFile(dir.filePath(cFiles.at(i).first));
//        bool ok = rawFile.open(QIODevice::WriteOnly | QIODevice::Text);
//        success = success && ok;
//        rawFile.write(cFiles.at(i).second.toLatin1());
//        rawFile.close();
//        mainHeader += includeLine.arg(messagesDirName + "/" + cFiles.at(i).first);
//    }

//    // CRC seeds
//    mainHeader += "\n\n// MESSAGE CRC KEYS\n\n";
//    mainHeader += "#undef MAVLINK_MESSAGE_KEYS\n";
//    mainHeader += "#define MAVLINK_MESSAGE_KEYS { ";
//    for (int i=0; i<highest_message_id; i++) {
//        mainHeader += QString::number(message_key[i]);
//        if (i < highest_message_id-1) mainHeader += ", ";
//    }
//    mainHeader += " }";

//    // Message lengths
//    mainHeader += "\n\n// MESSAGE LENGTHS\n\n";
//    mainHeader += "#undef MAVLINK_MESSAGE_LENGTHS\n";
//    mainHeader += "#define MAVLINK_MESSAGE_LENGTHS { ";
//    for (int i=0; i<highest_message_id; i++) {
//        mainHeader += QString::number(message_lengths[i]);
//        if (i < highest_message_id-1) mainHeader += ", ";
//    }
//    mainHeader += " }\n\n";

//    mainHeader += "#ifdef __cplusplus\n}\n#endif\n";
//    mainHeader += "#endif";
//    // Newline to make compiler happy
//    mainHeader += "\n";

//    // Write main header
//    QFile rawHeader(outputDirName + "/" + mainHeaderName);
//    bool ok = rawHeader.open(QIODevice::WriteOnly | QIODevice::Text);
//    success = success && ok;
//    rawHeader.write(mainHeader.toLatin1());
//    rawHeader.close();

//    // Write alias mavlink header
//    QFile mavlinkHeader(outputDirName + "/mavlink.h");
//    ok = mavlinkHeader.open(QIODevice::WriteOnly | QIODevice::Text);
//    success = success && ok;
//    QString mHeader = QString("/** @file\n *\t@brief MAVLink comm protocol.\n *\t@see http://pixhawk.ethz.ch/software/mavlink\n *\t Generated on %1\n */\n#ifndef MAVLINK_H\n#define MAVLINK_H\n\n").arg(date); // The main header includes all messages
//    // Mark all code as C code
//    //    mHeader += "\n#include \"" + mainHeaderName + "\"\n\n";
//    //    mHeader += "#pragma pack(push,1)\n#include mavlink_options.h\n#include \"" + mainHeaderName + "\"\n#ifdef MAVLINK_CHECK_LENGTH\n#include \"lengths.h\"\n#endif\n#pragma pack(pop)\n";
//    mHeader += "#pragma pack(push,1)\n#include \"mavlink_options.h\"\n#include \"" + mainHeaderName + "\"\n#ifdef MAVLINK_DATA\n#include \"mavlink_data.h\"\n#endif\n#pragma pack(pop)\n";
//    mHeader += "#endif\n";
//    mavlinkHeader.write(mHeader.toLatin1());
//    mavlinkHeader.close();

//    // Write C structs / lcm definitions
//    //    QFile lcmStructs(outputDirName + "/mavlink.lcm");
//    //    ok = lcmStructs.open(QIODevice::WriteOnly | QIODevice::Text);
//    //    success = success && ok;
//    //    lcmStructs.write(lcmStructDefs.toLatin1());

//    return success;
//}

///**
// *
// *  CALCULATE THE CHECKSUM
// *
// */

//#define X25_INIT_CRC 0xffff
//#define X25_VALIDATE_CRC 0xf0b8

///**
// * The checksum function adds the hash of one char at a time to the
// * 16 bit checksum (uint16_t).
// *
// * @param data new char to hash
// * @param crcAccum the already accumulated checksum
// **/
//void MAVLinkXMLParserV10::crcAccumulate(uint8_t data, uint16_t *crcAccum)
//{
//        /*Accumulate one byte of data into the CRC*/
//        uint8_t tmp;

//        tmp=data ^ (uint8_t)(*crcAccum &0xff);
//        tmp^= (tmp<<4);
//        *crcAccum = (*crcAccum>>8) ^ (tmp<<8) ^ (tmp <<3) ^ (tmp>>4);
//}

///**
// * @param crcAccum the 16 bit X.25 CRC
// */
//void MAVLinkXMLParserV10::crcInit(uint16_t* crcAccum)
//{
//        *crcAccum = X25_INIT_CRC;
//}


//const struct {
//    const char *prefix;
//    unsigned length;
//} length_map[] = {
//        { "array",  1 },
//        { "char",   1 },
//        { "uint8",  1 },
//        { "int8",   1 },
//        { "uint16", 2 },
//        { "int16",  2 },
//        { "uint32", 4 },
//        { "int32",  4 },
//        { "uint64", 8 },
//        { "int64",  8 },
//        { "float",  4 },
//        { "double", 8 },
//};

//unsigned itemLength( const QString &s1 )
//{
//    unsigned el1, i1, i2;
//    QString Ss1 = s1;

//    Ss1 = Ss1.replace("_"," ");
//    Ss1 = Ss1.simplified();
//    Ss1 = Ss1.section(" ",0,0);

//    el1 = i1 = 0;
//    i2 = sizeof(length_map)/sizeof(length_map[0]);

//    do {
//        if (Ss1.startsWith(length_map[i1].prefix))
//        {
//            el1 = length_map[i1].length;
//        }
//        i1++;
//    } while ( (el1 == 0) && (i1 < i2));
//    return el1;
//}

//bool structSort(const QString &s1, const QString &s2)
//{
//    unsigned el1, el2;

//    el1 = itemLength( s1 );
//    el2 = itemLength( s2 );
//    return el2 < el1;
//}
