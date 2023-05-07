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

BITMAPFILEHEADER *getFileHeader(FILE *stream)
{
    BITMAPFILEHEADER *header = malloc(sizeof(BITMAPFILEHEADER));
    fread(&header->bfType, sizeof(WORD), 1, stream);
    fread(&header->bfSize, sizeof(DWORD), 1, stream);
    fread(&header->bfReserved1, sizeof(WORD), 1, stream);
    fread(&header->bfReserved2, sizeof(WORD), 1, stream);
    fread(&header->bfOffBits, sizeof(DWORD), 1, stream);
    return header;
}

BITMAPINFOHEADER *getInfoHeader(FILE *stream)
{
    BITMAPINFOHEADER *header = malloc(sizeof(BITMAPINFOHEADER));
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
    return header;
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
    DWORD bins[16];
} Hist16Bins;

int main(int argc, char *argv[])
{
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
        printf("%s", "Incorrect arguments");
        return 1;
    }

    FILE *input = fopen(inputPath, "r");
    if (!input)
    {
        fprintf(stderr, "%s", "Failed to open input file");
        return 1;
    }

    FILE *output = NULL;
    if (outputPath)
    {
        output = fopen(outputPath, "w");
        if (!output)
        {
            fprintf(stderr, "%s", "Failed to open output file");
            return 1;
        }
    }

    BITMAPFILEHEADER *fileHeader = getFileHeader(input);
    bool isBitmap = fileHeader->bfType == 0x4D42;
    printFileHeader(fileHeader, isBitmap);
    if (!isBitmap)
    {
        fprintf(stderr, "%s", "Input file is not a bitmap");
        return 1;
    }

    BITMAPINFOHEADER *infoHeader = getInfoHeader(input);
    printInfoHeader(infoHeader);
    if (infoHeader->biBitCount != 24 || infoHeader->biCompression != 0)
    {
        fprintf(stderr, "%s", "Further operations are only supported for uncompressed 24-bit files");
        return 1;
    }

    free(fileHeader);
    free(infoHeader);
    return 0;
}
