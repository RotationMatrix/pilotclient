/* Copyright (C) 2019
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_AUTOPUBLISHCOMPONENT_H
#define BLACKGUI_COMPONENTS_AUTOPUBLISHCOMPONENT_H

#include "blackgui/overlaymessagesframe.h"
#include "blackmisc/simulation/autopublishdata.h"

#include <QFrame>
#include <QScopedPointer>

namespace Ui { class CAutoPublishComponent; }
namespace BlackGui
{
    namespace Components
    {
        //! Data automatically collected and be be sent to backend
        class CAutoPublishComponent : public COverlayMessagesFrame
        {
            Q_OBJECT

        public:
            //! Ctor
            explicit CAutoPublishComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CAutoPublishComponent();

            //! Read the files
            int readFiles();

        private:
            QScopedPointer<Ui::CAutoPublishComponent> ui;
            BlackMisc::Simulation::CAutoPublishData m_data;

            //! Analyze against DB data
            void analyzeAgainstDBData();

            //! Send to DB
            void sendToDb();

            //! Display data in JSON text field
            void displayData();

            //! Delete all files
            void deleteAllFiles();
        };
    } // ns
} // ns
#endif // guard
