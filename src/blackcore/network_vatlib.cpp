/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "network_vatlib.h"
#include <vector>
#include <exception>
#include <type_traits>

static_assert(! std::is_abstract<BlackCore::NetworkVatlib>::value, "Must implement all pure virtuals");

//TODO just placeholders to allow this to compile
#define CLIENT_NAME_VERSION "BlackBox 0.1"
#define CLIENT_VERSION_MAJOR 0
#define CLIENT_VERSION_MINOR 1
#define CLIENT_SIMULATOR_NAME "None"
#define CLIENT_PUBLIC_ID 0
#define CLIENT_PRIVATE_KEY ""

namespace BlackCore
{

    using namespace BlackMisc::PhysicalQuantities;
    using namespace BlackMisc::Geo;

    NetworkVatlib::NetworkVatlib()
        : m_net(Create_Cvatlib_Network()),
          m_fsdTextCodec(QTextCodec::codecForName("latin1"))
    {
        try
        {
            Q_ASSERT_X(m_fsdTextCodec, "NetworkVatlib", "Missing default wire text encoding");
            //TODO reinit m_fsdTextCodec from WireTextEncoding config setting if present

            m_net->InstallOnConnectionStatusChangedEvent(onConnectionStatusChanged, this);
            m_net->InstallOnTextMessageReceivedEvent(onTextMessageReceived, this);
            m_net->InstallOnRadioMessageReceivedEvent(onRadioMessageReceived, this);
            m_net->InstallOnPilotDisconnectedEvent(onPilotDisconnected, this);
            m_net->InstallOnControllerDisconnectedEvent(onControllerDisconnected, this);
            m_net->InstallOnPilotPositionUpdateEvent(onPilotPositionUpdate, this);
            m_net->InstallOnInterimPilotPositionUpdateEvent(onInterimPilotPositionUpdate, this);
            m_net->InstallOnAtcPositionUpdateEvent(onAtcPositionUpdate, this);
            m_net->InstallOnKickedEvent(onKicked, this);
            m_net->InstallOnPongEvent(onPong, this);
            m_net->InstallOnMetarReceivedEvent(onMetarReceived, this);
            m_net->InstallOnInfoQueryRequestReceivedEvent(onInfoQueryRequestReceived, this);
            m_net->InstallOnInfoQueryReplyReceivedEvent(onInfoQueryReplyReceived, this);
            m_net->InstallOnCapabilitiesReplyReceivedEvent(onCapabilitiesReplyReceived, this);
            m_net->InstallOnAtisReplyReceivedEvent(onAtisReplyReceived, this);
            m_net->InstallOnTemperatureDataReceivedEvent(onTemperatureDataReceived, this);
            m_net->InstallOnErrorReceivedEvent(onErrorReceived, this);
            m_net->InstallOnWindDataReceivedEvent(onWindDataReceived, this);
            m_net->InstallOnCloudDataReceivedEvent(onCloudDataReceived, this);
            m_net->InstallOnPilotInfoRequestReceivedEvent(onPilotInfoRequestReceived, this);
            m_net->InstallOnPilotInfoReceivedEvent(onPilotInfoReceived, this);

            QString capabilities;
            capabilities += m_net->capability_AtcInfo;
            capabilities += "=1:";
            capabilities += m_net->capability_InterimPos;
            capabilities += "=1:";
            capabilities += m_net->capability_ModelDesc;
            capabilities += "=1";

            m_net->CreateNetworkSession(CLIENT_NAME_VERSION, CLIENT_VERSION_MAJOR, CLIENT_VERSION_MINOR,
                CLIENT_SIMULATOR_NAME, CLIENT_PUBLIC_ID, CLIENT_PRIVATE_KEY, toFSD(capabilities));

            m_timer.start(c_updateIntervalMillisecs, this);
        }
        catch (...) { exceptionDispatcher(); }
    }

