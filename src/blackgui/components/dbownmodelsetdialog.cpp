/* Copyright (C) 2018
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbownmodelsetdialog.h"
#include "ui_dbownmodelsetdialog.h"

using namespace BlackMisc::Simulation;

namespace BlackGui
{
    namespace Components
    {
        CDbOwnModelSetDialog::CDbOwnModelSetDialog(QWidget *parent) :
            QDialog(parent),
            ui(new Ui::CDbOwnModelSetDialog)
        {
            ui->setupUi(this);
            this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        }

        CDbOwnModelSetDialog::~CDbOwnModelSetDialog()
        { }

        void CDbOwnModelSetDialog::setSimulator(const CSimulatorInfo &simulator)
        {
            ui->comp_OwnModelSet->setSimulator(simulator);
        }

        const CDbOwnModelSetComponent *CDbOwnModelSetDialog::modelSetComponent() const
        {
            return ui->comp_OwnModelSet;
        }
    } // ns
} // ns
