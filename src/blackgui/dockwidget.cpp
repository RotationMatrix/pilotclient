/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/components/marginsinput.h"
#include "blackgui/dockwidget.h"
#include "blackgui/guiapplication.h"
#include "blackgui/guiutility.h"
#include "blackgui/stylesheetutility.h"
#include "blackmisc/icons.h"
#include "blackmisc/logmessage.h"

#include <QCloseEvent>
#include <QFrame>
#include <QLayout>
#include <QLayoutItem>
#include <QMenu>
#include <QScopedPointer>
#include <QSettings>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStyle>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidgetAction>
#include <QWidget>
#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackGui::Components;
using namespace BlackGui::Settings;

namespace BlackGui
{
    CDockWidget::CDockWidget(bool allowStatusBar, QWidget *parent) :
        QDockWidget(parent),
        CEnableForFramelessWindow(CEnableForFramelessWindow::WindowTool, false, "framelessDockWidget", this),
        m_allowStatusBar(allowStatusBar)
    {
        // init settings
        this->ps_onStyleSheetsChanged();
        this->initTitleBarWidgets();

        // context menu
        this->m_input = new CMarginsInput(this);
        this->m_input->setMaximumWidth(150);
        this->m_marginMenuAction = new QWidgetAction(this);
        this->m_marginMenuAction->setDefaultWidget(this->m_input);

        this->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &CDockWidget::customContextMenuRequested, this, &CDockWidget::ps_showContextMenu);
        connect(this->m_input, &CMarginsInput::changedMargins, this, &CDockWidget::ps_menuChangeMargins);

