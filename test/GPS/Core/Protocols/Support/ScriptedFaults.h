#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {

/// Matching commands get a scripted reply, silence or write result instead of reaching the model, or reach it; either
/// way a hook may run after them. Rules match in order; a rule applies to its first..last matching occurrences.
class ScriptedFaults final : public ForwardingModel
{
public:
    struct Rule
    {
        std::function<bool(const QByteArray&)> matches;
        QByteArray reply = {};
        /// The result of the command's write, which the link fails or truncates; empty, the write completes.
        std::function<GPSWriteResult(const QByteArray&)> write = {};
        bool forward = false;
        std::function<void(ScriptedReceiver&)> after = {};
        /// Models that answer only the latest command drop earlier queued replies.
        bool clearReplies = false;
        int first = 1;
        int last = std::numeric_limits<int>::max();
        int seen = 0;
    };

    using ForwardingModel::ForwardingModel;

    std::vector<Rule> rules;

    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        for (auto& rule : rules) {
            if (!rule.matches(command)) {
                continue;
            }
            const int occurrence = ++rule.seen;
            if (occurrence < rule.first || occurrence > rule.last) {
                continue;
            }
            GPSWriteResult result;
            if (rule.forward) {
                result = _model.handleCommand(receiver, command, context);
            } else {
                if (rule.clearReplies) {
                    receiver.clearReplies();
                }
                if (!rule.reply.isEmpty()) {
                    receiver.queueReply(rule.reply);
                }
                const int size = static_cast<int>(command.size());
                result = rule.write ? rule.write(command) : GPSWriteResult{GPSWriteStatus::Completed, size, size};
            }
            if (rule.after) {
                rule.after(receiver);
            }
            return result;
        }
        return _model.handleCommand(receiver, command, context);
    }
};

inline std::function<bool(const QByteArray&)> startsWith(QByteArray prefix)
{
    return [prefix = std::move(prefix)](const QByteArray& command) { return command.startsWith(prefix); };
}

inline ScriptedFaults::Rule reply(std::function<bool(const QByteArray&)> matches, QByteArray bytes, int first = 1,
                                  int last = std::numeric_limits<int>::max())
{
    return {.matches = std::move(matches), .reply = std::move(bytes), .first = first, .last = last};
}

inline ScriptedFaults::Rule silence(QByteArray prefix)
{
    return {.matches = startsWith(std::move(prefix))};
}

/// The link fails or truncates the write of a command starting with @a prefix, as @a result says; the receiver never
/// sees it.
inline ScriptedFaults::Rule writeFails(QByteArray prefix, std::function<GPSWriteResult(const QByteArray&)> result)
{
    return {.matches = startsWith(std::move(prefix)), .write = std::move(result)};
}

inline ScriptedFaults::Rule afterCommand(QByteArray prefix, std::function<void(ScriptedReceiver&)> hook)
{
    return {.matches = startsWith(std::move(prefix)), .forward = true, .after = std::move(hook)};
}

/// Septentrio and Femtomes models answer only the latest command.
inline ScriptedFaults::Rule latestReply(QByteArray prefix, QByteArray bytes, int first = 1,
                                        int last = std::numeric_limits<int>::max())
{
    auto rule = reply(startsWith(std::move(prefix)), std::move(bytes), first, last);
    rule.clearReplies = true;
    return rule;
}

}  // namespace GPSTest
