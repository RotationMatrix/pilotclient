/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_COORDINATEGEODETIC_H
#define BLACKMISC_COORDINATEGEODETIC_H

#include "blackmisc/mathvector3dbase.h"
#include "blackmisc/geolatitude.h"
#include "blackmisc/geolongitude.h"
#include "blackmisc/pqlength.h"
#include "propertyindex.h"

namespace BlackMisc
{
    namespace Geo
    {

        /*!
         * Latitude and longitude interface
         * Interface for geodetic ccordinates
         */
        class ICoordinateGeodetic
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexLatitude = BlackMisc::CPropertyIndex::GlobalIndexICoordinateGeodetic,
                IndexLongitude,
                IndexLatitudeAsString,
                IndexLongitudeAsString
            };

            //! Destructor
            virtual ~ICoordinateGeodetic() {}

            //! Latitude
            virtual const CLatitude &latitude() const = 0;

            //! Longitude
            virtual const CLongitude &longitude() const = 0;

            //! \copydoc CValueObject::propertyByIndex
            virtual QVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! Latitude as string
            QString latitudeAsString() const { return this->latitude().toQString(true); }

            //! Longitude as string
            QString longitudeAsString() const { return this->longitude().toQString(true); }

            //! Great circle distance
            BlackMisc::PhysicalQuantities::CLength greatCircleDistance(const ICoordinateGeodetic &otherCoordinate) const;

            //! Initial bearing
            BlackMisc::PhysicalQuantities::CAngle initialBearing(const ICoordinateGeodetic &otherCoordinate) const;

            //! Can given index be handled
            static bool canHandleIndex(const BlackMisc::CPropertyIndex &index)
            {
                int i = index.frontCasted<int>();
                return (i >= static_cast<int>(IndexLatitude)) && (i <= static_cast<int>(IndexLongitudeAsString));
            }
        };

        //! Great circle distance between points
        BlackMisc::PhysicalQuantities::CLength greatCircleDistance(const ICoordinateGeodetic &coordinate1, const ICoordinateGeodetic &coordinate2);

        //! Initial bearing
        BlackMisc::PhysicalQuantities::CAngle initialBearing(const ICoordinateGeodetic &coordinate1, const ICoordinateGeodetic &coordinate2);

        //! Geodetic coordinate
        //! \sa http://www.esri.com/news/arcuser/0703/geoid1of3.html
        //! \sa http://http://www.gmat.unsw.edu.au/snap/gps/clynch_pdfs/coordcvt.pdf (page 5)
        //! \sa http://en.wikipedia.org/wiki/Geodetic_datum#Vertical_datum
        class CCoordinateGeodetic : public CValueObject, public ICoordinateGeodetic
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexLatitude = BlackMisc::CPropertyIndex::GlobalIndexCCoordinateGeodetic,
                IndexLongitude,
                IndexLatitudeAsString,
                IndexLongitudeAsString,
                IndexGeodeticHeight,
                IndexGeodeticHeightAsString
            };

        private:
            BLACK_ENABLE_TUPLE_CONVERSION(CCoordinateGeodetic)
            BlackMisc::Geo::CLatitude m_latitude; //!< Latitude
            BlackMisc::Geo::CLongitude m_longitude; //!< Longitude
            BlackMisc::PhysicalQuantities::CLength m_geodeticHeight; //!< height, ellipsoidal or geodetic height

        protected:
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

        public:
            //! Default constructor
            CCoordinateGeodetic() : m_latitude(), m_longitude(), m_geodeticHeight() {}

            //! Constructor by values
            CCoordinateGeodetic(CLatitude latitude, CLongitude longitude, BlackMisc::PhysicalQuantities::CLength height) :
                m_latitude(latitude), m_longitude(longitude), m_geodeticHeight(height) {}

            //! Constructor by values
            CCoordinateGeodetic(double latitudeDegrees, double longitudeDegrees, double heightMeters) :
                m_latitude(latitudeDegrees, BlackMisc::PhysicalQuantities::CAngleUnit::deg()), m_longitude(longitudeDegrees, BlackMisc::PhysicalQuantities::CAngleUnit::deg()), m_geodeticHeight(heightMeters, BlackMisc::PhysicalQuantities::CLengthUnit::m()) {}

            //! \copydoc ICoordinateGeodetic::latitude
            virtual const CLatitude &latitude() const override { return this->m_latitude; }

            //! \copydoc ICoordinateGeodetic::longitude
            virtual const CLongitude &longitude() const override { return this->m_longitude; }

            //! Height, ellipsoidal or geodetic height (used in GPS)
            //! This is approximately MSL (orthometric) height, aka elevation.
            //! \sa see http://www.gmat.unsw.edu.au/snap/gps/clynch_pdfs/coordcvt.pdf page 5
            //! \sa http://www.esri.com/news/arcuser/0703/geoid1of3.html
            const BlackMisc::PhysicalQuantities::CLength &geodeticHeight() const { return this->m_geodeticHeight; }

            //! \copydoc CValueObject::toQVariant
            virtual QVariant toQVariant() const override  { return QVariant::fromValue(*this); }

            //! \copydoc CValueObject::fromQVariant
            virtual void fromQVariant(const QVariant &variant) override { BlackMisc::setFromQVariant(this, variant); }

            //! \copydoc CValueObject::propertyByIndex
            virtual QVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const override;

            //! \copydoc CValueObject::setPropertyByIndex
            virtual void setPropertyByIndex(const QVariant &variant, const BlackMisc::CPropertyIndex &index) override;

            //! Switch unit of latitude / longitude
            CCoordinateGeodetic &switchUnit(const BlackMisc::PhysicalQuantities::CAngleUnit &unit)
            {
                this->m_latitude.switchUnit(unit);
                this->m_longitude.switchUnit(unit);
                return *this;
            }

            //! Switch unit of height
            CCoordinateGeodetic &switchUnit(const BlackMisc::PhysicalQuantities::CLengthUnit &unit)
            {
                this->m_geodeticHeight.switchUnit(unit);
                return *this;
            }

            //! Set latitude
            void setLatitude(const CLatitude &latitude) { this->m_latitude = latitude; }

            //! Set longitude
            void setLongitude(const CLongitude &longitude) { this->m_longitude = longitude; }

            //! Set height (ellipsoidal or geodetic height)
            void setGeodeticHeight(const BlackMisc::PhysicalQuantities::CLength &height) { this->m_geodeticHeight = height; }

            //! Equal operator ==
            bool operator ==(const CCoordinateGeodetic &other) const;

            //! Unequal operator !=
            bool operator !=(const CCoordinateGeodetic &other) const;

            //! \copydoc CValueObject::getValueHash
            virtual uint getValueHash() const override;

            //! \copydoc CValueObject::toJson
            virtual QJsonObject toJson() const override;

            //! \copydoc CValueObject::fromJson
            void fromJson(const QJsonObject &json) override;

            //! Register metadata
            static void registerMetadata();

            //! Members
            static const QStringList &jsonMembers();

            //! Coordinate by WGS84 position data
            static CCoordinateGeodetic fromWgs84(const QString &latitudeWgs84, const QString &longitudeWgs84, const BlackMisc::PhysicalQuantities::CLength geodeticHeight = BlackMisc::PhysicalQuantities::CLength());

        };

    } // namespace
} // namespace

BLACK_DECLARE_TUPLE_CONVERSION(BlackMisc::Geo::CCoordinateGeodetic, (o.m_latitude, o.m_longitude, o.m_geodeticHeight))
Q_DECLARE_METATYPE(BlackMisc::Geo::CCoordinateGeodetic)

#endif // guard
