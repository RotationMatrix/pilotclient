/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXTSIMULATOR_EMPTY_H
#define BLACKCORE_CONTEXTSIMULATOR_EMPTY_H

#include "blackcoreexport.h"
#include "contextsimulator.h"
#include "blackmisc/logmessage.h"

namespace BlackCore
{
    //! Empty context, used during shutdown/initialization
    class BLACKCORE_EXPORT CContextSimulatorEmpty : public IContextSimulator
    {
        Q_OBJECT

    public:
        //! Constructor
        CContextSimulatorEmpty(CRuntime *runtime) : IContextSimulator(CRuntimeConfig::NotUsed, runtime) {}

    public slots:
        //! \copydoc IContextSimulator::getSimulatorPluginInfo()
        virtual BlackMisc::Simulation::CSimulatorPluginInfo getSimulatorPluginInfo() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::Simulation::CSimulatorPluginInfo();
        }

        //! \copydoc IContextSimulator::getAvailableSimulatorPlugins()
        virtual BlackMisc::Simulation::CSimulatorPluginInfoList getAvailableSimulatorPlugins() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::Simulation::CSimulatorPluginInfoList();
        }

        //! \copydoc IContextSimulator::startSimulatorPlugin()
        virtual bool startSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo) override
        {
            Q_UNUSED(simulatorInfo);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextSimulator::getSimulatorStatus()
        virtual int getSimulatorStatus() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return 0;
        }

        //! \copydoc IContextSimulator::stopSimulatorPlugin()
        virtual void stopSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo) override
        {
            Q_UNUSED(simulatorInfo);
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc IContextSimulator::getAirportsInRange()
        virtual BlackMisc::Aviation::CAirportList getAirportsInRange() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::Aviation::CAirportList();
        }

        //! \copydoc IContextSimulator::getInstalledModels()
        virtual BlackMisc::Simulation::CAircraftModelList getInstalledModels() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::Simulation::CAircraftModelList();
        }

        //! \copydoc IContextSimulator::getInstalledModelsStartingWith
        virtual BlackMisc::Simulation::CAircraftModelList getInstalledModelsStartingWith(const QString modelString) const override
        {
            Q_UNUSED(modelString);
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::Simulation::CAircraftModelList();
        }

        //! \copydoc IContextSimulator::getInstalledModelsCount
        virtual int getInstalledModelsCount() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return 0;
        }

        //! \copydoc IContextSimulator::reloadInstalledModels
        virtual void reloadInstalledModels() override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc IContextSimulator::getSimulatorSetup
        virtual BlackMisc::Simulation::CSimulatorSetup getSimulatorSetup() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::Simulation::CSimulatorSetup();
        }

        //! \copydoc IContextSimulator::setTimeSynchronization
        virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) override
        {
            Q_UNUSED(enable);
            Q_UNUSED(offset);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextSimulator::isTimeSynchronized
        virtual bool isTimeSynchronized() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextSimulator::getMaxRenderedAircraft
        virtual int getMaxRenderedAircraft() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return 0;
        }

        //! \copydoc IContextSimulator::setMaxRenderedAircraft
        virtual void setMaxRenderedAircraft(int number) override
        {
            Q_UNUSED(number);
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc IContextSimulator::setMaxRenderedDistance
        virtual void setMaxRenderedDistance(const BlackMisc::PhysicalQuantities::CLength &distance)
        {
            Q_UNUSED(distance);
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc IContextSimulator::setMaxRenderedDistance
        virtual void deleteAllRenderingRestrictions() override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc IContextSimulator::isRenderingRestricted
        virtual bool isRenderingRestricted() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextSimulator::isRenderingEnabled
        virtual bool isRenderingEnabled() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextSimulator::getMaxRenderedDistance
        virtual BlackMisc::PhysicalQuantities::CLength getMaxRenderedDistance() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::PhysicalQuantities::CLength();
        }

        //! \copydoc IContextSimulator::getRenderedDistanceBoundary
        virtual BlackMisc::PhysicalQuantities::CLength getRenderedDistanceBoundary() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::PhysicalQuantities::CLength();
        }

        //! \copydoc IContextSimulator::getRenderRestrictionText
        virtual QString getRenderRestrictionText() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return QString();
        }

        //! \copydoc IContextSimulator::getTimeSynchronizationOffset
        virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::PhysicalQuantities::CTime();
        }

        //! \copydoc IContextSimulator::iconForModel
        virtual BlackMisc::CPixmap iconForModel(const QString &modelString) const override
        {
            Q_UNUSED(modelString);
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::CPixmap();
        }

        //! \copydoc IContextSimulator::highlightAircraft
        virtual void highlightAircraft(const BlackMisc::Simulation::CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const BlackMisc::PhysicalQuantities::CTime &displayTime) override
        {
            Q_UNUSED(aircraftToHighlight);
            Q_UNUSED(enableHighlight);
            Q_UNUSED(displayTime);
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc ISimulator::enableDebugMessages
        virtual void enableDebugMessages(bool driver, bool interpolator) override
        {
            Q_UNUSED(driver);
            Q_UNUSED(interpolator);
            logEmptyContextWarning(Q_FUNC_INFO);
        }
    };
} // namespace

#endif // guard
