#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char NOT_ENOUGH_ARGUMENTS[] = "Not enough arguments";
static const char TOO_MANY_ARGUMENTS[] = "Too many arguments";

static const char EXPRESSION_MODE[] = "expression";
static const char LINT_MODE[] = "lint";

static const char READ_ERROR[] = "Failed to open file for read";

static const char INNER_DELIMITER[] = ".";
static const char BAD_VAR_INPUT[] = "Input is not in section.key format";

static const char OUTER_DELIMITER[] = "+-*/ ";

static const char OPERATORS[] = "+-*/";
static const char BAD_OPERATOR_INPUT[] = "Failed to find an operator in sequence";

static const char SECTION_NOT_FOUND[] = "Failed to find section";
static const char KEY_NOT_FOUND[] = "Failed to find key";

static const char BAD_SECTION_NAME[] = "Bad section name";
static const char BAD_KEY_NAME[] = "Bad key name";

void raise(const char *errorMessage)
{
    fprintf(stderr, "%s\n", errorMessage);
    exit(1);
}

void raiseArgument(const char *errorMessage, char *argument)
{
    fprintf(stderr, "%s: \"%s\"\n", errorMessage, argument);
    exit(1);
}

typedef struct var
{
    char *section;
    int sectionLength;
    char *key;
    int keyLength;
    char *value;
    struct var *next;
} var;

typedef struct parseVarRes
{
    var *var;
    char *next;
} parseVarRes;

char *skipSpace(char *seq)
{
    char *nonSpace = seq;
    while (isspace(*nonSpace))
    {
        nonSpace += 1;
    }
    return nonSpace;
}

parseVarRes parseVar(char *seq, var *head)
{
    var *var = malloc(sizeof(struct var));
    if (head)
    {
        head->next = var;
    }
    var->value = NULL;

    char *sectionStart = seq;
    char *innerDelimiter = strpbrk(sectionStart, INNER_DELIMITER);
    if (!innerDelimiter)
    {
        destroyVar(var);
        raiseArgument(BAD_VAR_INPUT, sectionStart);
    }
    var->section = sectionStart;
    var->sectionLength = innerDelimiter - sectionStart;

    char *keyStart = innerDelimiter + 1;
    char *outerDelimiter = strpbrk(keyStart, OUTER_DELIMITER);
    var->key = keyStart;
    var->keyLength = outerDelimiter ? outerDelimiter - keyStart : strlen(keyStart);

    parseVarRes res = {var, outerDelimiter};
    return res;
}

typedef struct parseOperatorRes
{
    char *operator;
    char *next;
} parseOperatorRes;

parseOperatorRes parseOperator(char *seq)
{
}

int destroyVar(var *var)
{
    if (var->next)
    {
        destroyVar(var->next);
    }

    free(var->value);
    free(var);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        raise(NOT_ENOUGH_ARGUMENTS);
    }

    bool isExpression = strcmp(argv[2], EXPRESSION_MODE) == 0;
    bool isLint = strcmp(argv[2], LINT_MODE) == 0;

    if (argc > 3 && !isExpression || argc > 4)
    {
        raise(TOO_MANY_ARGUMENTS);
    }

    parseVarRes x = parseVar(skipSpace(argv[2]), NULL);
    printf("%s %s\n", x.var->section, x.var->key);
    destroyVar(x.var);

    x = parseVar(x.next, NULL);
    printf("%s %s\n", x.var->section, x.var->key);
    destroyVar(x.var);

    FILE *source = fopen(argv[1], "r");
    if (source == NULL)
    {
        raiseArgument(READ_ERROR, argv[1]);
    }
    fclose(source);

    return 0;
}