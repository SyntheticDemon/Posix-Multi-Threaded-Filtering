#include <iostream>
#include <unistd.h>
#include <bits/stdc++.h>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include "string.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#pragma pack(1)
#pragma once
using namespace std;
using namespace std::chrono;

typedef struct
{
    vector<vector<int>> reds;
    vector<vector<int>> greens;
    vector<vector<int>> blues;
} I;

struct apply_kernel_args
{
    int start;
    int end;
    int vertical_thread_count;
};

struct tilted_h_args
{
    int start;
    int end;
};

struct mirror_h_args
{
    int start;
    int end;
};

struct apply_kernel_vertically_args
{
    int start;
    int end;
    int y_offset;
};

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

void *apply_mini_mirror(void *thread_args)
{
    struct mirror_h_args *ag = (mirror_h_args *)(thread_args);

    for (int i = ag->start; i <= ag->end; i++)
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
void apply_mirror(int h_t_c, int v_t_c)
{

    new_image.reds = image.reds;
    new_image.greens = image.greens;
    new_image.blues = image.blues;

    int for_step = int(rows / h_t_c);
    int base = -1;
    vector<mirror_h_args *> ags;
    vector<pthread_t> thread_ids;
    for (int i = 0; i < h_t_c; i++)
    {
        mirror_h_args *ag = new mirror_h_args;
        ag->start = base + 1;
        ag->end = base + for_step;
        base += for_step;
        if (i == h_t_c - 1)
        {
            ag->end += (rows) % h_t_c;
        }

        ags.push_back(ag);
        thread_ids.push_back(pthread_t());
    }

    for (int i = 0; i < h_t_c; i++)
    {
        pthread_create(&thread_ids[i], NULL, apply_mini_mirror, ags[i]);
    }

    for (int i = 0; i < h_t_c; i++)
    {
        cout << "Joining horizontal thread" << endl;
        pthread_join(thread_ids[i], NULL);
    }
}

void *apply_kernel_vertically(void *thread_args)
{
    struct apply_kernel_vertically_args *ag = (apply_kernel_vertically_args *)(thread_args);
    int start = ag->start;
    int end = ag->end;
    int y_offset = ag->y_offset;
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
                        red_aggregate += (kernel_1[i][j] * image.reds[y_offset + i][x_offset - j]);
                        break;
                    case 1:
                        green_aggregate += (kernel_1[i][j] * image.greens[y_offset + i][x_offset - j]);
                        break;
                    case 2:
                        blue_aggregate += (kernel_1[i][j] * image.blues[y_offset + i][x_offset - j]);
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
    }
}
void *apply_kernel_star(void *thread_args)
{
    struct apply_kernel_args *ag = (apply_kernel_args *)(thread_args);
    int start = ag->start;
    int end = ag->end;
    int vertical_thread_count = ag->vertical_thread_count;
    vector<int> initial(cols, 0);
    for (int y_offset = start; y_offset <= end; y_offset++)
    {
        int thread_count = vertical_thread_count;
        int base = -1;
        if (vertical_thread_count == 0)
        {
            apply_kernel_vertically_args *ag = new apply_kernel_vertically_args;
            ag->start = base + 1;
            ag->end = base + cols;
            ag->y_offset = y_offset;
            apply_kernel_vertically(ag);
        }
        else
        {
            int for_step = int(cols / thread_count);
            vector<apply_kernel_vertically_args *> ags;
            vector<pthread_t> thread_ids;

            for (int i = 0; i < thread_count; i++)
            {
                apply_kernel_vertically_args *ag = new apply_kernel_vertically_args;
                ag->start = base + 1;
                ag->end = base + for_step;
                ag->y_offset = y_offset;
                base += for_step;
                if (i == thread_count - 1)
                {
                    ag->end += cols % thread_count;
                }

                ags.push_back(ag);
                thread_ids.push_back(pthread_t());
            }

            for (int i = 0; i < thread_count; i++)
            {
                pthread_create(&thread_ids[i], NULL, apply_kernel_vertically, ags[i]);
            }

            for (int i = 0; i < thread_count; i++)
            {
                cout << "Joining Vertical thread" << endl;
                pthread_join(thread_ids[i], NULL);
            }
        }
    }
    cout << " Finished  Horizontal" << endl;
    pthread_exit(0);
}

