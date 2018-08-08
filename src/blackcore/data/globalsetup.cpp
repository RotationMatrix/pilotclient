/* Copyright (C) 2015
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackcore/data/globalsetup.h"
#include "blackcore/application.h"
#include "blackmisc/json.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/network/server.h"
#include "blackmisc/network/user.h"
#include "blackmisc/stringutils.h"

#include <QVersionNumber>
#include <QJsonObject>
#include <QStringList>
#include <QStringBuilder>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Json;
using namespace BlackMisc::Network;

namespace BlackCore
{
    namespace Data
    {
        CGlobalSetup::CGlobalSetup() :
            CIdentifiable("CGlobalSetup"),
            ITimestampBased(0)
        {
            this->initDefaultValues();
        }

        void CGlobalSetup::initDefaultValues()
        {
            m_mappingMinimumVersion = CBuildConfig::getVersionString();
            m_dbRootDirectoryUrl = CUrl("https://datastore.swift-project.org/");
            m_vatsimBookingsUrl = CUrl("http://vatbook.euroutepro.com/xml2.php");
            m_vatsimMetarsUrls = CUrlList({"http://metar.vatsim.net/metar.php"});
            m_vatsimStatusFileUrls = CUrlList({ "https://status.vatsim.net" });
            m_vatsimDataFileUrls = CUrlList({ "http://info.vroute.net/vatsim-data.txt" });
            m_sharedUrls = CUrlList(
            {
                "https://datastore.swift-project.net/shared/",
                "http://www.siliconmind.de/datastore/shared/",
                "http://swift-project.org/datastore/shared/"
            });

            // spare: "https://vatsim-germany.org:50443/datastore/shared"
            m_newsUrls = CUrlList({ "http://swift-project.org/" });
            m_onlineHelpUrls = CUrlList({ "https://datastore.swift-project.org/page/swifthelpdispatcher.html" });
            m_mapUrls = CUrlList({ "map.swift-project.org/" });
            m_ncepGlobalForecastSystemUrl = CUrl("http://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_0p50.pl");
        }

        CUrl CGlobalSetup::getDbIcaoReaderUrl() const
        {
            return getDbRootDirectoryUrl();
        }

        CUrl CGlobalSetup::getDbModelReaderUrl() const
        {
            return getDbRootDirectoryUrl();
        }

        CUrl CGlobalSetup::getDbAirportReaderUrl() const
        {
            return getDbRootDirectoryUrl();
        }

        CUrl CGlobalSetup::getDbInfoReaderUrl() const
        {
            return getDbRootDirectoryUrl();
        }

        const CUrlList &CGlobalSetup::getSwiftSharedUrls() const
        {
            return m_sharedUrls;
        }

        CUrl CGlobalSetup::getCorrespondingSharedUrl(const CUrl &candidate) const
        {
            CUrlList sameHosts = this->getSwiftSharedUrls().findByHost(candidate.getHost());
            return sameHosts.frontOrDefault();
        }

        CUrlList CGlobalSetup::getSwiftBootstrapFileUrls() const
        {
            return getSwiftSharedUrls().withAppendedPath(CGlobalSetup::schemaVersionString() + "/bootstrap/" + CDirectoryUtils::bootstrapFileName());
        }

        CUrlList CGlobalSetup::getSwiftUpdateInfoFileUrls() const
        {
            return getSwiftSharedUrls().withAppendedPath(CGlobalSetup::schemaVersionString() + "/updateinfo/updateinfo.json");
        }

        CUrl CGlobalSetup::getDbHomePageUrl() const
        {
            return getDbRootDirectoryUrl().withAppendedPath("/page/index.php");
        }

        CUrl CGlobalSetup::getHelpPageUrl(const QString &context) const
        {
            const CUrlList urls(m_onlineHelpUrls);

            // we display in the standard browser,
            // so the user will realize if the URL does not work
            CUrl url = (urls.size() < 2) ? urls.frontOrDefault() : urls.getRandomUrl();
            if (url.isEmpty()) { return url; }

            // context string something like "application.moreSpecific.evenMoreSpecific"
            QString c = "client";
            if (QCoreApplication::instance())
            {
                c = QCoreApplication::instance()->applicationName();
            }
            if (!context.isEmpty()) { c += "." + context; }
            url.appendQuery("context", c);
            return url;
        }

        CUrl CGlobalSetup::getLegalDirectoryUrl() const
        {
            return getDbRootDirectoryUrl().withAppendedPath("/legal/");
        }

        CUrl CGlobalSetup::getDbLoginServiceUrl() const
        {
            return getDbRootDirectoryUrl().
                   withAppendedPath("/service/jsonauthenticate.php").
                   withSwitchedScheme("https", m_dbHttpsPort);
        }

        CUrl CGlobalSetup::getDbClientPingServiceUrl() const
        {
            return getDbRootDirectoryUrl().
                   withAppendedPath("/service/clientping.php").
                   withSwitchedScheme("https", m_dbHttpsPort);
        }

        CUrl CGlobalSetup::getDbClientPingServiceUrl(PingType type) const
        {
            CUrl pingUrl = this->getDbClientPingServiceUrl();
            if (pingUrl.isEmpty()) { CUrl(); }

            pingUrl.appendQuery("uuid", this->identifier().toUuidString());
            pingUrl.appendQuery("application", sApp->getApplicationNameAndVersion());
            if (type.testFlag(PingLogoff)) { pingUrl.appendQuery("logoff", "true"); }
            if (type.testFlag(PingShutdown)) { pingUrl.appendQuery("shutdown", "true"); }
            if (type.testFlag(PingStarted)) { pingUrl.appendQuery("started", "true"); }
            return pingUrl;
        }

        CUrl CGlobalSetup::getAlphaXSwiftBusFilesServiceUrl() const
        {
            return getDbRootDirectoryUrl().
                   withAppendedPath("/service/jsonalphaxswiftbusfiles.php").
                   withSwitchedScheme("https", m_dbHttpsPort);
        }

        bool CGlobalSetup::dbDebugFlag() const
        {
            if (!m_dbDebugFlag) { return false; }

            // further checks could go here
            const bool f = isDevelopment();
            return f;
        }

        void CGlobalSetup::setServerDebugFlag(bool debug)
        {
            m_dbDebugFlag = debug;
        }

        bool CGlobalSetup::hasSameType(const CGlobalSetup &otherSetup) const
        {
            return this->isDevelopment() == otherSetup.isDevelopment();
        }

        QString CGlobalSetup::buildBootstrapFileUrl(const QString &candidate)
        {
            if (candidate.isEmpty()) return ""; // not possible
            static const QString version(QString(CGlobalSetup::schemaVersionString()).append("/"));
            if (candidate.endsWith(CDirectoryUtils::bootstrapFileName())) { return candidate; }
            CUrl url(candidate);
            if (candidate.contains("/bootstrap"))
            {
                url.appendPath(CDirectoryUtils::bootstrapFileName());
            }
            else if (candidate.endsWith(CGlobalSetup::schemaVersionString()) || candidate.endsWith(version))
            {
                url.appendPath("/bootstrap/" + CDirectoryUtils::bootstrapFileName());
            }
            else if (candidate.endsWith("shared") || candidate.endsWith("shared/"))
            {
                url.appendPath(CGlobalSetup::schemaVersionString() + "/bootstrap/" + CDirectoryUtils::bootstrapFileName());
            }
            else
            {
                url.appendPath("shared/" + CGlobalSetup::schemaVersionString() + "/bootstrap/" + CDirectoryUtils::bootstrapFileName());
            }
            return url.getFullUrl();
        }

        CUrl CGlobalSetup::buildDbDataDirectoryUrl(const CUrl &candidate)
        {
            if (candidate.isEmpty()) return CUrl(); // not possible
            static const QString version(QString(schemaVersionString()).append("/"));
            if (candidate.pathEndsWith("dbdata") || candidate.pathEndsWith("dbdata/")) { return candidate; }
            CUrl url(candidate);
            if (candidate.pathEndsWith(schemaVersionString()) || candidate.pathEndsWith(version))
            {
                url.appendPath("/dbdata");
            }
            else if (candidate.pathEndsWith("shared") || candidate.pathEndsWith("shared/"))
            {
                url.appendPath(CGlobalSetup::schemaVersionString() + "/dbdata/");
            }
            else
            {
                url.appendPath("shared/" + CGlobalSetup::schemaVersionString() + "/dbdata/");
            }
            return url;
        }

        CGlobalSetup CGlobalSetup::fromJsonFile(const QString &fileNameAndPath, bool acceptCacheFormat)
        {
            CGlobalSetup setup;
            loadFromJsonFile(setup, fileNameAndPath, acceptCacheFormat);
            return setup;
        }

        const CUrlList &CGlobalSetup::getSwiftLatestNewsUrls() const
        {
            return m_newsUrls;
        }

        const CUrlList &CGlobalSetup::getSwiftMapUrls() const
        {
            return m_mapUrls;
        }

        CServerList CGlobalSetup::getFsdTestServersPlusHardcodedServers() const
        {
            static const CServerList hardcoded({ CServer::swiftFsdTestServer(), CServer::fscServer() });
            CServerList testServers(m_fsdTestServers);
            testServers.addIfAddressNotExists(hardcoded);
            return testServers;
        }

        bool CGlobalSetup::isSwiftVersionMinimumMappingVersion() const
        {
            if (!this->wasLoaded()) { return false; }
            if (m_mappingMinimumVersion.isEmpty()) { return false; }
            const QVersionNumber min = QVersionNumber::fromString(this->getMappingMinimumVersionString());
            return CBuildConfig::getVersion() >= min;
        }

        QString CGlobalSetup::convertToQString(bool i18n) const
        {
            return convertToQString(", ", i18n);
        }

        QString CGlobalSetup::convertToQString(const QString &separator, bool i18n) const
        {
            QString s =
                QStringLiteral("timestamp: ")
                % this->getFormattedUtcTimestampYmdhms()
                % separator
                % QStringLiteral("Global setup loaded: ")
                % boolToYesNo(this->wasLoaded())
                % separator

                % QStringLiteral("For development: ")
                % boolToYesNo(isDevelopment())
                % separator

                % QStringLiteral("Mapping min.version: ")
                % this->getMappingMinimumVersionString()
                % separator

                % QStringLiteral("Distribution URLs: ")
                % getSwiftUpdateInfoFileUrls().toQString(i18n)
                % separator
                % QStringLiteral("Bootstrap URLs: ")
                % getSwiftBootstrapFileUrls().toQString(i18n)
                % separator
                % QStringLiteral("News URLs: ")
                % getSwiftLatestNewsUrls().toQString(i18n)
                % separator
                % QStringLiteral("Help URLs: ")
                % m_onlineHelpUrls.toQString(i18n)
                % separator
                % QStringLiteral("swift map URLs: ")
                % getSwiftMapUrls().toQString(i18n)
                % separator;
            s +=
                QStringLiteral("DB root directory: ")
                % getDbRootDirectoryUrl().toQString(i18n)
                % separator
                % QStringLiteral("ICAO DB reader: ")
                % getDbIcaoReaderUrl().toQString(i18n)
                % separator
                % QStringLiteral("Model DB reader: ")
                % getDbModelReaderUrl().toQString(i18n)
                % separator
                % QStringLiteral("Airport DB reader: ")
                % getDbAirportReaderUrl().toQString(i18n)
                % separator
                % QStringLiteral("DB home page: ")
                % getDbHomePageUrl().toQString(i18n)
                % separator
                % QStringLiteral("DB login service: ")
                % getDbLoginServiceUrl().toQString(i18n)
                % separator
                % QStringLiteral("DB client ping service: ")
                % getDbClientPingServiceUrl().toQString(i18n);
            s +=
                QStringLiteral("VATSIM bookings: ")
                % getVatsimBookingsUrl().toQString(i18n)
                % separator
                % QStringLiteral("VATSIM METARs: ")
                % getVatsimMetarsUrls().toQString(i18n)
                % separator
                % QStringLiteral("VATSIM data file: ")
                % getVatsimDataFileUrls().toQString(i18n)
                % separator

                % QStringLiteral("FSD test servers: ")
                % getFsdTestServers().toQString(i18n)
                % separator

                % QStringLiteral("Crash report server: ")
                % getCrashReportServerUrl().toQString(i18n);

            return s;
        }

        CVariant CGlobalSetup::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            if (ITimestampBased::canHandleIndex(index)) { return ITimestampBased::propertyByIndex(index); }

            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexDbRootDirectory: return CVariant::fromValue(m_dbRootDirectoryUrl);
            case IndexDbHttpPort: return CVariant::fromValue(m_dbHttpPort);
            case IndexDbHttpsPort: return CVariant::fromValue(m_dbHttpsPort);
            case IndexDbLoginService: return CVariant::fromValue(this->getDbLoginServiceUrl());
            case IndexDbClientPingService: return CVariant::fromValue(this->getDbClientPingServiceUrl());
            case IndexVatsimStatus: return CVariant::fromValue(m_vatsimStatusFileUrls);
            case IndexVatsimData: return CVariant::fromValue(m_vatsimDataFileUrls);
            case IndexVatsimBookings: return CVariant::fromValue(m_vatsimDataFileUrls);
            case IndexVatsimMetars: return CVariant::fromValue(m_vatsimMetarsUrls);
            case IndexBootstrapFileUrls: return CVariant::fromValue(this->getSwiftBootstrapFileUrls());
            case IndexUpdateInfoFileUrls: return CVariant::fromValue(this->getSwiftUpdateInfoFileUrls());
            case IndexSharedUrls: return CVariant::fromValue(m_sharedUrls);
            case IndexNewsUrls: return CVariant::fromValue(m_newsUrls);
            case IndexSwiftMapUrls: return CVariant::fromValue(m_mapUrls);
            case IndexOnlineHelpUrls: return CVariant::fromValue(m_onlineHelpUrls);
            case IndexCrashReportServerUrl: return CVariant::fromValue(m_crashReportServerUrl);
            case IndexWasLoaded: return CVariant::fromValue(m_wasLoaded);
            case IndexMappingMinimumVersion: return CVariant::fromValue(m_mappingMinimumVersion);
            default: return CValueObject::propertyByIndex(index);
            }
        }

        void CGlobalSetup::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CGlobalSetup>(); return; }
            if (ITimestampBased::canHandleIndex(index))
            {
                ITimestampBased::setPropertyByIndex(index, variant);
                return;
            }

            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexDbRootDirectory: m_dbRootDirectoryUrl.setPropertyByIndex(index.copyFrontRemoved(), variant); break;
            case IndexDbHttpPort: m_dbHttpPort = variant.toInt(); break;
            case IndexDbHttpsPort: m_dbHttpsPort = variant.toInt(); break;
            case IndexDbLoginService: break; // cannot be changed
            case IndexDbClientPingService: break; // cannot be changed
            case IndexVatsimData: m_vatsimDataFileUrls = variant.value<CUrlList>(); break;
            case IndexVatsimBookings: m_vatsimBookingsUrl.setPropertyByIndex(index.copyFrontRemoved(), variant); break;
            case IndexVatsimMetars: m_vatsimMetarsUrls = variant.value<CUrlList>(); break;
            case IndexSharedUrls: m_sharedUrls = variant.value<CUrlList>(); break;
            case IndexNewsUrls: m_newsUrls = variant.value<CUrlList>(); break;
            case IndexOnlineHelpUrls: m_onlineHelpUrls = variant.value<CUrlList>(); break;
            case IndexSwiftMapUrls: m_mapUrls = variant.value<CUrlList>(); break;
            case IndexCrashReportServerUrl: m_crashReportServerUrl = variant.value<CUrl>(); break;
            case IndexWasLoaded: m_wasLoaded = variant.toBool(); break;
            case IndexMappingMinimumVersion: m_mappingMinimumVersion = variant.toQString(); break;
            default: CValueObject::setPropertyByIndex(index, variant); break;
            }
        }

        const QString &CGlobalSetup::schemaVersionString()
        {
            // This is not the current swift version, but the schema version
            static const QString v("0.7.0");
            return v;
        }
    } // ns
} // ns
