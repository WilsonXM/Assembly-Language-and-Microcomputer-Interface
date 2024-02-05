#include "header.hpp"

// 对数组进行初始化为0
void Initialization( double *array, int e, int length )
{
    for( int i = 0; i < length; i++ ) *(array + i) = e;
}

void processPixelRange(int startRow, int endRow, int biWidth, double* pixel_y, double* Lmax) {
    for (int i = startRow; i < endRow; ++i) {
        for (int j = 0; j < biWidth; ++j) {
            // 读取 pixel_y[i * biWidth + j] 的值
            double currentPixelY;
            {
                std::lock_guard<std::mutex> lock(LmaxMutex);
                currentPixelY = pixel_y[i * biWidth + j];
            }

            // 计算新的 pixel_y 值
            double newPixelY = 255.0 * (log(currentPixelY + 1.0) / log(*Lmax + 1.0));

            // 写入新的 pixel_y 值
            {
                std::lock_guard<std::mutex> lock(LmaxMutex);
                pixel_y[i * biWidth + j] = newPixelY;
            }
        }
    }
}

int main( int argc, char *argv[] )
{
    
    Word bfType;
    BMPFILEHEADER fileHeader;
    BMPINFOHEADER infoHeader;

    FILE *fp;           // point to the initial bmp
    FILE *logarithm;    // point to the bmp after the logarithmeic operation
    FILE *histogram;    // point to the bmp after the histogram equalization
    Timer timer;        // record the time

    long real_width;    // the real width of every line

    // 读取真彩图文件头，信息头数据
    fp = fopen( argv[2], "rb" );
    if( fp == NULL ){
        printf("Open Error!\n");
        return 0;
    }

    fseek( fp, 0, SEEK_SET );
    fread( &bfType, sizeof(Word), 1, fp );  // 单独读取 “BM” 
    if( bfType == 0x4d42 ){
        printf("OK, the image is bmp!\n");
        fread( &fileHeader, sizeof(BMPFILEHEADER), 1, fp );  // read the image file header
        fread( &infoHeader, sizeof(BMPINFOHEADER), 1, fp );  // read the image information header
    }else{
        printf("This is not a bmp image!\n");
        return -1;
    }

    real_width = REAL_WIDTH( infoHeader.biBitCount * infoHeader.biWidth );  // 实际每一行需要的字节数
    if( infoHeader.biBitCount == 24 ){
        // 读取真彩图中的rgb数据，将之存到一维数组中
        Byte *RGBdata_24 = NULL;
        RGBdata_24 = (Byte *)calloc( infoHeader.biHeight * real_width, 1 );
        fread( RGBdata_24, 1, infoHeader.biHeight * real_width, fp );
        fclose( fp );
        if( strcmp( argv[1], "logarithm" ) == 0 ){
            timer.Start();

            if( !(logarithm = fopen( argv[3], "wb" )) ){
                printf( "Open Error!\n" );
                return -1;
            }
            double *pixel_y = (double *)calloc( infoHeader.biWidth * infoHeader.biHeight, sizeof(double) );
            double *pixel_u = (double *)calloc( infoHeader.biWidth * infoHeader.biHeight, sizeof(double) );
            double *pixel_v = (double *)calloc( infoHeader.biWidth * infoHeader.biHeight, sizeof(double) );
            // 将rgb数据转换成yuv分别三个数组，同时找出最大的y并对y的值做规范处理到[0, 255]
            for( int i = 0; i < infoHeader.biHeight; i++ ){
                for( int j = 0; j < infoHeader.biWidth; j++ ){
                    double B = RGBdata_24[ i * real_width + j * 3 + 0 ];
                    double G = RGBdata_24[ i * real_width + j * 3 + 1 ];
                    double R = RGBdata_24[ i * real_width + j * 3 + 2 ];
                    pixel_y[ i * infoHeader.biWidth + j ] = 0.2990 * R + 0.5870 * G + 0.1140 * B;
                    pixel_u[ i * infoHeader.biWidth + j ] = -0.1471 * R - 0.2888 * G + 0.4360 * B ;
                    pixel_v[ i * infoHeader.biWidth + j ] = 0.6150 * R - 0.5150 * G - 0.1000 * B ;
                    if( Lmax < pixel_y[ i * infoHeader.biWidth + j ] ) Lmax = pixel_y[ i * infoHeader.biWidth + j ];
                    if( pixel_y[ i * infoHeader.biWidth + j ] < 0 ) pixel_y[ i * infoHeader.biWidth + j ] = 0;
                    if( pixel_y[ i * infoHeader.biWidth + j ] > 255 ) pixel_y[ i * infoHeader.biWidth + j ] = 255;
                }
            }
            // 遍历所有的Y值，对其作对数化增强处理--logarithm enhancement
            // 确定线程数量
            const int numThreads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;

            // 分配任务给各个线程
            int rowsPerThread = infoHeader.biHeight / numThreads;
            for (int i = 0; i < numThreads; ++i) {
                int startRow = i * rowsPerThread;
                int endRow = (i == numThreads - 1) ? infoHeader.biHeight : (i + 1) * rowsPerThread;
                threads.push_back(std::thread(processPixelRange, startRow, endRow, infoHeader.biWidth, pixel_y, &Lmax));
            }

            // 等待所有线程完成
            for (std::thread& t : threads) {
                t.join();
            }
            // for( int i = 0; i < infoHeader.biHeight; i++ )
            //     for( int j = 0; j < infoHeader.biWidth; j++ )
            //         pixel_y[ i * infoHeader.biWidth + j ] = 255.0000 * (log( pixel_y[ i * infoHeader.biWidth + j ] + 1 )) / (log( Lmax + 1 ));
            
            //--将规范化和对数化后的y和uv转换为rgb
            for( int i = 0; i < infoHeader.biHeight; i++ ){
                for( int j = 0; j < infoHeader.biWidth; j++ ){
                    double temp = 0;
                    temp = 1.0000 * pixel_y[ i * infoHeader.biWidth + j ] + 0.0000 * pixel_u[ i * infoHeader.biWidth + j ] + 1.1398 * pixel_v[ i * infoHeader.biWidth + j ];
                    if( temp < 0 ) temp = 0;
                    if( temp > 255 ) temp = 255;
                    RGBdata_24[ i * real_width + j * 3 + 2 ] = (Byte)temp;  //R
                    temp = 1.0000 * pixel_y[ i * infoHeader.biWidth + j ] - 0.3947 * pixel_u[ i * infoHeader.biWidth + j ] - 0.5806 * pixel_v[ i * infoHeader.biWidth + j ];
                    if( temp < 0 ) temp = 0;
                    if( temp > 255 ) temp = 255;
                    RGBdata_24[ i * real_width + j * 3 + 1 ] = (Byte)temp;  //G
                    temp = 1.0000 * pixel_y[ i * infoHeader.biWidth + j ] + 2.0321 * pixel_u[ i * infoHeader.biWidth + j ] + 0.0000 * pixel_v[ i * infoHeader.biWidth + j ];
                    if( temp < 0 ) temp = 0;
                    if( temp > 255 ) temp = 255;
                    RGBdata_24[ i * real_width + j * 3 + 0 ] = (Byte)temp;  //B
                }
            }
            //--将写好后的rgb数据连同之前的写进新的bmp
            fseek( logarithm, 0, SEEK_SET );
            fwrite( &bfType, sizeof(Word), 1, logarithm );
            fwrite( &fileHeader, sizeof(BMPFILEHEADER), 1, logarithm );
            fwrite( &infoHeader, sizeof(BMPINFOHEADER), 1, logarithm );
            fwrite( RGBdata_24, 1, infoHeader.biHeight * real_width, logarithm );

            timer.Stop();
            std::cout << "logarithm time: " << timer.GetElapsedMilliseconds() << "ms" << std::endl;

            free( RGBdata_24 );
            fclose( logarithm );
            printf( "OK! The logarithm enhancement of the image has been created!\n" );
        }
    }else{
        printf( "Please use a true color image" );
    }

    system( "pause" );
    return 0;
}