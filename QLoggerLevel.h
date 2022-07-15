#pragma once

/****************************************************************************************
 ** QLogger is a library to register and print logs into a file.
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
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

#include <QFlag>


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
 * @brief The LogMode enum class defines the way to display the log message.
 */
enum class LogMode
{
    Disabled = 0,
    OnlyConsole,
    OnlyFile,
    Full,
    Default
};

/**
 * @brief The LogFileTag enum class defines the additional tag for the log file name.
 */
enum class LogFileTag
{
    DateTime,
    Number,
    Default
};

/**
 * @brief The LogFileHandling enum class defines how the logfile is handled
 */
enum class LogFileHandling
{
    Single,        // single file, with original name
    SingleTagged,  // single file, with original name plus a tag according to LogFileTag
    Split,         // keep original name but split on predefined file size with tag according to LogFileTag 
    Default
};

/**
 * @brief The LogTextDisplay enum class defines which elements are written by log message.
 */
enum class LogMessageDisplay : unsigned int
{
    LogLevel      = 1<<0,
    ModuleName    = 1<<1,
    DateTime      = 1<<2,
    ThreadId      = 1<<3,
    Function      = 1<<4,
    File          = 1<<5,
    Line          = 1<<6,
    Message       = 1<<7,

    Default       = LogLevel|ModuleName|DateTime|ThreadId|File|Line|Message,
    Default2      = LogLevel|ModuleName|DateTime|ThreadId|File|Function|Message,
    Full          = 0xFF
};
Q_DECLARE_FLAGS(LogMessageDisplays, LogMessageDisplay)
Q_DECLARE_OPERATORS_FOR_FLAGS(LogMessageDisplays)


}
