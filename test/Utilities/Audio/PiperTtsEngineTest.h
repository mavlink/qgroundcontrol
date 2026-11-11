#pragma once

#include <QtCore/QTemporaryDir>

#include "UnitTest.h"

/// Tests PiperTtsEngine with stand-ins for Piper: shell scripts on a private PATH and voice files that hold no
/// model. The stand-ins write empty WAV files, so nothing is ever played.
class PiperTtsEngineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testNoExecutable();
    void _testExecutableIsNotPiper();
    void _testSkipsUnusableCandidate_data();
    void _testSkipsUnusableCandidate();
    void _testStartsPiperWithCurrentVoice();
    void _testPiperExitIsAnError();
    void _testChangingLocaleRestartsPiper();
    void _testSaysEachTextAsOneLine();
    void _testStopDiscardsCanceledUtterances();

private:
    QString _modelPath(const QString& voiceName) const;

    /// Holds the piper-voices the engine finds. The engine looks for voices once in the life of the process, so
    /// the directory lives as long as the suite.
    QTemporaryDir _dataDir;
};
