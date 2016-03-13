/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "aircraftmodelloaderxplane.h"
#include "xplaneutil.h"
#include "blackmisc/predicates.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/fileutils.h"

#include <QDirIterator>
#include <QTextStream>
#include <QFile>
#include <QRegularExpression>
#include <QDirIterator>

#include <functional>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Network;

namespace BlackMisc
{
    namespace Simulation
    {
        namespace XPlane
        {
            static void normalizePath(QString &path)
            {
                //! \todo KB consider CFileUtil::normalizeFilePathToQtStandard ??
                for (auto &e : path)
                {
                    if (e == '/' || e == ':' || e == '\\')
                    {
                        e = '/';
                    }
                }
            }

            CAircraftModelLoaderXPlane::CAircraftModelLoaderXPlane(const CSimulatorInfo &simInfo, const QString &rootDirectory, const QStringList &excludeDirs) :
                IAircraftModelLoader(simInfo, rootDirectory, excludeDirs)
            { }

            CAircraftModelLoaderXPlane::~CAircraftModelLoaderXPlane()
            {
                // that should be safe as long as the worker uses deleteLater (which it does)
                if (this->m_parserWorker) { this->m_parserWorker->waitForFinished(); }
            }

            CPixmap CAircraftModelLoaderXPlane::iconForModel(const QString &modelString, CStatusMessage &statusMessage) const
            {
                // X-Plane does not have previews. Maybe we can just use the textures?
                Q_UNUSED(modelString)
                Q_UNUSED(statusMessage)
                return {};
            }

            void CAircraftModelLoaderXPlane::startLoadingFromDisk(LoadMode mode, const CAircraftModelList &dbModels)
            {
                m_installedModels.clear();
                if (m_rootDirectory.isEmpty())
                {
                    emit loadingFinished(false, this->m_simulatorInfo);
                    return;
                }

                if (mode.testFlag(LoadInBackground))
                {
                    if (m_parserWorker && !m_parserWorker->isFinished()) { return; }
                    auto rootDirectory = m_rootDirectory;
                    auto excludedDirectories = m_excludedDirectories;
                    m_parserWorker = BlackMisc::CWorker::fromTask(this, "CAircraftModelLoaderXPlane::performParsing",
                                     [this, rootDirectory, excludedDirectories, dbModels]()
                    {
                        auto models = performParsing(rootDirectory, excludedDirectories);
                        mergeWithDbData(models, dbModels);
                        return models;
                    });
                    m_parserWorker->thenWithResult<CAircraftModelList>(this, [this](const auto & models)
                    {
                        this->updateInstalledModels(models);
                    });
                }
                else if (mode.testFlag(LoadDirectly))
                {
                    m_installedModels = performParsing(m_rootDirectory, m_excludedDirectories);
                    mergeWithDbData(m_installedModels, dbModels);
                    emit loadingFinished(true, this->m_simulatorInfo);
                }
            }

            bool CAircraftModelLoaderXPlane::isLoadingFinished() const
            {
                return !m_parserWorker || m_parserWorker->isFinished();
            }

            bool CAircraftModelLoaderXPlane::areModelFilesUpdated() const
            {
                const QDateTime cacheTs(getCacheTimestamp());
                if (!cacheTs.isValid()) { return true; }
                return CFileUtils::containsFileNewerThan(cacheTs, this->getRootDirectory(), true, {fileFilterCsl(), fileFilterFlyable()},  this->m_excludedDirectories);
            }

            bool CAircraftModelLoaderXPlane::hasCachedData() const
            {
                //! \todo KB
                return false;
            }

            QDateTime CAircraftModelLoaderXPlane::getCacheTimestamp() const
            {
                //! \todo KB add cache and report back
                return QDateTime();
            }

            const CAircraftModelList &CAircraftModelLoaderXPlane::getAircraftModels() const
            {
                return m_installedModels;
            }

            void CAircraftModelLoaderXPlane::updateInstalledModels(const CAircraftModelList &models)
            {
                m_installedModels = models;
                emit loadingFinished(true, this->m_simulatorInfo);
            }

            QString CAircraftModelLoaderXPlane::CSLPlane::getModelName() const
            {
                QString modelName = dirNames.join(' ');
                modelName += " ";
                modelName += objectName;
                modelName += " ";
                modelName += textureName;
                return modelName;
            }

