#include "QLogger.h"

#include "QLoggerWriter.h"

#include <QDateTime>
#include <QDir>

Q_DECLARE_METATYPE(QLogger::LogLevel);
Q_DECLARE_METATYPE(QLogger::LogMode);
Q_DECLARE_METATYPE(QLogger::LogFileDisplay);
Q_DECLARE_METATYPE(QLogger::LogMessageDisplay);

namespace QLogger
{

void QLog_(const QString &module, LogLevel level, const QString &message,
           const QString &function, const QString &file, int line)
{
   QLoggerManager::getInstance()->enqueueMessage(module, level, message, function, file, line);
}

static const int QUEUE_LIMIT = 100;

QLoggerManager::QLoggerManager()
   : mMutex(QMutex::Recursive)
{

}

QLoggerManager *QLoggerManager::getInstance()
{
   static QLoggerManager INSTANCE;

   return &INSTANCE;
}

bool QLoggerManager::addDestination(const QString &fileDest, const QString &module, LogLevel level,
                                    const QString &fileFolderDestination, LogMode mode,
                                    LogFileDisplay fileSuffixIfFull, LogMessageDisplays messageOptions, bool notify)
{
   QMutexLocker lock(&mMutex);

   if (!mModuleDest.contains(module))
   {
       const auto log = new QLoggerWriter((fileDest.isEmpty() ? mDefaultFileDestination : fileDest),
                                          (level == LogLevel::Warning ? mDefaultLevel : level),
                                          (fileFolderDestination.isEmpty() ? mDefaultFileDestinationFolder : QDir::fromNativeSeparators(fileFolderDestination)),
                                          (mode == LogMode::OnlyFile ? mDefaultMode : mode),
                                          (fileSuffixIfFull == LogFileDisplay::DateTime ? mDefaultFileSuffixIfFull : fileSuffixIfFull),
                                          (messageOptions.testFlag(LogMessageDisplay::Default) ? mDefaultMessageOptions : messageOptions));
      log->setMaxFileSize(mDefaultMaxFileSize);
      log->stop(mIsStop);

      if (notify) {
        const auto threadId = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));
        log->enqueue(QDateTime::currentDateTime(), threadId, module, LogLevel::Info, "", "", -1, "Adding destination!");
      }
      mModuleDest.insert(module, log);

      if (mode != LogMode::Disabled)
        log->start();

      return true;
   }

   return false;
}

bool QLoggerManager::addDestination(const QString &fileDest, const QStringList &modules, LogLevel level,
                                    const QString &fileFolderDestination, LogMode mode,
                                    LogFileDisplay fileSuffixIfFull, LogMessageDisplays messageOptions, bool notify)
{
   QMutexLocker lock(&mMutex);
   bool allAdded = false;

   for (const auto &module : modules)
   {
      if (!mModuleDest.contains(module))
      {
          const auto log = new QLoggerWriter((fileDest.isEmpty() ? mDefaultFileDestination : fileDest),
                                             (level == LogLevel::Warning ? mDefaultLevel : level),
                                             (fileFolderDestination.isEmpty() ? mDefaultFileDestinationFolder : QDir::fromNativeSeparators(fileFolderDestination)),
                                             (mode == LogMode::OnlyFile ? mDefaultMode : mode),
                                             (fileSuffixIfFull == LogFileDisplay::DateTime ? mDefaultFileSuffixIfFull : fileSuffixIfFull),
                                             (messageOptions.testFlag(LogMessageDisplay::Default) ? mDefaultMessageOptions : messageOptions));
         log->setMaxFileSize(mDefaultMaxFileSize);
         log->stop(mIsStop);

         mModuleDest.insert(module, log);

         if (!mIsStop)
         {
            if (notify) {
                const auto threadId
                        = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));
                log->enqueue(QDateTime::currentDateTime(), threadId, module, LogLevel::Info, "", "", -1, "Adding destination!");
            }
            if (mode != LogMode::Disabled)
              log->start();
         }

         allAdded = true;
      }
   }

   return allAdded;
}

