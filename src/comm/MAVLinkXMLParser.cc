#include <QFile>
#include <QDir>
#include <QPair>
#include <QList>
#include <QDateTime>
#include "MAVLinkXMLParser.h"

#include <QDebug>

MAVLinkXMLParser::MAVLinkXMLParser(QDomDocument* document, QString outputDirectory, QObject* parent) : QObject(parent),
doc(document),
outputDirName(outputDirectory)
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
    outputDirName = outputDirectory;
}

MAVLinkXMLParser::~MAVLinkXMLParser()
{
}

bool MAVLinkXMLParser::generate()
{
    // Process result
    bool success = true;

    // Only generate if output dir is correctly set
    if (outputDirName == "") return false;

    // print out the element names of all elements that are direct children
    // of the outermost element.
    QDomElement docElem = doc->documentElement();

    QList<QString> parseErrors();
    QDomNode n = docElem.firstChild();

    /*
    // Seek for element "messages" until end of document
    // ignoring all other tags
    while(!n.isNull())
    {
        if (n.toElement().tagName() == "messages")
        {
            break;
        }
        else
        {
            qDebug() << "IGNORED TAG" << n.toElement().tagName();
            n = n.nextSibling();
        }
    }

    qDebug() << "WORKING ON" << n.toElement().tagName();
*/
    QList< QPair<QString, QString> > cFiles;
    QString lcmStructDefs = "";

    while(!n.isNull()) {
        // Each child is a message
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {
            // Handle all message tags
            if (e.tagName() == "message")
            {
                // Get message name
                QString messageName = e.attribute("name", "").toLower();
                if (messageName.size() == 0)
                {
                    //parseErrors.append(tr("Missing name attribute at line "));
                }
                else
                {
                    // Get message id
                    bool ok;
                    int messageId = e.attribute("id", "-1").toInt(&ok, 10);

                    QString channelType = "mavlink_channel_t";
                    QString messageType = "mavlink_message_t";

                    // Build up function call
                    QString commentContainer = "/**\n * @brief Send a %1 message\n *\n%2 * @return length of the message in bytes (excluding serial stream start sign)\n */\n";
                    QString commentEntry = " * @param %1 %2\n";
                    QString idDefine = QString("#define MAVLINK_MSG_ID_%1 %2").arg(messageName.toUpper(), QString::number(messageId));
                    QString cStructName = QString("%1_t").arg(messageName);
                    QString cStruct = "typedef struct __%1 \n{\n%2\n} %1;";
                    QString cStructLines = "";
                    QString encode = "static inline uint16_t message_%1_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const %2* %1)\n{\n\treturn message_%1_pack(%3);\n}\n";

                    QString decode = "static inline void message_%1_decode(const mavlink_message_t* msg, %2* %1)\n{\n%3}\n";
                    QString pack = "static inline uint16_t message_%1_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg%2)\n{\n\tmsg->msgid = MAVLINK_MSG_ID_%3;\n\tuint16_t i = 0;\n\n%4\n\treturn finalize_message(msg, system_id, component_id, i);\n}\n\n";
                    QString compactSend = "#if !defined(_WIN32) && !defined(__linux) && !defined(__APPLE__)\n\n#include \"global_data.h\"\n\nstatic inline void message_%3_send(%1 chan%5)\n{\n\t%2 msg;\n\tmessage_%3_pack(global_data.param[PARAM_SYSTEM_ID], global_data.param[PARAM_COMPONENT_ID], &msg%4);\n\tmavlink_send_uart(chan, &msg);\n}\n\n#endif";
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
                        if (!e2.isNull())
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
                                decodeLines += QString("\tmessage_%1_get_%2(msg, %1->%2);\n").arg(messageName, fieldName);
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
                                decodeLines += QString("\t%1->%2 = message_%1_get_%2(msg);\n").arg(messageName, fieldName);
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

                            // Generate the message decoding function
                            // Array handling is different from simple types
                            if (fieldType.startsWith("array"))
                            {
                                unpacking += unpackingComment + QString("static inline uint16_t message_%1_get_%2(const mavlink_message_t* msg, int8_t* r_data)\n{\n%4\n}\n\n").arg(messageName, fieldName, unpackingCode);
                                decodeLines += "";
                                QString arrayLength = QString(fieldType.split("[").at(1).split("]").first());
                                prepends += "+" + arrayLength;
                            }
                            else
                            {
                                unpacking += unpackingComment + QString("static inline %1 message_%2_get_%3(const mavlink_message_t* msg)\n{\n%4\n}\n\n").arg(fieldType, messageName, fieldName, unpackingCode);
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
                    QString cFile = "// MESSAGE " + messageName.toUpper() + " PACKING\n\n" + idDefine + "\n\n" + cStruct + "\n\n" + commentContainer.arg(messageName.toLower(), commentLines) + pack + encode + "\n" + compactSend + "\n" + "// MESSAGE " + messageName.toUpper() + " UNPACKING\n\n" + unpacking + decode;
                    cFiles.append(qMakePair(QString("mavlink_message_%1.h").arg(messageName), cFile));
                }
            }
        }
        n = n.nextSibling();
    }

    // XML parsed and converted to C code. Now generating the files
    QDateTime now = QDateTime::currentDateTime().toUTC();
    QString dateFormat = "dddd, MMMM d yyyy, hh:mm UTC";
    QString date = now.toString(dateFormat);
    QString mainHeader = QString("/** @file\n *\t@brief MAVLink comm protocol.\n *\t@see http://pixhawk.ethz.ch/software/mavlink\n *\t Generated on %1\n */\n#ifndef MAVLINK_H\n#define MAVLINK_H\n\n").arg(date); // The main header includes all messages
    // Mark all code as C code
    mainHeader += "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";
    mainHeader += "\n#include \"protocol.h\"\n";
    QString includeLine = "#include \"%1\"\n";
    QString mainHeaderName = "mavlink.h";
    QString messagesDirName = "generated";
    QDir dir(outputDirName + "/" + messagesDirName);
    // Create directory if it doesn't exist, report result in success
    if (!dir.exists()) success = success && dir.mkdir(outputDirName);
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
