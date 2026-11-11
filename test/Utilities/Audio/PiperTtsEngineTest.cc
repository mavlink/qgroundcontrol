#include "PiperTtsEngineTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtTest/QSignalSpy>
#include <QtTextToSpeech/QVoice>

#include "PiperTtsEngine.h"
#include "TestFixtures.h"

namespace {

constexpr char kPiperLogCategory[] = "Utilities.PiperTtsEngine";

bool writePiperTestFile(const QString& path, const QByteArray& content, bool executable = false)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || (file.write(content) != content.size())) {
        return false;
    }
    file.close();
    return !executable || file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

QByteArray readPiperTestFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/// What a stand-in does once the engine has started it with a voice.
enum class PiperStandIn
{
    Speaks,  ///< Names a WAV file for each line it reads, as Piper does
    Exits,   ///< Exits at once with code 3, as Piper does when it cannot load the voice
};

/// A directory that is the whole PATH for as long as this lives, with the stand-ins for Piper the test puts in it
/// and what they record.
class PiperSearchPath
{
public:
    PiperSearchPath()
        : _restorePath("PATH")
    {
        (void) qputenv("PATH", _dir.path().toUtf8());
    }

    bool isValid() const { return _dir.isValid(); }

    /// Installs a stand-in that answers --help as Piper does.
    bool installPiper(const QString& name = QStringLiteral("piper-tts"), PiperStandIn behaviour = PiperStandIn::Speaks)
    {
        // Only shell built-ins: PATH holds nothing else.
        QByteArray script =
            "#!/bin/sh\n"
            "if [ \"$1\" = \"--help\" ]; then\n"
            "    echo \"usage: piper [-h] -m MODEL [-d OUTPUT_DIR]\"\n"
            "    echo \"  -m MODEL, --model MODEL\"\n"
            "    exit 0\n"
            "fi\n"
            "printf '%s\\n' \"$0\" \"$@\" > '" +
            recordPath("arguments") + "'\n";
        if (behaviour == PiperStandIn::Exits) {
            script += "exit 3\n";
        } else {
            script +=
                "count=0\n"
                "while IFS= read -r line; do\n"
                "    count=$((count + 1))\n"
                "    : > \"$4/$count.wav\"\n"
                "    printf '%s\\n' \"$line\" >> '" +
                recordPath("utterances") +
                "'\n"
                "    echo \"Wrote $4/$count.wav\"\n"
                "done\n";
        }
        return writePiperTestFile(_dir.filePath(name), script, true);
    }

    /// Installs a program with Piper's name that is something else.
    bool installOtherProgram(const QString& name)
    {
        return writePiperTestFile(_dir.filePath(name), "#!/bin/sh\necho \"usage: piper [--version]\"\n", true);
    }

    /// Installs an executable file that cannot be started.
    bool installBrokenProgram(const QString& name)
    {
        return writePiperTestFile(_dir.filePath(name), "#!/nonexistent/interpreter\n", true);
    }

    /// The path and the arguments of the stand-in the engine last started with a voice.
    QStringList arguments() const
    {
        return QString::fromUtf8(readPiperTestFile(_dir.filePath(QStringLiteral("arguments"))))
            .split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }

    /// The lines the stand-in has read.
    QStringList utterances() const
    {
        return QString::fromUtf8(readPiperTestFile(_dir.filePath(QStringLiteral("utterances"))))
            .split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }

    QString filePath(const QString& name) const { return _dir.filePath(name); }

private:
    QByteArray recordPath(const char* name) const { return _dir.filePath(QLatin1StringView(name)).toUtf8(); }

    QTemporaryDir _dir;
    TestFixtures::EnvVarFixture _restorePath;
};

/// True if a program with one of Piper's names is installed where the engine looks besides PATH.
bool piperInstalledForUser()
{
    const QStringList userPaths = {QDir::homePath() + QStringLiteral("/.local/bin")};
    return !QStandardPaths::findExecutable(QStringLiteral("piper-tts"), userPaths).isEmpty() ||
           !QStandardPaths::findExecutable(QStringLiteral("piper"), userPaths).isEmpty();
}

}  // namespace

