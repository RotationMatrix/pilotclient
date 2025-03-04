/* Copyright (C) 2019
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "cockpitcomaudiodialog.h"
#include "ui_cockpitcomaudiodialog.h"

namespace BlackGui::Components
{
    CCockpitComAudioDialog::CCockpitComAudioDialog(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::CCockpitComAudioDialog)
    {
        ui->setupUi(this);
        this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    }

    CCockpitComAudioDialog::~CCockpitComAudioDialog()
    { }

} // ns
