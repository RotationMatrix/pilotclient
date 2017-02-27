/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXT_CONTEXTAUDIO_IMPL_H
#define BLACKCORE_CONTEXT_CONTEXTAUDIO_IMPL_H

#include "blackcore/actionbind.h"
#include "blackcore/blackcoreexport.h"
#include "blackcore/context/contextaudio.h"
#include "blackcore/corefacadeconfig.h"
#include "blackcore/audio/audiosettings.h"
#include "blackcore/voicechannel.h"
#include "blackmisc/audio/audiodeviceinfolist.h"
#include "blackmisc/audio/notificationsounds.h"
#include "blackmisc/audio/voiceroom.h"
#include "blackmisc/audio/voiceroomlist.h"
#include "blackmisc/aviation/callsignset.h"
#include "blackmisc/aviation/comsystem.h"
#include "blackmisc/aviation/selcal.h"
#include "blackmisc/identifier.h"
#include "blackmisc/network/userlist.h"
#include "blackmisc/settingscache.h"
#include "blackmisc/icons.h"
#include "blacksound/selcalplayer.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <memory>

namespace BlackMisc
{
    class CDBusServer;
    namespace Audio { class CAudioDeviceInfo; }
    namespace Aviation { class CCallsign; }
}

namespace BlackCore
{
    class CCoreFacade;
    class IAudioInputDevice;
    class IAudioMixer;
    class IAudioOutputDevice;
    class IVoice;
    class IVoiceChannel;

    namespace Context
    {
        //! Audio context implementation
        class BLACKCORE_EXPORT CContextAudio : public IContextAudio
        {
            Q_CLASSINFO("D-Bus Interface", BLACKCORE_CONTEXTAUDIO_INTERFACENAME)
            Q_OBJECT

            friend class BlackCore::CCoreFacade;
            friend class IContextAudio;

        public:
            //! Destructor
            virtual ~CContextAudio();

        public slots:
            // Interface implementations
            //! \publicsection
            //! @{
            virtual BlackMisc::Audio::CVoiceRoomList getComVoiceRooms() const override;
            virtual BlackMisc::Audio::CVoiceRoomList getComVoiceRoomsWithAudioStatus() const override;
            virtual BlackMisc::Audio::CVoiceRoom getVoiceRoom(BlackMisc::Aviation::CComSystem::ComUnit comUnitValue, bool withAudioStatus) const override;
            virtual void setComVoiceRooms(const BlackMisc::Audio::CVoiceRoomList &newRooms) override;
            virtual void setOwnCallsignForRooms(const BlackMisc::Aviation::CCallsign &callsign) override;
            virtual BlackMisc::Aviation::CCallsignSet getRoomCallsigns(BlackMisc::Aviation::CComSystem::ComUnit comUnitValue) const override;
            virtual BlackMisc::Network::CUserList getRoomUsers(BlackMisc::Aviation::CComSystem::ComUnit comUnitValue) const override;
            virtual void leaveAllVoiceRooms() override;
            virtual BlackMisc::CIdentifier audioRunsWhere() const override;
            virtual BlackMisc::Audio::CAudioDeviceInfoList getAudioDevices() const override;
            virtual BlackMisc::Audio::CAudioDeviceInfoList getCurrentAudioDevices() const override;
            virtual void setCurrentAudioDevice(const BlackMisc::Audio::CAudioDeviceInfo &audioDevice) override;
            virtual void setVoiceOutputVolume(int volume) override;
            virtual int getVoiceOutputVolume() const override;
            virtual void setMute(bool muted) override;
            virtual bool isMuted() const override;
            virtual void playSelcalTone(const BlackMisc::Aviation::CSelcal &selcal) const override;
            virtual void playNotification(BlackMisc::Audio::CNotificationSounds::Notification notification, bool considerSettings) const override;
            virtual void enableAudioLoopback(bool enable = true) override;
            virtual bool isAudioLoopbackEnabled() const override;
            //! @}

            //! \addtogroup swiftdotcommands
            //! @{
            //! <pre>
            //! .mute                          mute             BlackCore::Context::CContextAudio
            //! .unmute                        unmute           BlackCore::Context::CContextAudio
            //! .vol .volume   volume 0..300   set volume       BlackCore::Context::CContextAudio
            //! </pre>
            //! @}
            //! \copydoc IContextAudio::parseCommandLine
            virtual bool parseCommandLine(const QString &commandLine, const BlackMisc::CIdentifier &originator) override;

        protected:
            //! Constructor
            CContextAudio(CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime);

            //! Register myself in DBus
            CContextAudio *registerWithDBus(BlackMisc::CDBusServer *server);

        private slots:

            //! \copydoc IVoice::connectionStatusChanged
            //! \sa IContextAudio::changedVoiceRooms
            void ps_connectionStatusChanged(IVoiceChannel::ConnectionStatus oldStatus, IVoiceChannel::ConnectionStatus newStatus);

            //! Init notification sounds
            void ps_initNotificationSounds();

            void ps_setVoiceTransmission(bool enable);

            //! User joined the room
            void ps_userJoinedRoom(const BlackMisc::Aviation::CCallsign &callsign);

            //! User left the room
            void ps_userLeftRoom(const BlackMisc::Aviation::CCallsign &callsign);

        private:
            //! Connection in transition
            bool inTransitionState() const;

            //! Voice channel by room
            QSharedPointer<IVoiceChannel> getVoiceChannelBy(const BlackMisc::Audio::CVoiceRoom &voiceRoom);

            const int MinUnmuteVolume = 20; //!< minimum volume when unmuted
            CActionBind m_actionPtt { "/Voice/Activate push-to-talk", BlackMisc::CIcons::radio16(), this, &CContextAudio::ps_setVoiceTransmission };
            std::unique_ptr<IVoice> m_voice; //!< underlying voice lib
            std::unique_ptr<IAudioMixer> m_audioMixer;
            int m_outVolumeBeforeMute = 90;

            // For easy access.
            QSharedPointer<IVoiceChannel> m_channel1;
            QSharedPointer<IVoiceChannel> m_channel2;
            std::unique_ptr<IAudioOutputDevice> m_voiceOutputDevice;
            std::unique_ptr<IAudioInputDevice> m_voiceInputDevice;

            QList<QSharedPointer<IVoiceChannel>> m_unusedVoiceChannels;
            QHash<BlackMisc::Aviation::CComSystem::ComUnit, QSharedPointer<IVoiceChannel>> m_voiceChannelMapping;
            BlackSound::CSelcalPlayer *m_selcalPlayer = nullptr;

            // settings
            BlackMisc::CSetting<BlackCore::Audio::TSettings> m_audioSettings { this };
        };
    } // namespace
} // namespace

#endif // guard
