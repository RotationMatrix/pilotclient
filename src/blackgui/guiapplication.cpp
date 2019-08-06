/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "blackgui/components/applicationclosedialog.h"
#include "blackgui/components/updateinfodialog.h"
#include "blackgui/components/aboutdialog.h"
#include "blackgui/components/setuploadingdialog.h"
#include "blackgui/splashscreen.h"
#include "blackgui/guiapplication.h"
#include "blackgui/guiutility.h"
#include "blackgui/registermetadata.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/data/globalsetup.h"
#include "blackcore/db/networkwatchdog.h"
#include "blackcore/db/infodatareader.h"
#include "blackcore/webdataservices.h"
#include "blackcore/setupreader.h"
#include "blackmisc/slot.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/datacache.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/loghandler.h"
#include "blackmisc/metadatautils.h"
#include "blackmisc/registermetadata.h"
#include "blackmisc/settingscache.h"
#include "blackmisc/verify.h"
#include "blackconfig/buildconfig.h"

#include <QAction>
#include <QCloseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QFont>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QSplashScreen>
#include <QMessageBox>
#include <QStyleFactory>
#include <QStringList>
#include <QStringBuilder>
#include <QStyle>
#include <QSysInfo>
#include <QUrl>
#include <QWidget>
#include <QWindow>
#include <QMainWindow>
#include <QMessageBox>
#include <QtGlobal>
#include <QWhatsThis>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Db;
using namespace BlackMisc::Network;
using namespace BlackGui::Components;
using namespace BlackCore;
using namespace BlackCore::Data;
using namespace BlackCore::Context;

BlackGui::CGuiApplication *sGui = nullptr; // set by constructor

namespace BlackGui
{
    CGuiApplication *CGuiApplication::instance()
    {
        return qobject_cast<CGuiApplication *>(CApplication::instance());
    }

    const BlackMisc::CLogCategoryList &CGuiApplication::getLogCategories()
    {
        static const CLogCategoryList l(CApplication::getLogCategories().join({ CLogCategory::guiComponent() }));
        return l;
    }

    CGuiApplication::CGuiApplication(const QString &applicationName, CApplicationInfo::Application application, const QPixmap &icon) :
        CApplication(applicationName, application, false)
    {
        this->addWindowModeOption();
        this->addWindowResetSizeOption();
        this->addWindowScaleSizeOption();

        // notify when app goes down
        connect(qGuiApp, &QGuiApplication::lastWindowClosed, this, &CGuiApplication::gracefulShutdown);

        // follow up on web data services
        connect(this, &CApplication::webDataServicesStarted, this, &CGuiApplication::onWebDataServicesStarted);

        if (!sGui)
        {
            CGuiApplication::registerMetadata();
            CApplication::init(false); // base class without metadata
            if (this->hasSetupReader()) { this->getSetupReader()->setCheckCmdLineBootstrapUrl(false); } // no connect checks on setup reader (handled with interactive setup loading)
            CGuiApplication::adjustPalette();
            this->setWindowIcon(icon);
            this->settingsChanged();
            this->setCurrentFontValues(); // most likely the default font and not any stylesheet font at this time
            sGui = this;

            connect(&m_styleSheetUtility, &CStyleSheetUtility::styleSheetsChanged, this, &CGuiApplication::onStyleSheetsChanged, Qt::QueuedConnection);
            connect(this, &CGuiApplication::startUpCompleted, this, &CGuiApplication::superviseWindowMinSizes, Qt::QueuedConnection);

            // splash screen
            connect(this->getSetupReader(), &CSetupReader::setupLoadingMessages, this, &CGuiApplication::displaySplashMessages, Qt::QueuedConnection);
        }
    }

    CGuiApplication::~CGuiApplication()
    {
        sGui = nullptr;
    }

    void CGuiApplication::registerMetadata()
    {
        CApplication::registerMetadata();
        BlackGui::registerMetadata();
    }

    void CGuiApplication::addWindowModeOption()
    {
        m_cmdWindowMode = QCommandLineOption({"w", "window"}, QCoreApplication::translate("main", "Windows: (n)ormal, (f)rameless, (t)ool."), "windowtype");
        this->addParserOption(m_cmdWindowMode);
    }

    void CGuiApplication::addWindowResetSizeOption()
    {
        m_cmdWindowSizeReset = QCommandLineOption({{"r", "resetsize"}, QCoreApplication::translate("main", "Reset window size (ignore saved values).")});
        this->addParserOption(m_cmdWindowSizeReset);
    }

    void CGuiApplication::addWindowScaleSizeOption()
    {
        // just added here to display it in help
        // parseScaleFactor() is used since it is needed upfront (before application is created)
        m_cmdWindowScaleSize = QCommandLineOption("scale", QCoreApplication::translate("main", "Scale: number."), "scalevalue");
        this->addParserOption(m_cmdWindowScaleSize);
    }

