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
/**************************************************************************************************/

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QStringList>

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
 * @brief Here is done the call to write the message in the module. First of all is confirmed
 * that the log level we want to write is less or equal to the level defined when we create the
 * destination.
 *
 * @param module The module that the message references.
 * @param level The level of the message.
 * @param message The message.
 */
void QLog_(const QString &module, LogLevel level, const QString &message);
/**
 * @brief Used to store Trace level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void QLog_Trace(const QString &module, const QString &message);
/**
 * @brief Used to store Debug level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void QLog_Debug(const QString &module, const QString &message);
/**
 * @brief Used to store Info level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void QLog_Info(const QString &module, const QString &message);
/**
 * @brief Used to store Warning level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void QLog_Warning(const QString &module, const QString &message);
/**
 * @brief Used to store Error level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void QLog_Error(const QString &module, const QString &message);
/**
 * @brief Used to store Fatal level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void QLog_Fatal(const QString &module, const QString &message);

/**
 * @brief The QLoggerWriter class writes the message and manages the file where it is printed.
 */
class QLoggerWriter : public QObject
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
    * @brief Gets the current maximum level.
    * @return The LogLevel.
    */
   LogLevel getLevel() const { return mLevel; }

   /**
    * @brief setLogLevel Sets the log level for this destination
    * @param level The max level to log messages
    */
   void setLogLevel(LogLevel level) { mLevel = level; }

   /**
    * @brief Within this method the message is written in the log file. If it would exceed
    * from 1 MByte, another file will be created and the log message will be stored in the
    * new one. The older file will be renamed with the date and time of this message to know
    * where it is updated.
    *
    * @param module The module that corresponds to the message.
    * @param message The message log.
    * @param messageLogLevel The log level of the message
    */
   void write(const QString &module, const QString &message, const LogLevel &messageLogLevel);

   /**
    * @brief Overload function of \r write that writes with date and time.
    *
    * @param module The module that corresponds to the message.
    * @param message The message log.
    * @param messageLogLevel The log level of the message
    * @param dt The date and time in string format
    */
   void write(const QString &module, const QString &message, const LogLevel &messageLogLevel, const QString &dt);

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

private:
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
};

/**
 * @brief The QLoggerManager class manages the different destination files that we would like to have.
 */
class QLoggerManager
{
public:
   /**
    * @brief Gets an instance to the QLoggerManager.
    * @return A pointer to the instance.
    */
   static QLoggerManager *getInstance();
   /**
    * @brief Converts the given level in a QString.
    * @param level The log level in LogLevel format.
    * @return The string with the name of the log level.
    */
   static QString levelToText(const LogLevel &level);
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
    * @brief Gets the QLoggerWriter instance corresponding to the module <em>module</em>.
    * @param module The module we look for.
    * @return Retrns a pointer to the object.
    */
   QLoggerWriter *getLogWriter(const QString &module) { return moduleDest.value(module); }

   /**
    * @brief This method closes the logger and the thread it represents.
    */
   void closeLogger();

   /**
    * @brief Queues a message while there is no writer set. Once a writer is added.
    * @param futureWriter The guessed writer where the message should be printed
    * @param logData The data to log in the form of: message, level and date & time.
    */
   void queueMessage(const QString module, const QVector<QVariant> &logData);

   /**
    * @brief Checks the queue and writes the messages if the writer is the correct one. The queue is emptied
    * for that module.
    * @param module The module to dequeue the messages from
    */
   void writeAndDequeueMessages(const QString &module);

   void pause();

   /**
    * @brief resumeQLogger Resumes all QLogWriters that where stop
    */
   void resume();

   /**
    * @brief overwriteLogLevel Overwrites the log level in all the destinations
    *
    * @param level The new log level
    */
   void overwriteLogLevel(LogLevel level);

   /**
    * @brief Mutex to make the method thread-safe.
    */
   QMutex mutex;

private:
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
    * @brief Default builder of the class. It starts the thread.
    */
   QLoggerManager();
};
}

#endif // QLOGGER_H
