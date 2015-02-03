/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "voice_vatlib.h"
#include "voice_channel_vatlib.h"
#include "audio_device_vatlib.h"
#include "audio_mixer_vatlib.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include <QDebug>
#include <QTimer>
#include <memory>
#include <mutex>

using namespace BlackMisc;
using namespace BlackMisc::Audio;
using namespace BlackMisc::Aviation;

namespace BlackCore
{
    /*
     * Constructor
     */
    CVoiceVatlib::CVoiceVatlib(QObject *parent) :
        IVoice(parent),
        m_audioService(Vat_CreateAudioService()),
        m_udpPort(Vat_CreateUDPAudioPort(m_audioService.data(), 3782))
    {
        Vat_SetVoiceErrorHandler(CVoiceVatlib::voiceErrorHandler);

        // do processing
        this->startTimer(10);
    }

    /*
     * Destructor
     */
    CVoiceVatlib::~CVoiceVatlib() {}

    QSharedPointer<IVoiceChannel> CVoiceVatlib::createVoiceChannel()
    {
        return QSharedPointer<IVoiceChannel>(new CVoiceChannelVatlib(m_audioService.data(), m_udpPort.data(), this));
    }

    std::unique_ptr<IAudioInputDevice> CVoiceVatlib::createInputDevice()
    {
        return make_unique<CAudioInputDeviceVatlib>(m_audioService.data(), this);
    }

    std::unique_ptr<IAudioOutputDevice> CVoiceVatlib::createOutputDevice()
    {
        return make_unique<CAudioOutputDeviceVatlib>(m_audioService.data(), this);
    }

    std::unique_ptr<IAudioMixer> CVoiceVatlib::createAudioMixer()
    {
        return make_unique<CAudioMixerVatlib>(this);
    }

    /* FIXME:
       Can the following methods be more general somehow?
       E.g.:
       template <typename Input, typename Output>
       connectVoice(Input input, Output output)
       {
           ...
       }
     */

    void CVoiceVatlib::connectVoice(IAudioInputDevice *device, IAudioMixer *mixer, IAudioMixer::InputPort inputPort)
    {
        auto audioInputVatlib = qobject_cast<CAudioInputDeviceVatlib*>(device);
        Q_ASSERT_X(audioInputVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioInputDeviceVatlib pointer.");

        auto audioMixerVatlib = qobject_cast<CAudioMixerVatlib*>(mixer);
        Q_ASSERT_X(audioMixerVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioMixerVatlib pointer.");

        Vat_ConnectProducerToProducerConsumer(audioInputVatlib->getVatLocalInputCodec(), 0, audioMixerVatlib->getVatAudioMixer(), inputPort);
    }

    void CVoiceVatlib::connectVoice(IVoiceChannel *channel, IAudioMixer *mixer, IAudioMixer::InputPort inputPort)
    {
        auto voiceChannelVatlib = qobject_cast<CVoiceChannelVatlib*>(channel);
        Q_ASSERT_X(voiceChannelVatlib, "CVoiceVatlib::connectVoice", "No valid CVoiceChannelVatlib pointer.");

        auto audioMixerVatlib = qobject_cast<CAudioMixerVatlib*>(mixer);
        Q_ASSERT_X(audioMixerVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioMixerVatlib pointer.");

        Vat_ConnectProducerConsumerToProducerConsumer(voiceChannelVatlib->getVoiceChannel(), 0, audioMixerVatlib->getVatAudioMixer(), inputPort);
    }

    void CVoiceVatlib::connectVoice(IAudioMixer *mixer, IAudioMixer::OutputPort outputPort, IAudioOutputDevice *device)
    {
        auto audioMixerVatlib = qobject_cast<CAudioMixerVatlib*>(mixer);
        Q_ASSERT_X(audioMixerVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioMixerVatlib pointer.");

        auto audioDeviceVatlib = qobject_cast<CAudioOutputDeviceVatlib*>(device);
        Q_ASSERT_X(audioDeviceVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioOutputDeviceVatlib pointer.");

        Vat_ConnectProducerConsumerToConsumer(audioMixerVatlib->getVatAudioMixer(), outputPort, audioDeviceVatlib->getVatLocalOutputCodec(), 0);
    }

    void CVoiceVatlib::connectVoice(IAudioMixer *mixer, IAudioMixer::OutputPort outputPort, IVoiceChannel *channel)
    {
        auto audioMixerVatlib = qobject_cast<CAudioMixerVatlib*>(mixer);
        Q_ASSERT_X(audioMixerVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioMixerVatlib pointer.");

        auto voiceChannelVatlib = qobject_cast<CVoiceChannelVatlib*>(channel);
        Q_ASSERT_X(voiceChannelVatlib, "CVoiceVatlib::connectVoice", "No valid CVoiceChannelVatlib pointer.");

        Vat_ConnectProducerConsumerToProducerConsumer(audioMixerVatlib->getVatAudioMixer(), outputPort, voiceChannelVatlib->getVoiceChannel(), 0);
    }

    void CVoiceVatlib::disconnectVoice(IAudioInputDevice *device)
    {
        auto audioInputVatlib = qobject_cast<CAudioInputDeviceVatlib*>(device);
        Q_ASSERT_X(audioInputVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioInputDeviceVatlib pointer.");
        Vat_ConnectProducerToConsumer(audioInputVatlib->getVatLocalInputCodec(), 0, nullptr, 0);
    }

    void CVoiceVatlib::disconnectVoice(IVoiceChannel *channel)
    {
        auto voiceChannelVatlib = qobject_cast<CVoiceChannelVatlib*>(channel);
        Q_ASSERT_X(voiceChannelVatlib, "CVoiceVatlib::connectVoice", "No valid CVoiceChannelVatlib pointer.");
        Vat_ConnectProducerConsumerToConsumer(voiceChannelVatlib->getVoiceChannel(), 0, nullptr, 0);
    }

    void CVoiceVatlib::disconnectVoice(IAudioMixer *mixer, IAudioMixer::OutputPort outputPort)
    {
        auto audioMixerVatlib = qobject_cast<CAudioMixerVatlib*>(mixer);
        Q_ASSERT_X(audioMixerVatlib, "CVoiceVatlib::connectVoice", "No valid CAudioMixerVatlib pointer.");
        Vat_ConnectProducerConsumerToConsumer(audioMixerVatlib->getVatAudioMixer(), outputPort, nullptr, 0);
    }

    /*
     * Process voice handling
     */
    void CVoiceVatlib::timerEvent(QTimerEvent *)
    {
        Q_ASSERT_X(m_audioService, "CVoiceVatlib", "VatAudioService invalid!");
        Vat_ExecuteTasks(m_audioService.data());
    }

    void CVoiceVatlib::voiceErrorHandler(const char *message)
    {
        CLogMessage(static_cast<CVoiceVatlib*>(nullptr)).error(message);
    }

} // namespace