    void CGuiApplication::addWindowStateOption()
    {
        m_cmdWindowStateMinimized =  QCommandLineOption({{"m", "minimized"}, QCoreApplication::translate("main", "Start minimized in system tray.")});
        this->addParserOption(m_cmdWindowStateMinimized);
    }

    Qt::WindowState CGuiApplication::getWindowState() const
    {
        if (m_cmdWindowStateMinimized.valueName() == "empty") { return Qt::WindowNoState; }
        if (m_parser.isSet(m_cmdWindowStateMinimized)) { return Qt::WindowMinimized; }
        return Qt::WindowNoState;
    }

    CEnableForFramelessWindow::WindowMode CGuiApplication::getWindowMode() const
    {
        if (this->isParserOptionSet(m_cmdWindowMode))
        {
            const QString v(this->getParserValue(m_cmdWindowMode));
            return CEnableForFramelessWindow::stringToWindowMode(v);
        }
        else
        {
            return CEnableForFramelessWindow::WindowNormal;
        }
    }

    void CGuiApplication::splashScreen(const QString &resource)
    {
        if (m_splashScreen)
        {
            m_splashScreen.reset(); // delete old one
        }
        if (!resource.isEmpty())
        {
            const QPixmap pm(resource);
            this->splashScreen(pm);
        }
    }

    void CGuiApplication::splashScreen(const QPixmap &pixmap)
    {
        if (m_splashScreen)
        {
            m_splashScreen.reset(); // delete old one
        }

        QFont splashFont;
        splashFont.setFamily("Arial");
        // splashFont.setBold(true);
        splashFont.setPointSize(10);
        splashFont.setStretch(125);

        m_splashScreen.reset(new CSplashScreen(pixmap.scaled(256, 256)));
        m_splashScreen->show();
        m_splashScreen->showStatusMessage(CBuildConfig::getVersionString());
        m_splashScreen->setSplashFont(splashFont);

        this->processEventsToRefreshGui();
    }

    void CGuiApplication::displaySplashMessage(const CStatusMessage &msg)
    {
        if (msg.isEmpty()) { return; }
        if (!m_splashScreen) { return; }
        if (this->isShuttingDown()) { return; }
        if (!m_splashScreen->isVisible())  { return; }
        m_splashScreen->showStatusMessage(msg);
    }

    void CGuiApplication::displaySplashMessages(const CStatusMessageList &msgs)
    {
        if (msgs.isEmpty()) { return; }
        for (const CStatusMessage &m : msgs)
        {
            if (!m_splashScreen) { return; }
            this->displaySplashMessage(m);
            this->processEventsToRefreshGui();
            if (!sGui) { return; }
        }
    }

