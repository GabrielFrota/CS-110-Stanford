/**
 * File: tsh-parse.h
 * -----------------
 * Exports a function that knows how to parse an
 * arbitrary string into tokens.
 */

#ifndef _tsh_parse_
#define _tsh_parse_

#include <stdbool.h>
#include <stddef.h>

/**
 * Accepts a command string, replaces all leading spaces after tokens
 * with '\0' characters, and updates the arguments array so that each
 * slot (up to a NULL terminating pointer) addresses the leading characters
 * of the in-place tokenized command line.  Substrings within single quotes
 * are taken to be single tokens (without the single quotes).
 * 
 * If, for instance, commandLine contains "ls -lta\0", it would be
 * updated in place to look like "ls\0-lta\0", arguments[0] would store
 * the address of the leading 'l', arguments[1] would store the address of
 * the '-', and arguments[2] would contain NULL.
 *
 * If commandLine contains "gcc    -o 'my awesome app'  a.c b.c   c.c\0",
 * it would be updated to contain "gcc\0   -o\0 'my awesome app\0  a.c\0b.c\0  c.c\0",
 * and arguments[0] would address the 'g', arguments[1] would address the '-', 
 * arguments[2] would contain the address of the 'm', etc (and arguments[6] would
 * be updated to contain NULL.
 *
 * parseline returns true if the user has requested a kBackground job, and 
 * false if the user has requested a kForeground job.
 */

bool parseline(char *commandLine, char *arguments[], size_t maxArguments);

#endif // _tsh_parse_
