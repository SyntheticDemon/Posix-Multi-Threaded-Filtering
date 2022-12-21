#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <chrono>
using namespace std::chrono;
#include "time.h"
#include <fstream>
#include "string.h"
#include <map>
using namespace std;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
#pragma once

typedef struct
{
    vector<vector<int>> reds;
    vector<vector<int>> greens;
    vector<vector<int>> blues;
} I;

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

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
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
    std::ifstream file(fileName);

    if (file)
    {
        file.seekg(0, std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);

        buffer = new char[length];
        file.read(&buffer[0], length);

        PBITMAPFILEHEADER file_header;
        PBITMAPINFOHEADER info_header;

        file_header = (PBITMAPFILEHEADER)(&buffer[0]);
        info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
        rows = info_header->biHeight;
        cols = info_header->biWidth;
        bufferSize = file_header->bfSize;
        return 1;
    }
    else
    {
        cout << "File" << fileName << " doesn't exist!" << endl;
        return 0;
    }
}

vector<vector<int>> kernel_1 = {{-2, -1, -2},
                                {-1, 1, 1},
                                {+2, 1, 2}};
I image;
I new_image;

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
    int count = 1;
    int extra = cols % 4;
    // cout << rows << cols;
    int row_padded = (cols * 3 + 3) & (~3);
    unsigned char *data = new unsigned char[row_padded];
    unsigned char tmp;
    vector<int> initial(cols, 0);
    unsigned char blue, red, green;
    for (int i = 0; i < rows; i++)
    {
        count += extra;
        image.blues.push_back(initial);
        image.greens.push_back(initial);
        image.reds.push_back(initial);
        for (int j = cols - 1; j >= 0; j--)
        {
            for (int k = 0; k < 3; k++)
            {
                switch (k)
                {
                case 0:
                    red = fileReadBuffer[end - count];
                    image.reds[i][j] = int(red);
                    count++;
                    break;
                case 1:
                    green = fileReadBuffer[end - count];
                    image.greens[i][j] = int(green);
                    count++;
                    break;
                case 2:
                    blue = fileReadBuffer[end - count];
                    image.blues[i][j] = int(blue);
                    count++;
                    break;
                }
            }
        }
    }
}

void apply_mirror()
{

    new_image.blues = image.blues;
    new_image.reds = image.reds;
    new_image.greens = image.greens;

    for (int i = 0; i < rows; i++)
    {
        for (int j = cols - 1; j >= 0; j--)
        {
            for (int k = 0; k < 3; k++)
            {
                switch (k)
                {
                case 0:
                    new_image.reds[i][j] = image.reds[i][cols - j];
                    break;
                case 1:
                    new_image.greens[i][j] = image.greens[i][cols - j];
                    break;
                case 2:
                    new_image.blues[i][j] = image.blues[i][cols - j];
                    break;
                }
                // cout << i << " " << cols - j << endl;
            }
        }
    }
}

void apply_kernel(vector<vector<int>> kernel)
{

    new_image.reds = image.reds;
    new_image.greens = image.greens;
    new_image.blues=image.blues;
    for (int y_offset = 0; y_offset < rows - 3; y_offset++)
    {
   
        for (int x_offset = cols - 1; x_offset >= 3; x_offset--)
        {
            int red_aggregate = 0;
            int green_aggregate = 0;
            int blue_aggregate = 0;

            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    for (int k = 0; k < 3; k++)
                    {

                        switch (k)
                        {
                        case 0:
                            red_aggregate += (kernel[i][j] * image.reds[y_offset + i][x_offset - j]);
                            break;
                        case 1:
                            green_aggregate += (kernel[i][j] * image.greens[y_offset + i][x_offset - j]);
                            break;
                        case 2:
                            blue_aggregate += (kernel[i][j] * image.blues[y_offset + i][x_offset - j]);
                            break;
                        }
                    }
                }
            }
            if (blue_aggregate < 0)
            {
                blue_aggregate = 0;
            }
            else if (blue_aggregate > 255)
            {
                blue_aggregate = 255;
            }
            if (red_aggregate < 0)
            {
                red_aggregate = 0;
            }
            else if (red_aggregate > 255)
            {
                red_aggregate = 255;
            }
            if (green_aggregate < 0)
            {
                green_aggregate = 0;
            }
            else if (green_aggregate > 255)
            {
                green_aggregate = 255;
            }

            new_image.blues[y_offset][x_offset] = blue_aggregate;
            new_image.reds[y_offset][x_offset] = red_aggregate;
            new_image.greens[y_offset][x_offset] = green_aggregate;
            // cout << blue_aggregate << " " << red_aggregate << " " << green_aggregate << endl;
        }
    }
}

