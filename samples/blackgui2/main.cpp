#include "introwindow.h"
#include "dockwindow.h"
#include "guimodeenums.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include <QtGlobal>
#include <QApplication>
#include <QMessageBox>
#include <QTranslator>

/*!
 * \brief Main
 * \param argc
 * \param argv
 * \return
 */
int main(int argc, char *argv[])
{
    // register
    Q_INIT_RESOURCE(blackgui);
    BlackMisc::initResources();
    BlackMisc::registerMetadata();
    // BlackMisc::displayAllUserMetatypesTypes();

    QFile file(":blackmisc/translations/blackmisc_i18n_de.qm");
    qDebug() << (file.exists() ? "Found translations in resources" : "No translations in resources");
    QTranslator translator;
    if (translator.load("blackmisc_i18n_de", ":blackmisc/translations/"))
    {
        qDebug() << "Translator loaded";
    }

    // app
    QApplication a(argc, argv);
    a.installTranslator(&translator);

    // modes
    GuiModes::WindowMode windowMode;
    GuiModes::CoreMode coreMode;

    // Dialog to decide external or internal core
    CIntroWindow intro;
    if (intro.exec() == QDialog::Rejected)
    {
        return 0;
    }
    else
    {
        coreMode = intro.getCoreMode();
        windowMode = intro.getWindowMode();
    }
    intro.close();

    // Window
    CDockWindow dockWindow(windowMode);
    dockWindow.show();
    return a.exec();
}
