/*=====================================================================
======================================================================*/

/**
 * @file
 *   @brief Implementation of class MAVLinkXMLParser
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include <QFile>
#include <QDir>
#include <QPair>
#include <QList>
#include <QMap>
#include <QDateTime>
#include <QLocale>
#include "MAVLinkXMLParser.h"

#include <QDebug>

MAVLinkXMLParser::MAVLinkXMLParser(QDomDocument* document, QString outputDirectory, QObject* parent) : QObject(parent),
    doc(document),
    outputDirName(outputDirectory),
    fileName("")
{
}

MAVLinkXMLParser::MAVLinkXMLParser(QString document, QString outputDirectory, QObject* parent) : QObject(parent)
{
    doc = new QDomDocument();
    QFile file(document);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString instanceText(QString::fromUtf8(file.readAll()));
        doc->setContent(instanceText);
    }
    fileName = document;
    outputDirName = outputDirectory;
}

MAVLinkXMLParser::~MAVLinkXMLParser()
{
}

/**
 * Generate C-code (C-89 compliant) out of the XML protocol specs.
 */
bool MAVLinkXMLParser::generate()
{
    // Process result
    bool success = true;

    // Only generate if output dir is correctly set
    if (outputDirName == "") {
        emit parseState(tr("<font color=\"red\">ERROR: No output directory given.\nAbort.</font>"));
        return false;
    }

    QString topLevelOutputDirName = outputDirName;

    // print out the element names of all elements that are direct children
    // of the outermost element.
    QDomElement docElem = doc->documentElement();
    QDomNode n = docElem;//.firstChild();
    QDomNode p = docElem;

    // Sanity check variables
    QList<int>* usedMessageIDs = new QList<int>();
    QMap<QString, QString>* usedMessageNames = new QMap<QString, QString>();
    QMap<QString, QString>* usedEnumNames = new QMap<QString, QString>();

    QList< QPair<QString, QString> > cFiles;
    QString lcmStructDefs = "";

    QString pureFileName;
    QString pureIncludeFileName;

    QFileInfo fInfo(this->fileName);
    pureFileName = fInfo.baseName().split(".", QString::SkipEmptyParts).first();

    // XML parsed and converted to C code. Now generating the files
    outputDirName += QDir::separator() + pureFileName;
    QDateTime now = QDateTime::currentDateTime().toUTC();
    QLocale loc(QLocale::English);
    QString dateFormat = "dddd, MMMM d yyyy, hh:mm UTC";
    QString date = loc.toString(now, dateFormat);
    QString includeLine = "#include \"%1\"\n";
    QString mainHeaderName = pureFileName + ".h";
    QString messagesDirName = ".";//"generated";
    QDir dir(outputDirName + "/" + messagesDirName);

    int mavlinkVersion = 0;

    // we need to gather the message lengths across multiple includes,
    // which we can do via detecting recursion
    static unsigned message_lengths[256];
    static int highest_message_id;
    static int recursion_level;

    if (recursion_level == 0) {
        highest_message_id = 0;
        memset(message_lengths, 0, sizeof(message_lengths));
    }


    // Start main header
    QString mainHeader = QString("/** @file\n *\t@brief MAVLink comm protocol.\n *\t@see http://pixhawk.ethz.ch/software/mavlink\n *\t Generated on %1\n */\n#ifndef " + pureFileName.toUpper() + "_H\n#define " + pureFileName.toUpper() + "_H\n\n").arg(date); // The main header includes all messages
    // Mark all code as C code
    mainHeader += "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";
    mainHeader += "\n#include \"../protocol.h\"\n";
    mainHeader += "\n#define MAVLINK_ENABLED_" + pureFileName.toUpper() + "\n\n";

    QString enums;


    // Run through root children
    while(!n.isNull()) {
        // Each child is a message
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {
            if (e.tagName() == "mavlink") {
                p = n;
                n = n.firstChild();
                while (!n.isNull()) {
                    e = n.toElement();
                    if (!e.isNull()) {
                        // Handle all include tags
                        if (e.tagName() == "include") {
                            QString incFileName = e.text();
                            // Load file
                            //QDomDocument includeDoc = QDomDocument();

                            // Prepend file path if it is a relative path and
                            // make it relative to opened file
                            QFileInfo fInfo(incFileName);

                            QString incFilePath;
                            if (fInfo.isRelative()) {
                                QFileInfo rInfo(this->fileName);
                                incFilePath = rInfo.absoluteDir().canonicalPath() + "/" + incFileName;
                                pureIncludeFileName = fInfo.baseName().split(".", QString::SkipEmptyParts).first();
                            }

                            QFile file(incFilePath);
                            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                                emit parseState(QString("<font color=\"green\">Included messages from file: %1</font>").arg(incFileName));
                                // NEW MODE: CREATE INDIVIDUAL FOLDERS
                                // Create new output directory, parse included XML and generate C-code
                                MAVLinkXMLParser includeParser(incFilePath, topLevelOutputDirName, this);
                                connect(&includeParser, SIGNAL(parseState(QString)), this, SIGNAL(parseState(QString)));
                                // Generate and write
                                recursion_level++;
                                // Abort if inclusion fails
                                if (!includeParser.generate()) return false;
                                recursion_level--;
                                mainHeader += "\n#include \"../" + pureIncludeFileName + "/" + pureIncludeFileName + ".h\"\n";


                                // OLD MODE: MERGE BOTH FILES
                                //                                        const QString instanceText(QString::fromUtf8(file.readAll()));
                                //                                        includeDoc.setContent(instanceText);
                                //                                // Get all messages
                                //                                QDomNode in = includeDoc.documentElement().firstChild();
                                //                                QDomElement ie = in.toElement();
                                //                                if (!ie.isNull())
                                //                                {
                                //                                    if (ie.tagName() == "messages" || ie.tagName() == "include")
                                //                                    {
                                //                                        QDomNode ref = n.parentNode().insertAfter(in, n);
                                //                                        if (ref.isNull())
                                //                                        {
                                //                                            emit parseState(QString("<font color=\"red\">ERROR: Inclusion failed: XML syntax error in file %1. Wrong/misspelled XML?\nAbort.</font>").arg(fileName));
                                //                                            return false;
                                //                                        }
                                //                                    }
                                //                                }

                                emit parseState(QString("<font color=\"green\">End of inclusion from file: %1</font>").arg(incFileName));
                            } else {
                                // Include file could not be opened
                                emit parseState(QString("<font color=\"red\">ERROR: Failed including file: %1, file is not readable. Wrong/misspelled filename?\nAbort.</font>").arg(fileName));
                                return false;
                            }

                        }
                        // Handle all enum tags
                        else if (e.tagName() == "version") {
                            //QString fieldType = e.attribute("type", "");
                            //QString fieldName = e.attribute("name", "");
                            QString fieldText = e.text();

                            // Check if version has been previously set
                            if (mavlinkVersion != 0) {
                                emit parseState(QString("<font color=\"red\">ERROR: Protocol version tag set twice, please use it only once. First version was %1, second version is %2.\nAbort.</font>").arg(mavlinkVersion).arg(fieldText));
                                return false;
                            }

                            bool ok;
                            int version = fieldText.toInt(&ok);
                            if (ok && (version > 0) && (version < 256)) {
                                // Set MAVLink version
                                mavlinkVersion = version;
                            } else {
                                emit parseState(QString("<font color=\"red\">ERROR: Reading version string failed: %1, string is not an integer number between 1 and 255.\nAbort.</font>").arg(fieldText));
                                return false;
                            }
                        }
                        // Handle all enum tags
                        else if (e.tagName() == "enums") {
                            // One down into the enums list
                            p = n;
                            n = n.firstChild();
                            while (!n.isNull()) {
                                e = n.toElement();

                                QString currEnum;
                                QString currEnumEnd;
                                // Comment
                                QString comment;

                                if(!e.isNull() && e.tagName() == "enum") {
                                    // Get enum name
                                    QString enumName = e.attribute("name", "").toLower();
                                    if (enumName.size() == 0) {
                                        emit parseState(tr("<font color=\"red\">ERROR: Missing required name=\"\" attribute for tag %2 near line %1\nAbort.</font>").arg(QString::number(e.lineNumber()), e.tagName()));
                                        return false;
                                    } else {
                                        // Sanity check: Accept only enum names not used previously
                                        if (usedEnumNames->contains(enumName)) {
                                            emit parseState(tr("<font color=\"red\">ERROR: Enum name %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(enumName, QString::number(e.lineNumber()), fileName));
                                            return false;
                                        } else {
                                            usedEnumNames->insert(enumName, QString::number(e.lineNumber()));
                                        }

                                        // Everything sane, starting with enum content
                                        currEnum = "enum " + enumName.toUpper() + "\n{\n";
                                        currEnumEnd = QString("\t%1_ENUM_END\n};\n\n").arg(enumName.toUpper());

                                        int nextEnumValue = 0;

                                        // Get the enum fields
                                        QDomNode f = e.firstChild();
                                        while (!f.isNull()) {
                                            QDomElement e2 = f.toElement();
                                            if (!e2.isNull() && e2.tagName() == "entry") {
                                                QString fieldValue = e2.attribute("value", "");

                                                // If value was given, use it, if not, use the enum iterator
                                                // value. The iterator value gets reset by manual values

                                                QString fieldName = e2.attribute("name", "");
                                                if (fieldValue.length() == 0) {
                                                    fieldValue = QString::number(nextEnumValue);
                                                    nextEnumValue++;
                                                } else {
                                                    bool ok;
                                                    nextEnumValue = fieldValue.toInt(&ok) + 1;
                                                    if (!ok) {
                                                        emit parseState(tr("<font color=\"red\">ERROR: Enum entry %1 has not a valid number (%2) in the value field.\nAbort.</font>").arg(fieldName, fieldValue));
                                                        return false;
                                                    }
                                                }

                                                // Add comment of field if there is one
                                                QString fieldComment;
                                                if (e2.text().length() > 0)
                                                {
                                                    QString sep(" | ");
                                                    QDomNode pp = e2.firstChild();
                                                    while (!pp.isNull()) {
                                                        QDomElement pp2 = pp.toElement();
                                                        if (pp2.isText() || pp2.isCDATASection())
                                                        {
                                                            fieldComment +=  pp2.nodeValue() + sep;
                                                        }
                                                        else if (pp2.isElement())
                                                        {
                                                            fieldComment += pp2.text() + sep;
                                                            pp = pp.nextSibling();
                                                        }
                                                    }
                                                    fieldComment = fieldComment.replace("\n", " ");
                                                    fieldComment = " /* " + fieldComment.simplified() + " */";
                                                }
                                                currEnum += "\t" + fieldName.toUpper() + "=" + fieldValue + "," + fieldComment + "\n";
                                            }
                                            else if(!e2.isNull() && e2.tagName() == "description")
                                            {
                                                comment = " " + e2.text().replace("\n", " ") + comment;
                                            }
                                            f = f.nextSibling();
                                        }
                                    }
                                    // Add the last parsed enum
                                    // Remove the last comma, as the last value has none
                                    // ENUM END MARKER IS LAST ENTRY, COMMA REMOVAL NOT NEEDED
                                    //int commaPosition = currEnum.lastIndexOf(",");
                                    //currEnum.remove(commaPosition, 1);

                                    enums += "/** @brief " + comment  + " */\n" + currEnum + currEnumEnd;
                                } // Element is non-zero and element name is <enum>
                                n = n.nextSibling();
                            } // While through <enums>
                            // One up, back into the <mavlink> structure
                            n = p;
                        }

                        // Handle all message tags
                        else if (e.tagName() == "messages") {
                            p = n;
                            n = n.firstChild();
                            while (!n.isNull()) {
                                e = n.toElement();
                                if(!e.isNull()) {
                                    //if (e.isNull()) continue;
                                    // Get message name
                                    QString messageName = e.attribute("name", "").toLower();
                                    if (messageName.size() == 0) {
                                        emit parseState(tr("<font color=\"red\">ERROR: Missing required name=\"\" attribute for tag %2 near line %1\nAbort.</font>").arg(QString::number(e.lineNumber()), e.tagName()));
                                        return false;
                                    } else {
                                        // Get message id
                                        bool ok;
                                        int messageId = e.attribute("id", "-1").toInt(&ok, 10);
                                        emit parseState(tr("Compiling message <strong>%1 \t(#%3)</strong> \tnear line %2").arg(messageName, QString::number(n.lineNumber()), QString::number(messageId)));

                                        // Sanity check: Accept only message IDs not used previously
                                        if (usedMessageIDs->contains(messageId)) {
                                            emit parseState(tr("<font color=\"red\">ERROR: Message ID %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(QString::number(messageId), QString::number(e.lineNumber()), fileName));
                                            return false;
                                        } else {
                                            usedMessageIDs->append(messageId);
                                        }

                                        // Sanity check: Accept only message names not used previously
                                        if (usedMessageNames->contains(messageName)) {
                                            emit parseState(tr("<font color=\"red\">ERROR: Message name %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(messageName, QString::number(e.lineNumber()), fileName));
                                            return false;
                                        } else {
                                            usedMessageNames->insert(messageName, QString::number(e.lineNumber()));
                                        }

                                        QString channelType("mavlink_channel_t");
                                        QString messageType("mavlink_message_t");

                                        // Build up function call
                                        QString commentContainer("/**\n * @brief Pack a %1 message\n * @param system_id ID of this system\n * @param component_id ID of this component (e.g. 200 for IMU)\n * @param msg The MAVLink message to compress the data into\n *\n%2 * @return length of the message in bytes (excluding serial stream start sign)\n */\n");
                                        QString commentPackChanContainer("/**\n * @brief Pack a %1 message\n * @param system_id ID of this system\n * @param component_id ID of this component (e.g. 200 for IMU)\n * @param chan The MAVLink channel this message was sent over\n * @param msg The MAVLink message to compress the data into\n%2 * @return length of the message in bytes (excluding serial stream start sign)\n */\n");
                                        QString commentSendContainer("/**\n * @brief Send a %1 message\n * @param chan MAVLink channel to send the message\n *\n%2 */\n");
                                        QString commentEncodeContainer("/**\n * @brief Encode a %1 struct into a message\n *\n * @param system_id ID of this system\n * @param component_id ID of this component (e.g. 200 for IMU)\n * @param msg The MAVLink message to compress the data into\n * @param %1 C-struct to read the message contents from\n */\n");
                                        QString commentDecodeContainer("/**\n * @brief Decode a %1 message into a struct\n *\n * @param msg The message to decode\n * @param %1 C-struct to decode the message contents into\n */\n");
                                        QString commentEntry(" * @param %1 %2\n");
                                        QString idDefine = QString("#define MAVLINK_MSG_ID_%1 %2").arg(messageName.toUpper(), QString::number(messageId));
                                        QString arrayDefines;
                                        QString cStructName = QString("mavlink_%1_t").arg(messageName);
                                        QString cStruct("typedef struct __%1 \n{\n%2\n} %1;");
                                        QString cStructLines;
                                        QString encode("static inline uint16_t mavlink_msg_%1_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const %2* %1)\n{\n\treturn mavlink_msg_%1_pack(%3);\n}\n");

                                        QString decode("static inline void mavlink_msg_%1_decode(const mavlink_message_t* msg, %2* %1)\n{\n%3}\n");
                                        QString pack("static inline uint16_t mavlink_msg_%1_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg%2)\n{\n\tuint16_t i = 0;\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\n%4\n\treturn mavlink_finalize_message(msg, system_id, component_id, i);\n}\n\n");
                                        QString packChan("static inline uint16_t mavlink_msg_%1_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg%2)\n{\n\tuint16_t i = 0;\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\n%4\n\treturn mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);\n}\n\n");
                                        QString compactSend("#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\n\nstatic inline void mavlink_msg_%3_send(%1 chan%5)\n{\n\t%2 msg;\n\tmavlink_msg_%3_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg%4);\n\tmavlink_send_uart(chan, &msg);\n}\n\n#endif");
                                        //QString compactStructSend = "#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\n\nstatic inline void mavlink_msg_%3_struct_send(%1 chan%5)\n{\n\t%2 msg;\n\tmavlink_msg_%3_encode(mavlink_system.sysid, mavlink_system.compid, &msg%4);\n\tmavlink_send_uart(chan, &msg);\n}\n\n#endif";
                                        QString unpacking;
                                        QString prepends;
                                        QString packParameters;
                                        QString packArguments("system_id, component_id, msg");
                                        QString packLines;
                                        QString decodeLines;
                                        QString sendArguments;
                                        QString commentLines;
                                        unsigned message_length = 0;


                                        // Get the message fields
                                        QDomNode f = e.firstChild();
                                        while (!f.isNull()) {
                                            QDomElement e2 = f.toElement();
                                            if (!e2.isNull() && e2.tagName() == "field") {
                                                QString fieldType = e2.attribute("type", "");
                                                QString fieldName = e2.attribute("name", "");
                                                QString fieldText = e2.text();

                                                QString unpackingCode;
                                                QString unpackingComment = QString("/**\n * @brief Get field %1 from %2 message\n *\n * @return %3\n */\n").arg(fieldName, messageName, fieldText);

                                                // Send arguments do not work for the version field
                                                if (!fieldType.contains("uint8_t_mavlink_version")) {
                                                    // Send arguments are the same for integral types and arrays
                                                    sendArguments += ", " + fieldName;
                                                    commentLines += commentEntry.arg(fieldName, fieldText.replace("\n", " "));
                                                }

                                                // MAVLink version field
                                                // this is a special field always containing the version define
                                                if (fieldType.contains("uint8_t_mavlink_version")) {
                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2; ///< %3\n").arg("uint8_t", fieldName, fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_uint8_t_by_index(%1, i, msg->payload); // %2\n").arg(mavlinkVersion).arg(fieldText);
                                                    // Add decode function for this type
                                                    decodeLines += QString("\t%1->%2 = mavlink_msg_%1_get_%2(msg);\n").arg(messageName, fieldName);
                                                }

                                                // Array handling is different from simple types
                                                else if (fieldType.startsWith("array")) {
                                                    int arrayLength = QString(fieldType.split("[").at(1).split("]").first()).toInt();
                                                    QString arrayType = fieldType.split("[").first();
                                                    packParameters += QString(", const ") + QString("int8_t*") + " " + fieldName;
                                                    packArguments += ", " + messageName + "->" + fieldName;

                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2[%3]; ///< %4\n").arg("int8_t", fieldName, QString::number(arrayLength), fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_%1_by_index(%2, %3, i, msg->payload); // %4\n").arg(arrayType, fieldName, QString::number(arrayLength), fieldText);
                                                    // Add decode function for this type
                                                    decodeLines += QString("\tmavlink_msg_%1_get_%2(msg, %1->%2);\n").arg(messageName, fieldName);
                                                    arrayDefines += QString("#define MAVLINK_MSG_%1_FIELD_%2_LEN %3\n").arg(messageName.toUpper(), fieldName.toUpper(), QString::number(arrayLength));
                                                } else if (fieldType.startsWith("string")) {
                                                    int arrayLength = QString(fieldType.split("[").at(1).split("]").first()).toInt();
                                                    QString arrayType = fieldType.split("[").first();
                                                    packParameters += QString(", const ") + QString("char*") + " " + fieldName;
                                                    packArguments += ", " + messageName + "->" + fieldName;

                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2[%3]; ///< %4\n").arg("char", fieldName, QString::number(arrayLength), fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_%1_by_index(%2, %3, i, msg->payload); // %4\n").arg(arrayType, fieldName, QString::number(arrayLength), e2.text());
                                                    // Add decode function for this type
                                                    decodeLines += QString("\tmavlink_msg_%1_get_%2(msg, %1->%2);\n").arg(messageName, fieldName);
                                                    arrayDefines += QString("#define MAVLINK_MSG_%1_FIELD_%2_LEN %3\n").arg(messageName.toUpper(), fieldName.toUpper(), QString::number(arrayLength));
                                                }
                                                // Expand array handling to all valid mavlink data types
                                                else if(fieldType.contains('[') && fieldType.contains(']')) {
                                                    int arrayLength = QString(fieldType.split("[").at(1).split("]").first()).toInt();
                                                    QString arrayType = fieldType.split("[").first();
                                                    packParameters += QString(", const ") + arrayType + "* " + fieldName;
                                                    packArguments += ", " + messageName + "->" + fieldName;

                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2[%3]; ///< %4\n").arg(arrayType, fieldName, QString::number(arrayLength), fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_array_by_index((const int8_t*)%1, sizeof(%2)*%3, i, msg->payload); // %4\n").arg(fieldName, arrayType, QString::number(arrayLength), fieldText);
                                                    // Add decode function for this type
                                                    decodeLines += QString("\tmavlink_msg_%1_get_%2(msg, %1->%2);\n").arg(messageName, fieldName);
                                                    arrayDefines += QString("#define MAVLINK_MSG_%1_FIELD_%2_LEN %3\n").arg(messageName.toUpper(), fieldName.toUpper(), QString::number(arrayLength));

                                                    unpackingCode = QString("\n\tmemcpy(r_data, msg->payload%1, sizeof(%2)*%3);\n\treturn sizeof(%2)*%3;").arg(prepends, arrayType, QString::number(arrayLength));

                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, %3* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, arrayType, unpackingCode);
                                                    //                                                    decodeLines += "";
                                                    prepends += QString("+sizeof(%1)*%2").arg(arrayType, QString::number(arrayLength));

                                                } else
                                                    // Handle simple types like integers and floats
                                                {
                                                    packParameters += ", " + fieldType + " " + fieldName;
                                                    packArguments += ", " + messageName + "->" + fieldName;

                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2; ///< %3\n").arg(fieldType, fieldName, fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_%1_by_index(%2, i, msg->payload); // %3\n").arg(fieldType, fieldName, e2.text());
                                                    // Add decode function for this type
                                                    decodeLines += QString("\t%1->%2 = mavlink_msg_%1_get_%2(msg);\n").arg(messageName, fieldName);
                                                }


                                                // message length calculation
                                                unsigned element_multiplier = 1;
                                                unsigned element_length = 0;
                                                const struct {
                                                    const char *prefix;
                                                    unsigned length;
                                                } length_map[] = {
                                                        { "array",  1 },
                                                        { "char",   1 },
                                                        { "uint8",  1 },
                                                        { "int8",   1 },
                                                        { "uint16", 2 },
                                                        { "int16",  2 },
                                                        { "uint32", 4 },
                                                        { "int32",  4 },
                                                        { "uint64", 8 },
                                                        { "int64",  8 },
                                                        { "float",  4 },
                                                        { "double", 8 },
                                                };
                                                if (fieldType.contains("[")) {
                                                    element_multiplier = fieldType.split("[").at(1).split("]").first().toInt();
                                                }
                                                for (unsigned i=0; i<sizeof(length_map)/sizeof(length_map[0]); i++) {
                                                    if (fieldType.startsWith(length_map[i].prefix)) {
                                                        element_length = length_map[i].length * element_multiplier;
                                                        break;
                                                    }
                                                }
                                                if (element_length == 0) {
                                                    emit parseState(tr("<font color=\"red\">ERROR: Unable to calculate length for %2 near line %1\nAbort.</font>").arg(QString::number(e.lineNumber()), fieldType));
                                                }
                                                message_length += element_length;

                                                //
                                                //                                                QString unpackingCode;

                                                if (fieldType == "uint8_t_mavlink_version") {
                                                    unpackingCode = QString("\treturn (%1)(msg->payload%2)[0];").arg("uint8_t", prepends);
                                                } else if (fieldType == "uint8_t" || fieldType == "int8_t") {
                                                    unpackingCode = QString("\treturn (%1)(msg->payload%2)[0];").arg(fieldType, prepends);
                                                } else if (fieldType == "uint16_t" || fieldType == "int16_t") {
                                                    unpackingCode = QString("\tgeneric_16bit r;\n\tr.b[1] = (msg->payload%1)[0];\n\tr.b[0] = (msg->payload%1)[1];\n\treturn (%2)r.s;").arg(prepends).arg(fieldType);
                                                } else if (fieldType == "uint32_t" || fieldType == "int32_t") {
                                                    unpackingCode = QString("\tgeneric_32bit r;\n\tr.b[3] = (msg->payload%1)[0];\n\tr.b[2] = (msg->payload%1)[1];\n\tr.b[1] = (msg->payload%1)[2];\n\tr.b[0] = (msg->payload%1)[3];\n\treturn (%2)r.i;").arg(prepends).arg(fieldType);
                                                } else if (fieldType == "float") {
                                                    unpackingCode = QString("\tgeneric_32bit r;\n\tr.b[3] = (msg->payload%1)[0];\n\tr.b[2] = (msg->payload%1)[1];\n\tr.b[1] = (msg->payload%1)[2];\n\tr.b[0] = (msg->payload%1)[3];\n\treturn (%2)r.f;").arg(prepends).arg(fieldType);
                                                } else if (fieldType == "uint64_t" || fieldType == "int64_t") {
                                                    unpackingCode = QString("\tgeneric_64bit r;\n\tr.b[7] = (msg->payload%1)[0];\n\tr.b[6] = (msg->payload%1)[1];\n\tr.b[5] = (msg->payload%1)[2];\n\tr.b[4] = (msg->payload%1)[3];\n\tr.b[3] = (msg->payload%1)[4];\n\tr.b[2] = (msg->payload%1)[5];\n\tr.b[1] = (msg->payload%1)[6];\n\tr.b[0] = (msg->payload%1)[7];\n\treturn (%2)r.ll;").arg(prepends).arg(fieldType);
                                                } else if (fieldType.startsWith("array")) {
                                                    // fieldtype formatis string[n] where n is the number of bytes, extract n from field type string
                                                    unpackingCode = QString("\n\tmemcpy(r_data, msg->payload%1, %2);\n\treturn %2;").arg(prepends, fieldType.split("[").at(1).split("]").first());
                                                } else if (fieldType.startsWith("string")) {
                                                    // fieldtype formatis string[n] where n is the number of bytes, extract n from field type string
                                                    unpackingCode = QString("\n\tstrcpy(r_data, msg->payload%1, %2);\n\treturn %2;").arg(prepends, fieldType.split("[").at(1).split("]").first());
                                                }


                                                // Generate the message decoding function
                                                if (fieldType.contains("uint8_t_mavlink_version")) {
                                                    unpacking += unpackingComment + QString("static inline %1 mavlink_msg_%2_get_%3(const mavlink_message_t* msg)\n{\n%4\n}\n\n").arg("uint8_t", messageName, fieldName, unpackingCode);
                                                    decodeLines += "";
                                                    prepends += "+sizeof(uint8_t)";
                                                }
                                                // Array handling is different from simple types
                                                else if (fieldType.startsWith("array")) {
                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, int8_t* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, unpackingCode);
                                                    decodeLines += "";
                                                    QString arrayLength = QString(fieldType.split("[").at(1).split("]").first());
                                                    prepends += "+" + arrayLength;
                                                } else if (fieldType.startsWith("string")) {
                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, char* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, unpackingCode);
                                                    decodeLines += "";
                                                    QString arrayLength = QString(fieldType.split("[").at(1).split("]").first());
                                                    prepends += "+" + arrayLength;
                                                } else if(fieldType.contains('[') && fieldType.contains(']')) {
                                                    // prevent this case from being caught in the following else
                                                } else {
                                                    unpacking += unpackingComment + QString("static inline %1 mavlink_msg_%2_get_%3(const mavlink_message_t* msg)\n{\n%4\n}\n\n").arg(fieldType, messageName, fieldName, unpackingCode);
                                                    decodeLines += "";
                                                    prepends += "+sizeof(" + e2.attribute("type", "void") + ")";
                                                }
                                            }
                                            f = f.nextSibling();
                                        }

                                        if (messageId > highest_message_id) {
                                            highest_message_id = messageId;
                                        }
                                        message_lengths[messageId] = message_length;

                                        cStruct = cStruct.arg(cStructName, cStructLines);
                                        lcmStructDefs.append("\n").append(cStruct).append("\n");
                                        pack = pack.arg(messageName, packParameters, messageName.toUpper(), packLines);
                                        packChan = packChan.arg(messageName, packParameters, messageName.toUpper(), packLines);
                                        encode = encode.arg(messageName).arg(cStructName).arg(packArguments);
                                        decode = decode.arg(messageName).arg(cStructName).arg(decodeLines);
                                        compactSend = compactSend.arg(channelType, messageType, messageName, sendArguments, packParameters);
                                        QString cFile = "// MESSAGE " + messageName.toUpper() + " PACKING\n\n" + idDefine + "\n\n" + cStruct + "\n\n" + arrayDefines + "\n\n" + commentContainer.arg(messageName.toLower(), commentLines) + pack + commentPackChanContainer.arg(messageName.toLower(), commentLines) + packChan + commentEncodeContainer.arg(messageName.toLower()) + encode + "\n" + commentSendContainer.arg(messageName.toLower(), commentLines) + compactSend + "\n" + "// MESSAGE " + messageName.toUpper() + " UNPACKING\n\n" + unpacking + commentDecodeContainer.arg(messageName.toLower()) + decode;
                                        cFiles.append(qMakePair(QString("mavlink_msg_%1.h").arg(messageName), cFile));
                                    } // Check if tag = message
                                } // Check if e = NULL
                                n = n.nextSibling();
                            } // While through <message>
                            n = p;

                        } // Check if tag = messages
                    } // Check if e = NULL
                    n = n.nextSibling();
                } // While through include and messages
                // One up - current node = parent
                n = p;

            } // Check if tag = mavlink
        } // Check if e = NULL
        n = n.nextSibling();
    } // While through root children

    // Add version to main header

    mainHeader += "// MAVLINK VERSION\n\n";
    mainHeader += QString("#ifndef MAVLINK_VERSION\n#define MAVLINK_VERSION %1\n#endif\n\n").arg(mavlinkVersion);
    mainHeader += QString("#if (MAVLINK_VERSION == 0)\n#undef MAVLINK_VERSION\n#define MAVLINK_VERSION %1\n#endif\n\n").arg(mavlinkVersion);

    // Add enums to main header

    mainHeader += "// ENUM DEFINITIONS\n\n";
    mainHeader += enums;
    mainHeader += "\n";

    mainHeader += "// MESSAGE DEFINITIONS\n\n";
    // Create directory if it doesn't exist, report result in success
    if (!dir.exists()) success = success && dir.mkpath(outputDirName + "/" + messagesDirName);
    for (int i = 0; i < cFiles.size(); i++) {
        QFile rawFile(dir.filePath(cFiles.at(i).first));
        bool ok = rawFile.open(QIODevice::WriteOnly | QIODevice::Text);
        success = success && ok;
        rawFile.write(cFiles.at(i).second.toLatin1());
        rawFile.close();
        mainHeader += includeLine.arg(messagesDirName + "/" + cFiles.at(i).first);
    }

    mainHeader += "\n\n// MESSAGE LENGTHS\n\n";
    mainHeader += "#undef MAVLINK_MESSAGE_LENGTHS\n";
    mainHeader += "#define MAVLINK_MESSAGE_LENGTHS { ";
    for (int i=0; i<highest_message_id; i++) {
        mainHeader += QString::number(message_lengths[i]);
        if (i < highest_message_id-1) mainHeader += ", ";
    }
    mainHeader += " }\n\n";

    mainHeader += "#ifdef __cplusplus\n}\n#endif\n";
    mainHeader += "#endif";
    // Newline to make compiler happy
    mainHeader += "\n";

    // Write main header
    QFile rawHeader(outputDirName + "/" + mainHeaderName);
    bool ok = rawHeader.open(QIODevice::WriteOnly | QIODevice::Text);
    success = success && ok;
    rawHeader.write(mainHeader.toLatin1());
    rawHeader.close();

    // Write alias mavlink header
    QFile mavlinkHeader(outputDirName + "/mavlink.h");
    ok = mavlinkHeader.open(QIODevice::WriteOnly | QIODevice::Text);
    success = success && ok;
    QString mHeader = QString("/** @file\n *\t@brief MAVLink comm protocol.\n *\t@see http://pixhawk.ethz.ch/software/mavlink\n *\t Generated on %1\n */\n#ifndef MAVLINK_H\n#define MAVLINK_H\n\n").arg(date); // The main header includes all messages
    // Mark all code as C code
    mHeader += "#include \"" + mainHeaderName + "\"\n\n";
    mHeader += "#endif\n";
    mavlinkHeader.write(mHeader.toLatin1());
    mavlinkHeader.close();

    // Write C structs / lcm definitions
    //    QFile lcmStructs(outputDirName + "/mavlink.lcm");
    //    ok = lcmStructs.open(QIODevice::WriteOnly | QIODevice::Text);
    //    success = success && ok;
    //    lcmStructs.write(lcmStructDefs.toLatin1());

    return success;
}
