#include <complex.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_TYPE 0x4D42
#define FILE_HEADER 14
#define DIB_HEADER 108
#define HEADER (FILE_HEADER + DIB_HEADER)
#define PLANE_NUMBER 1
#define BITS_PER_PIXEL 8
#define PALETTE_COLORS (1 << BITS_PER_PIXEL)
#define PALETTE_BYTES 4
#define PIXELS_PER_METRE 11812

const uint_fast8_t ZERO = 0;

void write32to16(uint16_t *target, uint_fast32_t value)
{
    target[0] = value;
    target[1] = (value >> 16);
}

void writeGrayscalePalette(FILE *target)
{
    for (uint_fast32_t value = 0; value < PALETTE_COLORS; value++)
    {
        for (uint_fast8_t i = 0; i < PALETTE_BYTES - 1; i++)
        {
            fwrite(&value, 1, 1, target);
            if (i == PALETTE_BYTES - 2)
            {
                fwrite(&ZERO, 1, 1, target);
            }
        }
    }
}

void writeHeaders(uint_fast32_t width, uint_fast32_t height, uint_fast32_t rowLength, FILE *target)
{
    uint16_t header[HEADER / 2] = {FILE_TYPE};

    uint_fast32_t palette = PALETTE_COLORS * PALETTE_BYTES;
    uint_fast32_t data = rowLength * height;
    uint_fast32_t offset = HEADER + palette;
    uint_fast32_t file = offset + data;

    write32to16(&header[1], file);
    write32to16(&header[5], offset);
    write32to16(&header[7], DIB_HEADER);
    write32to16(&header[9], width);
    write32to16(&header[11], height);
    header[13] = PLANE_NUMBER;
    header[14] = BITS_PER_PIXEL;
    write32to16(&header[17], data);
    write32to16(&header[19], PIXELS_PER_METRE);
    write32to16(&header[21], PIXELS_PER_METRE);
    write32to16(&header[23], PALETTE_COLORS);

    fwrite(header, HEADER, 1, target);

    writeGrayscalePalette(target);
}

uint_fast16_t getIterations(uint_fast32_t width, uint_fast32_t height, uint_fast32_t x, uint_fast32_t y, uint_fast16_t maxIterations)
{
    double mappedX = (double)x * 3 / width - 2;
    double mappedY = (double)y * 2 / height - 1;
    double complex c = mappedX + mappedY * I;
    double complex z = 0;
    for (uint_fast16_t i = 0; i < maxIterations; i++)
    {
        z = z * z + c;
        if (abs(z) > 2)
        {
            return i;
        }
    }
    return maxIterations;
}

uint_fast8_t mapIterationsToColor(uint_fast16_t n, uint_fast16_t max)
{
    return (float)n / max * 255;
}

void writePixels(uint_fast32_t width, uint_fast32_t height, uint_fast32_t rowLength, FILE *target, uint_fast16_t maxIterations)
{
    for (uint_fast32_t y = 0; y < height; y++)
    {
        for (uint_fast32_t x = 0; x < width; x++)
        {
            uint_fast16_t iterations = getIterations(width, height, x, y, maxIterations);
            uint_fast8_t color = mapIterationsToColor(iterations, maxIterations);
            fwrite(&color, 1, 1, target);
        }
        for (uint_fast8_t p = 0; p < rowLength - width; p++)
        {
            fwrite(&ZERO, 1, 1, target);
        }
    }
}

uint_fast8_t main(uint_fast8_t argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "The program accepts exactly three positional arguments: image width, height, output path, and max number of iterations");
        return 1;
    }

    uint_fast32_t width = atoi(argv[1]);
    uint_fast32_t height = atoi(argv[2]);
    uint_fast16_t maxIterations = atoi(argv[4]);
    if (width < 1 || height < 1 || maxIterations < 4)
    {
        fprintf(stderr, "At least one numerical value is too small");
        return 1;
    }

    FILE *output = fopen(argv[3], "w");
    if (!output)
    {
        fprintf(stderr, "Failed to access the output file");
        return 1;
    }

    uint_fast32_t rowLength = ceil(BITS_PER_PIXEL * width / 32) * 4;
    writeHeaders(width, height, rowLength, output);
    writePixels(width, height, rowLength, output, maxIterations);

    fclose(output);
    return 0;
}
