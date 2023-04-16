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
    "\n1) fetches multiple variables separated by whitespace"
    "\n2) performs operations from \"" M_OPERATORS "\" set"
    "\n"
    "\nIMPORTANT: all operations are conducted sequentially"
    "\n(in contradiction to the laws of mathematics)"
    "\n"
    "\nMaximal supported line length is 1024 characters"
    "\nMaximal output size is 1024 characters";

static const char LINT_MODE[] = M_LINT_MODE;
static const char BAD_ARGUMENTS[] = M_BAD_ARGUMENTS;

static const char READ_ERROR[] = "Failed to open file for read";

static const char INNER_DELIMITER[] = ".";
static const char BAD_VAR_INPUT[] = "Variable is not in \"section.key\" format";
static const char MISSING_VAR_INPUT[] = "Variable is missing";

static const char OUTER_DELIMITER[] = " ";

static const char OPERATORS[] = M_OPERATORS;
static const char BAD_OPERATOR_INPUT[] = "Some operators are missing";

static const int MAX_LINE = 1025;

static const char SECTION_START[] = "[";
static const char SECTION_END[] = "]";

static const char VAR_ASSIGNMENT[] = "=";

static const char SECTION_NOT_FOUND[] = "Failed to find section";
static const char KEY_NOT_FOUND[] = "Failed to find key";

static const char COMMENT[] = ";";

void raise(const char *errorMessage)
{
    fprintf(stderr, "%s\n", errorMessage);
    exit(1);
}

void errorArgument(const char *errorMessage, const char *argument)
{
    fprintf(stderr, "%s: \"%s\"\n", errorMessage, argument);
}

void raiseArgument(const char *errorMessage, const char *argument)
{
    if (!strlen(argument))
    {
        raise(errorMessage);
    }
    errorArgument(errorMessage, argument);
    exit(1);
}

