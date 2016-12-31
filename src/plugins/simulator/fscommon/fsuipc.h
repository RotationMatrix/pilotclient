/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_FSUIPC_H
#define BLACKSIMPLUGIN_FSUIPC_H

#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/weather/weathergrid.h"
#include <QStringList>

namespace BlackSimPlugin
{
    namespace FsCommon
    {
        //! Class representing a FSUIPC "interface"
        class CFsuipc : public QObject
        {
            Q_OBJECT

        public:
            //! Constructor
            CFsuipc();

            //! Destructor
            virtual ~CFsuipc();

            //! Open conenction with FSUIPC
            bool connect();

            //! Disconnect
            void disconnect();

            //! Is connected?
            bool isConnected() const { return m_connected; }

            //! Process reading and writing variables
            void process(BlackMisc::Simulation::CSimulatedAircraft &aircraft);

            //! Write variables
            bool write(const BlackMisc::Simulation::CSimulatedAircraft &aircraft);

            //! Write weather grid to simulator
            bool write(const BlackMisc::Weather::CWeatherGrid &weatherGrid);

            //! Get the version
            QString getVersion() const { return m_fsuipcVersion; }

            //! Read data from FSUIPC
            //! \param aircraft       object to be updated
            //! \param cockpit        update cockpit data
            //! \param situation      update situation data
            //! \param aircraftParts  update parts
            //! \return read
            //!
            bool read(BlackMisc::Simulation::CSimulatedAircraft &aircraft, bool cockpit, bool situation, bool aircraftParts);

            //! Error messages
            static const QStringList &errorMessages()
            {
                static const QStringList errors(
                {
                    "Okay",
                    "Attempt to Open when already Open",
                    "Cannot link to FSUIPC or WideClient",
                    "Failed to Register common message with Windows",
                    "Failed to create Atom for mapping filename",
                    "Failed to create a file mapping object",
                    "Failed to open a view to the file map",
                    "Incorrect version of FSUIPC, or not FSUIPC",
                    "Sim is not version requested",
                    "Call cannot execute, link not Open",
                    "Call cannot execute: no requests accumulated",
                    "IPC timed out all retries",
                    "IPC sendmessage failed all retries",
                    "IPC request contains bad data",
                    "Maybe running on WideClient, but FS not running on Server, or wrong FSUIPC",
                    "Read or Write request cannot be added, memory for Process is full",
                }
                );
                return errors;
            }

            //! Simulators
            static const QStringList &simulators()
            {
                static const QStringList sims(
                {
                    "any", "FS98", "FS2000", "CFS2", "CFS1", "Fly!", "FS2002", "FS2004", "FSX", "ESP", "P3D"
                }
                );
                return sims;
            }

            //! Log message category
            static QString getLogCategory() { return "swift.fscommon.fsuipc"; }

        protected:
            //! \copydoc QObject::timerEvent
            void timerEvent(QTimerEvent *event);

        private:
            struct FsuipcWeatherMessage
            {
                FsuipcWeatherMessage() = default;
                FsuipcWeatherMessage(unsigned int offset, const QByteArray &data, int leftTrials);
                int m_offset = 0;
                QByteArray m_messageData;
                int m_leftTrials = 0;
            };

            void clearAllWeather();
            void processWeatherMessages();

            bool m_connected = false;
            QString m_lastErrorMessage;
            QString m_fsuipcVersion;

            QVector<FsuipcWeatherMessage> m_weatherMessageQueue;
            unsigned int m_lastTimestamp = 0;

            //! Integer representing fractional
            static double intToFractional(double fractional);
        };
    }
}

#endif // guard
