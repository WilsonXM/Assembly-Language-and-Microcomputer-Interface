#include "header.hpp"

// 对数组进行初始化为0
void Initialization( double *array, int e, int length )
{
    for( int i = 0; i < length; i++ ) *(array + i) = e;
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
            double Lmax = 0;
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
            for( int i = 0; i < infoHeader.biHeight; i++ )
                for( int j = 0; j < infoHeader.biWidth; j++ )
                    pixel_y[ i * infoHeader.biWidth + j ] = 255.0000 * (log( pixel_y[ i * infoHeader.biWidth + j ] + 1 )) / (log( Lmax + 1 ));
            
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
        // else if( strcmp( argv[1], "histogram" ) == 0 ){
        //     timer.Start();

        //     if( !(histogram = fopen( argv[3], "wb" )) ){
        //         printf( "Open Error!\n" );
        //         return -1;
        //     }
        //     double histogram_red[256];
        //     double histogram_green[256];
        //     double histogram_blue[256];
        //     Initialization( histogram_red, 0, 256 );
        //     Initialization( histogram_green, 0, 256 );
        //     Initialization( histogram_blue, 0, 256 );
        //     //--对真彩图的三个通道进行直方图均衡化
        //     // 首先统计三个通道的256个值的像素个数，形成原来的直方图
        //     for( int i = 0; i < infoHeader.biHeight; i++ ){
        //         for( int j = 0; j < infoHeader.biWidth; j++ ){
        //             histogram_blue[ RGBdata_24[ i * real_width + j * 3 + 0 ] ]++;
        //             histogram_green[ RGBdata_24[ i * real_width + j * 3 + 1 ] ]++;
        //             histogram_red[ RGBdata_24[ i * real_width + j * 3 + 2 ] ]++;
        //         }
        //     }

        //     // 其次计算三个通道新的s[256]，也就是对直方图进行均衡化
        //     double N = infoHeader.biHeight * infoHeader.biWidth;
        //     Byte s_red[256]; double sum_red = 0;
        //     Byte s_green[256]; double sum_green = 0;
        //     Byte s_blue[256]; double sum_blue = 0;
        //     for( int i = 0; i < 256; i++ ){
        //         double temp = 0;
        //         sum_red = sum_red + histogram_red[i];
        //         temp = sum_red / N * 255;
        //         if( temp < 0 ) temp = 0;
        //         if( temp > 255 ) temp = 255;
        //         s_red[i] = (Byte)temp;

        //         sum_green = sum_green + histogram_green[i];
        //         temp = sum_green / N * 255;
        //         if( temp < 0 ) temp = 0;
        //         if( temp > 255 ) temp = 255;
        //         s_green[i] = (Byte)temp;

        //         sum_blue = sum_blue + histogram_blue[i];
        //         temp = sum_blue / N * 255;
        //         if( temp < 0 ) temp = 0;
        //         if( temp > 255 ) temp = 255;
        //         s_blue[i] = (Byte)temp;
        //     }

        //     // 对原图的每个按照均衡化后的对应值进行赋值
        //     for( int i = 0; i < infoHeader.biHeight; i++ ){
        //         for( int j = 0; j < infoHeader.biWidth; j++ ){
        //             RGBdata_24[ i * real_width + j * 3 + 0 ] = s_blue[RGBdata_24[ i * real_width + j * 3 ]];
        //             RGBdata_24[ i * real_width + j * 3 + 1 ] = s_green[RGBdata_24[ i * real_width + j * 3 + 1 ]];
        //             RGBdata_24[ i * real_width + j * 3 + 2 ] = s_red[RGBdata_24[ i * real_width + j * 3 + 2 ]];
        //         }
        //     }
        //     //--将写好后的rgb数据连同之前的写进新的bmp
        //     fseek( histogram, 0, SEEK_SET );
        //     fwrite( &bfType, sizeof(Word), 1, histogram );
        //     fwrite( &fileHeader, sizeof(BMPFILEHEADER), 1, histogram );
        //     fwrite( &infoHeader, sizeof(BMPINFOHEADER), 1, histogram );
        //     fwrite( RGBdata_24, 1, infoHeader.biHeight * real_width, histogram );

        //     timer.Stop();
        //     std::cout << "histogram time: " << timer.GetElapsedMilliseconds() << "ms" << std::endl;

        //     free( RGBdata_24 );
        //     fclose( histogram );
        //     printf( "OK! The histogram of the image has been created!\n" );
        // }
    }else{
        printf( "Please use a true color image" );
    }

    system( "pause" );
    return 0;
}