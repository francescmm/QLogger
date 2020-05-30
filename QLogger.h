#ifndef QLOGGER_H
#define QLOGGER_H

/****************************************************************************************
 ** QLogger is a library to register and print logs into a file.
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This lbirary is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
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

/**************************************************************************************************/
/***                                     GENERAL USAGE                                          ***/
/***                                                                                            ***/
/***  1. Create an instance: QLoggerManager *manager = QLoggerManager::getInstance();           ***/
/***  2. Add a destination:  manager->addDestination(filePathName, module, logLevel);           ***/
/***  3. Print the log in the file with: QLog_<Trace/Debug/Info/Warning/Error/Fatal>            ***/
/***                                                                                            ***/
/***  You can add as much destinations as you want. You also can add several modules for each   ***/
/***  log file.                                                                                 ***/
/***                                                                                            ***/
/**************************************************************************************************/

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>

namespace QLogger
{

/**
 * @brief The LogLevel enum class defines the level of the log message.
 */
enum class LogLevel
{
   Trace = 0,
   Debug,
   Info,
   Warning,
   Error,
   Fatal
};

/**
 * @brief The QLoggerWriter class writes the message and manages the file where it is printed.
 */
class QLoggerWriter : public QThread
{
   Q_OBJECT

public:
   /**
    * @brief Constructor that gets the complete path and filename to create the file. It also
    * configures the level of this file to filter the logs depending on the level.
    * @param fileDestination The complete path.
    * @param level The maximum level that is allowed.
    */
   explicit QLoggerWriter(const QString &fileDestination, LogLevel level);

   /**
    * @brief Gets the current level threshold.
    * @return The level.
    */
   LogLevel getLevel() const { return mLevel; }

   /**
    * @brief setLogLevel Sets the log level for this destination
    * @param level The new level threshold.
    */
   void setLogLevel(LogLevel level) { mLevel = level; }

   /**
    * @brief Writes a message in a file. If the file is full, it truncates it and prints a first line with the
    * information of the old file.
    *
    * @param message Pair of values consistent on the date and the message to be log.
    */
   void write(const QPair<QString, QString> &message);

   /**
    * @brief enqueue Enqueues a message to be written in the destiantion.
    * @param date The date and time of the log message.
    * @param threadId The thread where the message comes from.
    * @param module The module that writes the message.
    * @param level The log level of the message.
    * @param fileName The file name that prints the log.
    * @param line The line of the file name that prints the log.
    * @param message The message to log.
    */
   void enqueue(const QDateTime &date, const QString &threadId, const QString &module, LogLevel level,
                const QString fileName, int line, const QString &message);

   /**
    * @brief Stops the log writer
    * @param stop True to be stop, otherwise false
    */
   void stop(bool stop) { mIsStop = stop; }

   /**
    * @brief Returns if the log writer is stop from writing.
    * @return True if is stop, otherwise false
    */
   bool isStop() const { return mIsStop; }

   void run() override;

   void closeDestination();

private:
   bool quit = false;
   QWaitCondition queueNotEmpty;
   /**
    * @brief Path and name of the file that will store the logs.
    */
   QString mFileDestination;
   /**
    * @brief Maximum log level allowed for the file.
    */
   LogLevel mLevel;
   /**
    * @brief Defines if the QLogWriter is currently stop and doesn't write to file
    */
   bool mIsStop = false;

   QString renameFileIfFull();
   QVector<QPair<QString, QString>> messages;
   QMutex mutex;
};

/**
 * @brief The QLoggerManager class manages the different destination files that we would like to have.
 */
class QLoggerManager : public QObject
{
public:
   /**
    * @brief Gets an instance to the QLoggerManager.
    * @return A pointer to the instance.
    */
   static QLoggerManager *getInstance();

