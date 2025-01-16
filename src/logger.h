
#ifndef LOGGER_H
#define LOGGER_H

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

/**
 * Set the output file for the logger to the given file in the given directory.
 * 
 * @param logs_directory Directory where the log file will be stored. 
 *        Must be directory in the current working directory.
 * @param log_file File name of the log file.
 */
void init_logger(char* logs_directory, char* log_file);

/**
 * Logs a message with the given tag and message. 
 * Also accepts infinite number of arguments to be formatted into the message.
 * 
 * @param level Log level of the message.
 * @param tag Tag of the message.
 * @param message Message to be logged.
 */
void log(int level, char *tag, char *message, ...);

/**
 * Closes the logger file.
 */
void close_logger();

#endif