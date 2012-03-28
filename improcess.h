/*
 * improcess.h
 *
 *  Created on: Mar 13, 2012
 *      Author: Nihuepana
 */


#ifndef IMPROCESS_H_
#define IMPROCESS_H_

#include <stdlib.h>
#include <stdio.h>
#pragma pack(2)


typedef struct
{
    char signature[2];
    unsigned int fileSize;
    unsigned int reserved;
    unsigned int offset;
} BmpHeader;

typedef struct
{
    unsigned int headerSize;
    unsigned int width;
    unsigned int height;
    unsigned short planeCount;
    unsigned short bitDepth;
    unsigned int compression;
    unsigned int compressedImageSize;
    unsigned int horizontalResolution;
    unsigned int verticalResolution;
    unsigned int numColors;
    unsigned int importantColors;

} BmpImageInfo;

typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    //unsigned char reserved; Removed for convenience in fread; info.bitDepth/8 doesn't seem to work for some reason
} Rgb;

/*
 * Allocate a x*y*3 array of the type char
 */
int ***alloc3d(int xlen, int ylen, int zlen) {
	int ***array = malloc(xlen*sizeof(int**));
	int i, j;
	// Allocate pointers for each row
	for (i = 0; i < xlen; i++) {
		array[i] = malloc(ylen*sizeof(int*));
		// Allocate pointer for each column in the row
		for (j=0; j < ylen; j++) {
			array[i][j] = malloc(zlen*sizeof(int));
		}
	}

	return array;
}

/*
 * Allocates a 2D float array
 */
float **alloc2Dfloat(int xlen, int ylen) {
	float **array = malloc(xlen*sizeof(float*));
	int i;
	// Allocate pointers for each row
	for (i = 0; i < xlen; i++) {
		array[i] = malloc(ylen*sizeof(float));
	}

	return array;
}

/*
 * Allocates a 2D int array
 */
int **alloc2Dint(int xlen, int ylen) {
	int **array = malloc(xlen*sizeof(int*));
	int i;
	// Allocate pointers for each row
	for (i = 0; i < xlen; i++) {
		array[i] = malloc(ylen*sizeof(int));
	}

	return array;
}
/*
 * Deallocate a l*m*n char array. Has the disadvantage that the caller needs to know the
 * dimensions of the passed array
 */
void dealloc3D(int*** arr3D,int l,int m)
{
    int i,j;

    for(i=0;i<l;i++)
    {
        for(j=0;j<m;j++)
        {
                free(arr3D[i][j]);
        }
        free(arr3D[i]);
    }
    free(arr3D);
}

/*
 * Deallocate a l*m array.
 */

void dealloc2Dfloat(float** arr2D, int l)
{
    int i;

    for(i=0;i<l;i++)
    {
        free(arr2D[i]);
    }
    free(arr2D);
}

void dealloc2Dint(int** arr2D, int l)
{
    int i;

    for(i=0;i<l;i++)
    {
        free(arr2D[i]);
    }
    free(arr2D);
}

/* loadBMP:
 * loads a 24bit RGB .bmp file and returns its header. The entire reading part is structured around
 * fread not returning to origin when reading the stream.
 */

BmpHeader readBmpHeader(FILE *inFile) {
	rewind(inFile);
	BmpHeader header;
	if( fread(&header, 1, sizeof(BmpHeader), inFile) != sizeof(BmpHeader) ) {
				printf( "Error reading bmp header.\n" );
	}
	return header;
}

/*
 * Reads and returns the DIB header of a bmp file
 */

BmpImageInfo readBmpInfo(FILE *inFile) {
	// Make sure the file is at the right position
	fseek(inFile, 14, SEEK_SET);
	BmpImageInfo info;
	if( fread(&info, 1, sizeof(BmpImageInfo), inFile) != sizeof(BmpImageInfo)) {
		printf("Error reading info header.\n");
	}
	return info;
}

/*
 * Reads the palette or colormap of a bmp image
 */
Rgb *readPalette(FILE *inFile, BmpImageInfo info) {
	// Set the file position indicator at just past the info
	fseek(inFile, 14+sizeof(info), SEEK_SET);
	Rgb *palette;
	printf( "Reading palette.\n" );
	palette = (Rgb*)malloc(sizeof(Rgb) * info.numColors);
	if( fread(palette, sizeof(Rgb), info.numColors, inFile) != (info.numColors * sizeof(Rgb)) ) {
		printf( "Error reading palette.\n" );
	}
	return palette;
}

/*
 * Reads the actual bitmap values of a bmp image.
 */

