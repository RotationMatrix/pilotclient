/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*!
    \file
*/

#ifndef BLACKCORE_NETWORK_H
#define BLACKCORE_NETWORK_H

#include "../blackmisc/pqfrequency.h"
#include "../blackmisc/coordinategeodetic.h"
#include "../blackmisc/pqlength.h"
#include "../blackmisc/pqtime.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>

namespace BlackCore
{

    class INetwork : public QObject
    {
        Q_OBJECT;

    public:
        virtual ~INetwork() {}

    public slots:
        virtual void setServerDetails(const QString& hostname, quint16 port) = 0;
        virtual void setUserCredentials(const QString& username, const QString& password) = 0;
        virtual void initiateConnection() = 0;
        virtual void terminateConnection() = 0;
        virtual void sendPrivateTextMessage(const QString& callsign, const QString& msg) = 0;
        virtual void sendRadioTextMessage(const QVector<BlackMisc::PhysicalQuantities::CFrequency>& freqs, const QString& msg) = 0;
        virtual void sendIpQuery() = 0;
        virtual void sendFreqQuery(const QString& callsign) = 0;
        virtual void sendServerQuery(const QString& callsign) = 0;
        virtual void sendAtcQuery(const QString& callsign) = 0;
        virtual void sendNameQuery(const QString& callsign) = 0;
        virtual void replyToFreqQuery(const QString& callsign, const BlackMisc::PhysicalQuantities::CFrequency& freq) = 0;
        virtual void replyToNameQuery(const QString& callsign, const QString& realname) = 0;
        virtual void requestPlaneInfo(const QString& callsign) = 0;
        //TODO virtual void setPlanePosition(...) = 0;
        //TODO virtual void sendFlightPlan(...) = 0;
        virtual void sendPlaneInfo(const QString& callsign, const QString& acTypeICAO, const QString& airlineICAO, const QString& livery) = 0;
        virtual void ping(const QString& callsign) = 0;
        virtual void requestMetar(const QString& airportICAO) = 0;
        virtual void requestWeatherData(const QString& airportICAO) = 0;

    signals:
        void atcPositionUpdate(const QString& callsign, const BlackMisc::PhysicalQuantities::CFrequency& freq,
            const BlackMisc::Geo::CCoordinateGeodetic& pos, const BlackMisc::PhysicalQuantities::CLength& range);
        void atcDisconnected(const QString& callsign);
        void atisReplyReceived(const QString& callsign, const QString& data);
        //TODO void cloudDataReceived(...);
        void connectionStatusIdle();
        void connectionStatusConnecting();
        void connectionStatusConnected();
        void connectionStatusDisconnected();
        void connectionStatusError();
        //TODO void atisQueryReplyReceived(...);
        //TODO void nameQueryReplyReceived(const QString& callsign, const QString& name);
        //TODO void capabilitiesQueryReplyReceived(...);
        void ipQueryReplyReceived(const QString& ip);
        void freqQueryReplyReceived(const QString& callsign, const BlackMisc::PhysicalQuantities::CFrequency& freq);
        void serverQueryReplyReceived(const QString& callsign, const QString& hostname);
        void atcQueryReplyReceived(const QString& callsign, bool isATC);
        void nameQueryReplyReceived(const QString& callsign, const QString& realname);
        void freqQueryRequestReceived(const QString& callsign);
        void nameQueryRequestReceived(const QString& callsign);
        //TODO void interimPilotPositionUpdate(...);
        void kicked(const QString& msg);
        void metarReceived(const QString& data);
        void pilotDisconnected(const QString& callsign);
        void planeInfoReceived(const QString& callsign, const QString& acTypeICAO, const QString& airlineICAO, const QString& livery);
        void planeInfoRequestReceived(const QString& callsign);
        //TODO void pilotPositionUpdate(...);
        void pong(const QString& callsign, const BlackMisc::PhysicalQuantities::CTime& elapsedTime);
        void radioTextMessageReceived(const QString& callsign, const QString& msg, const QVector<BlackMisc::PhysicalQuantities::CFrequency>& freqs);
        void privateTextMessageReceived(const QString& fromCallsign, const QString& toCallsign, const QString& msg);
        //TODO void temperatureDataReceived(...);
        //TODO void windDataReceived(...);
    };

} //namespace BlackCore

#endif //BLACKCORE_NETWORK_H
