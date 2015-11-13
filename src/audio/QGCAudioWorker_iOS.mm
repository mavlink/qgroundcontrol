/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief TTS for iOS
 *
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include <QString>

#include "QGC.h"

#if (defined __ios__) && defined QGC_SPEECH_ENABLED

#import <AVFoundation/AVSpeechSynthesis.h>

class SpeakIOS
{
public:
    SpeakIOS    ();
    ~SpeakIOS   ();
    void speak  (QString msg );
private:
    AVSpeechSynthesizer *_synth;
};

SpeakIOS::SpeakIOS()
    : _synth([[AVSpeechSynthesizer alloc] init])
{

}

SpeakIOS::~SpeakIOS()
{
    [_synth release];
}

void SpeakIOS::speak(QString msg)
{
    while ([_synth isSpeaking]) {
        QGC::SLEEP::msleep(100);
    }
    NSString *msg_ns = [NSString stringWithCString:msg.toStdString().c_str() encoding:[NSString defaultCStringEncoding]];
    AVSpeechUtterance *utterance = [[[AVSpeechUtterance alloc] initWithString: msg_ns] autorelease];
    AVSpeechSynthesisVoice* currentVoice = [AVSpeechSynthesisVoice voiceWithLanguage:[AVSpeechSynthesisVoice currentLanguageCode]];
    utterance.voice = currentVoice;
    //utterance.voice = [AVSpeechSynthesisVoice voiceWithLanguage:@"en-US"];
    utterance.rate  = 0.5;
    [_synth speakUtterance:utterance];
}

//-- The one and only static singleton
SpeakIOS kSpeakIOS;

void iOSSpeak(QString msg)
{
    kSpeakIOS.speak(msg);
}

#endif