int ***readBitmap(FILE *inFile, BmpImageInfo info, int*** array) {
	Rgb *pixel = (Rgb*) malloc( sizeof(Rgb) );
	int read, j, i;
	for( j=0; j<info.height; j++ ) {
		// Commented out for speedup
		// printf( "------ Row %d\n", j+1 );
		read = 0;
		for( i=0; i<info.width; i++ ) {
			if( fread(pixel, 1, sizeof(Rgb), inFile) != sizeof(Rgb) ) {
					printf( "Error reading pixel!\n" );
			}
			array[j][i][1] = (int)(pixel -> red);
			array[j][i][2] = (int)(pixel -> green);
			array[j][i][3] = (int)(pixel -> blue);
//				printf("Red: %i, ", array[j][i][1]);
//				printf("Green: %i, ", array[j][i][2]);
//				printf("Blue: %i \n", array[j][i][3]);
			read += sizeof(Rgb);
			// Commented out for speedup
			// printf( "Pixel %d: %3d %3d %3d\n", i+1, pixel->red, pixel->green, pixel->blue );
		}

		if ( read % 4 != 0 ) {
			read = 4 - (read%4);
			printf( "Padding: %d bytes\n", read );
			fread( pixel, read, 1, inFile );
		}
	}

	return array;
}


int **padImage(BmpImageInfo info, int** image, int** outImage) {
	int i,j;
	for (i = 0; i < info.height +1; i++) {
		for (j = 0; j < info. width +1; j++) {
			if (i==0 || i == info.height +2 || j == 0 || j == info.width + 2)
				outImage[i][j] = 0;
			else
				outImage[i][j] = image[i-1][j-1];
		}
	}
	return outImage;
}


/*
 * Converts an rgb image to a grayscale image. The function doesn't weight the individual channel.
 */
float **toGrayscale(BmpImageInfo info, int*** rgbImage, float** gray) {
	int i,j;
	for (i = 0; i < info.height; i++ ) {
		for (j= 0; j < info.width; j++) {
			gray[i][j] = (((float)rgbImage[i][j][1]+(float)rgbImage[i][j][2]+(float)rgbImage[i][j][3])/3);
		}
	}
	return gray;
}

/*
 * Converts a grayscale image to binary, where all values above a provided threshold are set to 255 and
 * all values below the threshold are set to 0
 */
int **toBinaryImage(BmpImageInfo info, float** gray, int** binaryImage, int threshold) {
	int i, j;
	for (i = 0; i < info.height; i++ ) {
			for (j= 0; j < info.width; j++) {
				if (gray[i][j]>= threshold)
					binaryImage[i][j] = 1;
				else
					binaryImage[i][j] = 0;
			}
		}
	return binaryImage;
}

/*
 * Contains an implementation of the Moore neighbor algorithm for finding the contours of a binary
 * image. Boundaries are marked with 1 and everything else with 0.
 */
int **findBoundaries(BmpImageInfo info, int** binaryImage, int** boundaryImage) {
	int i, j, k;
	int **paddedImage;
	char inside = 0;

	// Define constants used for traversing the neightborhood starting from the pixel to the lower left of the starting pixel
	int neighborhood[8][2] = {{-1,-1},{0,-1},{1,-1},{1,0},{-1,0},{-1,1},{0,1},{1,1}};
	// Create a canvas for the boundary image of the size of the binaryImsage
	for (i = 0; i < info.height+1; i++) {
		for (j = 0; j < info.width+1; j++) {
			boundaryImage[i][j] = 0;
		}
	}

	// Pad the input image with a 0 boundary
	paddedImage = alloc2Dint(info.height+1, info.width+1);
	paddedImage = padImage(info, binaryImage, paddedImage);

	//Traverse the padded image
	for (i = 1; i < info.height; i++) {
		for (j = 1; j < info.width; j++) {
			// If a solid is found and inside is false an already discovered area is entered
			if (boundaryImage[i][j] == 1 && inside == 0) {
				inside = 1;
				// printf("Entered an already found boundary\n");
			}

			// No solid found and we're not inside
			else if (paddedImage [i][j] == 0 && inside == 0) {
				continue;
			}

			// No solid found and the previous step was inside means that the algorithm is leaving a border
			else if (paddedImage [i][j] == 0 && inside == 1) {
				inside = 0;
			}

			// If we are encountering a new border pixel
			else if (paddedImage [i][j] == 1 && inside == 0) {
				// printf("finding boundary\n");
				inside = 1;
				boundaryImage[i][j] = 1;

				// Traverse the boundary
				for (k = 0; k < 8; k++) {
					if (paddedImage[i+neighborhood[k][0]][j+neighborhood[k][1]] == 1 && boundaryImage[i+neighborhood[k][0]][j+neighborhood[k][1]] == 0) {
						boundaryImage[i+neighborhood[k][0]][j+neighborhood[k][1]] = 1;
						// printf("Set boundary at %i,%i\n",i+neighborhood[k][0],j+neighborhood[k][1]);
					}
				}

			}
		}
	}


	return boundaryImage;
}

int findVertexFaces(BmpImageInfo info, int height, int*** inputArray) {
	int i,j,k;

	for(i = 0; i < info.height; i++) {
		for (j = 0; j < info.width; j++) {
			for(k = 0; l < height; k++) {

			}

		}
	}
}



#endif /* IMPROCESS_H_ */
