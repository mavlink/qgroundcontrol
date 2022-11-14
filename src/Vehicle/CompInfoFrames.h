#ifndef COMPINFOFRAMES_H
#define COMPINFOFRAMES_H

#include <QObject>

#include "CompInfo.h"

class FramesBase;

class CompInfoFrames : public CompInfo
{
    Q_OBJECT

public:
    CompInfoFrames(uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName, const QString& translationJsonFileName) override;
};
#endif // COMPINFOFRAMES_H
