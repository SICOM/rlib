#include "rlib-internal.h"
#include "rlib_gd.h"
#include <string.h>

#ifdef HAVE_GD

/* This table maps one-byte chars to strings used for drawing the lines */
/*    Uppercase/Lowercase = Wide/Thin                                   */
/*    Black(Wide), Black(Thin) / White(Wide), White(Thin) = B, b / W, w */
/*   See Wikipedia's entry for Code 39                                  */
static char *code39_table[256] = {
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"bWBwbwBwb",
	"",
	"",
	"",
	"bWbWbWbwb",
	"bwbWbWbWb",
	"",
	"",
	"",
	"",
	"bWbwBwBwb",
	"bWbwbWbWb",
	"",
	"bWbwbwBwB",
	"BWbwbwBwb",
	"bWbWbwbWb",
	"bwbWBwBwb",
	"BwbWbwbwB",
	"bwBWbwbwB",
	"BwBWbwbwb",
	"bwbWBwbwB",
	"BwbWBwbwb",
	"bwBWBwbwb",
	"bwbWbwBwB",
	"BwbWbwBwb",
	"bwBWbwBwb",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"BwbwbWbwB",
	"bwBwbWbwB",
	"BwBwbWbwb",
	"bwbwBWbwB",
	"BwbwBWbwb",
	"bwBwBWbwb",
	"bwbwbWBwB",
	"BwbwbWBwb",
	"bwBwbWBwb",
	"bwbwBWBwb",
	"BwbwbwbWB",
	"bwBwbwbWB",
	"BwBwbwbWb",
	"bwbwBwbWB",
	"BwbwBwbWb",
	"bwBwBwbWb",
	"bwbwbwBWB",
	"BwbwbwBWb",
	"bwBwbwBWb",
	"bwbwBwBWb",
	"BWbwbwbwB",
	"bWBwbwbwB",
	"BWBwbwbwb",
	"bWbwBwbwB",
	"BWbwBwbwb",
	"bWBwBwbwb",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",	"",
	"",	"",	"",	"",	""
};

void gdImageBarcodeCharFromCode39String(gdImagePtr im, int *curx, int height, char *s) {	
	char *p = s;
	char ch;	
	int black;
	int white;

	black = gdImageColorAllocate(im, 0, 0, 0);
	white = gdImageColorAllocate(im, 255, 255, 255);

	while (*p != '\0') {
		ch = *p;
		switch (ch) {
		case 'B':
			gdImageFilledRectangle(im, *curx, 0, *curx+6, height-1, black); /* Wide black */
			*curx += 3;
			break;
		case 'W':
			gdImageFilledRectangle(im, *curx, 0, *curx+3, height-1, white); /* Wide white */
			*curx += 3;
			break;
		case 'b':
			gdImageFilledRectangle(im, *curx, 0, *curx+1, height-1, black); /* Thin black */
			*curx += 1;
			break;
		case 'w':
			gdImageFilledRectangle(im, *curx, 0, *curx+1, height-1, white); /* Thin white */
			*curx += 1;
			break;
		} 
		p++;
	}

	gdImageFilledRectangle(im, *curx, 0, *curx+1, height-1, white); /* Thin white */
	*curx += 1;
}

void gdImageBarcodeChar(gdImagePtr im, int *curx, int height, char c) {
	/* Lookup up the correct code string for the character and draw it */
	gdImageBarcodeCharFromCode39String(im, curx, height, code39_table[(int) c]);
}

int gd_barcode_png_to_file(char *filename, char *barcode, int height) {
	gdImagePtr im;
	int white;
	char *p = barcode;
	int curx = 0;
	FILE *pngout;
	int width = 16 * ( strlen(barcode) + 2 );

	/* Setup gd */
	im = gdImageCreate(width, height);
	white = gdImageColorAllocate(im, 255, 255, 255);

	/* Make it all white */
	gdImageFilledRectangle(im, 0, 0, width-1, height-1, white);

	/* Start character */
	gdImageBarcodeChar(im,&curx,height,'*');

	/* Now loop over the input string */
	while (*p != '\0') {
		gdImageBarcodeChar(im,&curx,height,*p); 
		p++;
	}

	/* End character */
	gdImageBarcodeChar(im,&curx,height,'*');

	/* Write it to a file */
	pngout = fopen(filename, "wb");

	/* Output the image to the disk file in PNG format. */
	gdImagePng(im, pngout);

	/* Close the files. */
	fclose(pngout);

	/* Destroy the image in memory. */
	gdImageDestroy(im);

	return TRUE;
}

#else

int gd_barcode_png_to_file(char *filename, char *barcode, int height) {
	return FALSE;
}

#endif