void PiperTtsEngineTest::initTestCase()
{
    UnitTest::initTestCase();

    // The engine looks for voices once in the life of the process. Let it find these, and only these.
    QVERIFY(_dataDir.isValid());
    const QDir dataDir(_dataDir.path());
    QVERIFY(dataDir.mkpath(QStringLiteral("empty")));
    QVERIFY(dataDir.mkpath(QStringLiteral("share/piper-voices/unusable")));
    for (const QString& locale : {QStringLiteral("en_US"), QStringLiteral("de_DE")}) {
        const QString model = _modelPath(locale + QStringLiteral("-test"));
        QVERIFY(writePiperTestFile(model, "not a model"));
        QVERIFY(writePiperTestFile(model + QStringLiteral(".json"),
                                   QStringLiteral(R"({"language": {"code": "%1"}})").arg(locale).toUtf8()));
    }
    QVERIFY(writePiperTestFile(_modelPath(QStringLiteral("unusable/no-config")), "not a model"));
    QVERIFY(writePiperTestFile(_modelPath(QStringLiteral("unusable/no-language")), "not a model"));
    QVERIFY(writePiperTestFile(_modelPath(QStringLiteral("unusable/no-language")) + QStringLiteral(".json"), "{}"));

    const TestFixtures::EnvVarFixture restoreDataHome("XDG_DATA_HOME");
    const TestFixtures::EnvVarFixture restoreDataDirs("XDG_DATA_DIRS");
    QVERIFY(qputenv("XDG_DATA_HOME", dataDir.filePath(QStringLiteral("empty")).toUtf8()));
    QVERIFY(qputenv("XDG_DATA_DIRS", dataDir.filePath(QStringLiteral("share")).toUtf8()));

    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(searchPath.installPiper());

    // The first engine's player makes Qt look for an audio service, and a host without one logs warnings about
    // that. Create it here, where the log is not checked.
    const PiperTtsEngine engine;
    // Sorted by voice name. A model without a configuration or a language is not a voice.
    const QList<QLocale> locales{QLocale(QStringLiteral("de_DE")), QLocale(QStringLiteral("en_US"))};
    if (engine.availableLocales() != locales) {
        QSKIP("Another suite in this process made the engine look for voices before this one");
    }
}

QString PiperTtsEngineTest::_modelPath(const QString& voiceName) const
{
    return _dataDir.filePath(QStringLiteral("share/piper-voices/%1.onnx").arg(voiceName));
}

void PiperTtsEngineTest::_testNoExecutable()
{
    if (piperInstalledForUser()) {
        QSKIP("Piper is installed in this user's ~/.local/bin");
    }

    const PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(!PiperTtsEngine::isAvailable());

    expectLogMessage(kPiperLogCategory, QtWarningMsg, QRegularExpression(QStringLiteral("not installed")));
    PiperTtsEngine engine;
    verifyExpectedLogMessage();

    QCOMPARE(engine.state(), QTextToSpeech::State::Error);
    QCOMPARE(engine.errorReason(), QTextToSpeech::ErrorReason::Initialization);
    QVERIFY(!engine.errorString().isEmpty());

    // There is nothing to say it with, and trying does not hide the error.
    engine.say(QStringLiteral("text"));
    QCOMPARE(engine.state(), QTextToSpeech::State::Error);
}

