#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char *textToEncode;
    BITMAPINFOHEADER infoHeader;
    uint_fast32_t rowLength;
    uint_fast32_t maxEncodingLength;
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

void printFileHeader(BITMAPFILEHEADER *header)
{
    printf("BITMAPFILEHEADER:\n"
           "\tbfType:\t\t\t0x%X (%c%c)\n"
           "\tbfSize:\t\t\t%d\n"
           "\tbfReserved1:\t\t0x%X\n"
           "\tbfReserved2:\t\t0x%X\n"
           "\tbfOffBits:\t\t%d\n",
           header->bfType,
           header->bfType,
           header->bfType >> 8,
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

void histogram(Refs *refs)
{
    Hist16Bins histBlue = {"Blue", {0}};
    Hist16Bins histGreen = {"Green", {0}};
    Hist16Bins histRed = {"Red", {0}};

    for (uint_fast32_t y = 0; y < refs->infoHeader.biHeight; y++)
    {
        for (uint_fast32_t x = 0; x < refs->infoHeader.biWidth; x++)
        {
            uint_fast8_t blue, green, red;

            fread(&blue, 1, 1, refs->input);
            fread(&green, 1, 1, refs->input);
            fread(&red, 1, 1, refs->input);

            histBlue.bins[blue / 16]++;
            histGreen.bins[green / 16]++;
            histRed.bins[red / 16]++;
        }
        for (uint_fast8_t p = 0; p < refs->rowLength - refs->infoHeader.biWidth * 3; p++)
        {
            fseek(refs->input, 1, SEEK_CUR);
        }
    }

    printHist16Bins(&histBlue);
    printHist16Bins(&histGreen);
    printHist16Bins(&histRed);
}

void grayscale(Refs *refs)
{
    for (uint_fast32_t y = 0; y < refs->infoHeader.biHeight; y++)
    {
        for (uint_fast32_t x = 0; x < refs->infoHeader.biWidth; x++)
        {
            uint_fast8_t blue, green, red;

            fread(&blue, 1, 1, refs->input);
            fread(&green, 1, 1, refs->input);
            fread(&red, 1, 1, refs->input);

            uint8_t grayscale = red * 0.299 + green * 0.587 + blue * 0.114;

            fwrite(&grayscale, 1, 1, refs->output);
            fwrite(&grayscale, 1, 1, refs->output);
            fwrite(&grayscale, 1, 1, refs->output);
        }
        for (uint_fast8_t p = 0; p < refs->rowLength - refs->infoHeader.biWidth * 3; p++)
        {
            uint_fast8_t padding;
            fread(&padding, 1, 1, refs->input);
            fwrite(&padding, 1, 1, refs->output);
        }
    }
}

void encode(Refs *refs)
{
    uint_fast32_t bits = 0;
    uint_fast8_t value;

    for (uint_fast32_t y = 0; y < refs->infoHeader.biHeight; y++)
    {
        for (uint_fast32_t b = 0; b < refs->rowLength; b++)
        {
            uint8_t bit = bits % 8;
            uint8_t byte = bits / 8;
            fread(&value, 1, 1, refs->input);
            // Modify least significant bit while there is data to encode
            if (byte == 0 || refs->textToEncode[byte - 1] != '\0' || bit != 0)
            {
                // Get particular bit of text
                if ((refs->textToEncode[byte] & (1 << bit)) >> bit)
                    // Set least significant bit to 1
                    value |= 0X01;
                else
                    // Set least significant bit to 0
                    value &= 0XFE;
                bits++;
            }
            fwrite(&value, 1, 1, refs->output);
        }
    }
}

void decode(Refs *refs)
{
    char *decoded = malloc(refs->maxEncodingLength + 1);
    decoded[refs->maxEncodingLength] = '\0';

    uint_fast32_t bits = 0;
    uint_fast8_t value;

    for (uint_fast32_t y = 0; y < refs->infoHeader.biHeight; y++)
    {
        for (uint_fast32_t b = 0; b < refs->rowLength; b++)
        {
            uint8_t bit = bits % 8;
            uint8_t byte = bits / 8;
            fread(&value, 1, 1, refs->input);
            // Set decoded bits unless '\0' is met
            if (byte != 0 && decoded[byte - 1] == '\0')
                break;
            // Get least significant bit from image
            if (value & 0X01)
                // Set bit to 1
                decoded[byte] |= 1 << bit;
            else
                // Set bit to 0
                decoded[byte] &= ~(1 << bit);
            bits++;
        }
    }

    printf("%s", decoded);
    free(decoded);
}

uint_fast8_t main(uint_fast8_t argc, char *argv[])
{
    Refs refs = {};

    char *inputPath = NULL;
    char *outputPath = NULL;

    switch (argc)
    {
    case 4:
        refs.textToEncode = argv[3];
    case 3:
        outputPath = argv[2];
    case 2:
        inputPath = argv[1];
        break;
    default:
        fprintf(stderr, "Incorrect arguments");
        return 1;
    }

    char mode = outputPath ? (refs.textToEncode ? 'e' : 'g') : '\0';

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
    printFileHeader(&fileHeader);

    if (fileHeader.bfType != 0x4D42)
        throw("Input file is not a bitmap", &refs);

    initInfoHeader(&refs.infoHeader, refs.input);
    printInfoHeader(&refs.infoHeader);

    if (refs.infoHeader.biBitCount != 24 || refs.infoHeader.biCompression != 0)
        throw("Further operations are only supported for uncompressed 24-bit files", &refs);

    refs.rowLength = ceil(24 * refs.infoHeader.biWidth / 32) * 4;
    refs.maxEncodingLength = refs.rowLength * (refs.infoHeader.biHeight);

    if (refs.textToEncode && strlen(refs.textToEncode) > refs.maxEncodingLength)
        throw("Image is too small to contain the whole text", &refs);

    if (!mode)
    {
        printf("\n(h)istogram/(d)ecode/(N)othing? ");

        mode = (char)fgetc(stdin);
        if (mode != 'h' && mode != 'd')
        {
            freeRefs(&refs);
            return 0;
        }

        fseek(refs.input, fileHeader.bfOffBits, SEEK_SET);
        printf("\n");
    }
    else
    {
        fseek(refs.input, 0, SEEK_SET);
        void *headers = malloc(fileHeader.bfOffBits);

        if (!headers)
            throw("Failed to allocate memory for new file headers", &refs);

        fread(headers, fileHeader.bfOffBits, 1, refs.input);
        fwrite(headers, fileHeader.bfOffBits, 1, refs.output);
        free(headers);
    }

    void (*action)(Refs *);
    if (mode == 'h')
        action = histogram;
    else if (mode == 'g')
        action = grayscale;
    else if (mode == 'e')
        action = encode;
    else if (mode == 'd')
        action = decode;
    else
    {
        fprintf(stderr, "%c", mode);
        throw(" - mode is not supported", &refs);
    }

    action(&refs);

    freeRefs(&refs);
    return 0;
}
