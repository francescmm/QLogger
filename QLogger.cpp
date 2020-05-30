#include "QLogger.h"

#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QVector>

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

Q_DECLARE_METATYPE(QLogger::LogLevel);

namespace QLogger
{

void QLog_(const QString &module, LogLevel level, const QString &message, const QString &file, int line)
{
   QLoggerManager::getInstance()->enqueueMessage(module, level, message, file, line);
}

static const int QUEUE_LIMIT = 100;

// QLoggerManager
QLoggerManager *QLoggerManager::INSTANCE = nullptr;
bool QLoggerManager::mIsStop = false;

QLoggerManager::QLoggerManager()
   : mutex(QMutex::Recursive)
{
   QDir dir(QDir::currentPath());

   if (!dir.exists("logs"))
      dir.mkdir("logs");
}

QLoggerManager *QLoggerManager::getInstance()
{
   static QLoggerManager instance;

   return &instance;
}

static QString levelToText(const LogLevel &level)
{
   switch (level)
   {
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

bool QLoggerManager::addDestination(const QString &fileDest, const QString &module, LogLevel level)
{
   QMutexLocker lock(&mutex);

   if (!moduleDest.contains(module))
   {
      const auto log = new QLoggerWriter(fileDest, level);
      log->stop(mIsStop);

      const auto threadId = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));

      log->enqueue(QDateTime::currentDateTime(), threadId, module, LogLevel::Info, "", -1, "Adding destination!");

      moduleDest.insert(module, log);

      log->start();

      return true;
   }

   return false;
}

bool QLoggerManager::addDestination(const QString &fileDest, const QStringList &modules, LogLevel level)
{
   QMutexLocker lock(&mutex);
   bool allAdded = false;

   for (const auto &module : modules)
   {
      if (!moduleDest.contains(module))
      {
         const auto log = new QLoggerWriter(fileDest, level);
         log->stop(mIsStop);

         const auto threadId
             = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));

         log->enqueue(QDateTime::currentDateTime(), threadId, module, LogLevel::Info, "", -1, "Adding destination!");

         moduleDest.insert(module, log);

         log->start();

         allAdded = true;
      }
   }

   return allAdded;
}

void QLoggerManager::writeAndDequeueMessages(const QString &module)
{
   QMutexLocker lock(&mutex);

   auto element = mNonWriterQueue.find(module);
   const auto queueEnd = mNonWriterQueue.end();
   const auto logWriter = moduleDest.value(module);

   if (element != queueEnd && logWriter && !logWriter->isStop())
   {
      const auto module = element.key();

      for (; element != queueEnd; ++element)
      {
         const auto values = element.value();
         const auto level = qvariant_cast<LogLevel>(element.value().at(2).toInt());

         if (logWriter->getLevel() <= level)
         {
            const auto datetime = values.at(0).toDateTime();
            const auto threadId = values.at(1).toString();
            const auto file = values.at(3).toString();
            const auto line = values.at(4).toInt();
            const auto message = element.value().at(5).toString();

            logWriter->enqueue(datetime, threadId, module, level, file, line, message);
         }
      }

      mNonWriterQueue.remove(module);
   }
}

void QLoggerManager::enqueueMessage(const QString &module, LogLevel level, const QString &message, QString file,
                                    int line)
{
   QMutexLocker lock(&mutex);
   const auto threadId = QString("%1").arg((quintptr)QThread::currentThread(), QT_POINTER_SIZE * 2, 16, QChar('0'));
   const auto fileName = file.mid(file.lastIndexOf('/') + 1);
   const auto logWriter = moduleDest.value(module);

   if (logWriter && !logWriter->isStop() && logWriter->getLevel() <= level)
   {
      writeAndDequeueMessages(module);

      logWriter->enqueue(QDateTime::currentDateTime(), threadId, module, level, fileName, line, message);
   }
   else if (!logWriter && mNonWriterQueue.count(module) < QUEUE_LIMIT)
      mNonWriterQueue.insert(
          module,
          { QDateTime::currentDateTime(), threadId, QVariant::fromValue<LogLevel>(level), fileName, line, message });
}

void QLoggerManager::pause()
{
   mIsStop = true;

   for (auto &logWriter : moduleDest)
      logWriter->stop(mIsStop);
}

void QLoggerManager::resume()
{
   mIsStop = false;

   for (auto &logWriter : moduleDest)
      logWriter->stop(mIsStop);
}

void QLoggerManager::overwriteLogLevel(LogLevel level)
{
   for (auto &logWriter : moduleDest)
      logWriter->setLogLevel(level);
}

QLoggerManager::~QLoggerManager()
{
   QMutexLocker locker(&mutex);

   for (const auto &dest : moduleDest.toStdMap())
      writeAndDequeueMessages(dest.first);

   for (auto dest : moduleDest)
   {
      dest->closeDestination();
      delete dest;
   }

   moduleDest.clear();
}

QLoggerWriter::QLoggerWriter(const QString &fileDestination, LogLevel level)
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
   if (file.size() >= MAX_SIZE)
   {
      const auto currentTime = QDateTime::currentDateTime();
      newName = newName.arg(currentTime.date().toString("dd_MM_yy"), currentTime.time().toString("hh_mm_ss"));
      renamed = file.rename(mFileDestination, newName);
   }

   return renamed ? newName : QString();
}

void QLoggerWriter::write(const QPair<QString, QString> &message)
{
   QFile file(mFileDestination);

   const auto newName = renameFileIfFull();

   if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append))
   {
      QTextStream out(&file);

      if (!newName.isEmpty())
         out << QString("%1 - Previous log %2\n").arg(message.first, newName);

      out << message.second;

      file.close();
   }
}

void QLoggerWriter::enqueue(const QDateTime &date, const QString &threadId, const QString &module, LogLevel level,
                            const QString fileName, int line, const QString &message)
{
   QString fileLine;

   if (!fileName.isEmpty() && line > 0)
      fileLine = QString(" {%1:%2}").arg(fileName, QString::number(line));

   const auto text
       = QString("[%1] [%2] [%3] [%4]%5 %6 \n")
             .arg(levelToText(level), module, date.toString("dd-MM-yyyy hh:mm:ss.zzz"), threadId, fileLine, message);

   QMutexLocker locker(&mutex);
   messages.append({ threadId, text });

   queueNotEmpty.wakeOne();
}

void QLoggerWriter::run()
{
   while (!quit)
   {
      mutex.lock();
      decltype(messages) copy;
      std::swap(copy, messages);
      mutex.unlock();

      for (const auto &msg : copy)
         write(msg);

      copy.clear();

      mutex.lock();
      queueNotEmpty.wait(&mutex);
      mutex.unlock();
   }
}

void QLoggerWriter::closeDestination()
{
   quit = true;
   queueNotEmpty.wakeOne();

   exit(0);
   wait();
}
}
