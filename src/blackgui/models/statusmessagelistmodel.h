/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_STATUSMESSAGELISTMODEL_H
#define BLACKGUI_STATUSMESSAGELISTMODEL_H

#include <QAbstractItemModel>
#include <QDBusConnection>
#include "blackmisc/statusmessagelist.h"
#include "blackgui/models/listmodelbase.h"

namespace BlackGui
{
    namespace Models
    {
        /*!
         * Server list model
         */
        class CStatusMessageListModel : public CListModelBase<BlackMisc::CStatusMessage, BlackMisc::CStatusMessageList>
        {

        public:

            //! Constructor
            explicit CStatusMessageListModel(QObject *parent = nullptr);

            //! Destructor
            virtual ~CStatusMessageListModel() {}

            //! \copydoc CListModelBase::data
            QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const override;
        };
    }
}
#endif // guard