   /**
    * @brief This method creates a QLoogerWriter that stores the name of the file and the log
    * level assigned to it. Here is added to the map the different modules assigned to each
    * log file. The method returns <em>false</em> if a module is configured to be stored in
    * more than one file.
    *
    * @param fileDest The file name and path to print logs.
    * @param module The module that will be stored in the file.
    * @param level The maximum level allowed.
    * @return Returns true if any error have been done.
    */
   bool addDestination(const QString &fileDest, const QString &module, LogLevel level);
   /**
    * @brief This method creates a QLoogerWriter that stores the name of the file and the log
    * level assigned to it. Here is added to the map the different modules assigned to each
    * log file. The method returns <em>false</em> if a module is configured to be stored in
    * more than one file.
    *
    * @param fileDest The file name and path to print logs.
    * @param modules The modules that will be stored in the file.
    * @param level The maximum level allowed.
    * @return Returns true if any error have been done.
    */
   bool addDestination(const QString &fileDest, const QStringList &modules, LogLevel level);

   /**
    * @brief enqueueMessage Enqueues a message in the corresponding QLoggerWritter.
    * @param module The module that writes the message.
    * @param level The level of the message.
    * @param message The message to log.
    * @param file The file that logs.
    * @param line The line in the file where the log comes from.
    */
   void enqueueMessage(const QString &module, LogLevel level, const QString &message, QString file, int line);

   /**
    * @brief pause Pauses all QLoggerWriters.
    */
   void pause();

   /**
    * @brief resume Resumes all QLoggerWriters that where paused.
    */
   void resume();

   /**
    * @brief overwriteLogLevel Overwrites the log level in all the destinations
    *
    * @param level The new log level
    */
   void overwriteLogLevel(LogLevel level);

private:
   /**
    * @brief Instance of the class.
    */
   static QLoggerManager *INSTANCE;

   /**
    * @brief Checks if the logger is stop
    */
   static bool mIsStop;

   /**
    * @brief Map that stores the module and the file it is assigned.
    */
   QMap<QString, QLoggerWriter *> moduleDest;

   /**
    * @brief Defines the queue of messages when no writters have been set yet.
    */
   QMultiMap<QString, QVector<QVariant>> mNonWriterQueue;

   /**
    * @brief Mutex to make the method thread-safe.
    */
   QMutex mutex;

   /**
    * @brief Default builder of the class. It starts the thread.
    */
   QLoggerManager();

   /**
    * @brief Destructor
    */
   ~QLoggerManager();

   /**
    * @brief Checks the queue and writes the messages if the writer is the correct one. The queue is emptied
    * for that module.
    * @param module The module to dequeue the messages from
    */
   void writeAndDequeueMessages(const QString &module);
};

/**
 * @brief Here is done the call to write the message in the module. First of all is confirmed
 * that the log level we want to write is less or equal to the level defined when we create the
 * destination.
 *
 * @param module The module that the message references.
 * @param level The level of the message.
 * @param message The message.
 */
void QLog_(const QString &module, LogLevel level, const QString &message, const QString &file = QString(),
           int line = -1);

#ifndef QLog_Trace
/**
 * @brief Used to store Trace level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Trace(module, message)                                                                                 \
      QLoggerManager::getInstance()->enqueueMessage(module, LogLevel::Trace, message, __FILE__, __LINE__)
#endif

#ifndef QLog_Debug
/**
 * @brief Used to store Debug level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Debug(module, message)                                                                                 \
      QLoggerManager::getInstance()->enqueueMessage(module, LogLevel::Debug, message, __FILE__, __LINE__)
#endif

#ifndef QLog_Info
/**
 * @brief Used to store Info level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Info(module, message)                                                                                  \
      QLoggerManager::getInstance()->enqueueMessage(module, LogLevel::Info, message, __FILE__, __LINE__)
#endif

#ifndef QLog_Warning
/**
 * @brief Used to store Warning level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Warning(module, message)                                                                               \
      QLoggerManager::getInstance()->enqueueMessage(module, LogLevel::Warning, message, __FILE__, __LINE__)
#endif

#ifndef QLog_Error
/**
 * @brief Used to store Error level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Error(module, message)                                                                                 \
      QLoggerManager::getInstance()->enqueueMessage(module, LogLevel::Error, message, __FILE__, __LINE__)
#endif

#ifndef QLog_Fatal
/**
 * @brief Used to store Fatal level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
#   define QLog_Fatal(module, message)                                                                                 \
      QLoggerManager::getInstance()->enqueueMessage(module, LogLevel::Fatal, message, __FILE__, __LINE__)
#endif

}

#endif // QLOGGER_H