void QLoggerManager::clearFileDestinationFolder(const QString &fileFolderDestination, int days)
{
    QDir dir(fileFolderDestination + QStringLiteral("/logs"));
    if (!dir.exists()) {
        return;
    }

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    const QFileInfoList list = dir.entryInfoList();

    const QDateTime now = QDateTime::currentDateTime();

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);

        if (fileInfo.lastModified().daysTo(now) >= days) {
            //remove file
            dir.remove(fileInfo.fileName());
        }
    }
}

void QLoggerManager::setDefaultFileDestinationFolder(const QString &fileDestinationFolder)
{
    mDefaultFileDestinationFolder = QDir::fromNativeSeparators(fileDestinationFolder);
}

void QLoggerManager::writeAndDequeueMessages(const QString &module)
{
   QMutexLocker lock(&mMutex);

   const auto logWriter = mModuleDest.value(module, Q_NULLPTR);

   if (logWriter && !logWriter->isStop())
   {
      const QList<QVector<QVariant>> values = mNonWriterQueue.values(module);
      for (const auto &values : values)
      {
         const auto level = qvariant_cast<LogLevel>(values.at(2).toInt());

         if (logWriter->getLevel() <= level)
         {
            const auto datetime = values.at(0).toDateTime();
            const auto threadId = values.at(1).toString();
            const auto function = values.at(3).toString();
            const auto file = values.at(4).toString();
            const auto line = values.at(5).toInt();
            const auto message = values.at(6).toString();

            logWriter->enqueue(datetime, threadId, module, level, function, file, line, message);
         }
      }

      mNonWriterQueue.remove(module);
   }
}

void QLoggerManager::enqueueMessage(const QString &module, LogLevel level, const QString &message, const QString& function,
                                    const QString& file, int line)
{
   QMutexLocker lock(&mMutex);
   const auto threadId = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));
   const auto fileName = file.mid(file.lastIndexOf('/') + 1);
   const auto logWriter = mModuleDest.value(module, Q_NULLPTR);

   if (logWriter && logWriter->getMode() != LogMode::Disabled && !logWriter->isStop() && logWriter->getLevel() <= level)
   {
      writeAndDequeueMessages(module);

      logWriter->enqueue(QDateTime::currentDateTime(), threadId, module, level, function, fileName, line, message);
   }
   else if (!logWriter && mNonWriterQueue.count(module) < QUEUE_LIMIT)
      mNonWriterQueue.insert(
          module,
          { QDateTime::currentDateTime(), threadId, QVariant::fromValue<LogLevel>(level), function, fileName, line, message });
}

void QLoggerManager::pause()
{
   QMutexLocker lock(&mMutex);

   mIsStop = true;

   for (auto &logWriter : mModuleDest)
      logWriter->stop(mIsStop);
}

void QLoggerManager::resume()
{
   QMutexLocker lock(&mMutex);

   mIsStop = false;

   for (auto &logWriter : mModuleDest)
       logWriter->stop(mIsStop);
}

void QLoggerManager::overwriteLogMode(LogMode mode)
{
    QMutexLocker lock(&mMutex);

    this->setDefaultMode(mode);

    for (auto &logWriter : mModuleDest)
        logWriter->setLogMode(mode);
}

void QLoggerManager::overwriteLogLevel(LogLevel level)
{
   QMutexLocker lock(&mMutex);

   this->setDefaultLevel(level);

   for (auto &logWriter : mModuleDest)
       logWriter->setLogLevel(level);
}

void QLoggerManager::overwriteMaxFileSize(int maxSize)
{
    QMutexLocker lock(&mMutex);

    this->setDefaultMaxFileSize(maxSize);

    for (auto &logWriter : mModuleDest)
        logWriter->setMaxFileSize(maxSize);
}

QLoggerManager::~QLoggerManager()
{
   QMutexLocker locker(&mMutex);

   for (const auto &dest : mModuleDest.toStdMap())
      writeAndDequeueMessages(dest.first);

   for (auto dest : qAsConst(mModuleDest))
   {
      dest->closeDestination();
      dest->deleteLater();
   }

   mModuleDest.clear();
}

}
