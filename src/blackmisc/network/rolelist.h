/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_NETWORK_ROLELIST_H
#define BLACKMISC_NETWORK_ROLELIST_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/collection.h"
#include "blackmisc/network/role.h"
#include "blackmisc/sequence.h"
#include "blackmisc/variant.h"

#include <QJsonArray>
#include <QMetaType>
#include <QString>
#include <QStringList>

namespace BlackMisc
{
    namespace Network
    {
        class CRole;

        //! Value object encapsulating a list of servers.
        class BLACKMISC_EXPORT CRoleList :
            public CSequence<CRole>,
            public BlackMisc::Mixin::MetaType<CRoleList>
        {
        public:
            BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CRoleList)

            //! Default constructor.
            CRoleList();

            //! Has role?
            bool hasRole(const QString &roleName) const;

            //! Has role?
            bool hasRole(const CRole &role) const;

            //! Has any role?
            bool hasAnyRole(const QStringList &roles) const;

            //! Construct from a base class object.
            CRoleList(const CSequence<CRole> &other);

            //! Roles as string
            QString namesAsString(const QString &separator = ", ") const;

            //! From our database JSON format
            static CRoleList fromDatabaseJson(const QJsonArray &array);
        };
    } //namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Network::CRoleList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::Network::CRole>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::Network::CRole>)

#endif //guard
