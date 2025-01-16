
#ifndef LOGGER_H
#define LOGGER_H

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3


/**
 * Logs a message with the given tag and message. 
 * Also accepts infinite number of arguments to be formatted into the message.
 * 
 * @param level Log level of the message.
 * @param tag Tag of the message.
 * @param message Message to be logged.
 */
void log(int level, char *tag, char *message, ...);

#endif