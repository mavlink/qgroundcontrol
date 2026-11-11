#pragma once

#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtTextToSpeech/QTextToSpeechEngine>
#include <QtTextToSpeech/QVoice>

class QProcess;
class QSoundEffect;

/// Text-to-speech engine backed by a system-installed Piper (https://github.com/OHF-Voice/piper1-gpl).
/// Piper is GPL-licensed, so it is only ever run as a separate process and never linked or bundled.
/// One Piper process is kept running so the voice model is loaded once, not per utterance.
class PiperTtsEngine : public QTextToSpeechEngine
{
    Q_OBJECT

public:
    explicit PiperTtsEngine(QObject* parent = nullptr);
    ~PiperTtsEngine() override;

    /// True if an executable that may be Piper and at least one voice model are installed. The engine reports an
    /// error after it is created if none of the executables turns out to be Piper.
    [[nodiscard]] static bool isAvailable();

    QTextToSpeech::Capabilities capabilities() const override;
    QList<QLocale> availableLocales() const override;
    QList<QVoice> availableVoices() const override;

    void say(const QString& text) override;
    void synthesize(const QString& text) override;
    void stop(QTextToSpeech::BoundaryHint boundaryHint) override;
    void pause(QTextToSpeech::BoundaryHint boundaryHint) override;
    void resume() override;

    double rate() const override;
    bool setRate(double rate) override;
    double pitch() const override;
    bool setPitch(double pitch) override;
    QLocale locale() const override;
    bool setLocale(const QLocale& locale) override;
    double volume() const override;
    bool setVolume(double volume) override;
    QVoice voice() const override;
    bool setVoice(const QVoice& voice) override;
    QTextToSpeech::State state() const override;
    QTextToSpeech::ErrorReason errorReason() const override;
    QString errorString() const override;

private:
    static QStringList _findCandidates();
    static QList<QVoice> _findVoices();
    static QList<QVoice> _searchVoices();

    /// Runs the next candidate with --help, without blocking, to tell Piper from other programs with its name.
    void _verifyNextCandidate();
    void _candidateVerified(const QString& path, bool isPiperTts);
    void _startProcess();
    void _stopProcess();
    void _readProcessOutput();
    void _playWavFile(const QString& path);
    void _finishUtterance();
    void _setState(QTextToSpeech::State state);
    void _setError(QTextToSpeech::ErrorReason reason, const QString& errorString);

    static constexpr std::chrono::seconds kVerifyTimeout{5};

    QStringList _candidates;
    QString _executable;
    QList<QVoice> _voices;
    QVoice _voice;
    QTemporaryDir _outputDir;
    QProcess* _probe = nullptr;
    QProcess* _process = nullptr;
    QSoundEffect* _player = nullptr;
    QString _playingFile;
    QByteArray _unsentInput;
    int _pendingCount = 0;
    int _discardCount = 0;
    double _volume = 1.0;
    QTextToSpeech::State _state = QTextToSpeech::State::Ready;
    QTextToSpeech::ErrorReason _errorReason = QTextToSpeech::ErrorReason::NoError;
    QString _errorString;
};
