/* Copyright (C) 2018
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "interpolationlogdisplay.h"
#include "ui_interpolationlogdisplay.h"
#include "blackgui/guiapplication.h"
#include "blackcore/context/contextnetworkimpl.h"
#include "blackcore/airspacemonitor.h"
#include "blackmisc/simulation/interpolationlogger.h"
#include "blackmisc/stringutils.h"

using namespace BlackCore;
using namespace BlackCore::Context;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::PhysicalQuantities;

namespace BlackGui
{
    namespace Components
    {
        CInterpolationLogDisplay::CInterpolationLogDisplay(QWidget *parent) :
            QFrame(parent),
            ui(new Ui::CInterpolationLogDisplay)
        {
            ui->setupUi(this);
            ui->tw_LogTabs->setCurrentIndex(TabFlow);
            constexpr int timeSecs = 5;
            ui->hs_UpdateTime->setValue(timeSecs);
            this->onSliderChanged(timeSecs);
            connect(&m_updateTimer, &QTimer::timeout, this, &CInterpolationLogDisplay::updateLog);
            connect(ui->hs_UpdateTime, &QSlider::valueChanged, this, &CInterpolationLogDisplay::onSliderChanged);
            connect(ui->pb_StartStop, &QPushButton::released, this, &CInterpolationLogDisplay::toggleStartStop);
            connect(ui->comp_CallsignCompleter, &CCallsignCompleter::validCallsignEntered, this, &CInterpolationLogDisplay::onCallsignEntered);

            CLedWidget::LedShape shape = CLedWidget::Rounded;
            ui->led_Parts->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "Parts received", "", 14);
            ui->led_Situation->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "Situation received", "", 14);
            ui->led_Elevation->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "Elevation received", "", 14);
            ui->led_Running->setValues(CLedWidget::Yellow, CLedWidget::Black, shape, "Running", "Stopped", 14);

            m_callsign = ui->comp_CallsignCompleter->getCallsign();
        }

        CInterpolationLogDisplay::~CInterpolationLogDisplay()
        {
            // void
        }

        void CInterpolationLogDisplay::setSimulator(CSimulatorCommon *simulatorCommon)
        {
            if (simulatorCommon && simulatorCommon == m_simulatorCommon) { return; } // same
            if (m_simulatorCommon)
            {
                this->disconnect(m_simulatorCommon);
                m_simulatorCommon->disconnect(this);
            }
            m_simulatorCommon = simulatorCommon;
            connect(m_simulatorCommon, &CSimulatorCommon::receivedRequestedElevation, this, &CInterpolationLogDisplay::onElevationReceived);
            connect(m_simulatorCommon, &CSimulatorCommon::requestedElevation, this, &CInterpolationLogDisplay::onElevationRequested);
        }

        void CInterpolationLogDisplay::setAirspaceMonitor(CAirspaceMonitor *airspaceMonitor)
        {
            if (airspaceMonitor && airspaceMonitor == m_airspaceMonitor) { return; } // same
            if (m_airspaceMonitor)
            {
                this->disconnect(m_airspaceMonitor);
                m_airspaceMonitor->disconnect(this);
            }
            m_airspaceMonitor = airspaceMonitor;

            connect(m_airspaceMonitor, &CAirspaceMonitor::addedAircraftSituation, this, &CInterpolationLogDisplay::onSituationAdded, Qt::QueuedConnection);
            connect(m_airspaceMonitor, &CAirspaceMonitor::addedAircraftParts, this, &CInterpolationLogDisplay::onPartsAdded, Qt::QueuedConnection);
        }

        void CInterpolationLogDisplay::updateLog()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const bool hasLogger = m_simulatorCommon && m_airspaceMonitor;
            if (!hasLogger || m_callsign.isEmpty())
            {
                ui->te_TextLog->setText("No logger attached or no callsign");
                this->stop();
                this->clear();
            }

            // only display visible tab
            if (ui->tw_LogTabs->currentWidget() == ui->tb_TextLog)
            {
                const QString log = m_simulatorCommon->latestLoggedDataFormatted(m_callsign);
                ui->te_TextLog->setText(log);
            }
            else if (ui->tw_LogTabs->currentWidget() == ui->tb_DataFlow)
            {
                ui->le_CG->setText(m_airspaceMonitor->getCG(m_callsign).valueRoundedWithUnit(CLengthUnit::ft(), 1));
                ui->le_CG->home(false);
                ui->le_Parts->setText(boolToYesNo(m_airspaceMonitor->isRemoteAircraftSupportingParts(m_callsign)));

                static const QString avgUpdateTime("%1ms");
                const QString avgUpdateTimeRounded = QString::number(m_simulatorCommon->getStatisticsAverageUpdateTimeMs(), 'f', 2);
                ui->le_AvgUpdateTimeMs->setText(avgUpdateTime.arg(avgUpdateTimeRounded));

                const CClient client = m_airspaceMonitor->getClientOrDefaultForCallsign(m_callsign);
                ui->le_GndFlag->setText(boolToYesNo(client.hasGndFlagCapability()));
            }
        }

        void CInterpolationLogDisplay::onSliderChanged(int timeSecs)
        {
            static const QString time("%1 secs");
            m_updateTimer.setInterval(timeSecs * 1000);
            ui->le_UpdateTime->setText(time.arg(timeSecs));
        }

        void CInterpolationLogDisplay::onCallsignEntered()
        {
            const CCallsign cs = ui->comp_CallsignCompleter->getCallsign();
            if (!m_simulatorCommon)
            {
                this->stop();
                return;
            }
            if (m_callsign == cs) { return; }

            // clear last callsign
            if (!m_callsign.isEmpty())
            {
                m_simulatorCommon->setLogInterpolation(false, m_callsign); // stop logging "old" callsign
                m_callsign = CCallsign(); // clear callsign
                this->clear();
            }

            // set new callsign or stop
            if (cs.isEmpty())
            {
                this->stop();
                return;
            }

            m_callsign = cs;
            m_simulatorCommon->setLogInterpolation(true, cs);
        }

        void CInterpolationLogDisplay::toggleStartStop()
        {
            const bool running = m_updateTimer.isActive();
            if (running) { this->stop(); }
            else { this->start(); }
        }

        void CInterpolationLogDisplay::start()
        {
            const int interval = ui->hs_UpdateTime->value() * 1000;
            m_updateTimer.start(interval);
            ui->pb_StartStop->setText(stopText());
            ui->led_Running->setOn(true);
        }

        void CInterpolationLogDisplay::stop()
        {
            m_updateTimer.stop();
            ui->pb_StartStop->setText(startText());
            ui->led_Running->setOn(false);
        }

        bool CInterpolationLogDisplay::logCallsign(const CCallsign &cs) const
        {
            if (!sGui || sGui->isShuttingDown()) { return false; }
            if (!m_airspaceMonitor || !m_simulatorCommon || m_callsign.isEmpty()) { return false; }
            if (cs != m_callsign) { return false; }
            return true;
        }

        void CInterpolationLogDisplay::onSituationAdded(const CAircraftSituation &situation)
        {
            const CCallsign cs = situation.getCallsign();
            if (!this->logCallsign(cs)) { return; }
            const CAircraftSituationList situations = m_airspaceMonitor->remoteAircraftSituations(cs);
            ui->tvp_AircraftSituations->updateContainerAsync(situations);
            ui->led_Situation->blink();
        }

        void CInterpolationLogDisplay::onPartsAdded(const CCallsign &callsign, const CAircraftParts &parts)
        {
            if (!this->logCallsign(callsign)) { return; }
            Q_UNUSED(parts);
            const CAircraftPartsList partsList = m_airspaceMonitor->remoteAircraftParts(callsign);
            ui->tvp_AircraftParts->updateContainerAsync(partsList);
            ui->led_Parts->blink();
        }

        void CInterpolationLogDisplay::onElevationReceived(const CElevationPlane &plane, const CCallsign &callsign)
        {
            if (!this->logCallsign(callsign)) { return; }
            m_elvReceived++;
            ui->le_Elevation->setText(plane.toQString());
            ui->led_Elevation->blink();
            ui->le_ElevationRec->setText(QString::number(m_elvReceived));
        }

        void CInterpolationLogDisplay::onElevationRequested(const CCallsign &callsign)
        {
            if (!this->logCallsign(callsign)) { return; }
            m_elvRequested++;
            ui->led_Elevation->blink();
            ui->le_ElevationReq->setText(QString::number(m_elvRequested));
        }

        void CInterpolationLogDisplay::clear()
        {
            ui->tvp_AircraftParts->clear();
            ui->tvp_AircraftSituations->clear();
            ui->te_TextLog->clear();
            ui->le_CG->clear();
            ui->le_Elevation->clear();
            ui->le_ElevationRec->clear();
            ui->le_ElevationReq->clear();
            ui->le_Parts->clear();
            ui->le_AvgUpdateTimeMs->clear();
            m_elvReceived = m_elvRequested = 0;
        }

        void CInterpolationLogDisplay::linkWithAirspaceMonitor()
        {
            if (!sGui || sGui->isShuttingDown() || !sGui->supportsContexts()) { return; }
            if (!sGui->getCoreFacade() || !sGui->getCoreFacade()->getCContextNetwork()) { return; }
            const CContextNetwork *cn = sGui->getCoreFacade()->getCContextNetwork();
            this->setAirspaceMonitor(cn->airspace());
        }

        const QString &CInterpolationLogDisplay::startText()
        {
            static const QString start("start");
            return start;
        }

        const QString &CInterpolationLogDisplay::stopText()
        {
            static const QString stop("stop");
            return stop;
        }
    } // ns
} // ns
