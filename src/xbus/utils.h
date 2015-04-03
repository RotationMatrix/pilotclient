/*  Copyright (C) 2013 VATSIM Community / contributors
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLACKSIM_XBUS_UTILS_H
#define BLACKSIM_XBUS_UTILS_H

#include <XPLM/XPLMDefs.h>
#include <XPLM/XPLMPlugin.h>
#include <XPLM/XPLMProcessing.h>
#include <XPLM/XPLMUtilities.h>
#include <QApplication>
#include <QSharedPointer>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <clocale>

/*!
 * \file
 */

/*!
 * Install a Qt message handler which outputs to the X-Plane debug log.
 */
class QXPlaneMessageHandler
{
    QXPlaneMessageHandler();
    QXPlaneMessageHandler(const QXPlaneMessageHandler &);

    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        QByteArray localMsg = msg.toLocal8Bit();
        QByteArray file(context.file);
        if (file.isEmpty()) file = "<unknown>";

        int line = context.line;

        char *buffer = new char[64 + localMsg.size() + file.size()];
        switch (type) {
        case QtDebugMsg:
            std::sprintf(buffer, "%s:%d: Debug: %s\n", file.constData(), line, localMsg.constData());
            XPLMDebugString(buffer);
            break;
        case QtWarningMsg:
            std::sprintf(buffer, "%s:%d: Warning: %s\n", file.constData(), line, localMsg.constData());
            XPLMDebugString(buffer);
            break;
        default:
        case QtCriticalMsg:
            std::sprintf(buffer, "%s:%d: Error: %s\n", file.constData(), line, localMsg.constData());
            XPLMDebugString(buffer);
            break;
        case QtFatalMsg:
            std::sprintf(buffer, "%s:%d: Fatal: %s\n", file.constData(), line, localMsg.constData());
            XPLMDebugString(buffer);
            std::abort();
        }
        delete[] buffer;
    }

public:
    /*!
     * Install the handler.
     */
    static void install()
    {
        qInstallMessageHandler(handler);
    }
};

/*!
 * QApplication subclass used by XBus.
 *
 * Only one instance of QApplication is allowed per process, so if any other
 * X-Plane plugin, completely unrelated to this pilot client, wants to use the
 * Qt framework, they can simply copy & paste this class into their project
 * and both X-Plane plugins will be able to share a single QApplication safely.
 */
class QSharedApplication : public QApplication
{
    Q_OBJECT

    QWeakPointer<QApplication> m_weakptr;

    QSharedApplication(QSharedPointer<QApplication> &ptr, int &argc, char **argv) : QApplication(argc, argv)
    {
        ptr.reset(this);
        m_weakptr = ptr;
    }

    static char *strdup(const char *s) { auto s2 = static_cast<char *>(std::malloc(std::strlen(s))); return std::strcpy(s2, s); }

public:
    /*!
     * Returns a shared pointer to the QApplication.
     *
     * The QApplication will not be destroyed while this shared pointer exists.
     */
    static QSharedPointer<QApplication> sharedInstance()
    {
        QSharedPointer<QApplication> ptr;
        if (! instance())
        {
            static int argc = 1;
            static char *argv[] = { strdup("X-Plane") };
      
#ifdef Q_OS_UNIX
            /* Workaround for #362 */
            char* xplocale = setlocale(LC_ALL, nullptr);
#endif
            
            new QSharedApplication(ptr, argc, argv);
            
#ifdef Q_OS_UNIX
            setlocale(LC_ALL, xplocale);
#endif
        }
        if (! instance()->inherits("QSharedApplication"))
        {
            qFatal("There is an unshared QApplication in another plugin");
        }
        return static_cast<QSharedApplication*>(instance())->m_weakptr;
    }
};

/*!
 * Runs the Qt event loop inside the X-Plane event loop.
 */
class QXPlaneEventLoop : public QObject
{
    Q_OBJECT

    QXPlaneEventLoop(QObject *parent) : QObject(parent)
    {
        XPLMRegisterFlightLoopCallback(callback, -1, nullptr);
    }

    ~QXPlaneEventLoop()
    {
        XPLMUnregisterFlightLoopCallback(callback, nullptr);
    }

    static float callback(float, float, int, void *)
    {
        QApplication::processEvents();
        QApplication::sendPostedEvents();
        return -1;
    }

public:
    /*!
     * Registers the X-Plane callback which calls into the Qt event loop,
     * unless one was already registered.
     */
    static void exec()
    {
        if (! QApplication::instance()->findChild<QXPlaneEventLoop *>())
        {
            new QXPlaneEventLoop(QApplication::instance());
        }
    }
};

#endif // guard
