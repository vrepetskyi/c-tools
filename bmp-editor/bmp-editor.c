#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;

// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader
typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

// https://docs.microsoft.com/pl-pl/previous-versions/dd183376(v=vs.85)
typedef struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct References
{
    FILE *input;
    FILE *output;
    BITMAPFILEHEADER *fileHeader;
    BITMAPINFOHEADER *infoHeader;
} References;

void freeReferences(References *ref)
{
    if (ref->input)
        fclose(ref->input);
    if (ref->output)
        fclose(ref->output);
    if (ref->fileHeader)
        free(ref->fileHeader);
    if (ref->infoHeader)
        free(ref->infoHeader);
}

void throw(char *error, References *ref)
{
    freeReferences(ref);
    fprintf(stderr, "%s", error);
    exit(1);
}

BITMAPFILEHEADER *setFileHeader(References *ref)
{
    BITMAPFILEHEADER *header = malloc(sizeof(BITMAPFILEHEADER));
    if (!header)
    {
        throw("Failed to allocate memory", ref);
    }
    FILE *stream = ref->input;
    fread(&header->bfType, sizeof(WORD), 1, stream);
    fread(&header->bfSize, sizeof(DWORD), 1, stream);
    fread(&header->bfReserved1, sizeof(WORD), 1, stream);
    fread(&header->bfReserved2, sizeof(WORD), 1, stream);
    fread(&header->bfOffBits, sizeof(DWORD), 1, stream);
    ref->fileHeader = header;
}

BITMAPINFOHEADER *setInfoHeader(References *ref)
{
    BITMAPINFOHEADER *header = malloc(sizeof(BITMAPINFOHEADER));
    if (!header)
    {
        throw("Failed to allocate memory", ref);
    }
    FILE *stream = ref->input;
    fread(&header->biSize, sizeof(DWORD), 1, stream);
    fread(&header->biWidth, sizeof(LONG), 1, stream);
    fread(&header->biHeight, sizeof(LONG), 1, stream);
    fread(&header->biPlanes, sizeof(WORD), 1, stream);
    fread(&header->biBitCount, sizeof(WORD), 1, stream);
    fread(&header->biCompression, sizeof(DWORD), 1, stream);
    fread(&header->biSizeImage, sizeof(DWORD), 1, stream);
    fread(&header->biXPelsPerMeter, sizeof(LONG), 1, stream);
    fread(&header->biYPelsPerMeter, sizeof(LONG), 1, stream);
    fread(&header->biClrUsed, sizeof(DWORD), 1, stream);
    fread(&header->biClrImportant, sizeof(DWORD), 1, stream);
    ref->infoHeader = header;
}

void printFileHeader(BITMAPFILEHEADER *header, bool isBitmap)
{
    printf("BITMAPFILEHEADER:\n"
           "\tbfType:\t\t\t0x%X (%s)\n"
           "\tbfSize:\t\t\t%d\n"
           "\tbfReserved1:\t\t0x%X\n"
           "\tbfReserved2:\t\t0x%X\n"
           "\tbfOffBits:\t\t%d\n",
           header->bfType,
           isBitmap ? "BM" : "!BM",
           header->bfSize,
           header->bfReserved1,
           header->bfReserved2,
           header->bfOffBits);
}

void printInfoHeader(BITMAPINFOHEADER *header)
{
    printf("BITMAPFILEHEADER:\n"
           "\tbiSize:\t\t\t%d\n"
           "\tbiWidth:\t\t%d\n"
           "\tbiHeight:\t\t%d\n"
           "\tbiPlanes:\t\t%d\n"
           "\tbiBitCount:\t\t%d\n"
           "\tbiCompression:\t\t%d\n"
           "\tbiSizeImage:\t\t%d\n"
           "\tbiXPelsPerMeter:\t%d\n"
           "\tbiYPelsPerMeter:\t%d\n"
           "\tbiClrUsed:\t\t%d\n"
           "\tbiClrImportant:\t\t%d\n",
           header->biSize,
           header->biWidth,
           header->biHeight,
           header->biPlanes,
           header->biBitCount,
           header->biCompression,
           header->biSizeImage,
           header->biXPelsPerMeter,
           header->biYPelsPerMeter,
           header->biClrUsed,
           header->biClrImportant);
}

typedef struct Hist16Bins
{
    char *name;
    uint_fast32_t bins[16];
} Hist16Bins;

void printHist16Bins(Hist16Bins *hist)
{
    printf("%s:\n", hist->name);

    uint_fast32_t sum = 0;
    for (uint_fast8_t i = 0; i < 16; i++)
    {
        sum += hist->bins[i];
    }

    for (uint_fast8_t i = 0; i < 16; i++)
    {
        const char *format = (i < 7) ? "\t%d-%d:\t\t\t%.2f%%\n" : "\t%d-%d:\t\t%.2f%%\n";
        float percent = (sum > 0) ? (hist->bins[i] * 100 / sum) : 0;
        printf(format, 16 * i, 16 * (i + 1) - 1, percent);
    }
}

uint_fast8_t main(uint_fast8_t argc, char *argv[])
{
    References ref = {};

    char *inputPath = NULL;
    char *outputPath = NULL;
    char *textToEncode = NULL;

    switch (argc)
    {
    case 4:
        textToEncode = argv[3];
    case 3:
        outputPath = argv[2];
    case 2:
        inputPath = argv[1];
        break;
    default:
        throw("Incorrect arguments", &ref);
    }

    ref.input = fopen(inputPath, "r");
    if (!ref.input)
        throw("Failed to open input file", &ref);

    if (outputPath)
    {
        ref.output = fopen(outputPath, "w");
        if (!ref.output)
            throw("Failed to open output file", &ref);
    }

    setFileHeader(&ref);
    bool isBitmap = ref.fileHeader->bfType == 0x4D42;
    printFileHeader(ref.fileHeader, isBitmap);
    if (!isBitmap)
        throw("Input file is not a bitmap", &ref);

    setInfoHeader(&ref);
    printInfoHeader(ref.infoHeader);
    if (ref.infoHeader->biBitCount != 24 || ref.infoHeader->biCompression != 0)
        throw("Further operations are only supported for uncompressed 24-bit files", &ref);

    fseek(ref.input, ref.fileHeader->bfOffBits, 0);

    Hist16Bins histBlue = {"Blue", {0}};
    Hist16Bins histGreen = {"Green", {0}};
    Hist16Bins histRed = {"Red", {0}};

    int_fast32_t rowLength = floor((24 * ref.infoHeader->biWidth + 31) / 32) * 4;
    uint8_t *row = malloc(sizeof(uint8_t) * rowLength);
    if (!row)
        throw("Failed to allocate memory", &ref);
    for (int_fast32_t r = 0; r < ref.infoHeader->biHeight; r++)
    {
        for (int_fast32_t c = 0; c < rowLength; c++)
        {
            fread(&row[c], sizeof(uint8_t), 1, ref.input);
        }
        for (int_fast32_t p = 0; p < ref.infoHeader->biWidth; p++)
        {
            uint_fast8_t blue = row[p * 3];
            uint_fast8_t green = row[p * 3 + 1];
            uint_fast8_t red = row[p * 3 + 2];

            histBlue.bins[blue / 16]++;
            histGreen.bins[green / 16]++;
            histRed.bins[red / 16]++;
        }
    }
    free(row);

    printHist16Bins(&histBlue);
    printHist16Bins(&histGreen);
    printHist16Bins(&histRed);

    freeReferences(&ref);
    return 0;
}
