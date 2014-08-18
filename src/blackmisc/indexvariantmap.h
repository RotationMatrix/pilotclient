/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#include "variant.h"
#include "propertyindex.h"
#include "valueobject.h"
#include <QVariantMap>
#include <QDBusArgument>

#ifndef BLACKMISC_INDEXVARIANTMAP_H
#define BLACKMISC_INDEXVARIANTMAP_H

namespace BlackMisc
{
    // Forward declaration
    class CPropertyIndex;

    /*!
     * Specialized value object compliant map for variants,
     * based on Column indexes
     */
    class CIndexVariantMap : public CValueObject
    {

    public:

        /*!
         * Constructor
         * \param wildcard when used in search, for setting values irrelevant
         */
        CIndexVariantMap(bool wildcard = false);

        //! Single value constructor
        CIndexVariantMap(const CPropertyIndex &index, const QVariant &value);

        //! Destructor
        virtual ~CIndexVariantMap() {}

        //! Add a value
        void addValue(const CPropertyIndex &index, const QVariant &value);

        //! Add a value as non QVariant
        template<class T> void addValue(const CPropertyIndex &index, const T &value) { this->m_values.insert(index, CVariant::fromValue(value)); }

        //! Is empty?
        bool isEmpty() const { return this->m_values.isEmpty(); }

        //! Value
        QVariant value(const CPropertyIndex &index) const { return this->m_values.value(index).toQVariant(); }

        //! Set value
        void value(const CPropertyIndex &index, const QVariant &value) { this->m_values.value(index, value); }

        //! Indexes
        QList<CPropertyIndex> indexes() const { return this->m_values.keys(); }

        //! Contains index?
        bool contains(const CPropertyIndex &index) const { return this->m_values.contains(index); }

        //! values
        QList<CVariant> values() const { return this->m_values.values(); }

        //! Wildcard, only relevant when used in search
        bool isWildcard() const { return this->m_wildcard; }

        //! Wildcard, only relevant when used in search
        void setWildcard(bool wildcard) { this->m_wildcard = wildcard; }

        //! clear
        void clear() { this->m_values.clear(); }

        //! Equal operator, required if maps are directly compared, not with CValueObject
        bool operator ==(const CIndexVariantMap &other) const;

        //! Equal operator, required if maps are directly compared, not with CValueObject
        bool operator !=(const CIndexVariantMap &other) const;

        //! Map
        const QMap<CPropertyIndex, CVariant> &map() const { return this->m_values; }

        //! \copydoc CValueObject::getValueHash
        virtual uint getValueHash() const override;

        //! \copydoc CValueObject::toQVariant
        virtual QVariant toQVariant() const override { return QVariant::fromValue(*this); }

        //! \copydoc CValueObject::fromQVariant
        virtual void fromQVariant(const QVariant &variant) override { BlackMisc::setFromQVariant(this, variant); }

        //! register metadata
        static void registerMetadata();

    protected:
        QMap<CPropertyIndex, CVariant> m_values; //!< values
        bool m_wildcard; //!< wildcard

        //! \copydoc CValueObject::convertToQString
        virtual QString convertToQString(bool i18n = false) const override;

        //! \copydoc CValueObject::getMetaTypeId
        virtual int getMetaTypeId() const override;

        //! \copydoc CValueObject::isA
        virtual bool isA(int metaTypeId) const override;

        //! \copydoc CValueObject::compareImpl
        virtual int compareImpl(const CValueObject &other) const override;

        //! \copydoc CValueObject::marshallToDbus
        virtual void marshallToDbus(QDBusArgument &argument) const override;

        //! \copydoc CValueObject::unmarshallFromDbus
        virtual void unmarshallFromDbus(const QDBusArgument &argument) override;
    };
}

Q_DECLARE_METATYPE(BlackMisc::CIndexVariantMap)

#endif // guard
