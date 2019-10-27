#include "QLogger.h"
#include <QDateTime>
#include <QDir>
#include <QTextStream>

/****************************************************************************************
 ** QLogger is a library to register and print logs into a file.
 ** Copyright (C) 2018  Francesc Martinez <es.linkedin.com/in/cescmm/en>
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2.1 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

namespace QLogger {
void QLog_Trace(const QString& module, const QString& message)
{
    QLog_(module, LogLevel::Trace, message);
}

void QLog_Debug(const QString& module, const QString& message)
{
    QLog_(module, LogLevel::Debug, message);
}

void QLog_Info(const QString& module, const QString& message)
{
    QLog_(module, LogLevel::Info, message);
}

void QLog_Warning(const QString& module, const QString& message)
{
    QLog_(module, LogLevel::Warning, message);
}

void QLog_Error(const QString& module, const QString& message)
{
    QLog_(module, LogLevel::Error, message);
}

void QLog_Fatal(const QString& module, const QString& message)
{
    QLog_(module, LogLevel::Fatal, message);
}

void QLog_(const QString& module, LogLevel level, const QString& message)
{
    const auto manager = QLoggerManager::getInstance();

    QMutexLocker(&manager->mutex);

    const auto logWriter = manager->getLogWriter(module);

    if (logWriter and !logWriter->isStop() and logWriter->getLevel() <= level)
        logWriter->write(module, message, level);
}

// QLoggerManager
QLoggerManager* QLoggerManager::INSTANCE = nullptr;
bool QLoggerManager::mIsStop = false;

QLoggerManager::QLoggerManager()
    : QThread()
    , mutex(QMutex::Recursive)
{
    start();

    QDir dir(QDir::currentPath());

    if (!dir.exists("logs"))
        dir.mkdir("logs");
}

QLoggerManager* QLoggerManager::getInstance()
{
    if (!INSTANCE)
        INSTANCE = new QLoggerManager();

    return INSTANCE;
}

QString QLoggerManager::levelToText(const LogLevel& level)
{
    switch (level) {
    case LogLevel::Trace:
        return "Trace";
    case LogLevel::Debug:
        return "Debug";
    case LogLevel::Info:
        return "Info";
    case LogLevel::Warning:
        return "Warning";
    case LogLevel::Error:
        return "Error";
    case LogLevel::Fatal:
        return "Fatal";
    }

    return QString();
}

bool QLoggerManager::addDestination(const QString& fileDest, const QString& module, LogLevel level)
{
    if (!moduleDest.contains(module)) {
        const auto log = new QLoggerWriter(fileDest, level);
        log->stop(mIsStop);

        moduleDest.insert(module, log);

        return true;
    }

    return false;
}

bool QLoggerManager::addDestination(const QString& fileDest, const QStringList& modules, LogLevel level)
{
    bool allAdded = false;

    for (const auto& module : modules) {
        if (!moduleDest.contains(module)) {
            const auto log = new QLoggerWriter(fileDest, level);
            log->stop(mIsStop);

            moduleDest.insert(module, log);
            allAdded = true;
        }
    }

    return allAdded;
}

void QLoggerManager::closeLogger()
{
    deleteLater();
    exit(0);
}

void QLoggerManager::pause()
{
    mIsStop = true;

    for (auto logWriter : moduleDest)
        logWriter->stop(mIsStop);
}

void QLoggerManager::resume()
{
    mIsStop = false;

    for (auto logWriter : moduleDest)
        logWriter->stop(mIsStop);
}

void QLoggerManager::overwriteLogLevel(LogLevel level)
{
    for (auto logWriter : moduleDest)
        logWriter->setLogLevel(level);
}

QLoggerWriter::QLoggerWriter(const QString& fileDestination, LogLevel level)
{
    mFileDestination = "logs/" + fileDestination;
    mLevel = level;
}

QString QLoggerWriter::renameFileIfFull()
{
    const auto MAX_SIZE = 1024 * 1024;
    const auto toRemove = mFileDestination.section('.', -1);
    const auto fileNameAux = mFileDestination.left(mFileDestination.size() - toRemove.size() - 1);
    auto renamed = false;
    auto newName = QString("%1%2").arg(fileNameAux, "_%1__%2.log");

    QFile file(mFileDestination);

    // Rename file if it's full
    if (file.size() >= MAX_SIZE) {
        const auto currentTime = QDateTime::currentDateTime();
        newName = newName.arg(currentTime.date().toString("dd_MM_yy"), currentTime.time().toString("hh_mm_ss"));
        renamed = file.rename(mFileDestination, newName);
    }

    return renamed ? newName : QString();
}

void QLoggerWriter::write(const QString& module, const QString& message, const LogLevel& messageLogLevel)
{
    QFile file(mFileDestination);

    const auto newName = renameFileIfFull();

    if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
        QTextStream out(&file);
        const auto dtFormat = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss.zzz");

        if (!newName.isEmpty())
            out << QString("%1 - Previous log %2\n").arg(dtFormat).arg(newName);

        const auto logLevel = QLoggerManager::levelToText(messageLogLevel);
        const auto text = QString("[%1] [%2] {%3} %4\n").arg(dtFormat).arg(logLevel).arg(module).arg(message);

        out << text;
        file.close();
    }
}
}
