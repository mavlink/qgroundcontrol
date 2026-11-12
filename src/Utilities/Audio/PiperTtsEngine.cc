#include "PiperTtsEngine.h"

#include <algorithm>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QSoundEffect>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(PiperTtsEngineLog, "Utilities.PiperTtsEngine")

PiperTtsEngine::PiperTtsEngine(QObject* parent)
    : QTextToSpeechEngine(parent)
    , _candidates(_findCandidates())
    , _voices(_findVoices())
    , _player(new QSoundEffect(this))
{
    (void) connect(_player, &QSoundEffect::statusChanged, this, [this]() {
        // stop() may have canceled the utterance while its WAV was loading.
        if (_playingFile.isEmpty()) {
            return;
        }
        if (_player->status() == QSoundEffect::Ready) {
            _player->play();
        } else if (_player->status() == QSoundEffect::Error) {
            qCWarning(PiperTtsEngineLog) << "Failed to play" << _playingFile;
            _finishUtterance();
        }
    });
    (void) connect(_player, &QSoundEffect::playingChanged, this, [this]() {
        if (!_player->isPlaying()) {
            _finishUtterance();
        }
    });

    if (_candidates.isEmpty() || _voices.isEmpty()) {
        _setError(QTextToSpeech::ErrorReason::Initialization, tr("Piper or a Piper voice is not installed"));
        return;
    }
    if (!_outputDir.isValid()) {
        _setError(QTextToSpeech::ErrorReason::Initialization, _outputDir.errorString());
        return;
    }

    const auto systemVoice = std::ranges::find(_voices, QLocale::system(), &QVoice::locale);
    _voice = (systemVoice != _voices.cend()) ? *systemVoice : _voices.constFirst();
    _verifyNextCandidate();
}

PiperTtsEngine::~PiperTtsEngine()
{
    if (_probe) {
        (void) _probe->disconnect(this);
        _probe->kill();
        (void) _probe->waitForFinished(1000);
    }
    _stopProcess();
}

bool PiperTtsEngine::isAvailable()
{
    // Called while the application starts, so no candidate is run here: the engine verifies them once it is created.
    return !_findCandidates().isEmpty() && !_findVoices().isEmpty();
}

QStringList PiperTtsEngine::_findCandidates()
{
    // Desktop launchers often don't have ~/.local/bin in PATH, which is where pip/uv/pipx install Piper.
    const QStringList fallbackPaths = {QDir::homePath() + QStringLiteral("/.local/bin")};

    // Some distributions name it piper-tts because "piper" is also a mouse configuration tool.
    QStringList candidates;
    for (const QString& name : {QStringLiteral("piper-tts"), QStringLiteral("piper")}) {
        candidates << QStandardPaths::findExecutable(name) << QStandardPaths::findExecutable(name, fallbackPaths);
    }
    (void) candidates.removeAll(QString());
    (void) candidates.removeDuplicates();
    return candidates;
}

void PiperTtsEngine::_verifyNextCandidate()
{
    if (_candidates.isEmpty()) {
        _setError(QTextToSpeech::ErrorReason::Initialization, tr("Piper or a Piper voice is not installed"));
        return;
    }

    const QString path = _candidates.takeFirst();
    _probe = new QProcess(this);
    _probe->setProcessChannelMode(QProcess::MergedChannels);
    (void) connect(_probe, &QProcess::finished, this, [this, path](int, QProcess::ExitStatus exitStatus) {
        _candidateVerified(path, (exitStatus == QProcess::NormalExit) && _probe->readAll().contains("--model"));
    });
    (void) connect(_probe, &QProcess::errorOccurred, this, [this, path](QProcess::ProcessError error) {
        // The only error that ends the process without finished().
        if (error == QProcess::FailedToStart) {
            _candidateVerified(path, false);
        }
    });
    QTimer::singleShot(kVerifyTimeout, _probe, &QProcess::kill);
    _probe->start(path, {QStringLiteral("--help")});
}

void PiperTtsEngine::_candidateVerified(const QString& path, bool isPiperTts)
{
    (void) _probe->disconnect(this);
    _probe->deleteLater();
    _probe = nullptr;

    if (!isPiperTts) {
        qCDebug(PiperTtsEngineLog) << "Skipping" << path << "which is not Piper text-to-speech";
        _verifyNextCandidate();
        return;
    }

    _executable = path;
    _startProcess();
    if (!_unsentInput.isEmpty() && (_process->state() != QProcess::NotRunning)) {
        _pendingCount = static_cast<int>(_unsentInput.count('\n'));
        (void) _process->write(_unsentInput);
    }
    _unsentInput.clear();
}

