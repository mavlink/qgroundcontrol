/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AudioOutputTest.h"
#include "AudioOutput.h"

AudioOutputTest::AudioOutputTest(void)
{

}

void AudioOutputTest::_testSpokenReplacements(void)
{
    QString result = AudioOutput::fixTextMessageForAudio(QStringLiteral("-10.5m, -10.5m. -10.5 m"));
    QCOMPARE(result, QStringLiteral(" negative 10 point 5 meters,  negative 10 point 5 meters.  negative 10 point 5  meters"));
    result = AudioOutput::fixTextMessageForAudio(QStringLiteral("-10m -10 m"));
    QCOMPARE(result, QStringLiteral(" negative 10 meters  negative 10  meters"));
    result = AudioOutput::fixTextMessageForAudio(QStringLiteral("foo -10m -10 m bar"));
    QCOMPARE(result, QStringLiteral("foo  negative 10 meters  negative 10  meters bar"));
    result = AudioOutput::fixTextMessageForAudio(QStringLiteral("-foom"));
    QCOMPARE(result, QStringLiteral("-foom"));
    result = AudioOutput::fixTextMessageForAudio(QStringLiteral("10 moo"));
    QCOMPARE(result, QStringLiteral("10 moo"));
    result = AudioOutput::fixTextMessageForAudio(QStringLiteral("10moo"));
    QCOMPARE(result, QStringLiteral("10moo"));
}