typedef struct Var
{
    char *section;
    int sectionLength;
    bool sectionFound;
    char *key;
    int keyLength;
    bool keyFound;
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

char *spaceBack(char *seq)
{
    char *nonSpace = seq;
    while (nonSpace && isspace(*nonSpace))
    {
        nonSpace -= 1;
    }
    return nonSpace + 1;
}

typedef struct ParseVarRes
{
    Var *var;
    char *next;
} ParseVarRes;

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

ParseVarRes parseVar(Var *head, char *seq, Operator *operator)
{
    Var *var = malloc(sizeof(struct Var));
    var->sectionFound = false;
    var->keyFound = false;
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
        destroyOperator(operator);
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
    if (outerDelimiter)
    {
        var->keyLength = outerDelimiter - keyStart;
    }
    else
    {
        var->keyLength = strlen(keyStart);
    }

    return (ParseVarRes){var, outerDelimiter};
}

ParseOperatorRes parseOperator(Operator *head, char *seq)
{
    char *target = seq;
    if (target && strlen(target) && strchr(OPERATORS, *target))
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
    return (ParseOperatorRes){NULL, target};
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
    Var *varHead = NULL;
    Var *var = NULL;
    int varLength = 0;
    Operator *operatorHead = NULL;
    Operator *operator= NULL;
    int operatorLength = 0;

    while (seq)
    {
        seq = skipSpaces(seq);

        ParseVarRes parseVarRes = parseVar(var, seq, operatorHead);
        var = parseVarRes.var;
        if (!varHead)
        {
            varHead = var;
        }
        varLength += 1;

        ParseOperatorRes parseOperatorRes = parseOperator(operator, skipSpaces(parseVarRes.next));
        if (parseOperatorRes.operator)
        {
            operator= parseOperatorRes.operator;
            if (!operatorHead)
            {
                operatorHead = operator;
            }
            operatorLength += 1;
        }
        else
        {
            if (operatorLength && varLength - operatorLength != 1)
            {
                destroyVar(varHead);
                destroyOperator(operatorHead);
                raise(BAD_OPERATOR_INPUT);
            }
        }

        seq = parseOperatorRes.next;
    }

    return (Query){rawQuery, varHead, operatorHead};
}

int hydrateQuery(Query query, FILE *source)
{
    char line[MAX_LINE];
    char section[MAX_LINE];
    int sectionLength = 0;
    while (fgets(line, sizeof(line), source) != NULL)
    {
        char *first = skipSpaces(line);
        char *last;

        if (strchr(SECTION_START, *first) && (last = strpbrk(first, SECTION_END)))
        {
            first += 1;
            sectionLength = last - first;
            memcpy(section, first, sizeof(char) * sectionLength);
            section[sectionLength] = 0;
            continue;
        }

        char *varAssignment = strpbrk(first, VAR_ASSIGNMENT);
        if (!varAssignment)
        {
            continue;
        }
        if (varAssignment - first)
        {
            last = spaceBack(varAssignment - 1);
        }
        int keyLength = last - first;
        Var *var = query.var;
        while (var)
        {
            if (
                sectionLength != var->sectionLength ||
                memcmp(section, var->section, sizeof(char) * sectionLength) != 0)
            {
                var = var->next;
                continue;
            }
            var->sectionFound = true;
            if (
                keyLength != var->keyLength ||
                memcmp(first, var->key, sizeof(char) * keyLength) != 0)
            {
                var = var->next;
                continue;
            }
            var->keyFound = true;
            char *value = skipSpaces(varAssignment + 1);
            int valueLength = strlen(value) + 1;
            var->value = malloc(sizeof(char) * valueLength);
            memcpy(var->value, value, sizeof(char) * valueLength);
            var = var->next;
        }
    }
    return 0;
}

bool isVarNumerical(Var *var)
{
    char resString[MAX_LINE];
    strcpy(resString, var->value);
    float resNumber = atoi(var->value);
    char recast[128];
    sprintf(recast, "%f", resNumber);
    return strncmp(resString, recast, strlen(resString) - 1) == 0;
}

int resolveQuery(Query query)
{
    bool expressionMode = (bool)query.operator;

    Var *var = query.var;
    while (var)
    {
        if (!var->sectionFound)
        {
            int length = var->sectionLength;
            char *section = malloc(sizeof(char) * length);
            memcpy(section, var->section, sizeof(char) * length);
            section[length] = 0;
            errorArgument(SECTION_NOT_FOUND, section);
            free(section);
            if (expressionMode)
            {
                destroyQuery(query);
                exit(1);
            }
            var = var->next;
            continue;
        }

        if (!var->keyFound)
        {
            int length = var->sectionLength + var->keyLength + 1;
            char *key = malloc(sizeof(char) * length);
            memcpy(key, var->section, sizeof(char) * length);
            key[length] = 0;
            errorArgument(KEY_NOT_FOUND, key);
            free(key);
            if (expressionMode)
            {
                destroyQuery(query);
                exit(1);
            }
        }

        if (!expressionMode)
        {
            printf("%s", var->value ? var->value : "\n");
        }

        var = var->next;
    }

    if (!expressionMode)
    {
        return 0;
    }

    var = query.var;
    bool isNumerical = isVarNumerical(var);
    char resString[MAX_LINE];
    int last;
    float resNumber;
    if (isNumerical)
    {
        resNumber = atoi(var->value);
    }
    else
    {
        last = strlen(var->value) - 1;
        strncpy(resString, var->value, last);
    }
    var = var->next;

    Operator *operator= query.operator;
    while (operator)
    {
        if (isVarNumerical(var) != isNumerical)
        {
            fprintf(stderr, "%s\n", "Unable to perform an operation between a string and a number");
            exit(1);
        }

        if (isNumerical)
        {
            int value = atoi(var->value);
            switch (operator->value[0])
            {
            case '+':
                resNumber += value;
                break;
            case '-':
                resNumber -= value;
                break;
            case '*':
                resNumber *= value;
                break;
            case '/':
                resNumber /= value;
                break;
            }
        }
        else
        {
            if (operator->value[0] != '+')
            {
                fprintf(stderr, "%s\n", "Only \"+\" operation is supported for strings");
                exit(1);
            }
            int length = strlen(var->value) - 1;
            strncpy(resString + last, var->value, length);
            last += length;
        }

        var = var->next;
        operator= operator->next;
    }

    if (isNumerical)
    {
        printf("%f\n", resNumber);
    }
    else
    {
        resString[last] = 0;
        printf("%s\n", resString);
    }

    return 0;
}

bool isValid(char target)
{
    int ascii = (int)target;
    if (
        ascii == 45 ||                // -
        (47 < ascii && ascii < 58) || // 0-9
        (64 < ascii && ascii < 91) || // A-Z
        (96 < ascii && ascii < 123)   // a-z
    )
    {
        return true;
    }
    return false;
}

int lint(FILE *source)
{
    int i = 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), source) != NULL)
    {
        i++;

        int p = 0;
        if (strchr(COMMENT, line[p]))
        {
            continue;
        }

        bool isSection = strchr(SECTION_START, line[p]);
        p += 1;

        while (line[p])
        {
            if (isSection)
            {
                if (strchr(SECTION_END, line[p]))
                {
                    break;
                }
            }
            else
            {
                if ((line[p] == ' ' && line[p + 1] == '=') || line[p] == '=')
                {
                    break;
                }
            }

            if (isValid(line[p]))
            {
                p += 1;
                continue;
            }

            printf("%d: %s", i, line);
            break;
        }
    }
    return 0;
}

FILE *openForRead(char *path)
{
    FILE *source = fopen(path, "r");
    if (source == NULL)
    {
        raiseArgument(READ_ERROR, path);
    }
    return source;
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

    if (lintMode)
    {
        FILE *source = openForRead(sourcePath);
        lint(source);
        fclose(source);
    }
    else
    {
        Query query = parseQuery(rawQuery);
        FILE *source = openForRead(sourcePath);
        hydrateQuery(query, source);
        fclose(source);
        resolveQuery(query);
        destroyQuery(query);
    }

    return 0;
}