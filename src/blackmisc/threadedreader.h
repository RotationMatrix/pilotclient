/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKMISC_THREADED_READER_H
#define BLACKMISC_THREADED_READER_H

//! \file

#include "blackmiscexport.h"
#include "blackmisc/network/urllist.h"
#include "blackmisc/network/entityflags.h"
#include "worker.h"
#include <QReadWriteLock>
#include <QDateTime>
#include <QTimer>
#include <QNetworkReply>
#include <QCoreApplication>
#include <atomic>

namespace BlackMisc
{
    //! Support for threaded based reading and parsing tasks such
    //! as data files via http, or file system and parsing (such as FSX models)
    class BLACKMISC_EXPORT CThreadedReader : public CContinuousWorker
    {
    public:
        //! Destructor
        virtual ~CThreadedReader();

        //! Thread safe, set update timestamp
        //! \threadsafe
        QDateTime getUpdateTimestamp() const;

        //! Thread safe, set update timestamp
        //! \threadsafe
        void setUpdateTimestamp(const QDateTime &updateTimestamp = QDateTime::currentDateTimeUtc());

        //! Was setup read within last xx milliseconds
        //! \threadsafe
        bool updatedWithinLastMs(qint64 timeLastMs);

        //! Request new reading
        //! \note override as required, default is to call initialize()
        virtual void requestReload();

        //! Set the update time
        //! \param updatePeriodMs <=0 stops the timer
        //! \threadsafe
        void setInterval(int updatePeriodMs);

        //! Get the timer interval (ms)
        //! \threadsafe
        int interval() const;

    public slots:
        //! Graceful shutdown
        //! \threadsafe
        void gracefulShutdown();

    protected:
        QTimer *m_updateTimer        = nullptr;  //!< update timer
        mutable QReadWriteLock m_lock {QReadWriteLock::Recursive}; //!< lock which can be used from the derived classes

        //! Constructor
        CThreadedReader(QObject *owner, const QString &name);

        //! When was reply last modified, -1 if N/A
        qint64 lastModifiedMsSinceEpoch(QNetworkReply *nwReply) const;

        //! Make sure everthing runs correctly in own thread
        void threadAssertCheck() const;

    private:
        QDateTime m_updateTimestamp; //!< when file/resource was read
    };
} // namespace

#endif // guard
