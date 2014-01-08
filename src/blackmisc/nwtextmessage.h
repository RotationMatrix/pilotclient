/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*!
    \file
*/

#ifndef BLACKMISC_TEXTMESSAGE_H
#define BLACKMISC_TEXTMESSAGE_H

#include "pqfrequency.h"
#include "avcallsign.h"
#include "valueobject.h"

namespace BlackMisc
{
    namespace Network
    {
        /*!
         * Value object encapsulating information of a textserver
         */
        class CTextMessage : public BlackMisc::CValueObject
        {
        public:
            /*!
             * Default constructor.
             */
            CTextMessage() : m_frequency(-1.0, BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz()) {}

            /*!
             * Constructor, radio message
             * \param message
             * \param frequency
             * \param sender
             */
            CTextMessage(const QString &message, const BlackMisc::PhysicalQuantities::CFrequency &frequency, const BlackMisc::Aviation::CCallsign &sender = BlackMisc::Aviation::CCallsign())
                : m_message(message), m_sender(sender), m_frequency(frequency)
            {
                this->m_frequency.switchUnit(BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz());
            }

            /*!
             * Constructor, private message
             * \param message
             * \param sender
             * \param recipient
             */
            CTextMessage(const QString &message, const BlackMisc::Aviation::CCallsign &sender, const BlackMisc::Aviation::CCallsign &recipient = BlackMisc::Aviation::CCallsign())
                : m_message(message), m_sender(sender), m_recipient(recipient), m_frequency(-1.0, BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz()) {}

            /*!
             * \brief QVariant, required for DBus QVariant lists
             * \return
             */
            virtual QVariant toQVariant() const
            {
                return QVariant::fromValue(*this);
            }

            /*!
             * Get callsign (from)
             * \return
             */
            const BlackMisc::Aviation::CCallsign &getSender() const { return m_sender; }

            /*!
             * Set callsign (from)
             * \param
             */
            void setSender(const BlackMisc::Aviation::CCallsign &sender) { m_sender = sender; }

            /*!
             * Get callsign (to)
             * \return
             */
            const BlackMisc::Aviation::CCallsign &getRecipient() const { return m_recipient; }

            /*!
             * Set callsign (to)
             * \param
             */
            void setRecipient(const BlackMisc::Aviation::CCallsign &recipient) { m_recipient = recipient; }

            /*
             * Send to frequency?
             * \param frequency
             * \return
             */
            bool isSendToFrequency(const BlackMisc::PhysicalQuantities::CFrequency &frequency) const;

            /*!
             * Send to UNICOM?
             * \return
             */
            bool isSendToUnicom() const;

            /*!
             * \brief Valid recipient?
             * \return
             */
            bool hasValidRecipient() const;

            /*!
             * Get message
             * \return
             */
            const QString &getMessage() const { return m_message; }

            /*!
             * Empty message
             * \return
             */
            bool isEmpty() const { return m_message.isEmpty(); }

            /*!
             * Set message
             * \param
             */
            void setMessage(const QString &message) { m_message = message.trimmed(); }

            /*!
             * Get frequency
             * \return
             */
            const BlackMisc::PhysicalQuantities::CFrequency &getFrequency() const { return m_frequency; }

            /*!
             * Set frequency
             * \param
             */
            void setFrequency(const BlackMisc::PhysicalQuantities::CFrequency &frequency) { m_frequency = frequency; }

            /*!
             * \brief Is private message?
             * \return
             */
            bool isPrivateMessage() const;

            /*!
             * \brief Is radio message?
             * \return
             */
            bool isRadioMessage() const;

            /*!
             * Whole message as formatted string.
             * Used to display message in a console window.
             * \param withSender
             * \param withRecipient
             * \param separator
             * \return
             */
            QString asString(bool withSender, bool withRecipient, QString separator = ", ") const;

            /*!
             * Toggle sender receiver, can be used to ping my own message
             */
            void toggleSenderRecipient();

            /*!
             * \brief Equal operator ==
             * \param other
             * @return
             */
            bool operator ==(const CTextMessage &other) const;

            /*!
             * \brief Unequal operator ==
             * \param other
             * @return
             */
            bool operator !=(const CTextMessage &other) const;

            /*!
             * \brief Value hash
             */
            virtual uint getValueHash() const;

            /*!
             * \brief Register metadata
             */
            static void registerMetadata();

        protected:
            /*!
             * \brief Rounded value as string
             * \param i18n
             * \return
             */
            virtual QString convertToQString(bool i18n = false) const;

            /*!
             * \brief Stream to DBus <<
             * \param argument
             */
            virtual void marshallToDbus(QDBusArgument &argument) const;

            /*!
             * \brief Stream from DBus >>
             * \param argument
             */
            virtual void unmarshallFromDbus(const QDBusArgument &argument);

        private:
            QString m_message;
            BlackMisc::Aviation::CCallsign m_sender;
            BlackMisc::Aviation::CCallsign m_recipient;
            BlackMisc::PhysicalQuantities::CFrequency m_frequency;
        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Network::CTextMessage)

#endif // guard
