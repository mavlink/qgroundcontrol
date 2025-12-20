/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>

namespace QGC::Internal {

/**
 * @brief Completion provider for MAVLink message handling in QGC code
 *
 * Provides autocomplete for:
 * - MAVLINK_MSG_ID_* constants in switch statements
 * - Message decode patterns (mavlink_msg_xxx_decode)
 * - Common QGC MAVLink patterns
 */
class MAVLinkCompletionProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    explicit MAVLinkCompletionProvider(QObject *parent = nullptr);

    // CompletionAssistProvider interface
    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;

    // IAssistProvider interface
    TextEditor::IAssistProcessor *createProcessor(
        const TextEditor::AssistInterface *assistInterface) const override;
};

/**
 * @brief Processor for MAVLink completion requests
 */
class MAVLinkCompletionProcessor : public TextEditor::IAssistProcessor
{
public:
    MAVLinkCompletionProcessor();
    ~MAVLinkCompletionProcessor() override;

    // IAssistProcessor interface
    TextEditor::IAssistProposal *perform() override;

private:
    struct MAVLinkMessage {
        QString name;           // e.g., "HEARTBEAT"
        int id;                 // e.g., 0
        QString description;    // e.g., "Vehicle heartbeat"
        QStringList fields;     // e.g., {"type", "autopilot", "base_mode", ...}
    };

    void loadMAVLinkMessages();
    static bool loadFromXml(const QString &projectRoot);
    static QString findMavlinkDefinitions(const QString &projectRoot);
    static QList<MAVLinkMessage> parseMavlinkXml(const QString &xmlPath, const QString &defsDir);

    TextEditor::IAssistProposal *createMessageIdProposal(const QString &prefix);
    TextEditor::IAssistProposal *createDecodeProposal(const QString &prefix);
    TextEditor::IAssistProposal *createFieldProposal(const QString &messageName);

    QString getCurrentPrefix() const;
    QString getCurrentContext() const;

    static QList<MAVLinkMessage> s_messages;
    static bool s_messagesLoaded;
    static QString s_projectRoot;
};

} // namespace QGC::Internal
