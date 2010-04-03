#include "AudioOutput.h"
#include <flite.h>

#include <phonon/mediaobject.h>
#include <QTemporaryFile>

#include <QDebug>

extern "C" {
#include <cmu_us_awb/voxdefs.h>
    //#include <cmu_us_slt/voxdefs.h>
    //cst_voice *REGISTER_VOX(const char *voxdir);
    //void UNREGISTER_VOX(cst_voice *vox);
    cst_voice *register_cmu_us_awb(const char *voxdir);
    void unregister_cmu_us_awb(cst_voice *vox);
    cst_voice *register_cmu_us_slt(const char *voxdir);
    void unregister_cmu_us_slt(cst_voice *vox);
    cst_voice *register_cmu_us_rms(const char *voxdir);
    void unregister_cmu_us_rms(cst_voice *vox);
};

AudioOutput::AudioOutput(QString voice, QObject* parent) : QObject(parent),
voice(NULL),
voiceIndex(0)
{
#ifndef _WIN32
    flite_init();
#endif

    switch (voiceIndex)
    {
    case 0:
        selectFemaleVoice();
        break;
    case 1:
        selectMaleVoice();
        break;
    default:
        selectNeutralVoice();
        break;
    }

    /*
    // List available voices
    QStringList voices = listVoices();
    foreach (QString s, voices)
    {
        qDebug() << "VOICE: " << s;
    }

    if (voice.length() > 0)
    {
        //this->voice = flite_voice_select(voice.toStdString().c_str());
    }
    else
    {
        // Take default voice
        //this->voice = flite_voice_select("awb");
    }
    */
}

bool AudioOutput::say(QString text, int severity)
{
    // Only give speech output on Linux and MacOS
#ifndef _WIN32
    QTemporaryFile file;
    file.setFileTemplate("XXXXXX.wav");
    if (file.open())
    {
        cst_wave* wav = flite_text_to_wave(text.toStdString().c_str(), this->voice);
        // file.fileName() returns the unique file name
        cst_wave_save(wav, file.fileName().toStdString().c_str(), "riff");
        Phonon::MediaObject *music =
                Phonon::createPlayer(Phonon::MusicCategory,
                                     Phonon::MediaSource(file.fileName().toStdString().c_str()));
        music->play();
        qDebug() << "Synthesized: " << text << ", tmp file:" << file.fileName().toStdString().c_str();
    }
#else
    qDebug() << "Synthesized: " << text << ", NO OUTPUT SUPPORT ON WINDOWS!";
#endif
    return true;
}

bool AudioOutput::alert(QString text)
{
    // Play alert sound

    // Say alert message
    return say(text, 2);
}

bool AudioOutput::startEmergency()
{
    return false;
}

bool AudioOutput::stopEmergency()
{
    return false;
}

void AudioOutput::selectFemaleVoice()
{
#ifndef _WIN32
    this->voice = register_cmu_us_slt(NULL);
#endif
}

void AudioOutput::selectMaleVoice()
{
#ifndef _WIN32
    this->voice = register_cmu_us_rms(NULL);
#endif
}


void AudioOutput::selectNeutralVoice()
{
#ifndef _WIN32
    this->voice = register_cmu_us_awb(NULL);
#endif
}

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    QStringList AudioOutput::listVoices(void)
    {


        cst_voice *voice;
        const cst_val *v;
        QStringList l;

#ifndef _WIN32
        /*
        printf("Voices available: ");
        for (v=flite_voice_list; v; v=val_cdr(v))
        {
            voice = val_voice(val_car(v));
            QString s;
            s.sprintf("%s",voice->name);
            printf("%s",voice->name);
            l.append(s);
        }
        printf("\n");
*/
#endif
        return l;

    }
#ifdef __cplusplus
}
#endif /* __cplusplus */
