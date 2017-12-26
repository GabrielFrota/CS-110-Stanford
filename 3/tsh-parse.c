/**
 * File: tsh-parse.c
 * -----------------
 * Presents the implementation of parseline, as documented
 * in tsh-parse.h
 */

#include "tsh-parse.h"
#include "tsh-constants.h"
#include <string.h>
#include "exit-utils.h"

static const int kArgumentsArrayTooSmall = 1;

const char const *kWhitespace = " \t\n\r";
static char *skip_whitespace(char *str) {
  str += strspn(str, kWhitespace);
  return str;
}

bool parseline(char *command, char *arguments[], size_t maxArguments) {
  exitUnless(maxArguments > 1, kArgumentsArrayTooSmall,
	     stderr, "arguments array passed to parseline must be of size 2 or more.");
  command = skip_whitespace(command);
  int count = 0;
  while (count < maxArguments && *command != '\0') {
    command = skip_whitespace(command);
    if (count == maxArguments || *command == '\0') break;
    bool quoted = *command == '\'';
    if (quoted) command++;
    arguments[count++] = command;
    char termch = quoted ? '\'' : ' ';
    char *end = strchr(command, termch);
    if (end == NULL) {
      if (quoted) {
	fprintf(stderr, "Unmatched \'.\n");
	count = 0;
      }
      break;
    }
    *end = '\0';
    command = end + 1;
  }

  if (count == maxArguments) {
    fprintf(stderr, "Too many tokens... ignoring command altogether\n");
    count = 0;
  }
  
  arguments[count] = NULL;  
  if (count == 0) return true;
  bool bg = *arguments[count - 1] == '&';
  if (bg) arguments[--count] = NULL;
  return bg;
}