            CAircraftModelList CAircraftModelLoaderXPlane::performParsing(const QString &rootDirectory, const QStringList &excludeDirectories)
            {
                CAircraftModelList allModels;
                allModels.push_back(parseCslPackages(CXPlaneUtil::xbusLegacyDir(), excludeDirectories));
                allModels.push_back(parseFlyableAirplanes(rootDirectory, excludeDirectories));
                return allModels;
            }

            //! Add model only if there no other model with the same model string
            void addUniqueModel(const CAircraftModel &model, CAircraftModelList &models)
            {
                if (models.containsModelString(model.getModelString()))
                {
                    CLogMessage(static_cast<CAircraftModelLoaderXPlane *>(nullptr)).warning("Model %1 exists already! Potential model string conflict! Ignoring it.") << model.getModelString();
                }
                models.push_back(model);
            }

            CAircraftModelList CAircraftModelLoaderXPlane::parseFlyableAirplanes(const QString &rootDirectory, const QStringList &excludeDirectories)
            {
                Q_UNUSED(excludeDirectories);
                if (rootDirectory.isEmpty()) { return {}; }

                QDir searchPath(rootDirectory, fileFilterFlyable());
                QDirIterator aircraftIt(searchPath, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

                CAircraftModelList installedModels;
                while (aircraftIt.hasNext())
                {
                    //! \todo KB I would consider exclude dirs here CFileUtils::matchesExcludeDirectory()
                    aircraftIt.next();

                    // <dirname> <filename> for the default model and <dirname> <filename> <texturedir> for the liveries
                    QString modelString = QString("%1 %2").arg(aircraftIt.fileInfo().dir().dirName(), aircraftIt.fileInfo().baseName());

                    CAircraftModel model;
                    model.setModelType(CAircraftModel::TypeOwnSimulatorModel);
                    model.setSimulatorInfo(m_simulatorInfo);
                    model.setFileName(aircraftIt.filePath());
                    model.setModelString(modelString);

                    QFile file(aircraftIt.filePath());
                    file.open(QIODevice::ReadOnly | QIODevice::Text);

                    QTextStream ts(&file);
                    //! \todo Do you rely on case sensitive parsing, mabe case insensitive would be better?
                    if (ts.readLine() == "I" && ts.readLine().contains("version") && ts.readLine() == "ACF")
                    {
                        while (!ts.atEnd())
                        {
                            QString line = ts.readLine();
                            QStringList tokens = line.split(' ');
                            if (tokens.size() != 3) { continue; }
                            if (tokens.at(1) == "acf/_ICAO")
                            {
                                CAircraftIcaoCode icao(tokens.at(2));
                                model.setAircraftIcaoCode(icao);
                                break;
                            }
                        }
                    }
                    file.close();

                    addUniqueModel(model, installedModels);

                    QDirIterator liveryIt(aircraftIt.fileInfo().canonicalPath() + "/liveries", QDir::Dirs | QDir::NoDotAndDotDot);
                    while (liveryIt.hasNext())
                    {
                        liveryIt.next();
                        QString modelStringWithLivery = modelString + liveryIt.fileName();
                        model.setModelString(modelStringWithLivery);

                        addUniqueModel(model, installedModels);
                    }
                }
                return installedModels;
            }

            CAircraftModelList CAircraftModelLoaderXPlane::parseCslPackages(const QString &rootDirectory, const QStringList &excludeDirectories)
            {
                Q_UNUSED(excludeDirectories);
                if (rootDirectory.isEmpty()) { return {}; }

                m_cslPackages.clear();

                QDir searchPath(rootDirectory, fileFilterCsl());
                QDirIterator it(searchPath, QDirIterator::Subdirectories);
                while (it.hasNext())
                {
                    QString packageFile = it.next();
                    //! \todo KB I would consider exclude dirs here CFileUtils::matchesExcludeDirectory()

                    QString packageFilePath = it.fileInfo().absolutePath();
                    QFile file(packageFile);
                    file.open(QIODevice::ReadOnly);
                    QString content;

                    QTextStream ts(&file);
                    content.append(ts.readAll());
                    file.close();

                    auto package = parsePackageHeader(packageFilePath, content);
                    if (package.hasValidHeader()) m_cslPackages.push_back(package);
                }

                CAircraftModelList installedModels;

                // Now we do a full run
                for (auto &package : m_cslPackages)
                {
                    QString packageFile(package.path);
                    packageFile += "/xsb_aircraft.txt";
                    QFile file(packageFile);
                    file.open(QIODevice::ReadOnly);
                    QString content;

                    QTextStream ts(&file);
                    content.append(ts.readAll());
                    file.close();
                    parseFullPackage(content, package);

                    for (const auto &plane : package.planes)
                    {
                        if (installedModels.containsModelString(plane.getModelName()))
                        {
                            CLogMessage(this).warning("Model %1 exists already! Potential model string conflict! Ignoring it.") << plane.getModelName();
                            continue;
                        }

                        CAircraftModel model(plane.getModelName(), CAircraftModel::TypeOwnSimulatorModel);
                        model.setFileName(plane.filePath);

                        CAircraftIcaoCode icao(plane.icao);
                        model.setAircraftIcaoCode(icao);

                        CLivery livery;
                        livery.setCombinedCode(plane.livery);
                        CAirlineIcaoCode airline(plane.airline);
                        livery.setAirlineIcaoCode(airline);
                        model.setLivery(livery);

                        CDistributor distributor(package.name);
                        model.setDistributor(distributor);

                        model.setSimulatorInfo(m_simulatorInfo);
                        installedModels.push_back(model);
                    }
                }
                return installedModels;
            }

            bool CAircraftModelLoaderXPlane::doPackageSub(QString &ioPath)
            {
                for (auto i = m_cslPackages.begin(); i != m_cslPackages.end(); ++i)
                {
                    if (strncmp(qPrintable(i->name), qPrintable(ioPath), i->name.size()) == 0)
                    {
                        ioPath.remove(0, i->name.size());
                        ioPath.insert(0, i->path);
                        return true;
                    }
                }
                return false;
            }

            bool CAircraftModelLoaderXPlane::parseExportCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                if (tokens.size() != 2)
                {
                    CLogMessage(this).warning("%1 - %2: EXPORT_NAME command requires 1 argument.") << path << lineNum;
                    return false;
                }

                auto p = std::find_if(m_cslPackages.begin(), m_cslPackages.end(), [&tokens](CSLPackage p) { return p.name == tokens[1]; });
                if (p == m_cslPackages.end())
                {
                    package.path = path;
                    package.name = tokens[1];
                    return true;
                }
                else
                {
                    CLogMessage(this).warning("WARNING: Package name %1 already in use by %2 reqested by use by %3") << tokens[1] << p->path << path;
                    return false;
                }
            }