        // connect
        connect(this, &QDockWidget::topLevelChanged, this, &CDockWidget::ps_onTopLevelChanged);
        connect(sGui, &CGuiApplication::styleSheetsChanged, this, &CDockWidget::ps_onStyleSheetsChanged);
        connect(this, &QDockWidget::visibilityChanged, this, &CDockWidget::ps_onVisibilityChanged);
    }

    CDockWidget::CDockWidget(QWidget *parent): CDockWidget(true, parent)
    { }

    void CDockWidget::setOriginalTitleBar()
    {
        if (!this->m_titleBarWidgetOriginal) { this->initTitleBarWidgets(); }
        if (this->titleBarWidget() == this->m_titleBarWidgetOriginal) return; // on purpose, as I do not know what happens when I call setTitleBar
        this->setTitleBarWidget(this->m_titleBarWidgetOriginal);
    }

    void CDockWidget::setEmptyTitleBar()
    {
        if (!this->m_titleBarWidgetOriginal) { this->initTitleBarWidgets(); }
        if (this->titleBarWidget() == this->m_titleBarWidgetEmpty) { return; } // on purpose, as I do not know what happens when I call setTitleBar
        this->setTitleBarWidget(this->m_titleBarWidgetEmpty);
    }

    void CDockWidget::setNullTitleBarWidget()
    {
        this->setTitleBarWidget(nullptr);
    }

    void CDockWidget::setMarginsWhenFloating(const QMargins &margins)
    {
        CSettingsDockWidget s = this->getSettings();
        s.setMarginsWhenFloating(margins);
        this->setSettings(s);
    }

    void CDockWidget::setMarginsWhenFloating(int left, int top, int right, int bottom)
    {
        this->setMarginsWhenFloating(QMargins(left, top, right, bottom));
    }

    QMargins CDockWidget::getMarginsWhenFloating() const
    {
        return this->getSettings().getMarginsWhenFloating();
    }

    void CDockWidget::setMarginsWhenFramelessFloating(const QMargins &margins)
    {
        CSettingsDockWidget s = this->getSettings();
        s.setMarginsWhenFramelessFloating(margins);
        this->setSettings(s);
    }

    void CDockWidget::setMarginsWhenFramelessFloating(int left, int top, int right, int bottom)
    {
        this->setMarginsWhenFramelessFloating(QMargins(left, top, right, bottom));
    }

    QMargins CDockWidget::getMarginsWhenFramelessFloating() const
    {
        return this->getSettings().getMarginsWhenFramelessFloating();
    }

    void CDockWidget::setMarginsWhenDocked(const QMargins &margins)
    {
        CSettingsDockWidget s = this->getSettings();
        s.setMarginsWhenDocked(margins);
        this->setSettings(s);
    }

    void CDockWidget::setMarginsWhenDocked(int left, int top, int right, int bottom)
    {
        this->setMarginsWhenDocked(QMargins(left, top, right, bottom));
    }

    QMargins CDockWidget::getMarginsWhenDocked() const
    {
        return this->getSettings().getMarginsWhenDocked();
    }

    bool CDockWidget::isWidgetVisible() const
    {
        return this->m_dockWidgetVisible && this->isVisible();
    }

    void CDockWidget::setWindowTitle(const QString &title)
    {
        this->m_windowTitleBackup = title;
        QDockWidget::setWindowTitle(title);
    }

    void CDockWidget::displayStatusMessage(const BlackMisc::CStatusMessage &statusMessage)
    {
        if (!this->m_allowStatusBar || !this->isFloating()) { return; }
        this->m_statusBar.displayStatusMessage(statusMessage);
    }

    void CDockWidget::displayStatusMessages(const BlackMisc::CStatusMessageList &statusMessages)
    {
        if (!this->m_allowStatusBar || !this->isFloating()) { return; }
        this->m_statusBar.displayStatusMessages(statusMessages);
    }

    void CDockWidget::showTitleWhenDocked(bool show)
    {
        this->m_windowTitleWhenDocked = show;
        if (show)
        {
            QDockWidget::setWindowTitle(this->m_windowTitleBackup);
        }
        else
        {
            QDockWidget::setWindowTitle("");
        }
    }

    void CDockWidget::resetWasAlreadyFloating()
    {
        this->m_wasAlreadyFloating = false;
        this->m_resetedFloating = true;
    }

    void CDockWidget::setPreferredSizeWhenFloating(const QSize &size)
    {
        this->m_preferredSizeWhenFloating = size;
    }

    void CDockWidget::setFrameless(bool frameless)
    {
        CEnableForFramelessWindow::setFrameless(frameless);

        // grip
        bool hasStatusBar = this->m_statusBar.getStatusBar();
        if (frameless)
        {
            if (hasStatusBar)
            {
                this->addFramelessSizeGripToStatusBar(this->m_statusBar.getStatusBar());
            }
        }
        else
        {
            if (hasStatusBar)
            {
                this->hideFramelessSizeGripInStatusBar();
            }
        }

        // margins
        if (this->isFloating())
        {
            this->setContentsMargins(frameless ? this->getMarginsWhenFramelessFloating() : this->getMarginsWhenFloating());
        }

        // resize
        if (frameless)
        {
            QWidget *innerWidget = this->widget(); // the inner widget containing the layout
            Q_ASSERT(innerWidget);
            this->resize(innerWidget->size());
        }

        this->forceStyleSheetUpdate(); // force style sheet reload
    }

    const QString &CDockWidget::propertyOuterWidget()
    {
        static const QString s("outerwidget");
        return s;
    }

    const QString &CDockWidget::propertyInnerWidget()
    {
        static const QString s("innerwidget");
        return s;
    }

    void CDockWidget::toggleFloating()
    {
        bool floating = !this->isFloating();
        if (!floating) { this->setFrameless(false); }
        this->setFloating(floating);
    }

    void CDockWidget::toggleVisibility()
    {
        if (this->isVisible())
        {
            this->hide();
        }
        else
        {
            this->show();
        }
    }

    void CDockWidget::toggleFrameless()
    {
        if (this->isFrameless())
        {
            this->setFrameless(false);
        }
        else
        {
            this->setFrameless(true);
        }
    }

    bool CDockWidget::restoreFromSettings()
    {
        const CSettingsDockWidget s = this->getSettings();
        if (s.isFloating() != this->isFloating())
        {
            this->toggleFloating();
        }
        if (s.isFramless() != this->isFrameless())
        {
            this->toggleFrameless();
        }
        const QByteArray geo(s.getGeometry());
        if (!geo.isEmpty())
        {
            return this->restoreGeometry(geo);
        }
        return true;
    }

    void CDockWidget::closeEvent(QCloseEvent *event)
    {
        if (this->isFloating())
        {
            this->toggleFloating();
            event->setAccepted(false); // refuse -> do not close
        }
        else
        {
            QDockWidget::closeEvent(event);
        }
    }

    void CDockWidget::paintEvent(QPaintEvent *event)
    {
        CStyleSheetUtility::useStyleSheetInDerivedWidget(this, QStyle::PE_FrameDockWidget);
        QDockWidget::paintEvent(event);
    }

    void CDockWidget::mouseMoveEvent(QMouseEvent *event)
    {
        if (!handleMouseMoveEvent(event)) { QDockWidget::mouseMoveEvent(event); } ;
    }

    void CDockWidget::mousePressEvent(QMouseEvent *event)
    {
        if (!handleMousePressEvent(event)) { QDockWidget::mousePressEvent(event); }
    }

    void CDockWidget::addToContextMenu(QMenu *contextMenu) const
    {
        if (this->isFloating())
        {
            contextMenu->addAction(BlackMisc::CIcons::dockTop16(), "Dock", this, &CDockWidget::toggleFloating);
            if (this->isFrameless())
            {
                contextMenu->addAction(BlackMisc::CIcons::tableSheet16(), "Normal window", this, &CDockWidget::toggleFrameless);
            }
            else
            {
                contextMenu->addAction(BlackMisc::CIcons::tableSheet16(), "Frameless", this, &CDockWidget::toggleFrameless);
            }
            contextMenu->addAction(BlackMisc::CIcons::refresh16(), "Redraw", this, SLOT(update()));
        }
        else
        {
            contextMenu->addAction(BlackMisc::CIcons::floatOne16(), "Float", this, &CDockWidget::toggleFloating);
        }
        contextMenu->addAction(BlackMisc::CIcons::load16(), "Restore", this, &CDockWidget::restoreFromSettings);
        contextMenu->addAction(BlackMisc::CIcons::save16(), "Save state", this, &CDockWidget::saveToSettings);

        // Margin action
        if (this->isFloating())
        {
            this->m_input->setMargins(this->contentsMargins());
            contextMenu->addAction(BlackMisc::CIcons::tableSheet16(), "Margins", this, &CDockWidget::ps_dummy);
            contextMenu->addAction(this->m_marginMenuAction);
        }
    }

    void CDockWidget::initialFloating()
    {
        // settings, ii here because name now is set
        this->initSettings();

        // init status bar, as we have now all structures set
        this->initStatusBarAndProperties();

        // for the first time resize
        if (!this->m_preferredSizeWhenFloating.isNull())
        {
            this->m_initialDockedMinimumSize = this->minimumSize();
            this->resize(this->m_preferredSizeWhenFloating);
        }

        // and move
        QPoint mainWindowPos = BlackGui::CGuiUtility::mainWindowPosition();
        if (!mainWindowPos.isNull())
        {
            int x = mainWindowPos.x() + this->m_offsetWhenFloating.x();
            int y = mainWindowPos.y() + this->m_offsetWhenFloating.y();
            this->move(x, y);
        }
    }

    QString CDockWidget::windowTitleOrBackup() const
    {
        QString t(windowTitle());
        if (t.isEmpty()) { return windowTitleBackup(); }
        return t;
    }

    void CDockWidget::ps_onTopLevelChanged(bool topLevel)
    {
        if (topLevel)
        {
            if (this->m_windowTitleBackup != QDockWidget::windowTitle())
            {
                QDockWidget::setWindowTitle(this->m_windowTitleBackup);
            }
            this->setNullTitleBarWidget();
            if (!this->m_wasAlreadyFloating)
            {
                this->initialFloating();
            }
            else
            {
                if (m_wasFrameless) { setFrameless(true); }
            }

            this->setContentsMargins(
                this->isFrameless() ?
                this->getMarginsWhenFramelessFloating() :
                this->getMarginsWhenFloating()
            );
            this->m_statusBar.show();
            this->m_wasAlreadyFloating = true;
        }
        else
        {
            // frameless
            this->setFrameless(false);

            if (!this->m_windowTitleWhenDocked) { QDockWidget::setWindowTitle(""); }
            this->m_statusBar.hide();
            this->setEmptyTitleBar();
            this->setContentsMargins(this->getMarginsWhenDocked());

            // sometimes floating sets a new minimum size, here we reset it
            if (this->minimumHeight() > this->m_initialDockedMinimumSize.height())
            {
                this->setMinimumSize(this->m_initialDockedMinimumSize);
            }
        }

        // relay
        emit this->widgetTopLevelChanged(this, topLevel);
    }

    void CDockWidget::initTitleBarWidgets()
    {
        this->m_titleBarWidgetOriginal = this->titleBarWidget();
        this->m_titleBarWidgetEmpty = new QWidget(this);
        this->setTitleBarWidget(this->m_titleBarWidgetEmpty);
    }

    void CDockWidget::initStatusBarAndProperties()
    {
        if (this->m_statusBar.getStatusBar()) { return; }

        // Typical reasons for asserts here
        // 1) Check the structure, we expect the following hierarchy:
        //    QDockWidget (CDockWidget/CDockWidgetInfoArea) -> QWidget (outer widget) -> QFrame (inner widget)
        //    Structure used for frameless floating windows
        // 2) Check if the "floating" flag is accidentally set for the dock widget in the GUI builder
        // 3) Is the dock widget promoted BlackGui::CDockWidgetInfoArea?
        QWidget *outerWidget = this->widget(); // the outer widget containing the layout
        Q_ASSERT_X(outerWidget, "CDockWidget::initStatusBar", "No outer widget");
        if (!outerWidget) { return; }
        outerWidget->setProperty("dockwidget", propertyOuterWidget());

        Q_ASSERT_X(outerWidget->layout(), "CDockWidget::initStatusBar", "No outer widget layout");
        if (!outerWidget->layout()) { return; }
        Q_ASSERT_X(outerWidget->layout()->itemAt(0) && outerWidget->layout()->itemAt(0)->widget(), "CDockWidget::initStatusBar", "No outer widget layout item");
        if (!outerWidget->layout()->itemAt(0) ||  !outerWidget->layout()->itemAt(0)->widget()) { this->m_allowStatusBar = false; return; }

        // Inner widget is supposed to be a QFrame / promoted QFrame
        QFrame *innerWidget = qobject_cast<QFrame *>(outerWidget->layout()->itemAt(0)->widget()); // the inner widget containing the layout
        Q_ASSERT_X(innerWidget, "CDockWidget::initStatusBar", "No inner widget");
        if (!innerWidget) { this->m_allowStatusBar = false; return; }
        innerWidget->setProperty("dockwidget", propertyInnerWidget());

        // status bar
        if (!this->m_allowStatusBar) { return; }
        this->m_statusBar.initStatusBar();

        // layout
        QVBoxLayout *vLayout = qobject_cast<QVBoxLayout *>(innerWidget->layout());
        Q_ASSERT_X(vLayout, "CDockWidget::initStatusBar", "No outer widget layout");
        if (!vLayout) { this->m_allowStatusBar = false; return; }
        vLayout->addWidget(this->m_statusBar.getStatusBar(), 0, Qt::AlignBottom); // 0->vertical stretch minimum

        // adjust stretching of the original widget. It was the only widget so far
        // and should occupy maximum space
        QWidget *compWidget = innerWidget->findChild<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
        Q_ASSERT(compWidget);
        if (!compWidget) { return; }
        QSizePolicy sizePolicy = compWidget->sizePolicy();
        sizePolicy.setVerticalStretch(1); // make the original widget occupying maximum space
        compWidget->setSizePolicy(sizePolicy);
    }

    void CDockWidget::ps_showContextMenu(const QPoint &pos)
    {
        QPoint globalPos = this->mapToGlobal(pos);
        QScopedPointer<QMenu> contextMenu(new QMenu(this));
        this->addToContextMenu(contextMenu.data());
        QAction *selectedItem = contextMenu.data()->exec(globalPos);
        Q_UNUSED(selectedItem);
    }

    void CDockWidget::ps_onVisibilityChanged(bool visible)
    {
        this->m_dockWidgetVisible = visible;
    }

    void CDockWidget::ps_menuChangeMargins(const QMargins &margins)
    {
        const bool frameless = this->isFrameless();
        const bool floating = this->isFloating();
        if (floating)
        {
            if (frameless)
            {
                this->setMarginsWhenFramelessFloating(margins);
            }
            else
            {
                this->setMarginsWhenFloating(margins);
            }
        }
        else
        {
            this->setMarginsWhenDocked(margins);
        }
        this->setContentsMargins(margins);
    }

    void CDockWidget::ps_settingsChanged()
    {
        // void
    }

    void CDockWidget::ps_dummy()
    {
        // void
    }

    void CDockWidget::ps_onStyleSheetsChanged()
    {
        // style sheet changes go here
    }

    void CDockWidget::forceStyleSheetUpdate()
    {
        QString qss = this->styleSheet();
        this->setStyleSheet(qss.isEmpty() ? " " : "");
        this->setStyleSheet(qss);
    }

    void CDockWidget::initSettings()
    {
        const QString name(this->getNameForSettings());
        CSettingsDockWidgets all = this->m_settings.getCopy();
        if (all.contains(name)) { return; }
        all.getByNameOrInitToDefault(name);
        this->m_settings.set(all);
    }

    QString CDockWidget::getNameForSettings() const
    {
        const QString name(this->m_windowTitleBackup.toLower().remove(' ')); // let`s see how far I get with that
        Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "no name");
        return name;
    }

    CSettingsDockWidget CDockWidget::getSettings() const
    {
        const CSettingsDockWidgets all = this->m_settings.getCopy();
        const QString name(this->getNameForSettings());
        const CSettingsDockWidget s = all.value(name);
        return s;
    }

    void CDockWidget::setSettings(const CSettingsDockWidget &settings)
    {
        const CSettingsDockWidget current = getSettings();
        if (current == settings) { return; }
        CSettingsDockWidgets all = this->m_settings.getCopy();
        const QString name(this->getNameForSettings());
        all.insert(name, settings);
        const CStatusMessage m = this->m_settings.set(all); // saved when shutdown
        if (m.isFailure())
        {
            CLogMessage::preformatted(m);
        }
    }

    void CDockWidget::saveToSettings()
    {
        CSettingsDockWidget s = this->getSettings();
        s.setFloating(this->isFloating());
        s.setFrameless(this->isFrameless());
        s.setGeometry(this->saveGeometry());
        this->setSettings(s);
    }
} // namespace