void PiperTtsEngineTest::_testExecutableIsNotPiper()
{
    if (piperInstalledForUser()) {
        QSKIP("Piper is installed in this user's ~/.local/bin");
    }

    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(searchPath.installOtherProgram(QStringLiteral("piper-tts")));
    QVERIFY(searchPath.installOtherProgram(QStringLiteral("piper")));
    // Both have the right name, which is all isAvailable() checks.
    QVERIFY(PiperTtsEngine::isAvailable());

    PiperTtsEngine engine;
    QSignalSpy errorSpy(&engine, &QTextToSpeechEngine::errorOccurred);
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);
    engine.say(QStringLiteral("text"));
    QCOMPARE(engine.state(), QTextToSpeech::State::Speaking);

    expectLogMessage(kPiperLogCategory, QtWarningMsg, QRegularExpression(QStringLiteral("not installed")));
    QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QCOMPARE(engine.state(), QTextToSpeech::State::Error);
    QCOMPARE(engine.errorReason(), QTextToSpeech::ErrorReason::Initialization);
    QVERIFY(!engine.errorString().isEmpty());
}

void PiperTtsEngineTest::_testSkipsUnusableCandidate_data()
{
    QTest::addColumn<bool>("startable");

    QTest::newRow("cannot be started") << false;
    QTest::newRow("another program") << true;
}

void PiperTtsEngineTest::_testSkipsUnusableCandidate()
{
    QFETCH(bool, startable);
    if (piperInstalledForUser()) {
        QSKIP("Piper is installed in this user's ~/.local/bin");
    }

    // piper-tts is tried before piper.
    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(startable ? searchPath.installOtherProgram(QStringLiteral("piper-tts"))
                      : searchPath.installBrokenProgram(QStringLiteral("piper-tts")));
    QVERIFY(searchPath.installPiper(QStringLiteral("piper")));

    const PiperTtsEngine engine;
    QTRY_VERIFY_WITH_TIMEOUT(!searchPath.arguments().isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(searchPath.arguments().constFirst(), searchPath.filePath(QStringLiteral("piper")));
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);
}

void PiperTtsEngineTest::_testStartsPiperWithCurrentVoice()
{
    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(searchPath.installPiper());

    const PiperTtsEngine engine;
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);
    QTRY_COMPARE_WITH_TIMEOUT(searchPath.arguments().size(), 5, TestTimeout::mediumMs());

    // The system locale's voice if there is one, otherwise the first.
    const QVoice voice = engine.voice();
    QCOMPARE(engine.locale(), voice.locale());
    QCOMPARE(engine.availableVoices(), QList<QVoice>{voice});
    if (voice.locale() != QLocale::system()) {
        QCOMPARE(voice.name(), QStringLiteral("de_DE-test"));
    }

    const QStringList arguments = searchPath.arguments();
    QCOMPARE(arguments.at(0), searchPath.filePath(QStringLiteral("piper-tts")));
    QCOMPARE(arguments.at(1), QStringLiteral("-m"));
    QCOMPARE(arguments.at(2), _modelPath(voice.name()));
    QCOMPARE(arguments.at(3), QStringLiteral("-d"));
    QVERIFY(QDir(arguments.at(4)).exists());
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);
}

void PiperTtsEngineTest::_testPiperExitIsAnError()
{
    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(searchPath.installPiper(QStringLiteral("piper-tts"), PiperStandIn::Exits));

    PiperTtsEngine engine;
    QSignalSpy errorSpy(&engine, &QTextToSpeechEngine::errorOccurred);
    expectLogMessage(kPiperLogCategory, QtWarningMsg, QRegularExpression(QStringLiteral("Piper exited with code 3")));
    QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QCOMPARE(engine.state(), QTextToSpeech::State::Error);
    QCOMPARE(engine.errorReason(), QTextToSpeech::ErrorReason::Initialization);
    QVERIFY(engine.errorString().contains(QLatin1Char('3')));

    engine.say(QStringLiteral("text"));
    QCOMPARE(engine.state(), QTextToSpeech::State::Error);
}