            bool CAircraftModelLoaderXPlane::parseDependencyCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                Q_UNUSED(package);
                if (tokens.size() != 2)
                {
                    CLogMessage(this).warning("%1 - %2: DEPENDENCY command requires 1 argument.") << path << lineNum;
                    return false;
                }

                if (std::count_if(m_cslPackages.begin(), m_cslPackages.end(), [&tokens](CSLPackage p) { return p.name == tokens[1]; }) == 0)
                {
                    CLogMessage(this).warning("WARNING: required package %1 not found. Aborting processing of this package.") << tokens[1];
                    return false;
                }

                return true;
            }

            //! Reads the next line from stream ignoring empty ones.
            //! Returns a null QString if stream is at the end.
            QString readLineFrom(QTextStream &stream)
            {
                QString line;
                do
                {
                    line = stream.readLine();
                }
                while (line.isEmpty() && !stream.atEnd());
                return line;
            }

            bool CAircraftModelLoaderXPlane::parseObjectCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                if (tokens.size() != 2)
                {
                    CLogMessage(this).warning("%1 - %2: OBJECT command requires 1 argument.") << path << lineNum;
                    return false;
                }

                QString relativePath(tokens[1]);
                normalizePath(relativePath);
                QString fullPath(relativePath);
                if (!doPackageSub(fullPath))
                {
                    CLogMessage(this).warning("%1 - %2: package not found.") << path << lineNum;
                    return false;
                }

                // Get obj header
                QFile objFile(fullPath);
                if (!objFile.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    CLogMessage(this).warning("Object %1 does not exist.") << fullPath;
                    return false;
                }
                QTextStream ts(&objFile);

                // First line is about line endings. We don't need it.
                readLineFrom(ts);

                // Version number.
                QString versionLine = readLineFrom(ts);
                if (versionLine.isNull()) { return false; }
                QString version = versionLine.split(QRegularExpression("\\s"), QString::SkipEmptyParts).at(0);

                // For version 7, there is another line 'obj'
                if (version == "700") { readLineFrom(ts); }

                // Texture
                QString textureLine = readLineFrom(ts);
                if (textureLine.isNull()) { return false; }
                QString texture = textureLine.split(QRegularExpression("\\s"), QString::SkipEmptyParts).at(0);

                objFile.close();

