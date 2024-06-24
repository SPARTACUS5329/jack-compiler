#pragma once
#define MAX_FILE_NAME_LENGTH 20
#define MAX_TOKEN_SIZE 200
#define MAX_TOKENS 200000
#define SEGMENT_REPRESENTATION_TABLE_SIZE 10
#define MAX_LINES 500
#define MAX_LINE_LENGTH 20
#define MAX_ASSEMBLY_LINE_LENGTH 300
#define MAX_LABEL_SIZE 50
#define MAX_FUNCTIONS 500
#define MAX_FILES 20
#define NUMBER_OF_KEYWORDS 10

char keywords[NUMBER_OF_KEYWORDS][MAX_TOKEN_SIZE] = {
    "class", "function", "constructor", "method", "static",
    "field", "int",      "char",        "bool",   "Array",
};

typedef struct {
  char *value;
  char *type;
  char *representation;
} token_t;
