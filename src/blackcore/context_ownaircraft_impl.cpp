/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "context_ownaircraft_impl.h"
#include "context_network.h"
#include "context_audio.h"
#include "context_runtime.h"
#include "context_settings.h"

using namespace BlackMisc;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Audio;

namespace BlackCore
{

    /*
     * Init this context
     */
    CContextOwnAircraft::CContextOwnAircraft(CRuntimeConfig::ContextMode mode, CRuntime *runtime) :
        IContextOwnAircraft(mode, runtime), m_automaticVoiceRoomResolution(true)
    {
        Q_ASSERT(this->getRuntime());
        Q_ASSERT(this->getRuntime()->getIContextSettings());

        // 1. Init own aircraft
        this->initOwnAircraft();
    }

    /*
     * Cleanup
     */
    CContextOwnAircraft::~CContextOwnAircraft() { }

    /*
     * Init own aircraft
     */
    void CContextOwnAircraft::initOwnAircraft()
    {
        Q_ASSERT(this->getRuntime());
        Q_ASSERT(this->getRuntime()->getIContextSettings());
        this->m_ownAircraft.initComSystems();
        this->m_ownAircraft.initTransponder();
        CAircraftSituation situation(
            CCoordinateGeodetic(
                CLatitude::fromWgs84("N 049° 18' 17"),
                CLongitude::fromWgs84("E 008° 27' 05"),
                CLength(0, CLengthUnit::m())),
            CAltitude(312, CAltitude::MeanSeaLevel, CLengthUnit::ft())
        );
        this->m_ownAircraft.setSituation(situation);
        this->m_ownAircraft.setPilot(this->getIContextSettings()->getNetworkSettings().getCurrentTrafficNetworkServer().getUser());

        // TODO: This would need to come from somewhere (mappings)
        // Own callsign, plane ICAO status, model used
        this->m_ownAircraft.setCallsign(CCallsign("BLACK"));
        this->m_ownAircraft.setIcaoInfo(CAircraftIcao("C172", "L1P", "GA", "GA", "0000ff"));

        // voice rooms
        this->resolveVoiceRooms();
    }

    /*
     * Resolve voice rooms
     */
    void CContextOwnAircraft::resolveVoiceRooms()
    {
        if (this->getRuntime()->isSlotLogForOwnAircraftEnabled()) this->getRuntime()->logSlot(Q_FUNC_INFO);
        if (this->m_voiceRoom1UrlOverride.isEmpty() && this->m_voiceRoom2UrlOverride.isEmpty() && !this->m_automaticVoiceRoomResolution) return;
        if (!this->getIContextNetwork()) return; // no chance to resolve rooms
        if (!this->getIContextAudio()) return; // no place to set rooms

        CVoiceRoomList rooms;
        if (this->m_automaticVoiceRoomResolution)
        {
            // requires correct frequencies set
            // but local network uses exactly this object here, so if frequencies are set here,
            // they are for network context as well
            rooms = this->getIContextNetwork()->getSelectedVoiceRooms();
        }
        else
        {
            rooms.push_back(CVoiceRoom());
            rooms.push_back(CVoiceRoom());
        }

        if (!this->m_voiceRoom1UrlOverride.isEmpty()) rooms[0] = CVoiceRoom(this->m_voiceRoom1UrlOverride);
        if (!this->m_voiceRoom2UrlOverride.isEmpty()) rooms[1] = CVoiceRoom(this->m_voiceRoom2UrlOverride);

        // set the rooms
        this->getIContextAudio()->setComVoiceRooms(rooms);
    }

    /*
     * Own Aircraft
     */
    void CContextOwnAircraft::updateOwnAircraft(const BlackMisc::Aviation::CAircraft &aircraft, const QString &originator)
    {
        if (this->getRuntime()->isSlotLogForOwnAircraftEnabled()) this->getRuntime()->logSlot(Q_FUNC_INFO, ownAircraft().toQString(), originator);

        // trigger the correct signals
        bool changedCockpit = this->updateOwnCockpit(aircraft.getCom1System(), aircraft.getCom2System(), aircraft.getTransponder(), originator);
        bool changedPosition = this->updateOwnPosition(aircraft.getPosition(), aircraft.getAltitude() , originator);
        bool changedSituation = this->updateOwnSituation(aircraft.getSituation(), originator);

        if (changedCockpit || changedPosition || changedSituation) this->resolveVoiceRooms();

        // all the rest
        this->m_ownAircraft = aircraft;
    }