QList<QVoice> PiperTtsEngine::_findVoices()
{
    // Cached because the voices are needed both to choose this engine and to create it.
    static const QList<QVoice> voices = _searchVoices();
    return voices;
}

QList<QVoice> PiperTtsEngine::_searchVoices()
{
    QList<QVoice> voices;
    QStringList names;

    const QStringList voiceDirs = QStandardPaths::locateAll(
        QStandardPaths::GenericDataLocation, QStringLiteral("piper-voices"), QStandardPaths::LocateDirectory);
    for (const QString& voiceDir : voiceDirs) {
        QDirIterator it(voiceDir, {QStringLiteral("*.onnx")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo model(it.next());
            const QString name = model.completeBaseName();
            if (names.contains(name)) {
                continue;
            }

            QFile configFile(model.filePath() + QStringLiteral(".json"));
            if (!configFile.open(QIODevice::ReadOnly)) {
                qCDebug(PiperTtsEngineLog) << "Skipping voice without config:" << model.filePath();
                continue;
            }
            const QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
            const QString languageCode =
                config.value(QStringLiteral("language")).toObject().value(QStringLiteral("code")).toString();
            if (languageCode.isEmpty()) {
                qCDebug(PiperTtsEngineLog) << "Skipping voice without language code:" << model.filePath();
                continue;
            }

            names.append(name);
            voices.append(createVoice(name, QLocale(languageCode), QVoice::Unknown, QVoice::Other, model.filePath()));
        }
    }

    std::ranges::sort(voices, {}, &QVoice::name);
    return voices;
}

void PiperTtsEngine::_startProcess()
{
    _stopProcess();
    if (_executable.isEmpty()) {
        // Still verifying the candidates. Piper is started with the current voice once one is verified.
        return;
    }

    _process = new QProcess(this);
    // Python Piper logs written files to stderr, the older C++ Piper prints them to stdout.
    _process->setProcessChannelMode(QProcess::MergedChannels);
    (void) connect(_process, &QProcess::readyReadStandardOutput, this, &PiperTtsEngine::_readProcessOutput);
    (void) connect(_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        _setError(QTextToSpeech::ErrorReason::Initialization, _process->errorString());
    });
    (void) connect(_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
        _setError(QTextToSpeech::ErrorReason::Initialization, tr("Piper exited with code %1").arg(exitCode));
    });

    const QString modelPath = voiceData(_voice).toString();
    qCDebug(PiperTtsEngineLog) << "Starting" << _executable << "with voice" << modelPath;
    _process->start(_executable, {QStringLiteral("-m"), modelPath, QStringLiteral("-d"), _outputDir.path()});
}

void PiperTtsEngine::_stopProcess()
{
    _pendingCount = 0;
    _discardCount = 0;
    if (!_process) {
        return;
    }

    (void) _process->disconnect(this);
    _process->closeWriteChannel();
    if (!_process->waitForFinished(1000)) {
        _process->kill();
        (void) _process->waitForFinished(1000);
    }
    _process->deleteLater();
    _process = nullptr;
}

void PiperTtsEngine::_readProcessOutput()
{
    while (_process->canReadLine()) {
        const QString line = QString::fromUtf8(_process->readLine()).trimmed();
        const qsizetype pathStart = line.indexOf(_outputDir.path());
        if ((pathStart < 0) || !line.endsWith(QStringLiteral(".wav"))) {
            qCDebug(PiperTtsEngineLog) << "Piper:" << line;
            continue;
        }

        const QString path = line.mid(pathStart);
        // Piper answers in order, so canceled utterances come before the current ones.
        if (_discardCount > 0) {
            _discardCount--;
            (void) QFile::remove(path);
            continue;
        }
        if (_pendingCount > 0) {
            _pendingCount--;
        }
        _playWavFile(path);
    }
}

void PiperTtsEngine::_playWavFile(const QString& path)
{
    if (!_playingFile.isEmpty()) {
        (void) QFile::remove(_playingFile);
    }
    _playingFile = path;
    _player->setSource(QUrl::fromLocalFile(path));
}

void PiperTtsEngine::_finishUtterance()
{
    if (!_playingFile.isEmpty()) {
        (void) QFile::remove(_playingFile);
        _playingFile.clear();
    }
    if ((_state == QTextToSpeech::State::Speaking) && (_pendingCount == 0)) {
        _setState(QTextToSpeech::State::Ready);
    }
}

