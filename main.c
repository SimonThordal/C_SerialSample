/*
 * main.c
 *
 *  Created on: Mar 13, 2012
 *      Author: Simon Thordal
 *
 */

#include <stddef.h>
#include "improcess.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main( int argc, char **argv ) {

	BmpHeader header;
	BmpImageInfo info, *infop;
	infop = &info;
	Rgb *palette;
	int i,j,k, noOfFiles;
	int ***rgbImage;
	int **grayImage;
	int **binaryImage;
	int **boundaryImage;
	int ***binary3DArray;
	char *imageName, *outfileName, buffer[8192];
	struct Face *head = NULL;
	//struct Face *temp;
	FILE *stlfile, *timelog;
	stlfile = fopen("out.stl","wb");

	// Scale factors
	float scale[3] = {1,1,1};


	// Variables for timing
	clock_t c0, c1, c2, c3;
	float dif;
	timelog = fopen("timelog.txt", "wb");

	//Specifying the image names
	char currentImageName[100];
	char currentOutfileName[50];
	imageName = "C:\\Users\\Nihuepana\\workspace\\BMPSample\\Default\\Images\\00";
	outfileName = "out_";

	// Arbitrary threshold, to be determined by Otsu later on
	int threshold = 123;

	// Setting stdout to buffered mode to avoid scanf appearing before printf. Unbuffered mode also fixes the problem however this slows down printf calls
	setvbuf(stdout, buffer, _IONBF, sizeof(buffer));


	//Get the number of files to be processed
	noOfFiles = 0;
	printf("How many image files need to be procesed?\n");
	scanf("%i", &noOfFiles);

	for (k = 0; k < noOfFiles; k++) {
		// Timer for full process of image prep
		c2 = clock();
		c0 = clock();
		//Construct the outfile name
		sprintf(currentOutfileName, "%s%i.txt", outfileName, k);

		FILE * outfile;
		// Output text field
		outfile = fopen(currentOutfileName, "wb");
			if (outfile == NULL) {
				printf("Error opening outfile\n");
				return -1;
			}
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog,"Outfile created in %.2f seconds.\n", dif);

		c0 = clock();
		//Constructing the image name
		sprintf(currentImageName,"%s%i.bmp", imageName, k+71);

		FILE * infile;
		infile = fopen(currentImageName, "rb");
		if (infile == NULL) {
			printf("Error opening file %s\n", currentImageName);
			return -1;
		}

		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Opened image %s in %.2f seconds.\n", currentImageName, dif);

		c0 = clock();
		// Read the header
		header = readBmpHeader(infile);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Read header in %.2f seconds.\n", dif);

		c1 = clock();
		// Read image info
		*infop = readBmpInfo(infile);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Read info in %.2f seconds.\n", dif);

		if (k == 0) {
			//Allocate the 3D array for binary values and open and close it with a layer of zeros
			binary3DArray = alloc3D(info.height, info.width, noOfFiles+2);
			for(i=0;i<info.height;i++) {
					for(j=0;j<info.width;j++) {
						binary3DArray[i][j][0] = 0;
						binary3DArray[i][j][noOfFiles+1] = 0;
					}
			}
		}

		c0 = clock();
		// Read the image palette
		if (info.numColors > 0) {
			palette = readPalette(infile, info);
		}
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Read palette in %.2f seconds.\n", dif);

		// Allocate an array for the bitmap
		c0 = clock();
		rgbImage = alloc3D(info.height, info.width, 3);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Allocated rgbImage in %.2f seconds\n", dif);

		c0 = clock();
		// Read the bitmap array
		rgbImage = readBitmap(infile, info, rgbImage);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Read bitmap in %.2f seconds.\n", dif);

		// Test if the array is returned
