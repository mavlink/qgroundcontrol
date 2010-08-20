/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
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
    if (outputDirName == "") return false;

    // print out the element names of all elements that are direct children
    // of the outermost element.
    QDomElement docElem = doc->documentElement();
    QDomNode n = docElem;//.firstChild();
    QDomNode p = docElem;

    // Sanity check variables
    QList<int>* usedMessageIDs = new QList<int>();
    QMap<QString, QString>* usedMessageNames = new QMap<QString, QString>();

    QList< QPair<QString, QString> > cFiles;
    QString lcmStructDefs = "";

    QString pureFileName;

    // Run through root children
    while(!n.isNull())
    {
        // Each child is a message
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull())
        {
            if (e.tagName() == "mavlink")
            {
                p = n;
                n = n.firstChild();
                while (!n.isNull())
                {
                    e = n.toElement();
                    if (!e.isNull())
                    {
                        // Handle all include tags
                        if (e.tagName() == "include")
                        {
                            QString fileName = e.text();
                            // Load file
                            QDomDocument includeDoc = QDomDocument();

                            // Prepend file path if it is a relative path and
                            // make it relative to opened file
                            QFileInfo fInfo(fileName);
                            if (fInfo.isRelative())
                            {
                                QFileInfo rInfo(this->fileName);
                                fileName = rInfo.absoluteDir().canonicalPath() + "/" + fileName;
                                pureFileName = rInfo.baseName().split(".").first();
                            }

                            QFile file(fileName);
                            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                            {
                                const QString instanceText(QString::fromUtf8(file.readAll()));
                                includeDoc.setContent(instanceText);

                                // Get all messages
                                QDomNode in = includeDoc.documentElement().firstChild();
                                //while (!in.isNull())
                                //{
                                    QDomElement ie = in.toElement();
                                    if (!ie.isNull())
                                    {
                                        if (ie.tagName() == "messages" || ie.tagName() == "include")
                                        {
                                            QDomNode ref = n.parentNode().insertAfter(in, n);
                                            if (ref.isNull())
                                            {
                                                emit parseState(QString("<font color=\"red\">ERROR: Inclusion failed: XML syntax error in file %1. Wrong/misspelled XML?\nAbort.</font>").arg(fileName));
                                                return false;
                                            }
                                        }
                                    }
                                    //in = in.nextSibling();
                                //}

                                emit parseState(QString("<font color=\"green\">Included messages from file: %1</font>").arg(fileName));
                            }
                            else
                            {
                                // Include file could not be opened
                                emit parseState(QString("<font color=\"red\">ERROR: Failed including file: %1, file is not readable. Wrong/misspelled filename?\nAbort.</font>").arg(fileName));
                                return false;
                            }

                        }
                        // Handle all message tags
                        else if (e.tagName() == "messages")
                        {
                            p = n;
                            n = n.firstChild();
                            while (!n.isNull())
                            {
                                e = n.toElement();
                                if(!e.isNull())
                                {
                                    //if (e.isNull()) continue;
                                    // Get message name
                                    QString messageName = e.attribute("name", "").toLower();
                                    if (messageName.size() == 0)
                                    {
                                        emit parseState(tr("<font color=\"red\">ERROR: Missing required name=\"\" attribute for tag %2 near line %1\nAbort.</font>").arg(QString::number(e.lineNumber()), e.tagName()));
                                        return false;
                                    }
                                    else
                                    {
                                        // Get message id
                                        bool ok;
                                        int messageId = e.attribute("id", "-1").toInt(&ok, 10);
                                        emit parseState(tr("Compiling message <strong>%1 \t(#%3)</strong> \tnear line %2").arg(messageName, QString::number(n.lineNumber()), QString::number(messageId)));

                                        // Sanity check: Accept only message IDs not used previously
                                        if (usedMessageIDs->contains(messageId))
                                        {
                                            emit parseState(tr("<font color=\"red\">ERROR: Message ID %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(QString::number(messageId), QString::number(e.lineNumber()), fileName));
                                            return false;
                                        }
                                        else
                                        {
                                            usedMessageIDs->append(messageId);
                                        }

                                        // Sanity check: Accept only message names not used previously
                                        if (usedMessageNames->contains(messageName))
                                        {
                                            emit parseState(tr("<font color=\"red\">ERROR: Message name %1 used twice, second occurence near line %2 of file %3\nAbort.</font>").arg(messageName, QString::number(e.lineNumber()), fileName));
                                            return false;
                                        }
                                        else
                                        {
                                            usedMessageNames->insert(messageName, QString::number(e.lineNumber()));
                                        }

                                        QString channelType = "mavlink_channel_t";
                                        QString messageType = "mavlink_message_t";

                                        // Build up function call
                                        QString commentContainer = "/**\n * @brief Send a %1 message\n *\n%2 * @return length of the message in bytes (excluding serial stream start sign)\n */\n";
                                        QString commentEntry = " * @param %1 %2\n";
                                        QString idDefine = QString("#define MAVLINK_MSG_ID_%1 %2").arg(messageName.toUpper(), QString::number(messageId));
                                        QString arrayDefines = "";
                                        QString cStructName = QString("mavlink_%1_t").arg(messageName);
                                        QString cStruct = "typedef struct __%1 \n{\n%2\n} %1;";
                                        QString cStructLines = "";
                                        QString encode = "static inline uint16_t mavlink_msg_%1_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const %2* %1)\n{\n\treturn mavlink_msg_%1_pack(%3);\n}\n";

                                        QString decode = "static inline void mavlink_msg_%1_decode(const mavlink_message_t* msg, %2* %1)\n{\n%3}\n";
                                        QString pack = "static inline uint16_t mavlink_msg_%1_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg%2)\n{\n\tuint16_t i = 0;\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\n%4\n\treturn mavlink_finalize_message(msg, system_id, component_id, i);\n}\n\n";
                                        QString compactSend = "#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\n\nstatic inline void mavlink_msg_%3_send(%1 chan%5)\n{\n\t%2 msg;\n\tmavlink_msg_%3_pack(mavlink_system.sysid, mavlink_system.compid, &msg%4);\n\tmavlink_send_uart(chan, &msg);\n}\n\n#endif";
                                        //QString compactStructSend = "#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS\n\nstatic inline void mavlink_msg_%3_struct_send(%1 chan%5)\n{\n\t%2 msg;\n\tmavlink_msg_%3_encode(mavlink_system.sysid, mavlink_system.compid, &msg%4);\n\tmavlink_send_uart(chan, &msg);\n}\n\n#endif";
                                        QString unpacking = "";
                                        QString prepends = "";
                                        QString packParameters = "";
                                        QString packArguments = "system_id, component_id, msg";
                                        QString packLines = "";
                                        QString decodeLines = "";
                                        QString sendArguments = "";
                                        QString commentLines = "";

                                        // Get the message fields
                                        QDomNode f = e.firstChild();
                                        while (!f.isNull())
                                        {
                                            QDomElement e2 = f.toElement();
                                            if (!e2.isNull() && e2.tagName() == "field")
                                            {
                                                QString fieldType = e2.attribute("type", "");
                                                QString fieldName = e2.attribute("name", "");
                                                QString fieldText = e2.text();

                                                // Send arguments are the same for integral types and arrays
                                                sendArguments += ", " + fieldName;

                                                // Array handling is different from simple types
                                                if (fieldType.startsWith("array"))
                                                {
                                                    int arrayLength = QString(fieldType.split("[").at(1).split("]").first()).toInt();
                                                    QString arrayType = fieldType.split("[").first();
                                                    packParameters += QString(", const ") + QString("int8_t*") + " " + fieldName;
                                                    packArguments += ", " + messageName + "->" + fieldName;

                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2[%3]; ///< %4\n").arg("int8_t", fieldName, QString::number(arrayLength), fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_%1_by_index(%2, %3, i, msg->payload); //%4\n").arg(arrayType, fieldName, QString::number(arrayLength), e2.text());
                                                    // Add decode function for this type
                                                    decodeLines += QString("\tmavlink_msg_%1_get_%2(msg, %1->%2);\n").arg(messageName, fieldName);
                                                    arrayDefines += QString("#define MAVLINK_MSG_%1_FIELD_%2_LEN %3").arg(messageName.toUpper(), fieldName.toUpper(), QString::number(arrayLength));
                                                }
                                                else if (fieldType.startsWith("string"))
                                                {
                                                    int arrayLength = QString(fieldType.split("[").at(1).split("]").first()).toInt();
                                                    QString arrayType = fieldType.split("[").first();
                                                    packParameters += QString(", const ") + QString("char*") + " " + fieldName;
                                                    packArguments += ", " + messageName + "->" + fieldName;

                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2[%3]; ///< %4\n").arg("char", fieldName, QString::number(arrayLength), fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_%1_by_index(%2, %3, i, msg->payload); //%4\n").arg(arrayType, fieldName, QString::number(arrayLength), e2.text());
                                                    // Add decode function for this type
                                                    decodeLines += QString("\tmavlink_msg_%1_get_%2(msg, %1->%2);\n").arg(messageName, fieldName);
                                                    arrayDefines += QString("#define MAVLINK_MSG_%1_FIELD_%2_LEN %3").arg(messageName.toUpper(), fieldName.toUpper(), QString::number(arrayLength));
                                                }
                                                else
                                                    // Handle simple types like integers and floats
                                                {
                                                    packParameters += ", " + fieldType + " " + fieldName;
                                                    packArguments += ", " + messageName + "->" + fieldName;

                                                    // Add field to C structure
                                                    cStructLines += QString("\t%1 %2; ///< %3\n").arg(fieldType, fieldName, fieldText);
                                                    // Add pack line to message_xx_pack function
                                                    packLines += QString("\ti += put_%1_by_index(%2, i, msg->payload); //%3\n").arg(fieldType, fieldName, e2.text());
                                                    // Add decode function for this type
                                                    decodeLines += QString("\t%1->%2 = mavlink_msg_%1_get_%2(msg);\n").arg(messageName, fieldName);
                                                }
                                                commentLines += commentEntry.arg(fieldName, fieldText);
                                                QString unpackingComment = QString("/**\n * @brief Get field %1 from %2 message\n *\n * @return %3\n */\n").arg(fieldName, messageName, fieldText);
                                                //
                                                QString unpackingCode;

                                                if (fieldType == "uint8_t" || fieldType == "int8_t")
                                                {
                                                    unpackingCode = QString("\treturn (%1)(msg->payload%2)[0];").arg(fieldType, prepends);
                                                }
                                                else if (fieldType == "uint16_t" || fieldType == "int16_t")
                                                {
                                                    unpackingCode = QString("\tgeneric_16bit r;\n\tr.b[1] = (msg->payload%1)[0];\n\tr.b[0] = (msg->payload%1)[1];\n\treturn (%2)r.s;").arg(prepends).arg(fieldType);
                                                }
                                                else if (fieldType == "uint32_t" || fieldType == "int32_t")
                                                {
                                                    unpackingCode = QString("\tgeneric_32bit r;\n\tr.b[3] = (msg->payload%1)[0];\n\tr.b[2] = (msg->payload%1)[1];\n\tr.b[1] = (msg->payload%1)[2];\n\tr.b[0] = (msg->payload%1)[3];\n\treturn (%2)r.i;").arg(prepends).arg(fieldType);
                                                }
                                                else if (fieldType == "float")
                                                {
                                                    unpackingCode = QString("\tgeneric_32bit r;\n\tr.b[3] = (msg->payload%1)[0];\n\tr.b[2] = (msg->payload%1)[1];\n\tr.b[1] = (msg->payload%1)[2];\n\tr.b[0] = (msg->payload%1)[3];\n\treturn (%2)r.f;").arg(prepends).arg(fieldType);
                                                }
                                                else if (fieldType == "uint64_t" || fieldType == "int64_t")
                                                {
                                                    unpackingCode = QString("\tgeneric_64bit r;\n\tr.b[7] = (msg->payload%1)[0];\n\tr.b[6] = (msg->payload%1)[1];\n\tr.b[5] = (msg->payload%1)[2];\n\tr.b[4] = (msg->payload%1)[3];\n\tr.b[3] = (msg->payload%1)[4];\n\tr.b[2] = (msg->payload%1)[5];\n\tr.b[1] = (msg->payload%1)[6];\n\tr.b[0] = (msg->payload%1)[7];\n\treturn (%2)r.ll;").arg(prepends).arg(fieldType);
                                                }
                                                else if (fieldType.startsWith("array"))
                                                {  // fieldtype formatis string[n] where n is the number of bytes, extract n from field type string
                                                    unpackingCode = QString("\n\tmemcpy(r_data, msg->payload%1, %2);\n\treturn %2;").arg(prepends, fieldType.split("[").at(1).split("]").first());
                                                }
                                                else if (fieldType.startsWith("string"))
                                                {  // fieldtype formatis string[n] where n is the number of bytes, extract n from field type string
                                                    unpackingCode = QString("\n\tstrcpy(r_data, msg->payload%1, %2);\n\treturn %2;").arg(prepends, fieldType.split("[").at(1).split("]").first());
                                                }

                                                // Generate the message decoding function
                                                // Array handling is different from simple types
                                                if (fieldType.startsWith("array"))
                                                {
                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, int8_t* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, unpackingCode);
                                                    decodeLines += "";
                                                    QString arrayLength = QString(fieldType.split("[").at(1).split("]").first());
                                                    prepends += "+" + arrayLength;
                                                }
                                                else if (fieldType.startsWith("string"))
                                                {
                                                    unpacking += unpackingComment + QString("static inline uint16_t mavlink_msg_%1_get_%2(const mavlink_message_t* msg, char* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, unpackingCode);
                                                    decodeLines += "";
                                                    QString arrayLength = QString(fieldType.split("[").at(1).split("]").first());
                                                    prepends += "+" + arrayLength;
                                                }
                                                else
                                                {
                                                    unpacking += unpackingComment + QString("static inline %1 mavlink_msg_%2_get_%3(const mavlink_message_t* msg)\n{\n%4\n}\n\n").arg(fieldType, messageName, fieldName, unpackingCode);
                                                    decodeLines += "";
                                                    prepends += "+sizeof(" + e2.attribute("type", "void") + ")";
                                                }
                                            }
                                            f = f.nextSibling();
                                        }

                                        cStruct = cStruct.arg(cStructName, cStructLines);
                                        lcmStructDefs.append("\n").append(cStruct).append("\n");
                                        pack = pack.arg(messageName, packParameters, messageName.toUpper(), packLines);
                                        encode = encode.arg(messageName).arg(cStructName).arg(packArguments);
                                        decode = decode.arg(messageName).arg(cStructName).arg(decodeLines);
                                        compactSend = compactSend.arg(channelType, messageType, messageName, sendArguments, packParameters);
                                        QString cFile = "// MESSAGE " + messageName.toUpper() + " PACKING\n\n" + idDefine + "\n\n" + cStruct + "\n\n" + arrayDefines + "\n\n" + commentContainer.arg(messageName.toLower(), commentLines) + pack + encode + "\n" + compactSend + "\n" + "// MESSAGE " + messageName.toUpper() + " UNPACKING\n\n" + unpacking + decode;
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
                n = p;

            } // Check if tag = mavlink
        } // Check if e = NULL
        n = n.nextSibling();
    } // While through root children

    // XML parsed and converted to C code. Now generating the files
    QDateTime now = QDateTime::currentDateTime().toUTC();
    QLocale loc(QLocale::English);
    QString dateFormat = "dddd, MMMM d yyyy, hh:mm UTC";
    QString date = loc.toString(now, dateFormat);
    QString mainHeader = QString("/** @file\n *\t@brief MAVLink comm protocol.\n *\t@see http://pixhawk.ethz.ch/software/mavlink\n *\t Generated on %1\n */\n#ifndef MAVLINK_H\n#define MAVLINK_H\n\n").arg(date); // The main header includes all messages
    // Mark all code as C code
    mainHeader += "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";
    mainHeader += "\n#include \"protocol.h\"\n";
    mainHeader += "\n#define MAVLINK_ENABLED_" + pureFileName.toUpper() + "\n\n";
    QString includeLine = "#include \"%1\"\n";
    QString mainHeaderName = "mavlink.h";
    QString messagesDirName = "generated";
    QDir dir(outputDirName + "/" + messagesDirName);
    // Create directory if it doesn't exist, report result in success
    if (!dir.exists()) success = success && dir.mkpath(outputDirName + "/" + messagesDirName);
    for (int i = 0; i < cFiles.size(); i++)
    {
        QFile rawFile(dir.filePath(cFiles.at(i).first));
        bool ok = rawFile.open(QIODevice::WriteOnly | QIODevice::Text);
        success = success && ok;
        rawFile.write(cFiles.at(i).second.toLatin1());
        mainHeader += includeLine.arg(messagesDirName + "/" + cFiles.at(i).first);
    }

    mainHeader += "#ifdef __cplusplus\n}\n#endif\n";
    mainHeader += "#endif";
    // Newline to make compiler happy
    mainHeader += "\n";

    // Write main header
    QFile rawHeader(outputDirName + "/" + mainHeaderName);
    bool ok = rawHeader.open(QIODevice::WriteOnly | QIODevice::Text);
    success = success && ok;
    rawHeader.write(mainHeader.toLatin1());

    // Write C structs / lcm definitions
    QFile lcmStructs(outputDirName + "/mavlink.lcm");
    ok = lcmStructs.open(QIODevice::WriteOnly | QIODevice::Text);
    success = success && ok;
    lcmStructs.write(lcmStructDefs.toLatin1());

    return success;
}
