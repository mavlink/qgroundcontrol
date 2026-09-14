#pragma once

#include "QGCCorePlugin.h"

class DigiviewManager;
class SVBackend;

class CustomPlugin final : public QGCCorePlugin
{
    Q_OBJECT
    Q_PROPERTY(int svInitialWelcomePromptId MEMBER kSVInitialWelcomePromptId CONSTANT)

public:
    explicit CustomPlugin(QObject* parent = nullptr);

    static QGCCorePlugin* instance();

    SVBackend* backend() const { return _backend; }
    QString firstRunPromptResource(int id) const final;

    static constexpr int kSVInitialWelcomePromptId = kFirstRunPromptIdsFirstCustomId + 1;

private:
    DigiviewManager* _digiviewManager = nullptr;
    SVBackend* _backend = nullptr;
};