// TODO vertical and horizontal threads are mismatched , lexically
void apply_kernel(vector<vector<int>> kernel, int horizontal_thread_count, int vertical_thread_count)
{

    vector<int> initial(cols - 1, 0);
    int for_step = int((rows - 3) / horizontal_thread_count);
    int base = -1;
    vector<apply_kernel_args *> ags;
    vector<pthread_t> thread_ids;

    new_image.reds = image.reds;
    new_image.greens = image.greens;
    new_image.blues = image.blues;

    for (int i = 0; i < horizontal_thread_count; i++)
    {
        apply_kernel_args *ag = new apply_kernel_args;
        ag->start = base + 1;
        ag->end = base + for_step;
        ag->vertical_thread_count = vertical_thread_count;
        base += for_step;
        if (i == horizontal_thread_count - 1)
        {
            ag->end += (rows - 3) % horizontal_thread_count;
        }

        ags.push_back(ag);
        thread_ids.push_back(pthread_t());
    }

    for (int i = 0; i < horizontal_thread_count; i++)
    {
        pthread_create(&thread_ids[i], NULL, apply_kernel_star, ags[i]);
    }

    for (int i = 0; i < horizontal_thread_count; i++)
    {
        cout << "Joining horizontal thread" << endl;
        pthread_join(thread_ids[i], NULL);
    }
}

void *tilted(void *args)
{
    int *l = (int *)(args);
    for (int j = cols - 1; j >= 0; j--)
    {
        for (int k = 0; k < 3; k++)
        {
            switch (k)
            {
            case 0:
                new_image.reds[*l][j] = image.reds[*l][j];
                break;
            case 1:
                new_image.greens[*l][j] = image.greens[*l][j];
                break;
            case 2:
                new_image.blues[*l][j] = image.blues[*l][j];
                break;
            }
            // cout << i << " " << cols - j << endl;
        }
    }
}
void *apply_tilted_square_h(void *thread_args)
{
    struct tilted_h_args *ag = (tilted_h_args *)(thread_args);

    for (int j = ag->start; j <= ag->end; j++)
        for (int k = 0; k < 3; k++)
        {
            switch (k)
            {
            case 0:
                new_image.reds[int(rows) / 2 + j][cols - j] = 255;
                new_image.reds[j][int(cols) / 2 + j] = 255;
                new_image.reds[int(rows) / 2 + j][j] = 255;
                new_image.reds[int(rows) / 2 - j][j] = 255;

                break;
            case 1:
                new_image.greens[int(rows) / 2 + j][cols - j] = 255;
                new_image.greens[j][int(cols) / 2 + j] = 255;
                new_image.greens[int(rows) / 2 + j][j] = 255;
                new_image.greens[int(rows) / 2 - j][j] = 255;

                break;
            case 2:
                new_image.blues[int(rows) / 2 + j][cols - j] = 255;
                new_image.blues[j][int(cols) / 2 + j] = 255;
                new_image.blues[int(rows) / 2 + j][j] = 255;
                new_image.blues[int(rows) / 2 - j][j] = 255;

                break;
            }
            // cout << i << " " << cols - j << endl;
        }
}

void apply_tilted_square(int h_t_c)
{

    new_image.reds = image.reds;
    new_image.greens = image.greens;
    new_image.blues = image.blues;

    vector<pthread_t> threads;

    int for_step = int(int(rows / 2) / h_t_c);
    int base = -1;
    vector<tilted_h_args *> ags;
    vector<pthread_t> thread_ids;

    for (int i = 0; i < h_t_c; i++)
    {
        tilted_h_args *ag = new tilted_h_args;
        ag->start = base + 1;
        ag->end = base + for_step;
        base += for_step;
        if (i == h_t_c - 1)
        {
            ag->end += int(rows / 2) % h_t_c;
        }

        ags.push_back(ag);
        thread_ids.push_back(pthread_t());
    }

    for (int i = 0; i < h_t_c; i++)
    {
        pthread_create(&thread_ids[i], NULL, apply_tilted_square_h, ags[i]);
    }

    for (int i = 0; i < h_t_c; i++)
    {
        cout << "Joining horizontal thread" << endl;
        pthread_join(thread_ids[i], NULL);
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

int main(int argc, char **argv)
{

    char *fileBuffer;
    int bufferSize;
    char *file_input = argv[1];
    char *file_output = argv[2];
    int h_t_c = atoi(argv[3]);
    int v_t_c = atoi(argv[4]);

    // char *file_input = argv[1];
    // char *file_output = argv[2];

    if (!fillAndAllocate(fileBuffer, file_input, rows, cols, bufferSize))
    {
        cout << "File read error" << endl;
        return 1;
    }
    getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);
    apply_mirror(h_t_c, v_t_c);
    image = new_image;
    auto start_time = high_resolution_clock::now();
    apply_kernel(kernel_1, h_t_c, v_t_c);
    image = new_image;
    apply_tilted_square(h_t_c);
    auto end_time = high_resolution_clock::now();
    writeOutBmp24(fileBuffer, file_output, bufferSize);

    auto duration = duration_cast<microseconds>(end_time - start_time);
    cout << "It took " << duration.count() << " " << endl;
    // write output file
    return 0;
}