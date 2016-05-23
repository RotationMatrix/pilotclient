/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_COUNTRYLIST_H
#define BLACKMISC_COUNTRYLIST_H

#include "blackmisc/db/datastoreobjectlist.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/collection.h"
#include "blackmisc/country.h"
#include "blackmisc/sequence.h"
#include "blackmisc/variant.h"

#include <QJsonArray>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <tuple>

namespace BlackMisc
{
    //! Value object encapsulating a list of countries.
    //! \remark: I do not use CCollection as I want to sort per column
    class BLACKMISC_EXPORT CCountryList :
        public CSequence<CCountry>,
        public BlackMisc::Db::IDatastoreObjectList<CCountry, CCountryList, QString>,
        public BlackMisc::Mixin::MetaType<CCountryList>
    {
    public:
        BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CCountryList)

        //! Default constructor.
        CCountryList() = default;

        //! Construct from a base class object.
        CCountryList(const CSequence<CCountry> &other);

        //! Find by ISO code
        CCountry findByIsoCode(const QString &isoCode) const;

        //! Find "best match" by country
        CCountry findBestMatchByCountryName(const QString &countryName) const;

        //! ISO/name string list
        QStringList toIsoNameList() const;

        //! Name/ISO string list
        QStringList toNameIsoList() const;

        //! Name string list
        QStringList toNameList() const;

        //! From our database JSON format
        static CCountryList fromDatabaseJson(const QJsonArray &array);
    };

} //namespace

Q_DECLARE_METATYPE(BlackMisc::CCountryList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::CCountry>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::CCountry>)

#endif //guard
