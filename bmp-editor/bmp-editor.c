#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    fread(&header->bfType, 2, 1, stream);
    fread(&header->bfSize, 4, 1, stream);
    fread(&header->bfReserved1, 2, 1, stream);
    fread(&header->bfReserved2, 2, 1, stream);
    fread(&header->bfOffBits, 4, 1, stream);
}

void initInfoHeader(BITMAPINFOHEADER *header, FILE *stream)
{
    fread(&header->biSize, 4, 1, stream);
    fread(&header->biWidth, 4, 1, stream);
    fread(&header->biHeight, 4, 1, stream);
    fread(&header->biPlanes, 2, 1, stream);
    fread(&header->biBitCount, 2, 1, stream);
    fread(&header->biCompression, 4, 1, stream);
    fread(&header->biSizeImage, 4, 1, stream);
    fread(&header->biXPelsPerMeter, 4, 1, stream);
    fread(&header->biYPelsPerMeter, 4, 1, stream);
    fread(&header->biClrUsed, 4, 1, stream);
    fread(&header->biClrImportant, 4, 1, stream);
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
    bool decode = false;

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
        fseek(refs.input, 0, SEEK_SET);
        void *headers = malloc(fileHeader.bfOffBits);
        if (!headers)
            throw("Failed to allocate memory", &refs);
        fread(headers, fileHeader.bfOffBits, 1, refs.input);
        fwrite(headers, fileHeader.bfOffBits, 1, refs.output);
        free(headers);
    }
    else
    {
        fseek(refs.input, fileHeader.bfOffBits, SEEK_SET);
        printf("\nDecode steganography? [y/N]: ");
        decode = fgetc(stdin) == 121;
        printf("\n");
    }

    int_fast32_t rowLength = floor((24 * infoHeader.biWidth + 31) / 32) * 4;
    int_fast32_t maxEncodingLength = rowLength * infoHeader.biHeight;
    char *decoded = malloc(sizeof(char) * (maxEncodingLength + 1));

    if (textToEncode && strlen(textToEncode) > maxEncodingLength)
        throw("Image is too small to contain the whole text", &refs);

    Hist16Bins histBlue = {"Blue", {0}};
    Hist16Bins histGreen = {"Green", {0}};
    Hist16Bins histRed = {"Red", {0}};

    uint8_t blue, green, red;
    for (int_fast32_t r = 0; r < infoHeader.biHeight; r++)
    {
        if (!refs.output)
        {
            if (!decode)
            {
                // Histogram
                for (int_fast32_t p = 0; p < infoHeader.biWidth; p++)
                {
                    fread(&blue, 1, 1, refs.input);
                    fread(&green, 1, 1, refs.input);
                    fread(&red, 1, 1, refs.input);

                    histBlue.bins[blue / 16]++;
                    histGreen.bins[green / 16]++;
                    histRed.bins[red / 16]++;
                }
                for (uint_fast8_t p = 0; p < rowLength - infoHeader.biWidth * 3; p++)
                {
                    fseek(refs.input, 1, SEEK_CUR);
                }
            }
            else
            {
                // Decode
            }
        }
        else if (!textToEncode)
        {
            // Grayscale
            for (int_fast32_t p = 0; p < infoHeader.biWidth; p++)
            {
                fread(&blue, 1, 1, refs.input);
                fread(&green, 1, 1, refs.input);
                fread(&red, 1, 1, refs.input);

                uint8_t grayscale = red * 0.299 + green * 0.587 + blue * 0.114;

                fwrite(&grayscale, 1, 1, refs.output);
                fwrite(&grayscale, 1, 1, refs.output);
                fwrite(&grayscale, 1, 1, refs.output);
            }
            for (uint_fast8_t p = 0; p < rowLength - infoHeader.biWidth * 3; p++)
            {
                fseek(refs.input, 1, SEEK_CUR);
                fwrite(&PADDING, 1, 1, refs.output);
            }
        }
        else
        {
            // Encode
        }
    }

    if (!decode && !refs.output)
    {
        printHist16Bins(&histBlue);
        printHist16Bins(&histGreen);
        printHist16Bins(&histRed);
    }

    freeRefs(&refs);
    return 0;
}
