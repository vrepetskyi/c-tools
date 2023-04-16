#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M_LINT_MODE "lint"
#define M_OPERATORS "+-*/"
#define M_BAD_ARGUMENTS "Exactly two positional arguments are required:\n1. source file\n2. query/flag \"" M_LINT_MODE "\""

static const int ARGUMENT_NUMBER = 2;

static const char HELP_MESSAGE[] = M_BAD_ARGUMENTS
    "\n"
    "\nSingle query either:"
    "\n1) fetches multiple variables separated by whitespace or \",\""
    "\n2) performs operations from \"" M_OPERATORS "\" set"
    "\n"
    "\nIMPORTANT: all operations are conducted sequentially"
    "\n(in contradiction to the laws of mathematics)";

static const char LINT_MODE[] = M_LINT_MODE;
static const char BAD_ARGUMENTS[] = M_BAD_ARGUMENTS;

static const char READ_ERROR[] = "Failed to open file for read";

static const char INNER_DELIMITER[] = ".";
static const char BAD_VAR_INPUT[] = "Variable is not in \"section.key\" format";
static const char MISSING_VAR_INPUT[] = "Variable is missing";

static const char OUTER_DELIMITER[] = M_OPERATORS " ,";

static const char OPERATORS[] = M_OPERATORS;
static const char BAD_OPERATOR_INPUT[] = "Some operators are missing";

static const char SECTION_NOT_FOUND[] = "Failed to find section";
static const char KEY_NOT_FOUND[] = "Failed to find key";

static const char BAD_SECTION_NAME[] = "Bad section name";
static const char BAD_KEY_NAME[] = "Bad key name";

static const char EMPTY_OUTPUT[] = "Nothing to output";

void raise(const char *errorMessage)
{
    fprintf(stderr, "%s\n", errorMessage);
    exit(1);
}

void raiseArgument(const char *errorMessage, const char *argument)
{
    if (!strlen(argument))
    {
        raise(errorMessage);
    }
    fprintf(stderr, "%s: \"%s\"\n", errorMessage, argument);
    exit(1);
}

typedef struct Var
{
    char *section;
    int sectionLength;
    char *key;
    int keyLength;
    char *value;
    struct Var *next;
} Var;

int destroyVar(Var *var)
{
    if (!var)
    {
        return 0;
    }
    destroyVar(var->next);
    free(var->value);
    free(var);
    return 0;
}

char *skipSpaces(char *seq)
{
    char *nonSpace = seq;
    while (nonSpace && isspace(*nonSpace))
    {
        nonSpace += 1;
    }
    return nonSpace;
}

typedef struct ParseVarRes
{
    Var *var;
    char *next;
} ParseVarRes;

ParseVarRes parseVar(Var *head, char *seq)
{
    Var *var = malloc(sizeof(struct Var));
    var->value = NULL;
    var->next = NULL;
    if (head)
    {
        head->next = var;
    }

    char *sectionStart = seq;
    char *innerDelimiter = strpbrk(sectionStart, INNER_DELIMITER);
    if (!innerDelimiter)
    {
        destroyVar(var);
        if (strlen(sectionStart))
        {
            raiseArgument(BAD_VAR_INPUT, sectionStart);
        }
        else
        {
            raise(MISSING_VAR_INPUT);
        }
    }
    var->section = sectionStart;
    var->sectionLength = innerDelimiter - sectionStart;

    char *keyStart = innerDelimiter + 1;
    char *outerDelimiter = strpbrk(keyStart, OUTER_DELIMITER);
    var->key = keyStart;
    var->keyLength = outerDelimiter ? outerDelimiter - keyStart : strlen(keyStart);

    return (ParseVarRes){var, outerDelimiter};
}

typedef struct Operator
{
    char *value;
    struct Operator *next;
} Operator;

int destroyOperator(Operator *operator)
{
    if (!operator)
    {
        return 0;
    }
    destroyOperator(operator->next);
    free(operator);
    return 0;
}

typedef struct ParseOperatorRes
{
    Operator *operator;
    char *next;
} ParseOperatorRes;

ParseOperatorRes parseOperator(Operator *head, char *seq)
{
    char *target = seq;
    if (!target)
    {
        return (ParseOperatorRes){NULL, target};
    }
    if (strchr(OPERATORS, *target))
    {
        Operator *operator= malloc(sizeof(struct Operator));
        operator->value = target;
        operator->next = NULL;
        if (head)
        {
            head->next = operator;
        }
        return (ParseOperatorRes){operator, target + 1 };
    }
}

typedef struct Query
{
    char *raw;
    Var *var;
    Operator *operator;
} Query;

int destroyQuery(Query query)
{
    destroyVar(query.var);
    destroyOperator(query.operator);
    return 0;
}

Query parseQuery(char *rawQuery)
{
    char *seq = rawQuery;
    Var *var = NULL;
    int varLength = 0;
    Operator *operator= NULL;
    int operatorLength = 0;

    while (seq)
    {
        seq = skipSpaces(seq);

        ParseVarRes parseVarRes = parseVar(var, seq);
        if (!var)
        {
            var = parseVarRes.var;
        }
        varLength += 1;

        ParseOperatorRes parseOperatorRes = parseOperator(operator, skipSpaces(parseVarRes.next));
        if (parseOperatorRes.operator)
        {
            if (!operator)
            {
                operator= parseOperatorRes.operator;
            }
            operatorLength += 1;
        }
        else
        {
            if (operatorLength && varLength - operatorLength != 1)
            {
                raise(BAD_OPERATOR_INPUT);
            }
        }

        seq = parseOperatorRes.next;
    }

    return (Query){rawQuery, var, operator};
}

int hydrateQuery(Query query, FILE *source)
{
}

char *resolveQuery(Query query)
{
    char *output = malloc(sizeof(char) * 4);
    strcpy(output, "Out");
    return output;
}

FILE *openForRead(char *path)
{
    FILE *source = fopen(path, "r");
    if (source == NULL)
    {
        raiseArgument(READ_ERROR, path);
    }
}

int main(int argc, char *argv[])
{
    if (argc < ARGUMENT_NUMBER + 1)
    {
        printf("%s\n", HELP_MESSAGE);
        return 0;
    }
    if (argc > ARGUMENT_NUMBER + 1)
    {
        raise(BAD_ARGUMENTS);
    }
    char *sourcePath = argv[1];
    char *rawQuery = argv[2];
    bool lintMode = strcmp(rawQuery, LINT_MODE) == 0;

    FILE *source = NULL;
    char *output = NULL;

    if (lintMode)
    {
        // TODO
    }
    else
    {
        Query query = parseQuery(rawQuery);
        source = openForRead(sourcePath);
        hydrateQuery(query, source);
        output = resolveQuery(query);
        destroyQuery(query);
    }

    if (source)
    {
        fclose(source);
    }

    if (output)
    {
        printf("%s\n", output);
        free(output);
    }
    else
    {
        printf("%s\n", EMPTY_OUTPUT);
    }
    return 0;
}