#include "compiler.h"
#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

static int fileCount = 0;
static int tokenCount = 0;
static char staticPrefix[MAX_FILE_NAME_LENGTH];

void error(const char *message) {
  perror(message);
  exit(1);
}

bool isEscape(const char c) {
  switch (c) {
  case ' ':
  case '\n':
  case '\t':
    return true;
  default:
    return false;
  }
}

bool isSymbol(const char c) {
  switch (c) {
  case '=':
  case '+':
  case '-':
  case '*':
  case '/':
  case '.':
  case ',':
  case '(':
  case ')':
  case '{':
  case '}':
  case '[':
  case ']':
  case ';':
  case '<':
  case '>':
    return true;
  default:
    return false;
  }
}

bool isKeyword(const char *token) {
  for (int i = 0; i < NUMBER_OF_KEYWORDS; i++) {
    if (streq(token, keywords[i], strlen(token)))
      return true;
  }
  return false;
}

bool isIntegerConstant(const char *token) {
  for (int i = 0; i < strlen(token); i++) {
    if (!isdigit(token[i]))
      return false;
  }
  return true;
}

bool isStringConstant(const char *token) {
  if (token[0] != '"' && token[0] != '\'')
    return false;
  char stringEncloser = token[0];
  if (token[strlen(token) - 1] != stringEncloser)
    return false;
  for (int i = 1; i < strlen(token) - 1; i++) {
    if (token[i] == stringEncloser)
      return false;
  }
  return true;
}

bool isIdentifier(const char *token) {
  if (!isalpha(token[0]) && token[0] != '_')
    return false;
  for (int i = 0; i < strlen(token) - 1; i++) {
    if (!isalpha(token[i]) && !isdigit(token[i]) && token[0] != '_')
      return false;
  }
  return true;
}

void addToken(char *tokenContainer, const char *token) {
  if (isKeyword(token)) {
    sprintf(tokenContainer, "<keyword> %s </keyword>", token);
  } else if (isSymbol(*token)) {
    sprintf(tokenContainer, "<symbol> %c </symbol>", *token);
  } else if (isIntegerConstant(token)) {
    sprintf(tokenContainer, "<integerConstant> %s </integerConstant>", token);
  } else if (isStringConstant(token)) {
    char stringConstant[strlen(token) - 2];
    for (int i = 1; i < strlen(token) - 1; i++) {
      stringConstant[i] = token[i];
    }
    stringConstant[strlen(token) - 2] = '\0';
    sprintf(tokenContainer, "<stringConstant> %s </stringConstant>",
            stringConstant);
  } else if (isIdentifier(token)) {
    sprintf(tokenContainer, "<identifier> %s </identifier>", token);
  } else {
    error("[addToken] Invalid token");
  }
}

char **tokenize(char *buffer) {
  int currIndex = 0;
  bool isTokenValid = false;
  char *currToken = malloc(MAX_TOKEN_SIZE * sizeof(char));
  if (currToken == NULL)
    error("[tokenize] Memory allocation error: currToken");
  char **tokens = malloc(MAX_TOKENS * sizeof(char *));
  if (tokens == NULL)
    error("[tokenize] Memory allocation error: tokens");
  for (int i = 0; i < MAX_TOKENS; i++)
    tokens[i] = malloc(MAX_TOKEN_SIZE * sizeof(char));

  while (buffer[currIndex] != '\0') {
    char currChar = buffer[currIndex];

    if (isEscape(currChar)) {
      if (!isTokenValid)
        goto next_char;
      addToken(tokens[tokenCount++], currToken);
      bzero(currToken, MAX_TOKEN_SIZE);
      isTokenValid = false;
    }

    else if (currChar == '"' || currChar == '\'') {
      if (isTokenValid)
        error("[tokenize] Invalid string literal");
      char stringEncloser = currChar;
      int length = 0;
      currToken[length++] = buffer[currIndex++];
      while (buffer[currIndex] != stringEncloser) {
        currChar = buffer[currIndex++];
        currToken[length++] = currChar;
      }
      currToken[length++] = stringEncloser;
      currToken[length] = '\0';
      addToken(tokens[tokenCount++], currToken);
      bzero(currToken, MAX_TOKEN_SIZE);
      isTokenValid = false;
    }

    else if (isSymbol(currChar)) {
      if (isTokenValid) {
        addToken(tokens[tokenCount++], currToken);
        bzero(currToken, MAX_TOKEN_SIZE);
      }
      addToken(tokens[tokenCount++], &currChar);
      isTokenValid = false;
    }

    else {
      int length = strlen(currToken);
      currToken[length] = currChar;
      currToken[length + 1] = '\0';
      isTokenValid = true;
    }

  next_char:
    currIndex++;
  }

  int i = 0;
  do {
    printf("%s\n", tokens[i]);
  } while (++i < tokenCount);

  return tokens;
}