    void CGuiApplication::processEventsToRefreshGui() const
    {
        if (this->isShuttingDown()) { return; }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    QWidget *CGuiApplication::mainApplicationWidget()
    {
        return CGuiUtility::mainApplicationWidget();
    }

    void CGuiApplication::registerMainApplicationWidget(QWidget *mainWidget)
    {
        CGuiUtility::registerMainApplicationWidget(mainWidget);
    }

    bool CGuiApplication::hasMinimumMappingVersion() const
    {
        if (this->getGlobalSetup().isSwiftVersionMinimumMappingVersion()) { return true; }

        const QString msg = QStringLiteral("Your are using swift version: '%1'.\nCreating mappings requires at least '%2'.").arg(CBuildConfig::getVersionString(), this->getGlobalSetup().getMappingMinimumVersionString());
        QMessageBox::warning(this->mainApplicationWindow(), "Version check", msg, QMessageBox::Close);
        return false;
    }

    QMainWindow *CGuiApplication::mainApplicationWindow()
    {
        return qobject_cast<QMainWindow *>(CGuiApplication::mainApplicationWidget());
    }

    IMainWindowAccess *CGuiApplication::mainWindowAccess()
    {
        IMainWindowAccess *m = qobject_cast<IMainWindowAccess *>(mainApplicationWidget());
        return m;
    }

    void CGuiApplication::initMainApplicationWidget(QWidget *mainWidget)
    {
        if (!mainWidget) { return; }
        if (m_uiSetupCompleted) { return; }
        m_uiSetupCompleted = true;

        const QString name = this->setExtraWindowTitle("", mainWidget);
        mainWidget->setObjectName(QCoreApplication::applicationName());
        mainWidget->setWindowIcon(m_windowIcon);
        mainWidget->setWindowIconText(name);
        CStyleSheetUtility::setQSysInfoProperties(mainWidget, true);
        CGuiUtility::registerMainApplicationWidget(mainWidget);
        emit this->uiObjectTreeReady();
    }

    void CGuiApplication::addWindowFlags(Qt::WindowFlags flags)
    {
        QWidget *maw = this->mainApplicationWidget();
        if (maw)
        {
            Qt::WindowFlags windowFlags = maw->windowFlags();
            windowFlags |= flags;
            maw->setWindowFlags(windowFlags);
        }
        else
        {
            QPointer<CGuiApplication> myself(this);
            connectOnce(this, &CGuiApplication::uiObjectTreeReady, this, [ = ]
            {
                if (!myself) { return; }
                this->addWindowFlags(flags);
            });
        }
    }

    QString CGuiApplication::setExtraWindowTitle(const QString &extraInfo, QWidget *mainWindowWidget) const
    {
        QString name(this->getApplicationNameVersionDetailed());
        if (!extraInfo.isEmpty()) { name = extraInfo % u' ' % name; }
        if (!mainWindowWidget) { return name; }
        mainWindowWidget->setWindowTitle(name);
        return name;
    }

    void CGuiApplication::setWindowIcon(const QPixmap &icon)
    {
        instance()->m_windowIcon = icon;
        QApplication::setWindowIcon(icon);
    }

    void CGuiApplication::exit(int retcode)
    {
        CApplication::exit(retcode);
    }

    void CGuiApplication::highDpiScreenSupport(double scaleFactor)
    {
        if (scaleFactor < 0)
        {
            qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        }
        else
        {
            QApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // DPI support
            QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // HiDPI pixmaps
            const QString sf = QString::number(scaleFactor, 'f', 2);
            qputenv("QT_SCALE_FACTOR", sf.toLatin1());
        }
    }

    bool CGuiApplication::isUsingHighDpiScreenSupport()
    {
        return CGuiUtility::isUsingHighDpiScreenSupport();
    }

    QScreen *CGuiApplication::currentScreen()
    {
        QWidget *w = CGuiApplication::mainApplicationWidget();
        const int s = QApplication::desktop()->screenNumber(w);
        if (s < QGuiApplication::screens().size()) { return QGuiApplication::screens()[s]; }
        return QGuiApplication::primaryScreen();
    }

    QRect CGuiApplication::currentScreenGeometry()
    {
        const QScreen *s = currentScreen();
        if (s) return s->geometry();
        return {};
    }

    void CGuiApplication::modalWindowToFront()
    {
        if (!QGuiApplication::modalWindow()) { return; }
        QGuiApplication::modalWindow()->raise();
    }

    bool CGuiApplication::saveWindowGeometryAndState(const QMainWindow *window) const
    {
        if (!window) { return false; }
        QSettings settings("swift-project.org", this->getApplicationName());
        settings.setValue("geometry", window->saveGeometry());
        settings.setValue("windowState", window->saveState());
        return true;
    }

    void CGuiApplication::resetWindowGeometryAndState()
    {
        QByteArray ba;
        QSettings settings("swift-project.org", this->getApplicationName());
        settings.setValue("geometry", ba);
        settings.setValue("windowState", ba);
    }

    bool CGuiApplication::restoreWindowGeometryAndState(QMainWindow *window)
    {
        if (!window) { return false; }
        const QSettings settings("swift-project.org", this->getApplicationName());
        const QString location = settings.fileName();
        CLogMessage(this).info(u"GUI settings are here: '%1'") << location;

        const QByteArray g = settings.value("geometry").toByteArray();
        const QByteArray s = settings.value("windowState").toByteArray();
        if (g.isEmpty() || s.isEmpty()) { return false; }

        // block for subscriber
        {
            const auto pattern = CLogPattern().withSeverity(CStatusMessage::SeverityError);
            const QString parameter = m_cmdWindowSizeReset.names().first();
            CLogSubscriber logSub(this, [&](const CStatusMessage & message)
            {
                // handles an error in restoreGeometry/State
                const int ret = QMessageBox::critical(sGui->mainApplicationWidget(), sGui->getApplicationNameAndVersion(),
                                                      QStringLiteral(
                                                              "Restoring the window state/geometry failed!\n"
                                                              "You need to reset the window size (command -%1).\n\n"
                                                              "Original msg: %2\n\n"
                                                              "We can try to reset the values and restart\n"
                                                              "Do you want to try?"
                                                      ).arg(parameter, message.getMessage()),
                                                      QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes)
                {
                    this->resetWindowGeometryAndState();
                    this->restartApplication();
                }
                // most likely crashing if we do nothing

            });
            logSub.changeSubscription(pattern);

            window->restoreGeometry(g);
            window->restoreState(s);
        }
        return true;
    }

    void CGuiApplication::onStartUpCompleted()
    {
        CApplication::onStartUpCompleted();
        this->setCurrentFontValues();

        const QString metricInfo = CGuiUtility::metricsInfo();
        CLogMessage(this).info(metricInfo);

        // window size
        if (m_minWidthChars > 0 || m_minHeightChars > 0)
        {
            const QSizeF fontMetricEstSize = CGuiUtility::fontMetricsEstimateSize(m_minWidthChars, m_minHeightChars);
            QWidget *mw = CGuiUtility::mainApplicationWidget();
            if (mw)
            {
                // setMinimumSizeInCharacters sets m_minHeightChars/m_minWidthChars
                QSize cs = mw->size();
                if (m_minWidthChars > 0)  { cs.setWidth(qRound(fontMetricEstSize.width())); }
                if (m_minHeightChars > 0) { cs.setHeight(qRound(fontMetricEstSize.height())); }
                mw->resize(cs);
            }
        }
        if (m_saveMainWidgetState && !this->isSet(m_cmdWindowSizeReset))
        {
            const Qt::KeyboardModifiers km = QGuiApplication::queryKeyboardModifiers();
            const bool shiftAlt = km.testFlag(Qt::ShiftModifier) && km.testFlag(Qt::AltModifier);
            if (!shiftAlt) { this->restoreWindowGeometryAndState(); }
        }

        if (m_splashScreen)
        {
            m_splashScreen->close(); // GUI
            m_splashScreen.reset();
        }
    }

    QString CGuiApplication::beautifyHelpMessage(const QString &helpText)
    {
        // just formatting Qt help message into HTML table
        if (helpText.isEmpty()) { return {}; }
        const QStringList lines(helpText.split('\n'));
        QString html;
        bool tableMode = false;
        bool pendingTr = false;
        for (const QString &l : lines)
        {
            QString lt(l.trimmed());
            if (!tableMode && lt.startsWith("-"))
            {
                tableMode = true;
                html += "<table>\n";
            }
            if (!tableMode)
            {
                html += l.toHtmlEscaped();
                html += "<br>";
            }
            else
            {
                // in table mode
                if (lt.startsWith("-"))
                {
                    if (pendingTr)
                    {
                        html += "</td></tr>\n";
                    }
                    html += "<tr><td>";
                    thread_local const QRegularExpression reg("[ ]{2,}");
                    html += lt.replace(reg, "</td><td>");
                    pendingTr = true;
                }
                else
                {
                    html += " ";
                    html += l.simplified().toHtmlEscaped();
                }
            }
        }
        html += "</table>\n";
        return html;
    }

    double CGuiApplication::parseScaleFactor(int argc, char *argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            if (qstrcmp(argv[i], "--scale") == 0 || qstrcmp(argv[i], "-scale") == 0)
            {
                if (i + 1 >= argc) { return -1.0; } // no value
                const QString factor(argv[i + 1]);
                bool ok;
                const double f = factor.toDouble(&ok);
                return ok ? f : -1.0;
            }
        }
        return -1.0;
    }

