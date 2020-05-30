#include "QLoggerWriter.h"

#include <QDateTime>
#include <QFile>
#include <QTextStream>

namespace
{
QString levelToText(const QLogger::LogLevel &level)
{
   switch (level)
   {
      case QLogger::LogLevel::Trace:
         return "Trace";
      case QLogger::LogLevel::Debug:
         return "Debug";
      case QLogger::LogLevel::Info:
         return "Info";
      case QLogger::LogLevel::Warning:
         return "Warning";
      case QLogger::LogLevel::Error:
         return "Error";
      case QLogger::LogLevel::Fatal:
         return "Fatal";
   }

   return QString();
}
}

namespace QLogger
{

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
                            const QString &fileName, int line, const QString &message)
{
   QString fileLine;

   if (!fileName.isEmpty() && line > 0 && mLevel <= LogLevel::Debug)
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