//			for (i = 0; i < info.height; i++ ) {
//				for (j = 0; j < info.width; j++) {
//					printf("Red: %i, ", rgbImage[j][i][1]);
//					printf("Green: %i, ", rgbImage[j][i][2]);
//					printf("Blue: %i \n", rgbImage[j][i][3]);
//				}
//			}

		// Create the grayscale image
		c0 = clock();
		grayImage = alloc2Dint(info.height, info.width);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Allocated grayscale image in %.2f seconds.\n", dif);

		c0 = clock();
		grayImage = toGrayscale(info, rgbImage, grayImage);
		dealloc3D(rgbImage, info.height, info.width);

		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Converted to grayscale in %.2f seconds.\n", dif);

		threshold = otsuThresh(grayImage, info);
		fprintf(timelog, "Found otsus threshold for image %i to %i\n", threshold, k+1);

		// Create the binary image with a simple threshold
		c0 = clock();
		binaryImage = alloc2Dint(info.height, info.width);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Allocated binary image in %.2f seconds.\n", dif);

		c0 = clock();
		binaryImage = toBinaryImage(info, grayImage, binaryImage, threshold);
		dealloc2Dint(grayImage, info.height);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Converted to binary in %.2f seconds.\n", dif);

		// Find the boundaries in the binaryimage
		c0 = clock();
		boundaryImage = alloc2Dint(info.height+1,info.width+1);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Allocated boundary image in %.2f seconds.\n", dif);

		c0 = clock();
		boundaryImage = findBoundaries(info, binaryImage, boundaryImage);
		dealloc2Dint(binaryImage, info.height);
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Found boundaries in %.2f seconds.\n", dif);

		//Create a textfile of the boundary image

		c0 = clock();
//		for (i = 0; i < info.height; i++) {
//			for (j = 0; j < info.width; j++) {
//				fprintf(outfile, "%i ", boundaryImage[i][j]);
//			}
//			fprintf(outfile, "\n");
//		}
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Printed to outfile in %.2f seconds.\n", dif);

		// Allocate and assign the values to a 3D array

		c0 = clock();
		for (i = 0; i < info.height; i++) {
			for (j = 0; j < info.width; j++) {
				binary3DArray[i][j][k+1] = boundaryImage[i][j];
			}
		}
		c1 = clock();
		dif = (float)(c1-c0)/CLOCKS_PER_SEC;
		fprintf(timelog, "Assinged to 3D array in %.2f seconds. \n", dif);

		// Garbage collection
		fclose(infile);
		fclose(outfile);


		dealloc2Dint(boundaryImage, info.height+1);

		c3 = clock();
		dif = (float)(c3-c2)/CLOCKS_PER_SEC;
		fprintf(timelog, "Prepared image in %.2f seconds. \n", dif);

	}
	c0 = clock();
	head = findVertexFaces(infop, noOfFiles+2, binary3DArray, scale);
	c1 = clock();
	dif = (float)(c1-c0)/CLOCKS_PER_SEC;
	fprintf(timelog, "Found vertex faces in %.2f seconds.\n",dif);
	// Print the results from the linked list
	c0 = clock();
//	temp = head;
//	fprintf(stlfile, "solid reconstruction\n");
//	while (temp != NULL) {
//		fprintf(stlfile, "   facet normal %e %e %e\n", temp->normal[0], temp->normal[1], temp->normal[2]);
//		fprintf(stlfile, "      outer loop\n");
//		for (i = 0; i < 3; i++)
//			fprintf(stlfile, "         vertex %e %e %e\n", temp->faceVertex[i].coords[0], temp->faceVertex[i].coords[1], temp->faceVertex[i].coords[2]);
//		fprintf(stlfile,"      endloop\n   endfacet\n");
//		temp = temp->next;
//
//	}
//	fprintf(stlfile, "endsolid reconstruction");

	outputBinary(stlfile, head);

	c1 = clock();
	dif = (float)(c1-c0)/CLOCKS_PER_SEC;
	fprintf(timelog,"Wrote STL file in %.2f seconds.\n",dif);
//	 Garbage collection
	fclose(stlfile);
	fclose(timelog);
	dealloc3D(binary3DArray,info.height,info.width);
	free(head);
	head = NULL;
	return 0;
}