    bool CGuiApplication::cmdLineErrorMessage(const QString &errorMessage, bool retry) const
    {
        const QString helpText(beautifyHelpMessage(m_parser.helpText()));
        constexpr int MaxLength = 60;

        QString htmlMsg;
        if (errorMessage.length() > MaxLength)
        {
            htmlMsg = "<html><head/><body><h4>" + errorMessage.left(MaxLength) + "..." + "</h4>" +
                      "Details: " + errorMessage + "<br><br>";
        }
        else
        {
            htmlMsg = "<html><head/><body><h4>" + errorMessage + "</h4>";
        }
        htmlMsg += helpText + "</body></html>";

        const int r = QMessageBox::warning(nullptr,
                                           QGuiApplication::applicationDisplayName(),
                                           htmlMsg, QMessageBox::Abort, retry ? QMessageBox::Retry : QMessageBox::NoButton);
        return (r == QMessageBox::Retry);
    }

    bool CGuiApplication::cmdLineErrorMessage(const CStatusMessageList &msgs, bool retry) const
    {
        if (msgs.isEmpty()) { return false; }
        if (!msgs.hasErrorMessages()) { return false; }
        static const CPropertyIndexList propertiesSingle({ CStatusMessage::IndexMessage });
        static const CPropertyIndexList propertiesMulti({ CStatusMessage::IndexSeverityAsString, CStatusMessage::IndexMessage });
        const QString helpText(CGuiApplication::beautifyHelpMessage(m_parser.helpText()));
        const QString msgsHtml = msgs.toHtml(msgs.size() > 1 ? propertiesMulti : propertiesSingle);
        const int r = QMessageBox::critical(nullptr,
                                            QGuiApplication::applicationDisplayName(),
                                            "<html><head><body>" + msgsHtml + "<br><br>" + helpText + "</body></html>", QMessageBox::Abort, retry ? QMessageBox::Retry : QMessageBox::NoButton);
        return (r == QMessageBox::Retry);
    }

    bool CGuiApplication::isCmdWindowSizeResetSet() const
    {
        return this->isParserOptionSet(m_cmdWindowSizeReset);
    }

