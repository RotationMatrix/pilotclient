/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "ecosystem.h"
#include "blackmisc/icon.h"
#include "blackmisc/verify.h"
#include "blackmisc/comparefunctions.h"

namespace BlackMisc
{
    namespace Network
    {
        QString CEcosystem::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            return this->getSystemString();
        }

        const CEcosystem &CEcosystem::unspecified()
        {
            static const CEcosystem e(Unspecified);
            return e;
        }

        const CEcosystem &CEcosystem::vatsim()
        {
            static const CEcosystem e(VATSIM);
            return e;
        }

        const CEcosystem &CEcosystem::swift()
        {
            static const CEcosystem e(Swift);
            return e;
        }

        const CEcosystem &CEcosystem::swiftTest()
        {
            static const CEcosystem e(SwiftTest);
            return e;
        }

        const CEcosystem &CEcosystem::privateFsd()
        {
            static const CEcosystem e(PrivateFSD);
            return e;
        }

        const QString &CEcosystem::getSystemString() const
        {
            static const QString u("unknown");
            static const QString v("VATSIM");
            static const QString s("swift");
            static const QString st("swift (testing)");
            static const QString fsd("FSD (private)");
            static const QString no("no system");

            switch (this->getSystem())
            {
            case VATSIM: return v;
            case Swift: return s;
            case SwiftTest: return st;
            case PrivateFSD: return fsd;
            case NoSystem: return no;
            case Unspecified:
            default: return u;
            }
        }

        CIcon CEcosystem::toIcon() const
        {
            switch (this->getSystem())
            {
            case VATSIM: return CIconList::allIcons().findByIndex(CIcons::NetworkVatsimLogo);
            case Swift: return CIconList::allIcons().findByIndex(CIcons::Swift24);
            case SwiftTest: return CIconList::allIcons().findByIndex(CIcons::Swift24);
            case PrivateFSD: return CIconList::allIcons().findByIndex(CIcons::StandardIconAppAircraft16);
            case NoSystem: return CIconList::allIcons().findByIndex(CIcons::StandardIconCrossCircle16);
            case Unspecified:
            default: return CIconList::allIcons().findByIndex(CIcons::StandardIconUnknown16);
            }
        }

        CVariant CEcosystem::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexSystem: return CVariant::fromValue(m_system);
            case IndexSystemString: return CVariant::fromValue(this->getSystemString());
            default: return CValueObject::propertyByIndex(index);
            }
        }

        void CEcosystem::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CEcosystem>(); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexSystem: m_system = variant.toInt(); break;
            default: CValueObject::setPropertyByIndex(index, variant); break;
            }
        }

        int CEcosystem::comparePropertyByIndex(const CPropertyIndex &index, const CEcosystem &compareValue) const
        {
            if (index.isMyself()) { return Compare::compare(this->m_system, compareValue.m_system); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexSystem: return Compare::compare(this->m_system, compareValue.m_system);
            default: break;
            }
            BLACK_VERIFY_X(false, Q_FUNC_INFO, qUtf8Printable("No comparison for index " + index.toQString()));
            return 0;
        }
    } // namespace
} // namespace
