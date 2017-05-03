/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
    utterance.voice = [AVSpeechSynthesisVoice voiceWithLanguage:@"en-US"];
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

