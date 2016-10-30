/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/atomicfile.h"
#include "blackmisc/algorithm.h"

#include <QDir>
#include <QFileInfo>
#include <QFlags>
#include <random>
#include <type_traits>

#if defined(Q_OS_POSIX)
#include <stdio.h>
#include <errno.h>
#elif defined(Q_OS_WIN32)
#include <windows.h>
#endif

//! \var qt_ntfs_permission_lookup
//! \see QFileDevice::Permissions
#ifdef Q_OS_WIN
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#else
int qt_ntfs_permission_lookup = 0;
#endif

namespace BlackMisc
{

    //! \private
    bool checkPermissions(CAtomicFile::OpenMode mode, const QFileInfo &fileInfo)
    {
        bool ok = true;
        qt_ntfs_permission_lookup++;
        if ((mode & CAtomicFile::ReadOnly) && ! fileInfo.isReadable()) { ok = false; }
        if ((mode & CAtomicFile::WriteOnly) && ! fileInfo.isWritable()) { ok = false; }
        qt_ntfs_permission_lookup--;
        return ok;
    }

    bool CAtomicFile::open(CAtomicFile::OpenMode mode)
    {
        m_originalFilename = fileName();
        QFileInfo fileInfo(fileName());
        if (exists() && ! checkPermissions(mode, fileInfo))
        {
            m_permissionError = true;
            setErrorString("Wrong permissions");
            return false;
        }
        setFileName(QFileInfo(fileInfo.dir(), ".tmp." + fileInfo.fileName() + "." + randomSuffix()).filePath());
        if (exists()) { remove(); }

        bool ok = true;
        if (mode & ReadOnly)
        {
            if (exists(m_originalFilename) && ! copy(m_originalFilename, fileName())) { ok = false; }
        }

        if (ok && ! QFile::open(mode)) { ok = false; }
        if (! ok) { setFileName(m_originalFilename); }
        return ok;
    }

    void CAtomicFile::close()
    {
        if (! isOpen()) { return; }

        QFile::close();

        if (error() == NoError) { replaceOriginal(); }
        setFileName(m_originalFilename);
    }

    bool CAtomicFile::checkedClose()
    {
        close();
        return error() == NoError;
    }

    void CAtomicFile::abandon()
    {
        if (! isOpen()) { return; }

        QFile::close();
        remove();
        setFileName(m_originalFilename);
    }

    CAtomicFile::FileError CAtomicFile::error() const
    {
        if (m_renameError) { return RenameError; }
        if (m_permissionError) { return PermissionsError; }
        return QFile::error();
    }

    void CAtomicFile::unsetError()
    {
        m_renameError = false;
        m_permissionError = false;
        QFile::unsetError();
    }

    QString CAtomicFile::randomSuffix()
    {
        constexpr auto max = 2176782335;
        return QString("%1").arg(std::uniform_int_distribution<std::decay_t<decltype(max)>>(0, max)(Private::defaultRandomGenerator()), 6, 36, QChar('0'));
    }

#if defined(Q_OS_POSIX)
    void CAtomicFile::replaceOriginal()
    {
        auto result = ::rename(qPrintable(fileName()), qPrintable(m_originalFilename));
        if (result < 0)
        {
            m_renameError = true;
            char s[1024] {};
            auto x = strerror_r(errno, s, sizeof(s));
            setErrorString(QString::fromLocal8Bit(s));
            static_assert(std::is_same<decltype(x), int>::value, "Non-standard signature of POSIX function strerror_r, check documentation.");
        }
    }
#elif defined(Q_OS_WIN32)
    void CAtomicFile::replaceOriginal()
    {
        auto encode = [](const QString &s)
        {
            const auto prefix = "\\\\?\\"; // support long paths: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247.aspx#maxpath
            return (prefix + QDir::toNativeSeparators(QDir::cleanPath(QFileInfo(s).absoluteFilePath()))).toStdWString();
        };
        auto result = MoveFileEx(encode(fileName()).c_str(), encode(m_originalFilename).c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        if (! result)
        {
            m_renameError = true;
            wchar_t *s = nullptr;
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(), 0, reinterpret_cast<LPWSTR>(&s), 0, nullptr);
            setErrorString(QString::fromWCharArray(s).simplified());
            LocalFree(reinterpret_cast<HLOCAL>(s));
        }
    }
#else
    void CAtomicFile::replaceOriginal()
    {
        if (exists(m_originalFilename)) { remove(m_originalFilename); }
        rename(m_originalFilename);
    }
#endif

}
