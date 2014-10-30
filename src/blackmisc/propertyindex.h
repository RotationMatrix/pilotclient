/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_PROPERTYINDEX_H
#define BLACKMISC_PROPERTYINDEX_H

#include "valueobject.h"
#include "blackmiscfreefunctions.h"
#include <initializer_list>

namespace BlackMisc
{

    /*!
     * Property index. The index can be nested, that's why it is a sequence
     * (e.g. PropertyIndexPilot, PropertyIndexRealname).
     */
    class CPropertyIndex : public CValueObjectStdTuple<CPropertyIndex>
    {
        // In the first trial I have used CSequence<int> as base class
        // This has created too much circular dependencies of the headers
        // CIndexVariantMap is used in CValueObject, CPropertyIndex in CIndexVariantMap

    public:
        //! Global index, make sure the indexes are unqiue (for using them in class hierarchy)
        enum GlobalIndex
        {
            GlobalIndexCValueObject           =   10, // GlobalIndexCValueObject needs to be set manually in CValueObject
            GlobalIndexCPhysicalQuantity      =   100,
            GlobalIndexCStatusMessage         =   200,
            GlobalIndexCNameVariantPair       =   300,
            GlobalIndexCCallsign              =  1000,
            GlobalIndexCAircraftIcao          =  1100,
            GlobalIndexCAircraft              =  1200,
            GlobalIndexCAircraftSituation     =  1300,
            GlobalIndexCAtcStation            =  1400,
            GlobalIndexCAirport               =  1500,
            GlobalIndexCModulator             =  2000,
            GlobalIndexCTransponder           =  2100,
            GlobalIndexICoordinateGeodetic    =  3000,
            GlobalIndexCCoordinateGeodetic    =  3100,
            GlobalIndexCClient                =  4000,
            GlobalIndexCUser                  =  4100,
            GlobalIndexCServer                =  4200,
            GlobalIndexCAircraftModel         =  4300,
            GlobalIndexCVoiceRoom             =  5000,
            GlobalIndexCSettingKeyboardHotkey =  6000,
            GlobalIndexCAircraftMapping       =  7000,
            GlobalIndexCAircraftCfgEntries    =  7100,
            GlobalIndexAbuseMode              = 20000 // property index abused as map key or otherwise
        };

        //! Default constructor.
        CPropertyIndex();

        //! Non nested index
        CPropertyIndex(int singleProperty);

        //! Initializer list constructor
        CPropertyIndex(std::initializer_list<int> il);

        //! Construct from a base class object.
        CPropertyIndex(const QList<int> &indexes);

        //! From string
        CPropertyIndex(const QString &indexes);

        //! Copy with first element removed
        CPropertyIndex copyFrontRemoved() const;

        //! Is nested index?
        bool isNested() const;

        //! Myself index, used with nesting
        bool isMyself() const { return this->m_indexes.isEmpty(); }

        //! Empty?
        bool isEmpty() const { return this->m_indexes.isEmpty(); }

        //! First element casted to given type, usually then PropertIndex enum
        template<class CastType> CastType frontCasted() const
        {
            Q_ASSERT(!this->m_indexes.isEmpty());
            int f = this->m_indexes.isEmpty() ? 0 : this->m_indexes.first();
            return static_cast<CastType>(f);
        }

    protected:
        //! \copydoc CValueObject::convertToQString
        virtual QString convertToQString(bool i18n = false) const override;

        //! \copydoc CValueObject::parseFromString
        virtual void parseFromString(const QString &indexes) override;

    private:
        BLACK_ENABLE_TUPLE_CONVERSION(CPropertyIndex)
        QList<int> m_indexes;
        QString m_indexString; //! I use a little trick here, the string is used with the tupel system, as it provides all operators, hash ..

        //! Convert list to string
        void listToString();

        //! String to list
        void stringToList();

    };
} //namespace

BLACK_DECLARE_TUPLE_CONVERSION(BlackMisc::CPropertyIndex, (o.m_indexString))
Q_DECLARE_METATYPE(BlackMisc::CPropertyIndex)

#endif //guard
