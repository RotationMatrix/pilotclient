/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of Swift Project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */
#define _CRT_SECURE_NO_WARNINGS

#include "blacksimplugin_freefunctions.h"
#include "blackmisc/project.h"
#include "fs9.h"
#include "lobby_client.h"
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QStringList>
#include <QScopedPointer>
#include <QMutexLocker>

namespace BlackSimPlugin
{
    namespace Fs9
    {
        CLobbyClient::CLobbyClient(QObject *parent)
            : QObject(parent),
              m_callbackWrapper(this, &CLobbyClient::directPlayMessageHandler),
              m_lobbyCallbackWrapper(this, &CLobbyClient::directPlayLobbyMessageHandler)
        {
            initDirectPlay();
        }

        CLobbyClient::~CLobbyClient()
        {
            CoUninitialize();
        }

        HRESULT CLobbyClient::initDirectPlay()
        {
            HRESULT hr;

            // Create and init IDirectPlay8Peer
            if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Peer, nullptr,
                                             CLSCTX_INPROC_SERVER,
                                             IID_IDirectPlay8Peer,
                                             (LPVOID *) &m_directPlayPeer)))
                return printDirectPlayError(hr);

            // Turn off parameter validation in release builds
            const DWORD dwInitFlags = 0;
            // const DWORD dwInitFlags = DPNINITIALIZE_DISABLEPARAMVAL;

            if (FAILED(hr = m_directPlayPeer->Initialize(&m_callbackWrapper, m_callbackWrapper.messageHandler, dwInitFlags)))
                return printDirectPlayError(hr);


            // Create and init IDirectPlay8LobbyClient
            if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8LobbyClient, nullptr,
                                             CLSCTX_INPROC_SERVER,
                                             IID_IDirectPlay8LobbyClient,
                                             (LPVOID *) &m_dpLobbyClient)))
                return printDirectPlayError(hr);

            if (FAILED(hr = m_dpLobbyClient->Initialize(&m_lobbyCallbackWrapper, m_lobbyCallbackWrapper.messageHandler, dwInitFlags)))
                return printDirectPlayError(hr);

            return S_OK;
        }

        HRESULT CLobbyClient::connectFs9ToHost(const QString address)
        {
            HRESULT hr = S_OK;

            GUID pAppGuid = CFs9Sdk::guid();

            // Set to true in order to automatically launch FS9. Perfect for testing.
            bool bLaunchNotFound = false;

            // Setup the DPL_CONNECT_INFO struct
            DPL_CONNECT_INFO dnConnectInfo;
            ZeroMemory(&dnConnectInfo, sizeof(DPL_CONNECT_INFO));
            dnConnectInfo.dwSize = sizeof(DPL_CONNECT_INFO);
            dnConnectInfo.pvLobbyConnectData = nullptr;
            dnConnectInfo.dwLobbyConnectDataSize = 0;
            dnConnectInfo.dwFlags = 0;
            if (bLaunchNotFound) dnConnectInfo.dwFlags |= DPLCONNECT_LAUNCHNOTFOUND;
            dnConnectInfo.guidApplication = pAppGuid;

            if (FAILED(hr = allocAndInitConnectSettings(address, &pAppGuid, &dnConnectInfo.pdplConnectionSettings)))
                return S_FALSE;

            hr = m_dpLobbyClient->ConnectApplication(&dnConnectInfo,
                    nullptr,
                    &m_applicationHandle,
                    INFINITE,
                    0);
            if (FAILED(hr))
            {
                if (hr == DPNERR_NOCONNECTION && !bLaunchNotFound)
                    qWarning() << "There were no waiting application.";
                else
                    return printDirectPlayError(hr);
            }
            else
            {
                qDebug() << "Connected!";
            }

            freeConnectSettings(dnConnectInfo.pdplConnectionSettings);

            return S_OK;
        }

        HRESULT CLobbyClient::allocAndInitConnectSettings(const QString &address, GUID *pAppGuid, DPL_CONNECTION_SETTINGS **ppdplConnectSettings)
        {
            HRESULT hr;

            IDirectPlay8Address *pHostAddress   = nullptr;
            IDirectPlay8Address *pDeviceAddress = nullptr;

            QScopedPointer<GUID> pSPGuid(new GUID);
            memcpy(pSPGuid.data(), &CLSID_DP8SP_TCPIP, sizeof(GUID));


            // Create a host address
            if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Address, nullptr, CLSCTX_INPROC_SERVER,
                                             IID_IDirectPlay8Address, reinterpret_cast<void **>(&pHostAddress))))
            {
                return printDirectPlayError(hr);
            }

            // Set the SP to pHostAddress
            if (FAILED(hr = pHostAddress->SetSP(pSPGuid.data())))
            {
                return printDirectPlayError(hr);
            }

            // Create a device address to specify which device we are using
            if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER,
                                             IID_IDirectPlay8Address, reinterpret_cast<void **>(&pDeviceAddress))))
            {
                return printDirectPlayError(hr);
            }

            // Set the SP to pDeviceAddress
            if (FAILED(hr = pDeviceAddress->SetSP(&CLSID_DP8SP_TCPIP)))
            {
                return printDirectPlayError(hr);
            }

            if (FAILED(hr = pHostAddress->BuildFromURLA(address.toLocal8Bit().data())))
            {
                return printDirectPlayError(hr);
            }

            // Setup the DPL_CONNECTION_SETTINGS
            DPL_CONNECTION_SETTINGS *pSettings = new DPL_CONNECTION_SETTINGS;

            // Allocate space for device address pointers
            // We cannot use QScopedArrayPointer, because memory needs to be valid
            // for the lifetime of DPL_CONNECTION_SETTINGS.
            IDirectPlay8Address **apDevAddress = new IDirectPlay8Address*[1];

            // Set the device addresses
            apDevAddress[0] = pDeviceAddress;

            QString session = BlackMisc::CProject::systemNameAndVersion();
            QScopedArrayPointer<wchar_t> wstrSessionName(new wchar_t[session.size() + 1]);
            session.toWCharArray(wstrSessionName.data());
            wstrSessionName[session.size()] = 0;

            // Fill in the connection settings
            ZeroMemory(pSettings, sizeof(DPL_CONNECTION_SETTINGS));
            pSettings->dwSize                       = sizeof(DPL_CONNECTION_SETTINGS);
            pSettings->dpnAppDesc.dwSize            = sizeof(DPN_APPLICATION_DESC);
            pSettings->dwFlags                      = 0;
            pSettings->dpnAppDesc.guidApplication   = *pAppGuid;
            pSettings->dpnAppDesc.guidInstance      = GUID_NULL;
            pSettings->dpnAppDesc.dwFlags           = DPNSESSION_NODPNSVR;
            pSettings->pdp8HostAddress              = pHostAddress;
            pSettings->ppdp8DeviceAddresses         = apDevAddress;
            pSettings->cNumDeviceAddresses          = 1;
            pSettings->dpnAppDesc.pwszSessionName = new WCHAR[wcslen(wstrSessionName.data()) + 1];
            wcscpy(pSettings->dpnAppDesc.pwszSessionName, wstrSessionName.data());

            // FIXME: Use players callsign
            QString playerName("Player");
            WCHAR wstrPlayerName[m_maxSizePlayerName];
            playerName.toWCharArray(wstrPlayerName);
            wstrPlayerName[playerName.size()] = 0;
            pSettings->pwszPlayerName = new WCHAR[wcslen(wstrPlayerName) + 1];
            wcscpy(pSettings->pwszPlayerName, wstrPlayerName);

            *ppdplConnectSettings = pSettings;

            return S_OK;
        }

        void CLobbyClient::freeConnectSettings(DPL_CONNECTION_SETTINGS *pSettings)
        {
            if (!pSettings) return;

            SafeDeleteArray(pSettings->pwszPlayerName);
            SafeDeleteArray(pSettings->dpnAppDesc.pwszSessionName);
            SafeDeleteArray(pSettings->dpnAppDesc.pwszPassword);
            SafeDeleteArray(pSettings->dpnAppDesc.pvReservedData);
            SafeDeleteArray(pSettings->dpnAppDesc.pvApplicationReservedData);
            SafeRelease(pSettings->pdp8HostAddress);
            SafeRelease(pSettings->ppdp8DeviceAddresses[0]);
            SafeDeleteArray(pSettings->ppdp8DeviceAddresses);
            SafeDelete(pSettings);
        }

        HRESULT CLobbyClient::directPlayMessageHandler(DWORD /* messageId */, void * /* msgBuffer */)
        {
            return S_OK;
        }

        HRESULT CLobbyClient::directPlayLobbyMessageHandler(DWORD messageId, void *msgBuffer)
        {
            switch (messageId)
            {
            case DPL_MSGID_DISCONNECT:
                {
                    PDPL_MESSAGE_DISCONNECT pDisconnectMsg;
                    pDisconnectMsg = (PDPL_MESSAGE_DISCONNECT)msgBuffer;

                    // We should free any data associated with the
                    // app here, but there is none.
                    break;
                }

            case DPL_MSGID_RECEIVE:
                {
                    PDPL_MESSAGE_RECEIVE pReceiveMsg;
                    pReceiveMsg = (PDPL_MESSAGE_RECEIVE)msgBuffer;

                    // The lobby app sent us data.  This sample doesn't
                    // expected data from the app, but it is useful
                    // for more complex clients.
                    break;
                }

            case DPL_MSGID_SESSION_STATUS:
                {
                    PDPL_MESSAGE_SESSION_STATUS pStatusMsg;
                    pStatusMsg = (PDPL_MESSAGE_SESSION_STATUS)msgBuffer;

                    QString message;
                    message.append(QString("%1: ").arg(pStatusMsg->hSender, 0, 16));
                    switch (pStatusMsg->dwStatus)
                    {
                    case DPLSESSION_CONNECTED:
                        message.append("Session connected"); break;
                    case DPLSESSION_COULDNOTCONNECT:
                        message.append("Session could not connect"); break;
                    case DPLSESSION_DISCONNECTED:
                        message.append("Session disconnected"); break;
                    case DPLSESSION_TERMINATED:
                        message.append("Session terminated"); break;
                    case DPLSESSION_HOSTMIGRATED:
                        message.append("Host migrated"); break;
                    case DPLSESSION_HOSTMIGRATEDHERE:
                        message.append("Host migrated to this client"); break;
                    default:
                        message.append("%1").arg(pStatusMsg->dwStatus);
                        break;
                    }
                    qDebug() << message;
                    break;
                }

            case DPL_MSGID_CONNECTION_SETTINGS:
                {
                    PDPL_MESSAGE_CONNECTION_SETTINGS pConnectionStatusMsg;
                    pConnectionStatusMsg = (PDPL_MESSAGE_CONNECTION_SETTINGS)msgBuffer;

                    // The app has changed the connection settings.
                    // This simple client doesn't handle this, but more complex clients may
                    // want to.
                    break;
                }
            }

            return S_OK;
        }
    }
}
