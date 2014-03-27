#include "introwindow.h"
#include "ui_introwindow.h"
#include <QDesktopServices>
#include <QUrl>

/*
 * Constructor
 */
CIntroWindow::CIntroWindow(QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::CIntroWindow)
{
    ui->setupUi(this);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

/*
 * Destructor
 */
CIntroWindow::~CIntroWindow() { }

/*
 * Window mode
 */
GuiModes::WindowMode CIntroWindow::getWindowMode() const
{
    if (this->ui->rb_WindowNormal->isChecked()) return GuiModes::WindowNormal;
    if (this->ui->rb_WindowFrameless->isChecked()) return GuiModes::WindowFrameless;
    qFatal("Illegal GUI status (window mode");
    return GuiModes::WindowNormal; // just for compiler warning
}

/*
 * Core mode
 */
GuiModes::CoreMode CIntroWindow::getCoreMode() const
{
    if (this->ui->rb_CoreExternal->isChecked())return GuiModes::CoreExternal;
    if (this->ui->rb_CoreExternalVoiceLocal->isChecked()) return GuiModes::CoreExternalVoiceLocal;
    if (this->ui->rb_CoreInGuiProcess->isChecked()) return GuiModes::CoreInGuiProcess;
    qFatal("Illegal GUI status (core mode");
    return GuiModes::CoreExternal; // just for compiler warning
}


/*
 * Button clicked
 */
void CIntroWindow::buttonClicked() const
{
    QObject *sender = QObject::sender();
    if (sender == this->ui->pb_ModelDb)
    {
        QDesktopServices::openUrl(QUrl("http://vatrep.vatsim-germany.org/page/index.php", QUrl::TolerantMode));
    }
    else if (sender == this->ui->pb_WebSite)
    {
        QDesktopServices::openUrl(QUrl("https://dev.vatsim-germany.org/", QUrl::TolerantMode));
    }
}
