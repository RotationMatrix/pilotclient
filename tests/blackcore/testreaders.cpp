/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "testreaders.h"
#include "expect.h"
#include "blackcore/data/globalsetup.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/aviation/aircrafticaocode.h"
#include "blackmisc/aviation/airlineicaocode.h"

using namespace BlackCore;
using namespace BlackCore::Settings;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Network;

namespace BlackCoreTest
{
    CTestReaders::CTestReaders(QObject *parent) :
        QObject(parent),
        m_icaoReader(this),
        m_modelReader(this)
    { }

    void CTestReaders::readIcaoData()
    {
        CUrl url(m_setup.get().dbIcaoReader());
        if (!this->pingServer(url)) { return; }
        m_icaoReader.start();
        Expect e(&this->m_icaoReader);
        EXPECT_UNIT(e)
        .send(&CIcaoDataReader::readInBackgroundThread, CEntityFlags::AllIcaoEntities)
        .expect(&CIcaoDataReader::dataRead, [url]()
        {
            qDebug() << "Read ICAO data from" << url.getFullUrl();
        })
        .wait(10);

        QVERIFY2(this->m_icaoReader.getAircraftIcaoCodesCount() > 0, "No aircraft ICAOs");
        QVERIFY2(this->m_icaoReader.getAirlineIcaoCodesCount() > 0, "No airline ICAOs");

        CAircraftIcaoCode aircraftIcao(this->m_icaoReader.getAircraftIcaoCodes().front());
        CAirlineIcaoCode airlineIcao(this->m_icaoReader.getAirlineIcaoCodes().front());
        QVERIFY2(aircraftIcao.hasCompleteData(), "Missing data for aircraft ICAO");
        QVERIFY2(airlineIcao.hasCompleteData(), "Missing data for airline ICAO");
    }

    void CTestReaders::readModelData()
    {
        CUrl url(m_setup.get().dbModelReader());
        if (!this->pingServer(url)) { return; }
        m_modelReader.start();
        Expect e(&this->m_modelReader);
        EXPECT_UNIT(e)
        .send(&CModelDataReader::readInBackgroundThread, CEntityFlags::AllIcaoEntities)
        .expect(&CModelDataReader::dataRead, [url]()
        {
            qDebug() << "Read model data " << url;
        })
        .wait(10);

        QVERIFY2(this->m_modelReader.getDistributorsCount() > 0, "No distributors");
        QVERIFY2(this->m_modelReader.getLiveriesCount() > 0, "No liveries");

        CLivery livery(this->m_modelReader.getLiveries().front());
        CDistributor distributor(this->m_modelReader.getDistributors().front());
        QVERIFY2(livery.hasCompleteData(), "Missing data for livery");
        QVERIFY2(distributor.hasCompleteData(), "Missing data for distributor");
    }

    bool CTestReaders::pingServer(const CUrl &url)
    {
        QString m;
        if (!CNetworkUtils::canConnect(url, m, 2500))
        {
            qWarning() << "Skipping unit test as" << url.getFullUrl() << "cannot be connected";
            return false;
        }
        return true;
    }
} // ns
