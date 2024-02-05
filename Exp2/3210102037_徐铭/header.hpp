#ifndef _HEADER_H_
#define _HEADER_H_
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <windows.h>
#include <x86intrin.h>
#include <thread>
#include <vector>
#include <mutex>

std::mutex LmaxMutex;
double Lmax = 0.0;    // 定义全局共享变量

#define REAL_WIDTH(i) (( i + 31 ) / 32 * 4)
// #define TIME_COUNTER(freq, begin, end) ((double)(end.QuadPart - begin.QuadPart) / (double)(freq.QuadPart))

/*Some pre-definitions of the data types and structure*/
typedef unsigned short Word;
typedef unsigned int dWord;
typedef unsigned char Byte;

/*image file header*/
typedef struct tagBMPFILEHEADER{
    dWord bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    dWord bfOffBits;
}BMPFILEHEADER;

/*image information header*/
typedef struct tagBMPINFOHEADER{
    dWord biSize;
    dWord biWidth;
    dWord biHeight;
    WORD biPlanes;
    WORD biBitCount;
    dWord biCompression;
    dWord biSizeImage;
    dWord biXPelsPerMeter;
    dWord biYPelsPerMeter;
    dWord biClrUsed;
    dWord biClrImportant;
}BMPINFOHEADER;

/*normal palette*/
typedef struct tagRGBQuad{
    Byte rgbBlue;    
    Byte rgbGreen;   
    Byte rgbRed;    
    Byte rgbReserved;
}RGBQuad;

/* compute the time */
class Timer {
public:
	Timer() : start_(), end_() { }

	void Start() {
		QueryPerformanceCounter(&start_);
	}

	void Stop() {
		QueryPerformanceCounter(&end_);
	}

	double GetElapsedMilliseconds() {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return (end_.QuadPart - start_.QuadPart) * 1000.0 / freq.QuadPart;
	}

private:
	LARGE_INTEGER start_;
	LARGE_INTEGER end_;
};

#endif