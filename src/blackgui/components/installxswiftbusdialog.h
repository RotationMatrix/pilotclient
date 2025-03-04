/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_INSTALLXSWIFTBUSDIALOG_H
#define BLACKGUI_COMPONENTS_INSTALLXSWIFTBUSDIALOG_H

#include <QString>
#include <QDialog>

namespace Ui { class CInstallXSwiftBusDialog; }
namespace BlackGui::Components
{
    /*!
     * CInstallXSwiftBusComponent as dialog
     */
    class CInstallXSwiftBusDialog : public QDialog
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CInstallXSwiftBusDialog(QWidget *parent = nullptr);

        //! Dtor
        virtual ~CInstallXSwiftBusDialog() override;

        //! \copydoc CInstallXSwiftBusComponent::setDefaultDownloadName
        void setDefaultDownloadName(const QString &defaultName);

    private:
        QScopedPointer<Ui::CInstallXSwiftBusDialog> ui;
    };
} // ns

#endif // guard