    bool CGuiApplication::displayInStatusBar(const CStatusMessage &message)
    {
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return false; }
        return m->displayInStatusBar(message);
    }

    bool CGuiApplication::displayInOverlayWindow(const CStatusMessage &message, int timeOutMs)
    {
        if (message.isEmpty()) { return false; }
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return IMainWindowAccess::displayInOverlayWindow(message, timeOutMs); }
        return m->displayInOverlayWindow(message, timeOutMs);
    }

    bool CGuiApplication::displayInOverlayWindow(const CStatusMessageList &messages, int timeOutMs)
    {
        if (messages.isEmpty()) { return false; }
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return IMainWindowAccess::displayInOverlayWindow(messages, timeOutMs); }
        return m->displayInOverlayWindow(messages, timeOutMs);
    }

    bool CGuiApplication::displayInOverlayWindow(const QString &html, int timeOutMs)
    {
        if (html.isEmpty()) { return false; }
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return IMainWindowAccess::displayInOverlayWindow(html, timeOutMs); }
        return m->displayInOverlayWindow(html, timeOutMs);
    }

    bool CGuiApplication::displayTextInConsole(const QString &text)
    {
        IMainWindowAccess *m = mainWindowAccess();
        BLACK_VERIFY_X(m, Q_FUNC_INFO, "No access interface");
        if (!m) { return false; }
        return m->displayTextInConsole(text);
    }

    void CGuiApplication::addMenuForSettingsAndCache(QMenu &menu)
    {
        QMenu *sm = menu.addMenu(CIcons::appSettings16(), "Settings");
        sm->setIcon(CIcons::appSettings16());
        QAction *a = sm->addAction(CIcons::disk16(), "Settings directory");
        bool c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const QString path(QDir::toNativeSeparators(CSettingsCache::persistentStore()));
            if (QDir(path).exists())
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("Reset settings");
        c = connect(a, &QAction::triggered, this, [ = ]
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            CSettingsCache::instance()->clearAllValues();
            this->displayTextInConsole("Cleared all settings!");
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("List settings files");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const QStringList files(CSettingsCache::instance()->enumerateStore());
            this->displayTextInConsole(files.join("\n"));
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        sm = menu.addMenu("Cache");
        sm->setIcon(CIcons::appSettings16());
        a = sm->addAction(CIcons::disk16(), "Cache directory");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            const QString path(QDir::toNativeSeparators(CDataCache::persistentStore()));
            if (QDir(path).exists())
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("Reset cache");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const QStringList files = CApplication::clearCaches();
            this->displayTextInConsole(u"Cleared caches! " % QString::number(files.size()) + " files");
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("List cache files");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const QStringList files(CDataCache::instance()->enumerateStore());
            this->displayTextInConsole(files.join("\n"));
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction(CIcons::disk16(), "Log directory");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            this->openStandardLogDirectory();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction(CIcons::swift24(), "Check for updates");
        c = connect(a, &QAction::triggered, this, &CGuiApplication::checkNewVersionMenu);
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction(CIcons::monitorError16(), "Network config. (console)");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const QString r = CNetworkUtils::createNetworkConfigurationReport(this->getNetworkConfigurationManager(), this->getNetworkAccessManager());
            this->displayTextInConsole(r);

            if (this->getNetworkWatchdog())
            {
                const QString w = this->getNetworkWatchdog()->getCheckInfo();
                this->displayTextInConsole(w);
            }
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuForStyleSheets(QMenu &menu)
    {
        QMenu *sm = menu.addMenu("Style sheet");
        QAction *aReload = sm->addAction(CIcons::refresh16(), "Reload");
        bool c = connect(aReload, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            this->reloadStyleSheets();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        QAction *aOpen = sm->addAction(CIcons::text16(), "Open qss file");
        c = connect(aOpen, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            this->openStandardWidgetStyleSheet();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuFile(QMenu &menu)
    {
        addMenuForSettingsAndCache(menu);
        addMenuForStyleSheets(menu);
        QAction *a = nullptr;
        bool c = false;
        if (this->getApplicationInfo().getApplication() != CApplicationInfo::Laucher)
        {
            menu.addSeparator();
            a = menu.addAction(CIcons::swiftLauncher24(), "Start swift launcher");
            c = connect(a, &QAction::triggered, this, &CGuiApplication::startLauncher);
            Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        }

        menu.addSeparator();
        a = menu.addAction("E&xit");
        // a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q)); // avoid accidentally closing
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            // a close event might already trigger a shutdown
            if (!sGui || sGui->isShuttingDown()) { return; }
            if (!this->mainApplicationWidget()) { return; }
            this->mainApplicationWidget()->close();

            // T596, do not shutdown here, as close can be canceled
            // if shutdown is called, there is no way back
            // this->gracefulShutdown();
        },
        Qt::QueuedConnection);
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuInternals(QMenu &menu)
    {
        QMenu *sm = menu.addMenu("JSON files/Templates");
        QAction *a = sm->addAction("JSON bootstrap");
        bool c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const CGlobalSetup s = this->getGlobalSetup();
            this->displayTextInConsole(s.toJsonString());
        }, Qt::QueuedConnection);
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = sm->addAction("JSON version update info (for info only)");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            const CUpdateInfo info = this->getUpdateInfo();
            this->displayTextInConsole(info.toJsonString());
        }, Qt::QueuedConnection);
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        if (this->hasWebDataServices())
        {
            a = menu.addAction("Services log.(console)");
            c = connect(a, &QAction::triggered, this, [ = ]()
            {
                if (!sGui || sGui->isShuttingDown()) { return; }
                this->displayTextInConsole(this->getWebDataServices()->getReadersLog());
                CLogMessage(this).info(u"Displayed services log.");
            }, Qt::QueuedConnection);
            Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

            a = sm->addAction("JSON DB info (for info only)");
            c = connect(a, &QAction::triggered, this, [ = ]()
            {
                if (!sGui || sGui->isShuttingDown()) { return; }
                if (!this->getWebDataServices()->getDbInfoDataReader()) { return; }
                const CDbInfoList info = this->getWebDataServices()->getDbInfoDataReader()->getInfoObjects();
                this->displayTextInConsole(u"DB info:\n" % info.toJsonString());
            }, Qt::QueuedConnection);
            Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

            a = sm->addAction("JSON shared info (for info only)");
            c = connect(a, &QAction::triggered, this, [ = ]()
            {
                if (!sGui || sGui->isShuttingDown()) { return; }
                if (!this->getWebDataServices()->getDbInfoDataReader()) { return; }
                const CDbInfoList info = this->getWebDataServices()->getSharedInfoDataReader()->getInfoObjects();
                this->displayTextInConsole(u"Shared info:\n" % info.toJsonString());
            }, Qt::QueuedConnection);
            Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        }

        a = menu.addAction("Metadata (slow)");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            this->displayTextInConsole(getAllUserMetatypesTypes());
        }, Qt::QueuedConnection);
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuWindow(QMenu &menu)
    {
        QPointer<QWidget> w = CGuiApplication::mainApplicationWidget();
        if (!w) { return; }
        const QSize iconSize = CIcons::empty16().size();
        static QPixmap iconEmpty;

        QPixmap icon = w->style()->standardIcon(QStyle::SP_TitleBarMaxButton).pixmap(iconSize);
        QAction *a = menu.addAction(icon.isNull() ? iconEmpty : icon.scaled(iconSize), "Fullscreen");
        bool c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!w) { return; }
            w->showFullScreen();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        icon = w->style()->standardIcon(QStyle::SP_TitleBarMinButton).pixmap(iconSize);
        a = menu.addAction(icon.isNull() ? iconEmpty : icon.scaled(iconSize), "Minimize");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!w) { return; }
            w->showMinimized();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        icon = w->style()->standardIcon(QStyle::SP_TitleBarNormalButton).pixmap(iconSize);
        a = menu.addAction(icon.isNull() ? iconEmpty : icon.scaled(iconSize), "Normal");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!w) { return; }
            w->showNormal();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction("Toggle stay on top");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!w) { return; }
            this->toggleStayOnTop();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::addMenuHelp(QMenu &menu)
    {
        QPointer<QWidget> w = mainApplicationWidget();
        if (!w) { return; }
        QAction *a = menu.addAction(w->style()->standardIcon(QStyle::SP_TitleBarContextHelpButton), "Online help");

        bool c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            this->showHelp();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");

        a = menu.addAction(QApplication::windowIcon(), "About swift");
        c = connect(a, &QAction::triggered, this, [ = ]()
        {
            if (!w) { return; }
            CAboutDialog dialog(w);
            dialog.exec();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);

        // https://joekuan.wordpress.com/2015/09/23/list-of-qt-icons/
        a = menu.addAction(QApplication::style()->standardIcon(QStyle::SP_TitleBarMenuButton), "About Qt");
        c = connect(a, &QAction::triggered, this, []()
        {
            QApplication::aboutQt();
        });
        Q_ASSERT_X(c, Q_FUNC_INFO, "Connect failed");
        Q_UNUSED(c);
    }

    void CGuiApplication::showHelp(const QString &context) const
    {
        if (this->isShuttingDown()) { return; }
        const CGlobalSetup gs = this->getGlobalSetup();
        const CUrl helpPage = gs.getHelpPageUrl(context);
        if (helpPage.isEmpty())
        {
            CLogMessage(this).warning(u"No help page");
            return;
        }
        QDesktopServices::openUrl(helpPage);
    }

    void CGuiApplication::showHelp(const QObject *qObject) const
    {
        if (this->isShuttingDown()) { return; }
        if (!qObject || qObject->objectName().isEmpty()) { this->showHelp(); }
        this->showHelp(qObject->objectName());
    }

    bool CGuiApplication::triggerShowHelp(const QWidget *widget, QEvent *event)
    {
        if (!widget) { return false; }
        if (!event)  { return false; }
        const QEvent::Type t = event->type();
        if (t != QEvent::EnterWhatsThisMode) { return false; }
        QWhatsThis::leaveWhatsThisMode();
        event->accept();
        if (!widget->isVisible()) { return true; } // ignore invisble ones
        const QPointer<const QWidget> wp(widget);
        QTimer::singleShot(0, sGui, [ = ]
        {
            if (!wp || !sGui || sGui->isShuttingDown()) { return; }
            sGui->showHelp(widget);
        });
        return true;
    }

    const CStyleSheetUtility &CGuiApplication::getStyleSheetUtility() const
    {
        return m_styleSheetUtility;
    }

    QString CGuiApplication::getWidgetStyle() const
    {
        QString currentWidgetStyle(QApplication::style()->metaObject()->className());
        if (currentWidgetStyle.startsWith('Q')) { currentWidgetStyle.remove(0, 1); }
        return currentWidgetStyle.replace("Style", "");
    }

    bool CGuiApplication::reloadStyleSheets()
    {
        return m_styleSheetUtility.read();
    }

    bool CGuiApplication::openStandardWidgetStyleSheet()
    {
        const QString fn = CStyleSheetUtility::fileNameAndPathStandardWidget();
        return QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
    }

    bool CGuiApplication::openStandardLogDirectory()
    {
        const QString path(QDir::toNativeSeparators(CDirectoryUtils::logDirectory()));
        if (!QDir(path).exists()) { return false; }
        return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }

    bool CGuiApplication::updateFont(const QString &fontFamily, const QString &fontSize, const QString &fontStyle, const QString &fontWeight, const QString &fontColor)
    {
        return m_styleSheetUtility.updateFont(fontFamily, fontSize, fontStyle, fontWeight, fontColor);
    }

    bool CGuiApplication::updateFont(const QString &qss)
    {
        return m_styleSheetUtility.updateFont(qss);
    }

    bool CGuiApplication::resetFont()
    {
        return m_styleSheetUtility.resetFont();
    }

    void CGuiApplication::setMinimumSizeInCharacters(int widthChars, int heightChars)
    {
        m_minWidthChars = widthChars;
        m_minHeightChars = heightChars;
    }

    bool CGuiApplication::interactivelySynchronizeSetup(int timeoutMs)
    {
        bool ok = false;
        do
        {
            const CStatusMessageList msgs = this->synchronizeSetup(timeoutMs);
            if (msgs.hasErrorMessages())
            {
                CSetupLoadingDialog dialog(msgs, this->mainApplicationWidget());
                if (sGui)
                {
                    static const QString style = sGui->getStyleSheetUtility().styles(
                    {
                        CStyleSheetUtility::fileNameFonts(),
                        CStyleSheetUtility::fileNameStandardWidget()
                    });
                    dialog.setStyleSheet(style);
                }

                const int r = dialog.exec();
                if (r == QDialog::Rejected)
                {
                    break; // exit with false state, as file was not loaded
                }
                else
                {
                    // run loop again and sync again
                }
            }
            else
            {
                // setup loaded
                ok = true;
                break;
            }
        }
        while (!ok);
        return ok;
    }

    bool CGuiApplication::parseAndSynchronizeSetup(int timeoutMs)
    {
        if (!this->parseAndStartupCheck()) { return false; }
        return this->interactivelySynchronizeSetup(timeoutMs);
    }

    QDialog::DialogCode CGuiApplication::showCloseDialog(QMainWindow *mainWindow, QCloseEvent *closeEvent)
    {
        this->saveSettingsOnShutdown(false); // saving itself will be handled in dialog
        const bool needsDialog = this->hasUnsavedSettings();
        if (!needsDialog) { return QDialog::Accepted; }
        if (!m_closeDialog)
        {
            m_closeDialog = new CApplicationCloseDialog(mainWindow);
            if (mainWindow && !mainWindow->windowTitle().isEmpty())
            {
                m_closeDialog->setWindowTitle(mainWindow->windowTitle());
                m_closeDialog->setModal(true);
            }
        }

        // dialog will handle the saving
        const QDialog::DialogCode c = static_cast<QDialog::DialogCode>(m_closeDialog->exec());

        // settings already saved when reaching here
        switch (c)
        {
        case QDialog::Rejected:
            if (closeEvent) { closeEvent->ignore(); }
            break;
        default:
            break;
        }
        return c;
    }

    void CGuiApplication::cmdLineHelpMessage()
    {
        if (CBuildConfig::isRunningOnWindowsNtPlatform())
        {
            const QString helpText(CGuiApplication::beautifyHelpMessage(m_parser.helpText()));
            QMessageBox::information(nullptr, QGuiApplication::applicationDisplayName(),
                                     "<html><head/><body>" + helpText + "</body></html>");
        }
        else
        {
            CApplication::cmdLineHelpMessage();
        }
    }

    void CGuiApplication::cmdLineVersionMessage() const
    {
        if (CBuildConfig::isRunningOnWindowsNtPlatform())
        {
            QMessageBox::information(nullptr,
                                     QGuiApplication::applicationDisplayName(),
                                     QGuiApplication::applicationDisplayName() + ' ' +
                                     QCoreApplication::applicationVersion());
        }
        else
        {
            CApplication::cmdLineVersionMessage();
        }
    }

    bool CGuiApplication::parsingHookIn()
    {
        return true;
    }

    void CGuiApplication::onCoreFacadeStarted()
    {
        if (this->supportsContexts())
        {
            connect(this->getIContextApplication(), &IContextApplication::requestDisplayOnConsole, this, &CGuiApplication::displayTextInConsole);
        }
    }

    void CGuiApplication::checkNewVersion(bool onlyIfNew)
    {
        if (!m_updateDialog)
        {
            // without parent stylesheet is not inherited
            m_updateDialog = new CUpdateInfoDialog(this->mainApplicationWidget());
        }

        if (onlyIfNew && !m_updateDialog->isNewVersionAvailable()) { return; }
        const int result = m_updateDialog->exec();
        if (result != QDialog::Accepted) { return; }
    }

    QString CGuiApplication::getFontInfo() const
    {
        const QWidget *w = this->mainApplicationWidget();
        if (!w) { return QStringLiteral("Font info not available"); }
        return QStringLiteral("Family: '%1', average width: %2").
               arg(w->font().family()).
               arg(w->fontMetrics().averageCharWidth());
    }

    bool CGuiApplication::toggleStayOnTop()
    {
        QMainWindow *w = CGuiApplication::mainApplicationWindow();
        if (!w) { return false; }
        const bool onTop = CGuiUtility::toggleStayOnTop(w);
        CLogMessage(w).info(onTop ? QStringLiteral("Window on top") : QStringLiteral("Window not always on top"));
        emit this->alwaysOnTop(onTop);
        return onTop;
    }

    void CGuiApplication::triggerNewVersionCheck(int delayedMs)
    {
        if (!m_updateSetting.get()) { return; }
        QTimer::singleShot(delayedMs, this, [ = ]
        {
            if (!sGui || sGui->isShuttingDown()) { return; }
            if (m_updateDialog) { return; }
            this->checkNewVersion(true);
        });
    }

    void CGuiApplication::gracefulShutdown()
    {
        if (m_shutdownInProgress) { return; }
        CApplication::gracefulShutdown();
        if (m_saveMainWidgetState)
        {
            this->saveWindowGeometryAndState();
        }
    }

    void CGuiApplication::settingsChanged()
    {
        // changing widget style is slow, so I try to prevent setting it when nothing changed
        const QString widgetStyle = m_guiSettings.get().getWidgetStyle();
        const QString currentWidgetStyle(this->getWidgetStyle());
        Q_ASSERT_X(CThreadUtils::isCurrentThreadApplicationThread(), Q_FUNC_INFO, "Wrong thread");
        if (!stringCompare(widgetStyle, currentWidgetStyle, Qt::CaseInsensitive))
        {
            const QStringList availableStyles = QStyleFactory::keys();
            if (availableStyles.contains(widgetStyle))
            {
                // changing style freezes the application, so it must not be done in flight mode
                if (this->getIContextNetwork() && this->getIContextNetwork()->isConnected())
                {
                    CLogMessage(this).validationError(u"Cannot change style while connected to network");
                }
                else
                {
                    // QStyle *style = QApplication::setStyle(widgetStyle);
                    QStyle *style = QStyleFactory::create(widgetStyle);
                    // That can crash
                    QApplication::setStyle(style); // subject of crash
                    if (style)
                    {
                        CLogMessage(this).info(u"Changed style to '%1', req.: '%2'") << style->objectName() << widgetStyle;
                    }
                    else
                    {
                        CLogMessage(this).error(u"Unable to set requested style '%1'") << widgetStyle;
                    }
                }
            } // valid style
        }
    }

    void CGuiApplication::checkNewVersionMenu()
    {
        this->checkNewVersion(false);
    }

    void CGuiApplication::adjustPalette()
    {
        // only way to change link color
        // https://stackoverflow.com/q/5497799/356726
        // Ref T84
        QPalette newPalette(qApp->palette());
        const QColor linkColor(135, 206, 250);
        newPalette.setColor(QPalette::Link, linkColor);
        newPalette.setColor(QPalette::LinkVisited, linkColor);
        qApp->setPalette(newPalette);
    }

    void CGuiApplication::onStyleSheetsChanged()
    {
        const QFont f = CGuiUtility::currentFont();
        if (f.pointSize() != m_fontPointSize || f.family() != m_fontFamily)
        {
            emit this->fontChanged();
            CLogMessage(this).info(this->getFontInfo());
        }
        emit this->styleSheetsChanged();
    }

    void CGuiApplication::setCurrentFontValues()
    {
        const QFont font = CGuiUtility::currentFont();
        m_fontFamily = font.family();
        m_fontPointSize = font.pointSize();
    }

    void CGuiApplication::onWebDataServicesStarted(bool success)
    {
        if (success)
        {
            connect(this->getWebDataServices(), &CWebDataServices::databaseReaderMessages, this, &CGuiApplication::displaySplashMessages, Qt::QueuedConnection);
        }
    }

    void CGuiApplication::superviseWindowMinSizes()
    {
        CGuiUtility::superviseMainWindowMinSizes();
    }
} // ns
