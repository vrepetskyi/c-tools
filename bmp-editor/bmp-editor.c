#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const uint8_t PADDING = 0;

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;

typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER;

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
} BITMAPINFOHEADER;

typedef struct Refs
{
    FILE *input;
    FILE *output;
} Refs;

void freeRefs(Refs *refs)
{
    if (refs->input)
        fclose(refs->input);
    if (refs->output)
        fclose(refs->output);
}

void throw(char *error, Refs *refs)
{
    freeRefs(refs);
    fprintf(stderr, "%s", error);
    exit(1);
}

void initFileHeader(BITMAPFILEHEADER *header, FILE *stream)
{
    fread(&header->bfType, sizeof(WORD), 1, stream);
    fread(&header->bfSize, sizeof(DWORD), 1, stream);
    fread(&header->bfReserved1, sizeof(WORD), 1, stream);
    fread(&header->bfReserved2, sizeof(WORD), 1, stream);
    fread(&header->bfOffBits, sizeof(DWORD), 1, stream);
}

void initInfoHeader(BITMAPINFOHEADER *header, FILE *stream)
{
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
    printf("BITMAPINFOHEADER:\n"
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
    Refs refs = {};

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
        throw("Incorrect arguments", &refs);
    }

    refs.input = fopen(inputPath, "r");
    if (!refs.input)
        throw("Failed to open input file", &refs);

    if (outputPath)
    {
        refs.output = fopen(outputPath, "w");
        if (!refs.output)
            throw("Failed to open output file", &refs);
    }

    BITMAPFILEHEADER fileHeader;
    initFileHeader(&fileHeader, refs.input);
    bool isBitmap = fileHeader.bfType == 0x4D42;
    printFileHeader(&fileHeader, isBitmap);
    if (!isBitmap)
        throw("Input file is not a bitmap", &refs);

    BITMAPINFOHEADER infoHeader;
    initInfoHeader(&infoHeader, refs.input);
    printInfoHeader(&infoHeader);
    if (infoHeader.biBitCount != 24 || infoHeader.biCompression != 0)
        throw("Further operations are only supported for uncompressed 24-bit files", &refs);

    if (refs.output)
    {
        fseek(refs.input, 0, 0);
        void *headers = malloc(fileHeader.bfOffBits);
        if (!headers)
            throw("Failed to allocate memory", &refs);
        fread(headers, fileHeader.bfOffBits, 1, refs.input);
        fwrite(headers, fileHeader.bfOffBits, 1, refs.output);
        free(headers);
    }
    else
        fseek(refs.input, fileHeader.bfOffBits, 0);

    Hist16Bins histBlue = {"Blue", {0}};
    Hist16Bins histGreen = {"Green", {0}};
    Hist16Bins histRed = {"Red", {0}};

    int_fast32_t rowLength = floor((24 * infoHeader.biWidth + 31) / 32) * 4;
    uint8_t *row = malloc(sizeof(uint8_t) * rowLength);
    if (!row)
        throw("Failed to allocate memory", &refs);
    for (int_fast32_t r = 0; r < infoHeader.biHeight; r++)
    {
        for (int_fast32_t c = 0; c < rowLength; c++)
        {
            fread(&row[c], sizeof(uint8_t), 1, refs.input);
        }
        for (int_fast32_t p = 0; p < infoHeader.biWidth; p++)
        {
            uint_fast8_t blue = row[p * 3];
            uint_fast8_t green = row[p * 3 + 1];
            uint_fast8_t red = row[p * 3 + 2];

            histBlue.bins[blue / 16]++;
            histGreen.bins[green / 16]++;
            histRed.bins[red / 16]++;

            if (refs.output)
            {
                if (textToEncode)
                {
                }
                else
                {
                    uint8_t grayscale = red * 0.299 + green * 0.587 + blue * 0.114;
                    fwrite(&grayscale, sizeof(uint8_t), 1, refs.output);
                    fwrite(&grayscale, sizeof(uint8_t), 1, refs.output);
                    fwrite(&grayscale, sizeof(uint8_t), 1, refs.output);
                }
                if (p == infoHeader.biWidth - 1)
                {
                    for (uint_fast8_t o = 0; o < rowLength - infoHeader.biWidth * 3; o++)
                    {
                        fwrite(&PADDING, sizeof(uint8_t), 1, refs.output);
                    }
                }
            }
        }
    }
    free(row);

    printHist16Bins(&histBlue);
    printHist16Bins(&histGreen);
    printHist16Bins(&histRed);

    freeRefs(&refs);
    return 0;
}
