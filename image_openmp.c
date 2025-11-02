#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include "image.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <image file> <filter type>\n", argv[0]);
        return -1;
    }

    Image srcImage, destImage;
    srcImage.data = stbi_load(argv[1], &srcImage.width, &srcImage.height, &srcImage.bpp, 0);
    if (!srcImage.data) {
        printf("Error: Could not open image %s\n", argv[1]);
        return -1;
    }

    destImage.width = srcImage.width;
    destImage.height = srcImage.height;
    destImage.bpp = srcImage.bpp;
    destImage.data = malloc(srcImage.width * srcImage.height * srcImage.bpp);

    enum KernelTypes kernelType = GetKernelType(argv[2]);
    if (kernelType == -1) {
        Usage();
        stbi_image_free(srcImage.data);
        free(destImage.data);
        return -1;
    }

    Matrix algorithm;
    switch (kernelType) {
        case EDGE:
            memcpy(algorithm, (Matrix){{-1,-1,-1},{-1,8,-1},{-1,-1,-1}}, sizeof(Matrix));
            break;
        case SHARPEN:
            memcpy(algorithm, (Matrix){{0,-1,0},{-1,5,-1},{0,-1,0}}, sizeof(Matrix));
            break;
        case BLUR:
            memcpy(algorithm, (Matrix){{1/9.0,1/9.0,1/9.0},{1/9.0,1/9.0,1/9.0},{1/9.0,1/9.0,1/9.0}}, sizeof(Matrix));
            break;
        case GAUSE_BLUR:
            memcpy(algorithm, (Matrix){{1,2,1},{2,4,2},{1,2,1}}, sizeof(Matrix));
            for (int i=0;i<3;i++)for(int j=0;j<3;j++)algorithm[i][j]/=16.0;
            break;
        case EMBOSS:
            memcpy(algorithm, (Matrix){{-2,-1,0},{-1,1,1},{0,1,2}}, sizeof(Matrix));
            break;
        case IDENTITY:
        default:
            memcpy(algorithm, (Matrix){{0,0,0},{0,1,0},{0,0,0}}, sizeof(Matrix));
            break;
    }

    #pragma omp parallel for collapse(2)
    for (int row = 0; row < srcImage.height; row++) {
        for (int pix = 0; pix < srcImage.width; pix++) {
            for (int bit = 0; bit < srcImage.bpp; bit++) {
                destImage.data[Index(pix,row,srcImage.width,bit,srcImage.bpp)] =
                    getPixelValue(&srcImage,pix,row,bit,algorithm);
            }
        }
    }

    stbi_write_png("output.png", destImage.width, destImage.height, destImage.bpp, destImage.data, destImage.width * destImage.bpp);
    printf("Output written to output.png\n");

    stbi_image_free(srcImage.data);
    free(destImage.data);
    return 0;
}