    /*
     * Own position
     */
    bool CContextOwnAircraft::updateOwnPosition(const BlackMisc::Geo::CCoordinateGeodetic &position, const BlackMisc::Aviation::CAltitude &altitude, const QString &originator)
    {
        if (this->getRuntime()->isSlotLogForOwnAircraftEnabled()) this->getRuntime()->logSlot(Q_FUNC_INFO, position.toQString(), altitude.toQString());
        bool changed = (this->m_ownAircraft.getPosition() == position);
        if (changed) this->m_ownAircraft.setPosition(position);

        if (this->m_ownAircraft.getAltitude() != altitude)
        {
            changed = true;
            this->m_ownAircraft.setAltitude(altitude);
        }

        if (changed) emit this->changedAircraftPosition(this->m_ownAircraft, originator);
        return changed;
    }

    /*
     * Update own situation
     */
    bool CContextOwnAircraft::updateOwnSituation(const BlackMisc::Aviation::CAircraftSituation &situation, const QString &originator)
    {
        if (this->getRuntime()->isSlotLogForOwnAircraftEnabled()) this->getRuntime()->logSlot(Q_FUNC_INFO, situation.toQString());
        bool changed = this->m_ownAircraft.getSituation() == situation;
        if (!changed) return changed;

        this->m_ownAircraft.setSituation(situation);
        emit this->changedAircraftSituation(this->m_ownAircraft, originator);
        return changed;
    }

    /*
     * Own cockpit data
     */
    bool CContextOwnAircraft::updateOwnCockpit(const BlackMisc::Aviation::CComSystem &com1, const BlackMisc::Aviation::CComSystem &com2, const BlackMisc::Aviation::CTransponder &transponder, const QString &originator)
    {
        if (this->getRuntime()->isSlotLogForOwnAircraftEnabled()) this->getRuntime()->logSlot(Q_FUNC_INFO, com1.toQString(), com2.toQString(), transponder.toQString());
        bool changed = false;
        if (com1 != this->m_ownAircraft.getCom1System())
        {
            this->m_ownAircraft.setCom1System(com1);
            changed = true;
        }
        if (com2 != this->m_ownAircraft.getCom2System())
        {
            this->m_ownAircraft.setCom2System(com2);
            changed = true;
        }
        if (transponder != this->m_ownAircraft.getTransponder())
        {
            this->m_ownAircraft.setTransponder(transponder);
            changed = true;
        }
        if (changed)
        {
            emit this->changedAircraftCockpit(this->m_ownAircraft, originator);
            this->resolveVoiceRooms();
        }
        return changed;
    }

    void CContextOwnAircraft::setAudioOutputVolumes(int outputVolumeCom1, int outputVolumeCom2)
    {
        CComSystem com1 = this->m_ownAircraft.getCom1System();
        com1.setVolumeOutput(outputVolumeCom1);
        this->m_ownAircraft.setCom1System(com1);

        CComSystem com2 = this->m_ownAircraft.getCom2System();
        com2.setVolumeOutput(outputVolumeCom2);
        this->m_ownAircraft.setCom2System(com1);

        if (this->getIContextAudio()) this->getIContextAudio()->setVolumes(com1, com2);
    }

    /*
     * Tune in / out voice room
     */
    void CContextOwnAircraft::changedAtcStationOnlineConnectionStatus(const CAtcStation &atcStation, bool connected)
    {
        // any of our active frequencies?
        Q_UNUSED(connected);
        if (atcStation.getFrequency() != this->m_ownAircraft.getCom1System().getFrequencyActive() &&
                atcStation.getFrequency() != this->m_ownAircraft.getCom2System().getFrequencyActive()) return;
        this->resolveVoiceRooms();
    }

    /*
     *  Voice room URLs
     */
    void CContextOwnAircraft::setAudioVoiceRoomOverrideUrls(const QString &voiceRoom1Url, const QString &voiceRoom2Url)
    {
        this->m_voiceRoom1UrlOverride = voiceRoom1Url.trimmed();
        this->m_voiceRoom2UrlOverride = voiceRoom2Url.trimmed();
        this->resolveVoiceRooms();
    }

    /*
     * Own aircraft
     */
    CAircraft CContextOwnAircraft::getOwnAircraft() const
    {
        if (this->getRuntime()->isSlotLogForOwnAircraftEnabled()) this->getRuntime()->logSlot(Q_FUNC_INFO, this->m_ownAircraft.toQString());
        return this->m_ownAircraft;
    }

} // namespace