void PiperTtsEngine::_setState(QTextToSpeech::State state)
{
    if (_state != state) {
        _state = state;
        emit stateChanged(_state);
    }
}

void PiperTtsEngine::_setError(QTextToSpeech::ErrorReason reason, const QString& errorString)
{
    qCWarning(PiperTtsEngineLog) << errorString;
    _errorReason = reason;
    _errorString = errorString;
    emit errorOccurred(_errorReason, _errorString);
    _setState(QTextToSpeech::State::Error);
}

QTextToSpeech::Capabilities PiperTtsEngine::capabilities() const
{
    return QTextToSpeech::Capability::Speak;
}

QList<QLocale> PiperTtsEngine::availableLocales() const
{
    QList<QLocale> locales;
    for (const QVoice& voice : _voices) {
        if (!locales.contains(voice.locale())) {
            locales.append(voice.locale());
        }
    }
    return locales;
}

QList<QVoice> PiperTtsEngine::availableVoices() const
{
    QList<QVoice> voices;
    for (const QVoice& voice : _voices) {
        if (voice.locale() == _voice.locale()) {
            voices.append(voice);
        }
    }
    return voices;
}

void PiperTtsEngine::say(const QString& text)
{
    if (!_probe && (!_process || (_process->state() == QProcess::NotRunning))) {
        return;
    }

    // Piper treats each line as one utterance.
    const QString line = text.simplified();
    _setState(QTextToSpeech::State::Speaking);
    if (line.isEmpty()) {
        // Nothing to say, but an earlier utterance may still be waiting for Piper or playing
        if ((_pendingCount == 0) && _unsentInput.isEmpty() && _playingFile.isEmpty()) {
            _setState(QTextToSpeech::State::Ready);
        }
        return;
    }

    if (_probe) {
        // Written to Piper once a candidate is verified.
        _unsentInput.append(line.toUtf8() + '\n');
        return;
    }

    _pendingCount++;
    (void) _process->write(line.toUtf8() + '\n');
}

void PiperTtsEngine::synthesize(const QString& text)
{
    Q_UNUSED(text);
    qCWarning(PiperTtsEngineLog) << "synthesize() is not supported";
}

void PiperTtsEngine::stop(QTextToSpeech::BoundaryHint boundaryHint)
{
    Q_UNUSED(boundaryHint);
    _unsentInput.clear();
    _discardCount += _pendingCount;
    _pendingCount = 0;
    _player->stop();
    _finishUtterance();
}

void PiperTtsEngine::pause(QTextToSpeech::BoundaryHint boundaryHint)
{
    Q_UNUSED(boundaryHint);
}

void PiperTtsEngine::resume() {}

double PiperTtsEngine::rate() const
{
    return 0.0;
}

bool PiperTtsEngine::setRate(double rate)
{
    Q_UNUSED(rate);
    return false;
}

double PiperTtsEngine::pitch() const
{
    return 0.0;
}

bool PiperTtsEngine::setPitch(double pitch)
{
    Q_UNUSED(pitch);
    return false;
}

QLocale PiperTtsEngine::locale() const
{
    return _voice.locale();
}

bool PiperTtsEngine::setLocale(const QLocale& locale)
{
    const auto it = std::ranges::find(_voices, locale, &QVoice::locale);
    if (it == _voices.cend()) {
        return false;
    }
    return setVoice(*it);
}

double PiperTtsEngine::volume() const
{
    return _volume;
}

bool PiperTtsEngine::setVolume(double volume)
{
    _volume = std::clamp(volume, 0.0, 1.0);
    _player->setVolume(static_cast<float>(_volume));
    return true;
}

QVoice PiperTtsEngine::voice() const
{
    return _voice;
}

bool PiperTtsEngine::setVoice(const QVoice& voice)
{
    if (!_voices.contains(voice)) {
        return false;
    }
    if (voice == _voice) {
        return true;
    }

    _voice = voice;
    stop(QTextToSpeech::BoundaryHint::Immediate);
    _startProcess();
    return true;
}

QTextToSpeech::State PiperTtsEngine::state() const
{
    return _state;
}

QTextToSpeech::ErrorReason PiperTtsEngine::errorReason() const
{
    return _errorReason;
}

QString PiperTtsEngine::errorString() const
{
    return _errorString;
}
