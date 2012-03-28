/*
 * main.c
 *
 *  Created on: Mar 13, 2012
 *      Author: Simon Thordal
 */

#include <stddef.h>
#include "improcess.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main( int argc, char **argv ) {

	BmpHeader header;
	BmpImageInfo info;
	Rgb *palette;
	int i,j,k, noOfFiles;
	int ***rgbImage;
	float **grayImage;
	int **binaryImage;
	int **boundaryImage;
	char *imageName, *outfileName, buffer[8192];
	int ***binary3DArray;
	time_t start, end;
	double dif;
	//Specifying the image names
	char currentImageName[100];
	char currentOutfileName[50];
	imageName = "C:\\Users\\Nihuepana\\workspace\\BMPSample\\Default\\Images\\IMG_";
	outfileName = "out_";
	// Arbitrary threshold, to be determined by Otsu later on
	int threshold = 170;
	// Setting stdout to buffered mode to avoid scanf appearing before printf. Unbuffered mode also fixes the problem however this slows down printf calls
	setvbuf(stdout, buffer, _IONBF, sizeof(buffer));


	//Get the number of files to be processed
	noOfFiles = 0;
	printf("How many image files need to be processed?\n");
	scanf("%i", &noOfFiles);

	for (k = 0; k < noOfFiles; k++) {

		time(&start);
		//Construct the outfile name
		sprintf(currentOutfileName, "%s%i.txt", outfileName, k);

		FILE * outfile;
		// Output text field
		outfile = fopen(currentOutfileName, "wb");
			if (outfile == NULL) {
				printf("Error opening outfile\n");
				return -1;
			}
		time(&end);
		dif = difftime(end,start);
		printf("Outfile created in %.2f seconds.\n", dif);

		time(&start);
		//Constructing the image name
		sprintf(currentImageName,"%s%i.bmp", imageName, 7204+k);

		FILE * infile;
		infile = fopen(currentImageName, "rb");
		if (infile == NULL) {
			printf("Error opening file %s\n", currentImageName);
			return -1;
		}

		time(&end);
		dif = difftime(end,start);
		printf("Opened image %s in %.2f seconds.\n", currentImageName, dif);

		time(&start);
		// Read the header
		header = readBmpHeader(infile);
		time(&end);
		dif = difftime(end,start);
		printf("Read header in %.2f seconds.\n", dif);

		time(&start);
		// Read image info
		info = readBmpInfo(infile);
		time(&end);
		dif = difftime(end,start);
		printf("Read info in %.2f seconds.\n", dif);

		if (k == 0) {
			//Allocate the 3D array for binary values
			binary3DArray = alloc3d(info.height, info.width, noOfFiles);
		}

		time(&start);
		// Read the image palette
		if (info.numColors > 0) {
			palette = readPalette(infile, info);
		}
		time(&end);
		dif = difftime(end,start);
		printf("Read palette in %.2f seconds.\n", dif);

		// Allocate an array for the bitmap
		rgbImage = alloc3d(info.height, info.width, 3);

		time(&start);
		// Read the bitmap array
		rgbImage = readBitmap(infile, info, rgbImage);
		time(&end);
		dif = difftime(end,start);
		printf("Read bitmap in %.2f seconds.\n", dif);

		// Test if the array is returned
		//	for (i = 0; i < info.height; i++ ) {
		//		for (j = 0; j < info.width; j++) {
		//			printf("Red: %i, ", array[j][i][1]);
		//			printf("Green: %i, ", array[j][i][2]);
		//			printf("Blue: %i \n", array[j][i][3]);
		//		}
		//	}

		// Create the grayscale image

		grayImage = alloc2Dfloat(info.height, info.width);

		time(&start);
		grayImage = toGrayscale(info, rgbImage, grayImage);
		time(&end);
		dif = difftime(end,start);
		printf("Converted to grayscale in %.2f seconds.\n", dif);

		// Create the binary image with a simple threshold

		binaryImage = alloc2Dint(info.height, info.width);

		time(&start);
		binaryImage = toBinaryImage(info, grayImage, binaryImage, threshold);
		time(&end);
		dif = difftime(end,start);
		printf("Converted to binary in %.2f seconds.\n", dif);

		// Find the boundaries in the binaryimage

		boundaryImage = alloc2Dint(info.height+1,info.width+1);

		time(&start);
		boundaryImage = findBoundaries(info, binaryImage, boundaryImage);
		time(&end);
		dif = difftime(end,start);
		printf("Found boundaries in %.2f seconds.\n", dif);

		//Create a textfile of the boundary image

		time(&start);
		for (i = 0; i < info.height; i++) {
			for (j = 0; j < info.width; j++) {
				fprintf(outfile, "%i ", boundaryImage[i][j]);
			}
			fprintf(outfile, "\n");
		}
		time(&end);
		dif = difftime(end,start);
		printf("Printed to outfile in %.2f seconds.\n", dif);

		// Allocate and assign the values to a 3D array

		time(&start);
		for (i = 0; i < info.height; i++) {
			for (j = 0; j < info.width; j++) {
				binary3DArray[i][j][k] = boundaryImage[i][j];
			}
		}
		time(&end);
		dif = difftime(end,start);
		printf("Assinged to 3D array in %.2f seconds. \n", dif);

		// Garbage collection
		fclose(infile);
		fclose(outfile);

		dealloc3D(rgbImage, info.height, info.width);

		dealloc2Dfloat(grayImage, info.height);

		dealloc2Dint(binaryImage, info.height);

		dealloc2Dint(boundaryImage, info.height);
	}

	dealloc3D(binary3DArray,info.height,info.width);
	return 0;

}