void apply_tilted_square()
{

    new_image.reds = image.reds;
    new_image.greens = image.greens;
    new_image.blues = image.blues;
    for (int j = 0; j < int(cols) / 2; j++)
    {
        for (int k = 0; k < 3; k++)
        {
            switch (k)
            {
            case 0:
                new_image.reds[int(rows) / 2 + j][cols - j] = 255;
                new_image.reds[ j][int(cols)/2 + j] = 255;
                new_image.reds[int(rows) / 2 + j][j] = 255;
                new_image.reds[int(rows) / 2 - j][j] = 255;

                break;
            case 1:
                new_image.greens[int(rows) / 2 + j][cols - j] = 255;
                new_image.greens[j][int(cols)/2 + j] = 255;
                new_image.greens[int(rows) / 2 + j][j] = 255;
                new_image.greens[int(rows) / 2 - j][j] = 255;

                break;
            case 2:
                new_image.blues[int(rows) / 2 + j][cols - j] = 255;
                new_image.blues[j][int(cols)/2 + j] = 255;
                new_image.blues[int(rows) / 2 + j][j] = 255;
                new_image.blues[int(rows) / 2 - j][j] = 255;

                break;
            }
            // cout << i << " " << cols - j << endl;
        }
    }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
    
    std::ofstream write(nameOfFileToCreate);
    if (!write)
    {
        cout << "Failed to write " << nameOfFileToCreate << endl;
        return;
    }
    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++)
    {
        count += extra;
        for (int j = cols - 1; j >= 0; j--)
        {
            for (int k = 0; k < 3; k++)
            {

                switch (k)
                {
                case 0:
                    fileBuffer[bufferSize - count] = new_image.reds[i][j];
                    // count++;

                    // cout << new_image.reds[i][j] << endl;
                    break;
                case 1:
                    fileBuffer[bufferSize - count] = new_image.greens[i][j];
                    // count++;

                    // cout << new_image.greens[i][j] << endl;
                    break;

                case 2:
                    fileBuffer[bufferSize - count] = new_image.blues[i][j];
                    // count++;

                    // cout << new_image.blues[i][j] << endl;
                    break;
                }
                count++;
            }
        }
    }
    write.write(fileBuffer, bufferSize);
}

int main(int argc, char *argv[])
{

    char *fileBuffer;
    int bufferSize;
    char *file_input = argv[1];
    char *file_output = argv[2];

    // char *file_input = argv[1];
    // char *file_output = argv[2];

    if (!fillAndAllocate(fileBuffer, file_input, rows, cols, bufferSize))
    {
        cout << "File read error" << endl;
        return 1;
    }
    getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);
    apply_mirror();
    image=new_image;
    auto start_time = high_resolution_clock::now();
    apply_kernel(kernel_1);
    auto end_time = high_resolution_clock::now();
    image = new_image;
    apply_tilted_square();

    writeOutBmp24(fileBuffer, file_output, bufferSize);
    // write output file
    auto duration = duration_cast<microseconds>(end_time - start_time);
    cout << "It took " << duration.count() << " " << endl;

    return 0;
}