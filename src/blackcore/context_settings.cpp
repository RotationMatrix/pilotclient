/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "context_settings.h"
#include "coreruntime.h"
#include "blackmisc/settingutilities.h"

using namespace BlackMisc::Settings;
using namespace BlackMisc::Network;
using namespace BlackMisc;

namespace BlackCore
{

    /*
     * Init this context
     */
    CContextSettings::CContextSettings(CCoreRuntime *parent) : IContextSettings(parent), m_settingsNetwork()
    {
        // create some dummy settings
        // this would actually be reading the settings from disk ..

        this->m_settingsNetwork.setCurrentNetworkServer(
            CServer("Testserver", "Client project testserver", "vatsim-germany.org", 6809,
                    CUser("guest", "Guest Client project", "", "guest")));
        this->m_settingsNetwork.addTrafficNetworkServer(this->m_settingsNetwork.getCurrentNetworkServer());
        this->m_settingsNetwork.addTrafficNetworkServer(
            CServer("Europe C2", "Real VATSIM Server", "88.198.19.202", 6809,
                    CUser("vatsimid", "Black Client", "", "vatsimpw"))
        );
    }

    /*
     * Network settings
     */
    BlackMisc::Settings::CSettingsNetwork CContextSettings::getNetworkSettings() const
    {
        return this->m_settingsNetwork;
    }

    /*
     * Pass value
     */
    BlackMisc::CStatusMessages CContextSettings::value(const QString &path, const QString &command, const QVariant &value)
    {
        Q_ASSERT(path.length() > 3);
        Q_ASSERT(path.indexOf('/') >= 0);
        QString nextLevelPath = CSettingUtilities::removeLeadingPath(path);
        BlackMisc::CStatusMessages msgs = CSettingUtilities::wrongPathMessages(path);
        bool changed = false;
        if (path.startsWith(IContextSettings::PathNetworkSettings()))
        {
            msgs = this->m_settingsNetwork.value(nextLevelPath, command, value, changed);
            if (changed) emit this->changedNetworkSettings();
        }
        return msgs;
    }

} // namespace