                package.planes.push_back(CSLPlane());
                QFileInfo fileInfo(fullPath);

                QStringList dirNames;
                dirNames.append(relativePath.split('/', QString::SkipEmptyParts));
                // Replace the first one being the package name with the package root dir
                QString packageRootDir = package.path.mid(package.path.lastIndexOf('/') + 1);
                dirNames.replace(0, packageRootDir);
                // Remove the last one being the obj itself
                dirNames.removeLast();

                package.planes.back().dirNames = dirNames;
                package.planes.back().objectName = fileInfo.completeBaseName();
                package.planes.back().textureName = texture;
                package.planes.back().filePath = fullPath;
                return true;
            }

            bool CAircraftModelLoaderXPlane::parseTextureCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                if (tokens.size() != 2)
                {
                    CLogMessage(this).warning("%1 - %2: TEXTURE command requires 1 argument.") << path << lineNum;
                    return false;
                }

                // Load regular texture
                QString relativeTexPath = tokens[1];
                normalizePath(relativeTexPath);
                QString absoluteTexPath(relativeTexPath);

                if (!doPackageSub(absoluteTexPath))
                {
                    CLogMessage(this).warning("%1 - %2: package not found.") << path << lineNum;
                    return false;
                }

                QFileInfo fileInfo(absoluteTexPath);
                if (!fileInfo.exists())
                {
                    CLogMessage(this).warning("Texture %1 does not exist.") << absoluteTexPath;
                    return false;
                }

