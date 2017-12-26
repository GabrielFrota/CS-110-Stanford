/**
 * File: tsh_parse_test.c
 * ----------------------
 * Provides a test framework to exercise that
 * parseline function exported by tsh_parse.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tsh-parse.h"

static const char *const kCommandPrompt = "parse-test>";
static const unsigned short kMaxCommandLength = 2048; // in characters
static const unsigned short kMaxArgumentCount = 128;

static void pullrest() {
  while (getchar() != '\n');
}

static void readcommand(char command[], size_t len) {
  char control[64] = {'\0'};
  command[0] = '\0';
  sprintf(control, "%%%zu[^\n]%%c", len);
  while (true) {
    printf("%s ", kCommandPrompt);
    char termch;
    if (scanf(control, command, &termch) < 2) { pullrest(); return; }
    if (termch == '\n') return;
    fprintf(stderr, "Command shouldn't exceed %hu characters.  Ignoring.\n", kMaxCommandLength);
    pullrest();
  }
}

int main(int argc, char *argv[]) {
  while (true) {
    char command[kMaxCommandLength + 1];
    readcommand(command, kMaxCommandLength);
    if (feof(stdin)) break;
    char *arguments[kMaxArgumentCount];
    bool bg = parseline(command, arguments, kMaxArgumentCount);
    if (arguments[0] == NULL) continue;
    if (strcasecmp(arguments[0], "quit") == 0) exit(0);
    printf("Arguments:\n");
    size_t i = 0;
    for (; arguments[i] != NULL; i++) {
      printf(" arguments[%zu] = \"%s\"\n", i, arguments[i]);
    }
    printf(" arguments[%zu] = %s\n", i, arguments[i] == NULL ? "NULL" : "non-NULL");
    printf("Background? %s\n", bg ? "yes" : "no");
  }
  return 0;
}