    NetworkVatlib::~NetworkVatlib()
    {
        try
        {
            m_timer.stop();

            if (m_net->IsNetworkConnected())
            {
                m_net->LogoffAndDisconnect(0);
            }

            m_net->DestroyNetworkSession();
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::timerEvent(QTimerEvent*)
    {
        try
        {
            if (m_net->IsValid() && m_net->IsSessionExists())
            {
                m_net->DoProcessing();
            }
        }
        catch (...) { exceptionDispatcher(); }
    }

    QByteArray NetworkVatlib::toFSD(QString qstr) const
    {
        return m_fsdTextCodec->fromUnicode(qstr);
    }

    QString NetworkVatlib::fromFSD(const char* cstr) const
    {
        return m_fsdTextCodec->toUnicode(cstr);
    }

    void NetworkVatlib::exceptionDispatcher()
    {
        try
        {
            throw;
        }
        catch (NetworkNotConnectedException& e)
        {
            // this could be caused by a race condition during normal operation, so not an error
            qDebug() << "NetworkNotConnectedException: " << e.what();
        }
        catch (VatlibException& e)
        {
            qFatal("VatlibException: %s", e.what());
        }
        catch (std::exception& e)
        {
            qFatal("NetworkVatlib: std::exception: %s", e.what());
        }
        catch (...)
        {
            qFatal("NetworkVatlib: unknown exception");
        }
    }

    /********************************** * * * * * * * * * * * * * * * * * * * ************************************/
    /**********************************             INetwork slots            ************************************/
    /********************************** * * * * * * * * * * * * * * * * * * * ************************************/

    void NetworkVatlib::setServerDetails(const QString& host, quint16 port)
    {
        m_serverHost = host;
        m_serverPort = port;
    }

    void NetworkVatlib::setUserCredentials(const QString& username, const QString& password)
    {
        m_username = username;
        m_password = password;
    }

    void NetworkVatlib::initiateConnection()
    {
        try
        {
            m_net->ConnectAndLogon();
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::terminateConnection()
    {
        try
        {
            m_net->LogoffAndDisconnect(c_logoffTimeoutSeconds);
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendPrivateTextMessage(const QString& callsign, const QString& msg)
    {
        try
        {
            m_net->SendPrivateTextMessage(toFSD(callsign), toFSD(msg));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendRadioTextMessage(const QVector<CFrequency>& freqs, const QString& msg)
    {
        try
        {
            std::vector<INT> freqsVec;
            for (int i = 0; i < freqs.size(); ++i)
            {
                freqsVec.push_back(freqs[i].value(CFrequencyUnit::kHz()));
            }
            m_net->SendRadioTextMessage(freqsVec.size(), freqsVec.data(), toFSD(msg));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendIpQuery()
    {
        try
        {
            m_net->SendInfoQuery(Cvatlib_Network::infoQuery_IP, ""/*TODO ask Gary*/);
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendFreqQuery(const QString& callsign)
    {
        try
        {
            m_net->SendInfoQuery(Cvatlib_Network::infoQuery_Freq, toFSD(callsign));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendServerQuery(const QString& callsign)
    {
        try
        {
            m_net->SendInfoQuery(Cvatlib_Network::infoQuery_Server, toFSD(callsign));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendAtcQuery(const QString& callsign)
    {
        try
        {
            m_net->SendInfoQuery(Cvatlib_Network::infoQuery_ATC, toFSD(callsign));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendNameQuery(const QString& callsign)
    {
        try
        {
            m_net->SendInfoQuery(Cvatlib_Network::infoQuery_Name, toFSD(callsign));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::replyToFreqQuery(const QString& callsign, const BlackMisc::PhysicalQuantities::CFrequency& freq)
    {
        try
        {
            m_net->ReplyToInfoQuery(Cvatlib_Network::infoQuery_Freq, toFSD(callsign), toFSD(""/*TODO*/));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::replyToNameQuery(const QString& callsign, const QString& realname)
    {
        try
        {
            m_net->ReplyToInfoQuery(Cvatlib_Network::infoQuery_Name, toFSD(callsign), toFSD(realname));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::requestPlaneInfo(const QString& callsign)
    {
        try
        {
            m_net->RequestPlaneInfo(toFSD(callsign));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::sendPlaneInfo(const QString& callsign, const QString& acTypeICAO, const QString& airlineICAO, const QString& livery)
    {
        try
        {
            const QByteArray acTypeICAObytes = toFSD(acTypeICAO);
            const QByteArray airlineICAObytes = toFSD(airlineICAO);
            const QByteArray liverybytes = toFSD(livery);
            std::vector<const char*> keysValues;
            if (! acTypeICAO.isEmpty())
            {
                keysValues.push_back(m_net->acinfo_Equipment);
                keysValues.push_back(acTypeICAObytes);
            }
            if (! airlineICAO.isEmpty())
            {
                keysValues.push_back(m_net->acinfo_Airline);
                keysValues.push_back(airlineICAObytes);
            }
            if (! livery.isEmpty())
            {
                keysValues.push_back(m_net->acinfo_Livery);
                keysValues.push_back(liverybytes);
            }
            keysValues.push_back(0);
            m_net->SendPlaneInfo(toFSD(callsign), keysValues.data());
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::ping(const QString& callsign)
    {
        try
        {
            m_net->PingUser(toFSD(callsign));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::requestMetar(const QString& airportICAO)
    {
        try
        {
            m_net->RequestMetar(toFSD(airportICAO));
        }
        catch (...) { exceptionDispatcher(); }
    }

    void NetworkVatlib::requestWeatherData(const QString& airportICAO)
    {
        try
        {
            m_net->RequestWeatherData(toFSD(airportICAO));
        }
        catch (...) { exceptionDispatcher(); }
    }

    /********************************** * * * * * * * * * * * * * * * * * * * ************************************/
    /**********************************           shimlib callbacks           ************************************/
    /********************************** * * * * * * * * * * * * * * * * * * * ************************************/

    NetworkVatlib* cbvar_cast(void* cbvar)
    {
        return static_cast<NetworkVatlib*>(cbvar);
    }

    void NetworkVatlib::onConnectionStatusChanged(Cvatlib_Network*, Cvatlib_Network::connStatus, Cvatlib_Network::connStatus newStatus, void* cbvar)
    {
        switch (newStatus)
        {
        case Cvatlib_Network::connStatus_Idle:          emit cbvar_cast(cbvar)->connectionStatusIdle(); break;
        case Cvatlib_Network::connStatus_Connecting:    emit cbvar_cast(cbvar)->connectionStatusConnecting(); break;
        case Cvatlib_Network::connStatus_Connected:     emit cbvar_cast(cbvar)->connectionStatusConnected(); break;
        case Cvatlib_Network::connStatus_Disconnected:  emit cbvar_cast(cbvar)->connectionStatusDisconnected(); break;
        case Cvatlib_Network::connStatus_Error:         emit cbvar_cast(cbvar)->connectionStatusError(); break;
        }
    }

    void NetworkVatlib::onTextMessageReceived(Cvatlib_Network*, const char* from, const char* to, const char* msg, void* cbvar)
    {
        emit cbvar_cast(cbvar)->privateTextMessageReceived(cbvar_cast(cbvar)->fromFSD(from), cbvar_cast(cbvar)->fromFSD(to), cbvar_cast(cbvar)->fromFSD(msg));
    }

    void NetworkVatlib::onRadioMessageReceived(Cvatlib_Network*, const char* from, INT numFreq, INT* freqList, const char* msg, void* cbvar)
    {
        QVector<CFrequency> freqs;
        for (int i = 0; i < numFreq; ++i)
        {
            freqs.push_back(CFrequency(freqList[i], CFrequencyUnit::kHz()));
        }
        emit cbvar_cast(cbvar)->radioTextMessageReceived(cbvar_cast(cbvar)->fromFSD(from), cbvar_cast(cbvar)->fromFSD(msg), freqs);
    }

    void NetworkVatlib::onPilotDisconnected(Cvatlib_Network*, const char* callsign, void* cbvar)
    {
        emit cbvar_cast(cbvar)->pilotDisconnected(cbvar_cast(cbvar)->fromFSD(callsign));
    }

    void NetworkVatlib::onControllerDisconnected(Cvatlib_Network*, const char* callsign, void* cbvar)
    {
        emit cbvar_cast(cbvar)->atcDisconnected(cbvar_cast(cbvar)->fromFSD(callsign));
    }

    void NetworkVatlib::onPilotPositionUpdate(Cvatlib_Network*, const char* callsign, Cvatlib_Network::PilotPosUpdate pos, void* cbvar)
    {
        //TODO
    }

    void NetworkVatlib::onInterimPilotPositionUpdate(Cvatlib_Network*, const char* callsign, Cvatlib_Network::PilotPosUpdate pos, void* cbvar)
    {
        //TODO
    }

    void NetworkVatlib::onAtcPositionUpdate(Cvatlib_Network*, const char* callsign, Cvatlib_Network::ATCPosUpdate pos, void* cbvar)
    {
        emit cbvar_cast(cbvar)->atcPositionUpdate(cbvar_cast(cbvar)->fromFSD(callsign), CFrequency(pos.frequency, CFrequencyUnit::kHz()),
            CCoordinateGeodetic(pos.lat, pos.lon, 0), CLength(pos.visibleRange, CLengthUnit::NM()));
    }

    void NetworkVatlib::onKicked(Cvatlib_Network*, const char* reason, void* cbvar)
    {
        emit cbvar_cast(cbvar)->kicked(cbvar_cast(cbvar)->fromFSD(reason));
    }

    void NetworkVatlib::onPong(Cvatlib_Network*, const char* callsign, INT elapsedTime, void* cbvar)
    {
        emit cbvar_cast(cbvar)->pong(cbvar_cast(cbvar)->fromFSD(callsign), CTime(elapsedTime, CTimeUnit::s()));
    }

    void NetworkVatlib::onMetarReceived(Cvatlib_Network*, const char* data, void* cbvar)
    {
        emit cbvar_cast(cbvar)->metarReceived(cbvar_cast(cbvar)->fromFSD(data));
    }

    void NetworkVatlib::onInfoQueryRequestReceived(Cvatlib_Network*, const char* callsign, Cvatlib_Network::infoQuery type, const char*, void* cbvar)
    {
        switch (type)
        {
        case Cvatlib_Network::infoQuery_Freq:   emit cbvar_cast(cbvar)->freqQueryRequestReceived(cbvar_cast(cbvar)->fromFSD(callsign)); break;
        case Cvatlib_Network::infoQuery_Name:   emit cbvar_cast(cbvar)->nameQueryRequestReceived(cbvar_cast(cbvar)->fromFSD(callsign)); break;
        }
    }

    void NetworkVatlib::onInfoQueryReplyReceived(Cvatlib_Network*, const char* callsign, Cvatlib_Network::infoQuery type, const char* data, const char* data2, void* cbvar)
    {
        switch (type)
        {
        case Cvatlib_Network::infoQuery_Freq:   emit cbvar_cast(cbvar)->freqQueryReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), CFrequency(0/*TODO ask Gary*/, CFrequencyUnit::kHz())); break;
        case Cvatlib_Network::infoQuery_Server: emit cbvar_cast(cbvar)->serverQueryReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(data)); break;
        case Cvatlib_Network::infoQuery_ATC:    emit cbvar_cast(cbvar)->atcQueryReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), *data == 'Y'); break; //TODO ask Gary
        case Cvatlib_Network::infoQuery_Name:   emit cbvar_cast(cbvar)->nameQueryReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(data)); break;
        case Cvatlib_Network::infoQuery_IP:     emit cbvar_cast(cbvar)->ipQueryReplyReceived(cbvar_cast(cbvar)->fromFSD(data)); break; //TODO ask Gary can this refer to another pilot?
        }
    }

    void NetworkVatlib::onCapabilitiesReplyReceived(Cvatlib_Network*, const char* callsign, const char** keysValues, void* cbvar)
    {
        //TODO
    }

    void NetworkVatlib::onAtisReplyReceived(Cvatlib_Network*, const char* callsign, Cvatlib_Network::atisLineType, const char* data, void* cbvar)
    {
        emit cbvar_cast(cbvar)->atisReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(data));
    }

    void NetworkVatlib::onTemperatureDataReceived(Cvatlib_Network*, Cvatlib_Network::TempLayer layers[4], INT pressure, void* cbvar)
    {
        //TODO
    }

    void NetworkVatlib::onErrorReceived(Cvatlib_Network*, Cvatlib_Network::error type, const char* msg, const char* data, void* cbvar)
    {
        switch (type)
        {
        case Cvatlib_Network::error_Ok:                     return;
        case Cvatlib_Network::error_CallsignTaken:          qCritical() << "The requested callsign is already taken"; break;
        case Cvatlib_Network::error_CallsignInvalid:        qCritical() << "The requested callsign is not valid"; break;
        case Cvatlib_Network::error_CIDPasswdInvalid:       qCritical() << "Wrong user ID or password"; break;
        case Cvatlib_Network::error_ProtoVersion:           qCritical() << "This server uses an unsupported protocol version"; break;
        case Cvatlib_Network::error_LevelTooHigh:           qCritical() << "You are not authorized to use the requested pilot rating"; break;
        case Cvatlib_Network::error_ServerFull:             qCritical() << "The server is full"; break;
        case Cvatlib_Network::error_CIDSuspended:           qCritical() << "Your user account is suspended"; break;
        case Cvatlib_Network::error_InvalidPosition:        qCritical() << "You are not authorized to use the requested pilot rating"; break;
        case Cvatlib_Network::error_SoftwareNotAuthorized:  qCritical() << "This client software has not been authorized for use on this network"; break;

        case Cvatlib_Network::error_Syntax:                 qWarning() << "Malformed packet: Syntax error: %s" << cbvar_cast(cbvar)->fromFSD(data); break;
        case Cvatlib_Network::error_SourceInvalid:          qDebug() << "Server: source invalid (" << cbvar_cast(cbvar)->fromFSD(data); break;
        case Cvatlib_Network::error_CallsignNotExists:      qDebug() << "Shim lib: " << cbvar_cast(cbvar)->fromFSD(msg) << " (" << cbvar_cast(cbvar)->fromFSD(data) << ")"; break;
        case Cvatlib_Network::error_NoFP:                   qDebug() << "Server: no flight plan"; break;
        case Cvatlib_Network::error_NoWeather:              qDebug() << "Server: requested weather profile does not exist"; break;

            // we have no idea what these mean
        case Cvatlib_Network::error_Registered:
        case Cvatlib_Network::error_InvalidControl:         qWarning() << "Server: " << cbvar_cast(cbvar)->fromFSD(msg); break;

        default:                                            qFatal("VATSIM shim library: %s (error %d)", cbvar_cast(cbvar)->fromFSD(msg), type); break;
        }
    }

    void NetworkVatlib::onWindDataReceived(Cvatlib_Network*, Cvatlib_Network::WindLayer layers[4], void* cbvar)
    {
        //TODO
    }

    void NetworkVatlib::onCloudDataReceived(Cvatlib_Network*, Cvatlib_Network::CloudLayer layers[2], Cvatlib_Network::StormLayer storm, float vis, void* cbvar)
    {
        //TODO
    }

    void NetworkVatlib::onPilotInfoRequestReceived(Cvatlib_Network*, const char* callsign, void* cbvar)
    {
        emit cbvar_cast(cbvar)->planeInfoRequestReceived(cbvar_cast(cbvar)->fromFSD(callsign));
    }

    void NetworkVatlib::onPilotInfoReceived(Cvatlib_Network* net, const char* callsign, const char** keysValues, void* cbvar)
    {
        const char* acTypeICAO;
        const char* airlineICAO;
        const char* livery;
        while (*keysValues)
        {
            QString key (*keysValues);
            keysValues++;
                 if (key == net->acinfo_Equipment)  { acTypeICAO = *keysValues; }
            else if (key == net->acinfo_Airline)    { airlineICAO = *keysValues; }
            else if (key == net->acinfo_Livery)     { livery = *keysValues; }
            keysValues++;
        }
        emit cbvar_cast(cbvar)->planeInfoReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(acTypeICAO),
            cbvar_cast(cbvar)->fromFSD(airlineICAO), cbvar_cast(cbvar)->fromFSD(livery));
    }

} //namespace BlackCore