char *initialize(const char *fileName) {
  FILE *file = fopen(fileName, "r");
  if (file == NULL)
    error("[initialize] File not found");

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc((fileSize + 1) * sizeof(char));
  if (buffer == NULL)
    error("[initialize] Unable to allocate memory");

  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead != fileSize)
    error("[initialize] Error reading file");

  buffer[fileSize] = '\0';

  fclose(file);

  return buffer;
}

void writeToFile(char **instructions, const char *fileName) {
  FILE *file;

  file = fopen(fileName, "w");
  if (file == NULL)
    error("Error in opening out file");
  printf("Writing to %s ...\n\n", fileName);

  for (int i = 0; i < tokenCount; i++) {
    fprintf(file, "%s\n", instructions[i]);
  }

  fclose(file);
}

void translateFile(const char *readStream, const char *writeStream) {
  switch (sscanf(readStream, "%[^.]", staticPrefix)) {
    char *lastSlash;
    const char *lastDot;

  case 1:
    break;

  case 0:
    lastSlash = strrchr(readStream, '/');
    if (lastSlash == NULL)
      error("[initialize] Invalid directory-file format");
    lastSlash++;

    lastDot = strrchr(lastSlash, '.');
    if (lastDot == NULL || !streq(lastDot, ".jack", 4)) {
      printf("[translateFile] Skipping %s: Invalid file extension", readStream);
      return;
    }
    strncpy(staticPrefix, lastSlash, lastDot - lastSlash);
    staticPrefix[lastDot - lastSlash] = '\0';
    break;

  default:
    printf("[translateFile] Skipping %s: Invalid file naming convention",
           readStream);
    return;
  }

  char *buffer = initialize(readStream);
  printf("\nTranslating %s...\n\n", readStream);
  char **tokens = tokenize(buffer);
  writeToFile(tokens, writeStream);
  printf("Successfully translated!\n");
}

int main(int argc, char *argv[]) {
  if (argc < 3)
    error("[main] Not enough input arguments");
  const char *readStream = argv[1];
  const char *writeStream = argv[2];
  const char *extensionStarting = strstr(readStream, "/");
  if (extensionStarting == NULL) {
    // handle single file case
    char *extension = strstr(readStream, ".");
    if (extension == NULL || !streq(extension, ".jack", 4))
      error("[main] Error in file naming convention");
    translateFile(readStream, writeStream);
    goto exit_program;
  }

  // handle directory case
  DIR *dir = opendir(readStream);
  if (!dir)
    error("[main] Error in opening directory");

  struct dirent *entry;
  struct stat statbuf;
  char fullpath[1024];
  char **files;
  files = (char **)malloc(MAX_FILES * sizeof(char *));
  for (int i = 0; i < MAX_FILES; i++) {
    files[i] = (char *)malloc(100 * sizeof(char));
    if (files[i] == NULL)
      error("Memory allocation error");
  }

  while ((entry = readdir(dir)) != NULL) {
    if (streq(entry->d_name, ".", 1) || streq(entry->d_name, "..", 2))
      continue;

    sprintf(fullpath, "%s/%s", readStream, entry->d_name);

    if (stat(fullpath, &statbuf) == -1)
      error("[main] stat");

    strcpy(files[fileCount++], entry->d_name);
  }

  closedir(dir);

  for (int i = 0; i < fileCount; i++) {
    char readFile[MAX_FILE_NAME_LENGTH];
    sprintf(readFile, "%s", files[i]);
    char readFilePath[MAX_FILE_NAME_LENGTH];
    sprintf(readFilePath, "%s/%s", readStream, readFile);
    char *vmExtension = strstr(readFile, ".");
    char readFileName[MAX_FILE_NAME_LENGTH];
    strncpy(readFileName, readFile, vmExtension - readFile);
    readFileName[vmExtension - readFile] = '\0';
    char writeFilePath[MAX_FILE_NAME_LENGTH];
    sprintf(writeFilePath, "%s/%s.xml", readStream, readFileName);
    translateFile(readFilePath, writeFilePath);
  }

exit_program:
  return 0;
}