                package.planes.back().textureName = fileInfo.completeBaseName();
                return true;
            }

            bool CAircraftModelLoaderXPlane::parseAircraftCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                Q_UNUSED(package)
                // AIRCAFT <min> <max> <path>
                if (tokens.size() != 4)
                {
                    CLogMessage(this).warning("%1 - %2: AIRCRAFT command requires 3 arguments.") << path << lineNum;
                }

                // Flyable aircrafts are parsed by a different method. We don't know any aircraft files in CSL packages.
                // If there is one, implement this method here.
                CLogMessage(this).warning("Not implemented yet.");
                return true;
            }

            bool CAircraftModelLoaderXPlane::parseObj8AircraftCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                Q_UNUSED(package)
                // OBJ8_AIRCRAFT <path>
                if (tokens.size() != 2)
                {
                    CLogMessage(this).warning("%1 - %2: OBJ8_AIRCARFT command requires 1 argument.") << path << lineNum;
                }

                // RW: I need an example of the file to properly implement and test it.
                CLogMessage(this).warning("Not implemented yet.");
                return false;
            }

            bool CAircraftModelLoaderXPlane::parseObj8Command(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                Q_UNUSED(package)
                // OBJ8 <group> <animate YES|NO> <filename>
                if (tokens.size() != 4)
                {
                    CLogMessage(this).warning("%1 - %2: OBJ8_AIRCARFT command requires 3 arguments.") << path << lineNum;
                }

                // RW: I need an example of the file to properly implement and test it.
                CLogMessage(this).warning("Not implemented yet.");
                return false;
            }

            bool CAircraftModelLoaderXPlane::parseHasGearCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                Q_UNUSED(tokens)
                Q_UNUSED(package)
                Q_UNUSED(path)
                Q_UNUSED(lineNum)
                return true;
            }

            bool CAircraftModelLoaderXPlane::parseIcaoCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                // ICAO <code>
                if (tokens.size() != 2)
                {
                    CLogMessage(this).warning("%1 - %2: ICAO command requires 1 argument.") << path << lineNum;
                    return false;
                }

                QString icao = tokens[1];
                package.planes.back().icao = icao;
                return true;
            }

            bool CAircraftModelLoaderXPlane::parseAirlineCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                // AIRLINE <code> <airline>
                if (tokens.size() != 3)
                {
                    CLogMessage(this).warning("%1 - %2: AIRLINE command requires 2 arguments.") << path << lineNum;
                    return false;
                }

                QString icao = tokens[1];
                package.planes.back().icao = icao;
                QString airline = tokens[2];
                package.planes.back().airline = airline;
                return true;
            }

            bool CAircraftModelLoaderXPlane::parseLiveryCommand(const QStringList &tokens, CSLPackage &package, const QString &path, int lineNum)
            {
                // LIVERY <code> <airline> <livery>
                if (tokens.size() != 4)
                {
                    CLogMessage(this).warning("%1 - %2: LIVERY command requires 3 arguments.") << path << lineNum;
                    return false;
                }

                QString icao = tokens[1];
                package.planes.back().icao = icao;
                QString airline = tokens[2];
                package.planes.back().airline = airline;
                QString livery = tokens[3];
                package.planes.back().livery = livery;
                return true;
            }

            bool CAircraftModelLoaderXPlane::parseDummyCommand(const QStringList & /* tokens */, CSLPackage & /* package */, const QString & /* path */, int /*lineNum*/)
            {
                return true;
            }

            CAircraftModelLoaderXPlane::CSLPackage CAircraftModelLoaderXPlane::parsePackageHeader(const QString &path, const QString &content)
            {
                using command = std::function<bool(const QStringList &, CSLPackage &, const QString &, int)>;
                using namespace std::placeholders;

                const QMap<QString, command> commands
                {
                    { "EXPORT_NAME", std::bind(&CAircraftModelLoaderXPlane::parseExportCommand, this, _1, _2, _3, _4) }
                };

                CSLPackage package;
                int lineNum = 0;

                QString localCopy(content);
                QTextStream in(&localCopy);
                while (!in.atEnd())
                {
                    ++lineNum;
                    QString line = in.readLine();
                    auto tokens = line.split(QRegularExpression("\\s+"));
                    if (!tokens.empty())
                    {
                        auto it = commands.find(tokens[0]);
                        if (it != commands.end())
                        {
                            bool result = it.value()(tokens, package, path, lineNum);
                            // Stop loop once we found EXPORT command
                            if (result) break;
                        }
                    }
                }
                return package;
            }

            void CAircraftModelLoaderXPlane::parseFullPackage(const QString &content, CSLPackage &package)
            {
                using command = std::function<bool(const QStringList &, CSLPackage &, const QString &, int)>;
                using namespace std::placeholders;

                const QMap<QString, command> commands
                {
                    { "EXPORT_NAME", std::bind(&CAircraftModelLoaderXPlane::parseDummyCommand, this, _1, _2, _3, _4) },
                    { "DEPENDENCY", std::bind(&CAircraftModelLoaderXPlane::parseDependencyCommand, this, _1, _2, _3, _4) },
                    { "OBJECT", std::bind(&CAircraftModelLoaderXPlane::parseObjectCommand, this, _1, _2, _3, _4) },
                    { "TEXTURE", std::bind(&CAircraftModelLoaderXPlane::parseTextureCommand, this, _1, _2, _3, _4) },
                    { "AIRCRAFT", std::bind(&CAircraftModelLoaderXPlane::parseAircraftCommand, this, _1, _2, _3, _4) },
                    { "OBJ8_AIRCRAFT", std::bind(&CAircraftModelLoaderXPlane::parseObj8AircraftCommand, this, _1, _2, _3, _4) },
                    { "OBJ8", std::bind(&CAircraftModelLoaderXPlane::parseObj8Command, this, _1, _2, _3, _4) },
                    { "HASGEAR", std::bind(&CAircraftModelLoaderXPlane::parseHasGearCommand, this, _1, _2, _3, _4) },
                    { "ICAO", std::bind(&CAircraftModelLoaderXPlane::parseIcaoCommand, this, _1, _2, _3, _4) },
                    { "AIRLINE", std::bind(&CAircraftModelLoaderXPlane::parseAirlineCommand, this, _1, _2, _3, _4) },
                    { "LIVERY", std::bind(&CAircraftModelLoaderXPlane::parseLiveryCommand, this, _1, _2, _3, _4) },
                };

                int lineNum = 0;

                QString localCopy(content);
                QTextStream in(&localCopy);

                while (!in.atEnd())
                {
                    ++lineNum;
                    QString line = in.readLine();
                    auto tokens = line.split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
                    if (!tokens.empty())
                    {
                        auto it = commands.find(tokens[0]);
                        if (it != commands.end())
                        {
                            bool result = it.value()(tokens, package, package.path, lineNum);
                            if (!result)
                            {
                                CLogMessage(this).warning("Ignoring CSL package %1") << package.name;
                                break;
                            }
                        }
                        else
                        {
                            CLogMessage(this).warning("Unrecognized command %1 in %2") << tokens[0] << package.name;
                            break;
                        }
                    }
                }
            }

            const QString &CAircraftModelLoaderXPlane::fileFilterFlyable()
            {
                static const QString f("*.acf");
                return f;
            }

            const QString &CAircraftModelLoaderXPlane::fileFilterCsl()
            {
                static const QString f("xsb_aircraft.txt");
                return f;
            }

        } // namespace
    } // namespace
} // namespace
