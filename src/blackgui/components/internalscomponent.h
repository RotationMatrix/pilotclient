/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_INTERNALSCOMPONENT_H
#define BLACKGUI_INTERNALSCOMPONENT_H

#include "blackgui/blackguiexport.h"
#include "blackmisc/aviation/aircraftparts.h"

#include <QObject>
#include <QScopedPointer>
#include <QWidget>

class QShowEvent;

namespace Ui { class CInternalsComponent; }
namespace BlackGui
{
    namespace Components
    {
        //! Internals for debugging, statistics
        class BLACKGUI_EXPORT CInternalsComponent :
            public QWidget
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CInternalsComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CInternalsComponent();

        protected:
            //! \copydoc QWidget::showEvent
            virtual void showEvent(QShowEvent *event) override;

        private:
            //! Send aircraft parts
            void sendAircraftParts();

            //! Current parts in UI
            void setCurrentParts();

            //! Enable / disable debugging
            void enableDebug(int state);

            //! Send the text message
            void sendTextMessage();

            //! Send a dummy status message
            void logStatusMessage();

            //! Show log files
            void showLogFiles();

            //! Request parts (aka aircraft config) from network
            void requestPartsFromNetwork();

            //! Selector has been changed
            void selectorChanged();

            //! Log in simulator
            void displayLogInSimulator();

            //! Set the context flags
            void contextFlagsToGui();

            QScopedPointer<Ui::CInternalsComponent> ui;
        };
    } // namespace
} // namespace

#endif // guard