void PiperTtsEngineTest::_testChangingLocaleRestartsPiper()
{
    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(searchPath.installPiper());

    PiperTtsEngine engine;
    QTRY_COMPARE_WITH_TIMEOUT(searchPath.arguments().value(2), _modelPath(engine.voice().name()),
                              TestTimeout::mediumMs());

    QVERIFY(!engine.setLocale(QLocale(QStringLiteral("fr_FR"))));
    QVERIFY(!engine.setVoice(QVoice()));

    const QLocale german(QStringLiteral("de_DE"));
    const QLocale other = (engine.locale() == german) ? QLocale(QStringLiteral("en_US")) : german;
    QVERIFY(engine.setLocale(other));
    QCOMPARE(engine.locale(), other);
    QCOMPARE(engine.voice().locale(), other);
    QCOMPARE(engine.availableVoices(), QList<QVoice>{engine.voice()});
    QTRY_COMPARE_WITH_TIMEOUT(searchPath.arguments().value(2), _modelPath(other.name() + QStringLiteral("-test")),
                              TestTimeout::mediumMs());
}

void PiperTtsEngineTest::_testSaysEachTextAsOneLine()
{
    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(searchPath.installPiper());
    // Qt's own complaint about the empty WAV files the stand-in names.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("^QSoundEffect")));

    // Said while the engine is still checking that the program is Piper: sent once Piper runs.
    PiperTtsEngine engine;
    expectLogMessage(kPiperLogCategory, QtWarningMsg, QRegularExpression(QStringLiteral("Failed to play .*/1\\.wav")));
    engine.say(QStringLiteral("  first\n utterance "));
    QCOMPARE(engine.state(), QTextToSpeech::State::Speaking);
    QTRY_COMPARE_WITH_TIMEOUT(engine.state(), QTextToSpeech::State::Ready, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(searchPath.utterances(), QStringList{QStringLiteral("first utterance")});

    // Text with nothing in it is not sent: the next utterance is Piper's second.
    engine.say(QStringLiteral(" \n "));
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);

    expectLogMessage(kPiperLogCategory, QtWarningMsg, QRegularExpression(QStringLiteral("Failed to play .*/2\\.wav")));
    engine.say(QStringLiteral("second"));
    QCOMPARE(engine.state(), QTextToSpeech::State::Speaking);
    QTRY_COMPARE_WITH_TIMEOUT(engine.state(), QTextToSpeech::State::Ready, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(searchPath.utterances(), (QStringList{QStringLiteral("first utterance"), QStringLiteral("second")}));
}

void PiperTtsEngineTest::_testStopDiscardsCanceledUtterances()
{
    PiperSearchPath searchPath;
    QVERIFY(searchPath.isValid());
    QVERIFY(searchPath.installPiper());
    // Qt's own complaint about the empty WAV files the stand-in names.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("^QSoundEffect")));

    // Canceled before Piper runs: never sent.
    PiperTtsEngine engine;
    engine.say(QStringLiteral("forgotten"));
    engine.stop(QTextToSpeech::BoundaryHint::Default);
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);
    QTRY_COMPARE_WITH_TIMEOUT(searchPath.arguments().size(), 5, TestTimeout::mediumMs());
    const QDir outputDir(searchPath.arguments().at(4));

    engine.say(QStringLiteral("one"));
    engine.say(QStringLiteral("two"));
    engine.stop(QTextToSpeech::BoundaryHint::Default);
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);

    // Piper still answers for the two it was sent. Their WAV files are deleted, not played.
    QTRY_COMPARE_WITH_TIMEOUT(searchPath.utterances().size(), 2, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(outputDir.entryList({QStringLiteral("*.wav")}, QDir::Files).isEmpty(),
                             TestTimeout::mediumMs());
    QCOMPARE(engine.state(), QTextToSpeech::State::Ready);

    // The next answer is for the next utterance, and is played.
    expectLogMessage(kPiperLogCategory, QtWarningMsg, QRegularExpression(QStringLiteral("Failed to play .*/3\\.wav")));
    engine.say(QStringLiteral("three"));
    QCOMPARE(engine.state(), QTextToSpeech::State::Speaking);
    QTRY_COMPARE_WITH_TIMEOUT(engine.state(), QTextToSpeech::State::Ready, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(searchPath.utterances(),
             (QStringList{QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three")}));
}

UT_REGISTER_TEST(PiperTtsEngineTest, TestLabel::Unit, TestLabel::Utilities)
