/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "voiceroom.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include "blackmisc/propertyindex.h"
#include <QChar>
#include <QStringList>
#include <tuple>

namespace BlackMisc
{
    namespace Audio
    {
        /*
         * Constructor
         */
        CVoiceRoom::CVoiceRoom(const QString &voiceRoomUrl, bool connected) :
            m_connected(connected), m_audioPlaying(false)
        {
            this->setVoiceRoomUrl(voiceRoomUrl);
        }

        /*
         * Property by index
         */
        QVariant CVoiceRoom::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return this->toQVariant(); }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexAudioPlaying:
                return QVariant(this->isAudioPlaying());
            case IndexConnected:
                return QVariant(this->isConnected());
            case IndexChannel:
                return QVariant(this->getChannel());
            case IndexHostname:
                return QVariant(this->getHostname());
            case IndexUrl:
                return QVariant(this->getVoiceRoomUrl());
            default:
                return CValueObject::propertyByIndex(index);
            }
        }

        /*
         * Property by index
         */
        void CVoiceRoom::setPropertyByIndex(const QVariant &variant, const CPropertyIndex &index)
        {
            if (index.isMyself())
            {
                this->convertFromQVariant(variant);
                return;
            }

            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexAudioPlaying:
                this->setAudioPlaying(variant.toBool());
                break;
            case IndexConnected:
                this->setConnected(variant.toBool());
                break;
            case IndexChannel:
                this->setChannel(variant.value<QString>());
                break;
            case IndexHostname:
                this->setHostName(variant.value<QString>());
                break;
            case IndexUrl:
                this->setVoiceRoomUrl(variant.value<QString>());
                break;
            default:
                CValueObject::setPropertyByIndex(variant, index);
                break;
            }
        }

        /*
         * To string
         */
        QString CVoiceRoom::convertToQString(bool /* i18n */) const
        {
            if (!this->isValid()) return "Invalid";
            QString s = this->getVoiceRoomUrl(false);
            s.append(this ->isConnected() ? " connected" : " unconnected");
            if (this->m_audioPlaying) s.append(" playing");
            return s;
        }

        /*
         * Server URL
         */
        QString CVoiceRoom::getVoiceRoomUrl(bool noProtocol) const
        {
            if (!this->isValid()) return "";
            QString url(noProtocol ? "" : CVoiceRoom::protocolComplete());
            url.append(this->m_hostname);
            url.append("/");
            url.append(this->m_channel);
            return url;
        }

        /*
         * Voice room URL
         */
        void CVoiceRoom::setVoiceRoomUrl(const QString &serverUrl)
        {
            if (serverUrl.isEmpty())
            {
                m_hostname = "";
                m_channel = "";
            }
            else if (serverUrl.contains("/"))
            {
                QString url = serverUrl.trimmed().toLower();
                url.replace(CVoiceRoom::protocolComplete(), "");
                url.replace(CVoiceRoom::protocol(), "");
                QStringList splitParts = serverUrl.split("/");
                m_hostname = splitParts.at(0);
                m_channel = splitParts.at(1);
            }
        }

        /*
         * ATIS voice channel
         */
        bool CVoiceRoom::isAtis() const
        {
            return (this->m_channel.contains("ATIS", Qt::CaseInsensitive));
        }
    } // Voice
} // BlackMisc
