/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "threadedreader.h"

namespace BlackMisc
{

    CThreadedReader::CThreadedReader(QObject *owner, const QString &name) :
        CContinuousWorker(owner, name),
        m_updateTimer(new QTimer(this))
    {  }


    QDateTime CThreadedReader::getUpdateTimestamp() const
    {
        QReadLocker(&this->m_lock);
        return this->m_updateTimestamp;
    }

    void CThreadedReader::setUpdateTimestamp(const QDateTime &updateTimestamp)
    {
        QWriteLocker(&this->m_lock);
        this->m_updateTimestamp = updateTimestamp;
    }

    void CThreadedReader::requestStop()
    {
        QWriteLocker(&this->m_lock);
        this->m_stopped = true;
        this->m_updateTimer->stop();
    }

    CThreadedReader::~CThreadedReader()
    {
        cleanup();
    }

    void CThreadedReader::cleanup()
    {
        // cleanup code would go here
    }

    bool CThreadedReader::isFinished() const
    {
        if (CContinuousWorker::isFinished()) { return true; }
        QReadLocker(&this->m_lock);
        return this->m_stopped;
    }

    void CThreadedReader::setInterval(int updatePeriodMs)
    {
        Q_ASSERT(this->m_updateTimer);
        QWriteLocker(&this->m_lock);
        if (updatePeriodMs < 1)
        {
            this->m_updateTimer->stop();
        }
        else
        {
            this->m_updateTimer->start(updatePeriodMs);
        }
    }

    int CThreadedReader::interval() const
    {
        QReadLocker rl(&this->m_lock);
        return this->m_updateTimer->interval();
    }

    void CThreadedReader::threadAssertCheck() const
    {
        Q_ASSERT_X(QCoreApplication::instance()->thread() != QThread::currentThread(), "CThreadedReader::threadAssertCheck", "Needs to run in own thread");
        Q_ASSERT_X(QObject::thread() == QThread::currentThread(), "CThreadedReader::threadAssertCheck", "Needs to run in own thread");
    }

} // namespace
