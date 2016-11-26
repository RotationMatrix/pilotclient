/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbinfo.h"
#include "blackmisc/comparefunctions.h"
#include <QPainter>

using namespace BlackMisc::Network;

namespace BlackMisc
{
    namespace Db
    {
        CDbInfo::CDbInfo(int key, const QString &tableName, int entries) :
            IDatastoreObjectWithIntegerKey(key),
            m_tableName(tableName.trimmed().toLower()), m_entries(entries)
        {
            this->setEntity(this->getEntity());
            Q_ASSERT_X(tableName.isEmpty() || this->m_entity != CEntityFlags::NoEntity, Q_FUNC_INFO, "Wrong entity");
        }

        bool CDbInfo::isValid() const
        {
            return this->m_entity != CEntityFlags::NoEntity && !this->m_tableName.isEmpty();
        }

        CEntityFlags::Entity CDbInfo::getEntity() const
        {
            if (this->m_entity != CEntityFlags::NoEntity) { return this->m_entity; }
            const QString tn(this->getTableName());
            return CEntityFlags::singleEntityByName(tn);
        }

        const QString &CDbInfo::getSharedFileName() const
        {
            const CEntityFlags::Entity entity = CEntityFlags::singleEntityByName(this->getTableName());
            return CDbInfo::entityToSharedName(entity);
        }

        const QString &CDbInfo::getServiceName() const
        {
            const CEntityFlags::Entity entity = CEntityFlags::singleEntityByName(this->getTableName());
            return CDbInfo::entityToServiceName(entity);
        }

        void CDbInfo::setEntity(CEntityFlags::Entity entity)
        {
            this->m_entity = entity;
        }

        bool CDbInfo::matchesEntity(CEntityFlags::Entity entity) const
        {
            return entity.testFlag(CEntityFlags::entityToEntityFlag(this->getEntity()));
        }

        void CDbInfo::setTableName(const QString &tableName)
        {
            m_tableName = tableName.trimmed().toLower();
            this->m_entity = this->getEntity();
        }

        QString CDbInfo::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            QString s("Table %1 with entries %1");
            return s.arg(this->m_tableName).arg(this->m_entries);
        }

        CVariant CDbInfo::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTableName:
                return CVariant::fromValue(m_tableName);
            case IndexEntries:
                return CVariant::fromValue(m_entries);
            case IndexEntity:
                return CVariant::fromValue(m_entity);
            default:
                return (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) ?
                       IDatastoreObjectWithIntegerKey::propertyByIndex(index) :
                       CValueObject::propertyByIndex(index);
            }
        }

        void CDbInfo::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CDbInfo>(); return; }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTableName:
                this->setTableName(variant.toQString());
                break;
            case IndexEntries:
                this->setTableName(variant.toQString());
                break;
            case IndexEntity:
                this->setEntity(static_cast<CEntityFlags::Entity>(variant.toInt()));
                break;
            default:
                return (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) ?
                       IDatastoreObjectWithIntegerKey::setPropertyByIndex(index, variant) :
                       CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }

        int CDbInfo::comparePropertyByIndex(const CPropertyIndex &index, const CDbInfo &compareValue) const
        {
            if (index.isMyself()) { return getTableName().compare(compareValue.getTableName(), Qt::CaseInsensitive); }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { return IDatastoreObjectWithIntegerKey::comparePropertyByIndex(index, compareValue);}
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTableName:
                return getTableName().compare(compareValue.getTableName(), Qt::CaseInsensitive);
            case IndexEntries:
                return Compare::compare(this->getEntries(), compareValue.getEntries());
            case IndexEntity:
                return Compare::compare(this->getEntity(), compareValue.getEntity());
            default:
                Q_ASSERT_X(false, Q_FUNC_INFO, "No comparison possible");
            }
            return 0;
        }

        CDbInfo CDbInfo::fromDatabaseJson(const QJsonObject &json, const QString &prefix)
        {
            if (!existsKey(json, prefix))
            {
                // when using relationship, this can be null
                return CDbInfo();
            }
            const int id(json.value(prefix + "id").toInt());
            const int entries(json.value(prefix + "entries").toInt());
            const QString tableName(json.value(prefix + "tablename").toString());
            CDbInfo dbInfo(id, tableName, entries);
            dbInfo.setKeyAndTimestampFromDatabaseJson(json, prefix);
            return dbInfo;
        }

        const QStringList &CDbInfo::sharedFileNames()
        {
            static const QStringList names({"aircrafticao.json", "airlineicao.json", "airports.json", "countries.json", "distributors.json", "liveries.json", "models.json" });
            return names;
        }

        const QStringList &CDbInfo::serviceNames()
        {
            static const QStringList names({"jsonaircrafticao.php", "jsonairlineicao.php", "jsonairport.php", "jsoncountry.php", "jsondistributor.php", "jsonlivery.php", "jsonaircraftmodel.php" });
            return names;
        }

        const QString &CDbInfo::entityToSharedName(CEntityFlags::Entity entity)
        {
            static const QString empty;
            switch (entity)
            {
            case CEntityFlags::AircraftIcaoEntity: return sharedFileNames().at(0);
            case CEntityFlags::AirlineIcaoEntity: return sharedFileNames().at(1);
            case CEntityFlags::AirportEntity: return sharedFileNames().at(2);
            case CEntityFlags::CountryEntity: return sharedFileNames().at(3);
            case CEntityFlags::DistributorEntity: return sharedFileNames().at(4);
            case CEntityFlags::LiveryEntity: return sharedFileNames().at(5);
            case CEntityFlags::ModelEntity: return sharedFileNames().at(6);
            default:
                break;
            }
            return empty;
        }

        const QString &CDbInfo::entityToServiceName(CEntityFlags::Entity entity)
        {
            static const QString empty;
            switch (entity)
            {
            case CEntityFlags::AircraftIcaoEntity: return serviceNames().at(0);
            case CEntityFlags::AirlineIcaoEntity: return serviceNames().at(1);
            case CEntityFlags::AirportEntity: return serviceNames().at(2);
            case CEntityFlags::CountryEntity: return serviceNames().at(3);
            case CEntityFlags::DistributorEntity: return serviceNames().at(4);
            case CEntityFlags::LiveryEntity: return serviceNames().at(5);
            case CEntityFlags::ModelEntity: return serviceNames().at(6);
            default:
                break;
            }
            return empty;
        }
    } // namespace
} // namespace
