/* Copyright (C) 2013 VATSIM Community / contributors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "fsuipc.h"
#include <Windows.h>
// bug in FSUIPC_User.h, windows.h not included, so we have to import it first
#include "FSUIPC/FSUIPC_User.h"

#include "blacksim/fscommon/bcdconversions.h"
#include "blackmisc/logmessage.h"
#include <QDebug>
#include <QLatin1Char>
#include <QDateTime>

using namespace BlackMisc;
using namespace BlackSim::FsCommon;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::PhysicalQuantities;

namespace BlackSimPlugin
{
    namespace FsCommon
    {

        CFsuipc::CFsuipc() { }

        CFsuipc::~CFsuipc()
        {
            this->disconnect();
        }

        bool CFsuipc::connect()
        {
            DWORD result;
            this->m_lastErrorMessage = "";
            if (this->m_connected) return this->m_connected; // already connected
            if (FSUIPC_Open(SIM_ANY, &result))
            {
                this->m_connected = true;
                int simIndex = static_cast<int>(FSUIPC_FS_Version);
                QString sim(
                    (simIndex >= 0 && simIndex < CFsuipc::simulators().size()) ?
                    CFsuipc::simulators().at(simIndex) :
                    "Unknown FS");
                QString ver("%1.%2.%3.%4%5");
                ver = ver.arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 28))))
                      .arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 24))))
                      .arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 20))))
                      .arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 16))))
                      .arg((FSUIPC_Version & 0xffff) ? "a" + (FSUIPC_Version & 0xff) - 1 : "");
                this->m_fsuipcVersion = QString("FSUIPC %1 (%2)").arg(ver).arg(sim);
                CLogMessage(this).info("FSUIPC connected: %1") << this->m_fsuipcVersion;
            }
            else
            {
                this->m_connected = false;
                int index = static_cast<int>(result);
                this->m_lastErrorMessage = CFsuipc::errorMessages().at(index);
                CLogMessage(this).info("FSUIPC not connected: %1") << this->m_lastErrorMessage;
            }
            return this->m_connected;
        }

        void CFsuipc::disconnect()
        {
            FSUIPC_Close(); // Closing when it wasn't open is okay, so this is safe here
            this->m_connected = false;
        }

        bool CFsuipc::write(const CSimulatedAircraft &aircraft)
        {
            if (!this->isConnected()) { return false; }

            Q_UNUSED(aircraft);
            //! \todo FSUIPC write values
            return false;
        }

        bool CFsuipc::read(CSimulatedAircraft &aircraft, bool cockpit, bool situation, bool aircraftParts)
        {
            DWORD dwResult;
            char localFsTimeRaw[3];
            char modelNameRaw[256];
            qint16 com1ActiveRaw = 0, com2ActiveRaw = 0, com1StandbyRaw = 0, com2StandbyRaw = 0;
            qint16 transponderCodeRaw = 0;
            qint8 xpdrModeSb3Raw = 0, xpdrIdentSb3Raw = 0;
            qint32 groundspeedRaw = 0, pitchRaw = 0, bankRaw = 0, headingRaw = 0;
            qint64 altitudeRaw = 0;
            qint32 groundAltitudeRaw = 0;
            qint64 latitudeRaw = 0, longitudeRaw = 0;
            qint16 lightsRaw = 0;
            qint16 onGroundRaw = 0;
            qint32 flapsControlRaw = 0, gearControlRaw = 0, spoilersControlRaw = 0;
            qint16 numberOfEngines = 0;
            qint16 engine1CombustionFlag = 0, engine2CombustionFlag = 0, engine3CombustionFlag = 0, engine4CombustionFlag = 0;


            // http://www.projectmagenta.com/all-fsuipc-offsets/
            // https://www.ivao.aero/softdev/ivap/fsuipc_sdk.asp
            // http://squawkbox.ca/doc/sdk/fsuipc.php

            if (!this->isConnected()) { return false; }
            if (!(aircraftParts || situation || cockpit)) { return false; }

            bool read = false;
            bool cockpitN = !cockpit;
            bool situationN = !situation;
            bool aircraftPartsN = ! aircraftParts;

            if (FSUIPC_Read(0x0238, 3, localFsTimeRaw, &dwResult) &&

                    // COM settings
                    (cockpitN || FSUIPC_Read(0x034e, 2, &com1ActiveRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x3118, 2, &com2ActiveRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x311a, 2, &com1StandbyRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x311c, 2, &com2StandbyRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x0354, 2, &transponderCodeRaw, &dwResult)) &&

                    // COM Settings, transponder, SB3
                    (cockpitN || FSUIPC_Read(0x7b91, 1, &xpdrModeSb3Raw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x7b93, 1, &xpdrIdentSb3Raw, &dwResult)) &&

                    // Speeds, situation
                    (situationN || FSUIPC_Read(0x02b4, 4, &groundspeedRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0578, 4, &pitchRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x057c, 4, &bankRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0580, 4, &headingRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0570, 8, &altitudeRaw, &dwResult)) &&

                    // Position
                    (situationN || FSUIPC_Read(0x0560, 8, &latitudeRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0568, 8, &longitudeRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0020, 4, &groundAltitudeRaw, &dwResult)) &&

                    // model name
                    (aircraftPartsN || FSUIPC_Read(0x3d00, 256, &modelNameRaw, &dwResult)) &&

                    // aircraft parts
                    (aircraftPartsN || FSUIPC_Read(0x0D0C, 2, &lightsRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0366, 2, &onGroundRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0BDC, 4, &flapsControlRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0BE8, 4, &gearControlRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0BD0, 4, &spoilersControlRaw, &dwResult)) &&

                    // engines
                    (aircraftPartsN || FSUIPC_Read(0x0AEC, 2, &numberOfEngines, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0894, 2, &engine1CombustionFlag, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x092C, 2, &engine2CombustionFlag, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x09C4, 2, &engine3CombustionFlag, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0A5C, 2, &engine4CombustionFlag, &dwResult)) &&

                    // If we wanted other reads/writes at the same time, we could put them here
                    FSUIPC_Process(&dwResult))
            {
                read = true;

                // time, basically as a heartbeat
                QString fsTime;
                fsTime.sprintf("%02d:%02d:%02d", localFsTimeRaw[0], localFsTimeRaw[1], localFsTimeRaw[2]);

                if (cockpit)
                {
                    // COMs
                    CComSystem com1 = aircraft.getCom1System();
                    CComSystem com2 = aircraft.getCom2System();
                    CTransponder xpdr = aircraft.getTransponder();

                    // 2710 => 12710 => / 100.0 => 127.1
                    com1ActiveRaw = (10000 + CBcdConversions::bcd2Dec(com1ActiveRaw));
                    com2ActiveRaw = (10000 + CBcdConversions::bcd2Dec(com2ActiveRaw));
                    com1StandbyRaw = (10000 + CBcdConversions::bcd2Dec(com1StandbyRaw));
                    com2StandbyRaw = (10000 + CBcdConversions::bcd2Dec(com2StandbyRaw));
                    com1.setFrequencyActiveMHz(com1ActiveRaw / 100.0);
                    com2.setFrequencyActiveMHz(com2ActiveRaw / 100.0);
                    com1.setFrequencyStandbyMHz(com1StandbyRaw / 100.0);
                    com2.setFrequencyStandbyMHz(com2StandbyRaw / 100.0);

                    transponderCodeRaw = static_cast<qint16>(CBcdConversions::bcd2Dec(transponderCodeRaw));
                    xpdr.setTransponderCode(transponderCodeRaw);
                    // Mode by SB3
                    if (xpdrIdentSb3Raw != 0)
                    {
                        //! \todo Reset value for FSUIPC
                        xpdr.setTransponderMode(CTransponder::StateIdent);
                    }
                    else
                    {
                        xpdr.setTransponderMode(
                            xpdrModeSb3Raw == 0 ? CTransponder::ModeC : CTransponder::StateStandby
                        );
                    }
                    aircraft.setCockpit(com1, com2, xpdr);
                } // cockpit

                if (situation)
                {
                    // position
                    const double latCorrectionFactor = 90.0 / (10001750.0 * 65536.0 * 65536.0);
                    const double lonCorrectionFactor = 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);
                    CAircraftSituation situation = aircraft.getSituation();
                    CCoordinateGeodetic position = situation.getPosition();
                    CLatitude lat(latitudeRaw * latCorrectionFactor, CAngleUnit::deg());
                    CLongitude lon(longitudeRaw * lonCorrectionFactor, CAngleUnit::deg());
                    CLength groundAltitude(groundAltitudeRaw / 256.0, CLengthUnit::m());
                    position.setLatitude(lat);
                    position.setLongitude(lon);
                    position.setGeodeticHeight(groundAltitude);
                    situation.setPosition(position);

                    // speeds, situation
                    const double angleCorrectionFactor = 360.0 / 65536.0 / 65536.0; // see FSUIPC docu
                    CAngle pitch = CAngle(pitchRaw * angleCorrectionFactor, CAngleUnit::deg());
                    CAngle bank = CAngle(bankRaw * angleCorrectionFactor, CAngleUnit::deg());
                    CHeading heading = CHeading(headingRaw * angleCorrectionFactor, CHeading::True, CAngleUnit::deg());
                    CSpeed groundspeed(groundspeedRaw / 65536.0, CSpeedUnit::m_s());
                    CAltitude altitude(altitudeRaw / (65536.0 * 65536.0), CAltitude::MeanSeaLevel, CLengthUnit::m());
                    situation.setBank(bank);
                    situation.setHeading(heading);
                    situation.setPitch(pitch);
                    situation.setGroundspeed(groundspeed);
                    situation.setAltitude(altitude);
                    aircraft.setSituation(situation);

                } // situation

                if (aircraftParts)
                {

                    // model
                    const QString modelName = QString(modelNameRaw); // to be used to distinguish offsets for different models
                    aircraft.setModelString(modelName);

                    CAircraftLights lights(lightsRaw & (1 << 4), lightsRaw & (1 << 2), lightsRaw & (1 << 3), lightsRaw & (1 << 1),
                                           lightsRaw & (1 << 0), lightsRaw & (1 << 8));

                    QList<bool> helperList { engine1CombustionFlag != 0, engine2CombustionFlag != 0,
                                             engine3CombustionFlag != 0, engine4CombustionFlag != 0
                                           };

                    CAircraftEngineList engines;
                    for (int index = 0; index < numberOfEngines; ++index)
                    {
                        engines.push_back(CAircraftEngine(index + 1, helperList.at(index)));
                    }

                    CAircraftParts parts(lights, gearControlRaw == 16383, flapsControlRaw * 100 / 16383,
                                         spoilersControlRaw == 16383, engines, onGroundRaw == 1);

                    aircraft.setParts(parts);
                } // parts
            } // read
            return read;
        }

        double CFsuipc::intToFractional(double fractional)
        {
            double f = fractional / 10.0;
            if (f < 1.0) return f;
            return intToFractional(f);
        }
    } // namespace
} // namespace
