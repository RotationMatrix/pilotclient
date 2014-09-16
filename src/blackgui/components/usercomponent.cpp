/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "usercomponent.h"
#include "ui_usercomponent.h"
#include "blackmisc/nwuserlist.h"

namespace BlackGui
{
    namespace Components
    {
        CUserComponent::CUserComponent(QWidget *parent) :
            QTabWidget(parent),
            CDockWidgetInfoAreaComponent(this),
            CRuntimeBasedComponent(nullptr, false),
            ui(new Ui::CUserComponent), m_timerComponent(nullptr)
        {
            ui->setupUi(this);
            this->m_timerComponent = new CTimerBasedComponent(SLOT(update()), this);
        }

        CUserComponent::~CUserComponent()
        {
            delete ui;
        }

        int CUserComponent::countClients() const
        {
            Q_ASSERT(this->ui->tvp_Clients);
            return this->ui->tvp_Clients->rowCount();
        }

        int CUserComponent::countUsers() const
        {
            Q_ASSERT(this->ui->tvp_AllUsers);
            return this->ui->tvp_AllUsers->rowCount();
        }

        void CUserComponent::update()
        {
            Q_ASSERT(this->ui->tvp_AllUsers);
            Q_ASSERT(this->ui->tvp_Clients);
            Q_ASSERT(this->getIContextNetwork());

            if (this->getIContextNetwork()->isConnected())
            {
                bool withData = countUsers() > 0 || countClients() > 0;
                if (withData && !isVisibleWidget())
                {
                    // KWB remove: qDebug() will be removed soo
                    qDebug() << this->objectName() << "Skipping update";
                    return;
                }

                // load data
                this->ui->tvp_Clients->updateContainer(this->getIContextNetwork()->getOtherClients());
                this->ui->tvp_AllUsers->updateContainer(this->getIContextNetwork()->getUsers());
            }
        }
    }
} // guard
