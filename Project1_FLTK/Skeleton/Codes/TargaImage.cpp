///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// constants
const int           RED = 0;                // red channel
const int           GREEN = 1;                // green channel
const int           BLUE = 2;                // blue channel
const unsigned char BACKGROUND[3] = { 0, 0, 0 };      // background color


// Computes n choose s, efficiently
double Binomial(int n, int s)
{
	double        res;

	res = 1;
	for (int i = 1; i <= s; i++)
		res = (n - i + 1) * res / i;

	return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
	data = new unsigned char[width * height * 4];
	ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char* d)
{
	int i;

	width = w;
	height = h;
	data = new unsigned char[width * height * 4];

	for (i = 0; i < width * height * 4; i++)
		data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image)
{
	width = image.width;
	height = image.height;
	data = NULL;
	if (image.data != NULL) {
		data = new unsigned char[width * height * 4];
		memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
	}
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
	if (data)
		delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
	unsigned char* rgb = new unsigned char[width * height * 3];
	int		    i, j;

	if (!data)
		return NULL;

	// Divide out the alpha
	for (i = 0; i < height; i++)
	{
		int in_offset = i * width * 4;
		int out_offset = i * width * 3;

		for (j = 0; j < width; j++)
		{
			RGBA_To_RGB(data + (in_offset + j * 4), rgb + (out_offset + j * 3));
		}
	}

	return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char* filename)
{
	TargaImage* out_image = Reverse_Rows();

	if (!out_image)
		return false;

	if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
	{
		cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
		return false;
	}

	delete out_image;

	return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char* filename)
{
	unsigned char* temp_data;
	TargaImage* temp_image;
	TargaImage* result;
	int		        width, height;

	if (!filename)
	{
		cout << "No filename given." << endl;
		return NULL;
	}// if

	temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
	if (!temp_data)
	{
		cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
		width = height = 0;
		return NULL;
	}
	temp_image = new TargaImage(width, height, temp_data);
	free(temp_data);

	result = temp_image->Reverse_Rows();

	delete temp_image;

	return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{

	if ((width == 0) && (height == 0))
	{
		//grayscale before load image
		ClearToBlack();
		cout << "Grayscale: no image\n";
		return false;
	}// if
	else
	{
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				int index = (i * width + j) * 4;
				unsigned char   rgbGray[3];

				RGBA_To_RGB(data + index, rgbGray);
				data[index] = data[index + 1] = data[index + 2] = 0.299 * rgbGray[0] + 0.587 * rgbGray[1] + 0.114 * rgbGray[2];//grayscale function
					//This operation should not affect alpha in any way.
			}
		}
		//for (int i = 0; i < width * height * 4; i += 4)
		//{
		//    unsigned char   rgbGray[3];

		//    RGBA_To_RGB(data + i, rgbGray);
		//    data[i] = data[i + 1] = data[i + 2] = 0.299 * rgbGray[0] + 0.587 * rgbGray[1] + 0.114 * rgbGray[2];//grayscale function
		//    //This operation should not affect alpha in any way.
		//}
		return true;
	}
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
	if ((width == 0) && (height == 0))
	{
		//Quant_Uniform before load image
		ClearToBlack();
		cout << "Quant_Uniform: no image\n";
		return false;
	}// if
	else
	{
		for (int i = 0; i < width * height * 4; i += 4)
		{
			unsigned char   rgbUni[3];

			RGBA_To_RGB(data + i, rgbUni);

			//0-31->0, 224-255->224
			data[i] = rgbUni[0] / 32 * 32;//r: 8 shades of red
			data[i + 1] = rgbUni[1] / 32 * 32;//g: 8 shades of green
			data[i + 2] = rgbUni[2] / 64 * 64;//b: 4 shades of blue
			data[i + 3] = 255;
		}
		return true;
	}
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity()
{
	if ((width == 0) && (height == 0))
	{
		//Quant_Populosity before load image
		ClearToBlack();
		cout << "Quant_Populosity: no image\n";
		return false;
	}// if
	else
	{
		std::vector<populoData> populo;
		populo.reserve(32768);
		for (int i = 0; i < width * height * 4; i += 4)
		{
			unsigned char   rgbUni[3];

			RGBA_To_RGB(data + i, rgbUni);

			//32 shades: 0-7->0, 248-255->248
			populoData temp;
			temp.rgb[0] = data[i] = rgbUni[0] / 8;
			temp.rgb[1] = data[i + 1] = rgbUni[1] / 8;
			temp.rgb[2] = data[i + 2] = rgbUni[2] / 8;

			unsigned int sameInd = -1;
			for (int j = 0; j < populo.size(); j++)
			{
				if (temp == populo[j])//call populoData opoerator==
				{
					//duplicate
					sameInd = j;
					populo.at(sameInd).count++;
					break;
				}
			}
			if (sameInd == -1)
			{
				//unique or empty array
				temp.count = 1;
				populo.push_back(temp);
			}
			//std::cout << (int)temp.rgb[0] << ' ' << (int)temp.rgb[1] << ' ' << (int)temp.rgb[2] << std::endl;
		}


		std::stable_sort(populo.begin(), populo.end(), biggerCount);//most popula
		if (populo.size() > 256)
		{
			//delete the least popular elements
			populo.erase(populo.begin() + 256, populo.end());
		}

		/*for (int i = 0; i < populo.size(); i++) {
			std::cout << i << " : " << (int)populo[i].rgb[0] << ' ' << (int)populo[i].rgb[1] << ' ' << (int)populo[i].rgb[2] << ' ' << populo[i].count << std::endl;
		}*/

		if (populo.size() > 0)
		{
			for (int i = 0; i < width * height * 4; i += 4)
			{
				//find the closest color
				unsigned int minInd = 0;
				double min = euclideanDistance(populo.at(0), data, i);
				for (int j = 1; j < populo.size(); j++) {
					double temp = euclideanDistance(populo.at(j), data, i);
					if (min > temp)
					{
						minInd = j;
						min = temp;
					}
				}

				//change to this color
				for (int j = 0; j < 3; j++)
				{
					data[i + j] = populo.at(minInd).rgb[j] * 8;
				}
				data[i + 3] = 255;
			}
		}
		else
		{
			for (int i = 0; i < width * height * 4; i += 4)
			{
				//change to this color
				for (int j = 0; j < 3; j++)
				{
					data[i + j] *= 8;
				}
				data[i + 3] = 255;
			}
		}
		return true;
	}
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
	if ((width == 0) && (height == 0))
	{
		//Dither_Threshold before load image
		ClearToBlack();
		cout << "Dither_Threshold: no image\n";
		return false;
	}// if
	else
	{
		if (To_Grayscale())
		{
			for (int i = 0; i < width * height * 4; i += 4)
			{
				data[i] = data[i + 1] = data[i + 2] = (unsigned char)thresholdFunc((double)data[i], 255 * 0.5);
				data[i + 3] = (unsigned char)255;
				//std::cout << (int)data[i] << ' ' << (int)data[i + 1] << ' ' << (int)data[i + 2] << std::endl;
			}
			return true;
		}
		else {
			ClearToBlack();
			return false;
		}
	}
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
	if ((width == 0) && (height == 0))
	{
		//Dither_Threshold before load image
		ClearToBlack();
		cout << "Dither_Random: no image\n";
		return false;
	}// if
	else
	{
		if (To_Grayscale())
		{
			for (int i = 0; i < width * height * 4; i += 4)
			{
				data[i] = data[i + 1] = data[i + 2] = (unsigned char)thresholdFunc((double)(data[i] + (rand() % 102) - 51), 255 * 0.5);
				data[i + 3] = (unsigned char)255;
				//std::cout << (int)data[i] << ' ' << (int)data[i + 1] << ' ' << (int)data[i + 2] << std::endl;
			}
			return true;
		}
		else {
			ClearToBlack();
			return false;
		}
	}
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
	if ((width == 0) && (height == 0))
	{
		//Dither_Bright before load image
		ClearToBlack();
		cout << "Dither_FS: no image\n";
		return false;
	}// if
	else
	{
		if (To_Grayscale())
		{
			double* gray = new double[height * width];

			for (int i = 0; i < height; i++)
			{
				for (int j = 0; j < width; j++)
				{
					int dataIndex = (i * width + j);
					gray[dataIndex] = data[dataIndex * 4];
				}
			}

			for (int i = 0; i < height; i++)
			{
				int j = (i % 2 == 0) ? 0 : width - 1;
				for (; (j < width) && (j >= 0);)
				{
					int index = (i * width + j);
					int downIndex = ((i + 1) * width + j);
					double error = gray[index] - thresholdFunc(gray[index], 255 * 0.5);//oldpixel-newpixel

					int rightIndex, downRightIndex, downLeftIndex;
					if (i % 2 == 0)
					{
						rightIndex = index + 1;
						downRightIndex = downIndex + 1;
						downLeftIndex = downIndex - 1;
					}
					else
					{
						rightIndex = index - 1;
						downRightIndex = downIndex - 1;
						downLeftIndex = downIndex + 1;
					}

					if (i % 2 == 0)
					{
						//[row][col+1]->7/16
						if (j + 1 < width)
						{
							gray[rightIndex] += error * (7.0 / 16.0);
						}

						//[row+1][]
						if (i + 1 < height)
						{
							if (j - 1 >= 0)
							{
								//[row+1][col-1]->3/16
								gray[downLeftIndex] += error * (3.0 / 16.0);
							}

							//[row+1][col]->5/16
							gray[downIndex] += error * (5.0 / 16.0);

							if (j + 1 < width)
							{
								//[row+1][col+1]->1/16
								gray[downRightIndex] += error * (1.0 / 16.0);
							}
						}
					}
					else
					{
						//[row][col+1]->7/16
						if (j - 1 >= 0)
						{
							gray[rightIndex] += error * (7.0 / 16.0);
						}

						//[row+1][]
						if (i + 1 < height)
						{
							if (j + 1 < width)
							{
								//[row+1][col-1]->3/16
								gray[downLeftIndex] += error * (3.0 / 16.0);
							}

							//[row+1][col]->5/16
							gray[downIndex] += error * (5.0 / 16.0);

							if (j - 1 >= 0)
							{
								//[row+1][col+1]->1/16
								gray[downRightIndex] += error * (1.0 / 16.0);
							}
						}
					}
					if (i % 2 == 0)
					{
						j++;
					}
					else
					{
						j--;
					}
				}
			}

			for (int i = 0; i < height; i++)
			{
				for (int j = 0; j < width; j++)
				{
					int dataIndex = (i * width + j);

					data[dataIndex * 4] = data[dataIndex * 4 + 1] = data[dataIndex * 4 + 2] = thresholdFunc(gray[dataIndex], 255 * 0.5);
				}
			}
			return true;
		}
		else
		{
			ClearToBlack();
			return false;
		}
	}
}// Dither_FS


	///////////////////////////////////////////////////////////////////////////////
	//
	//      Dither the image while conserving the average brightness.  Return 
	//  success of operation.
	//
	///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
	if ((width == 0) && (height == 0))
	{
		//Dither_Bright before load image
		ClearToBlack();
		cout << "Dither_Bright: no image\n";
		return false;
	}// if
	else
	{
		unsigned long long int intensityCount[256];//256 kind of intensity
		for (int i = 0; i < 255; i++)
		{
			//initialize
			intensityCount[i] = 0;
		}
		unsigned char threshold;

		if (To_Grayscale())//change to grayscale
		{
			unsigned long long int count = 0;//the total intensity

			for (int i = 0; i < width * height * 4; i += 4)
			{
				count += (unsigned long long int)data[i];//the intensity total
				intensityCount[(int)data[i]]++;//the number of a certain intensity
			}

			long long int countReal = count / 255;//[0, 255] to [0, 1]
			if (count % 255 != 0)
			{
				countReal++;
			}

			countReal = (width * height) - countReal;
			//std::cout << countReal << std::endl;

			int ind = 0;
			while ((countReal > 0) && (ind < 256))
			{
				countReal -= intensityCount[ind];
				ind++;
			}
			ind--;
			threshold = (unsigned char)ind;
			//std::cout << ind << ' ' << (int)threshold << std::endl;

			for (int i = 0; i < width * height * 4; i += 4)
			{
				data[i] = data[i + 1] = data[i + 2] = (unsigned char)thresholdFunc((double)data[i], threshold);
				data[i + 3] = (unsigned char)255;
			}
			return true;
		}
		else {
			ClearToBlack();
			return false;
		}
	}
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
	if ((width == 0) && (height == 0))
	{
		//Dither_Threshold before load image
		ClearToBlack();
		cout << "Dither_Cluster: no image\n";
		return false;
	}// if
	else
	{
		if ((width == 0) && (height == 0))
		{
			ClearToBlack();
			return false;
		}
		else {
			double mask[4][4] = // threshold matrix
			{
				{(255 * 0.7059), (255 * 0.3529), (255 * 0.5882), (255 * 0.2353)},
				{(255 * 0.0588), (255 * 0.9412), (255 * 0.8235), (255 * 0.4118)},
				{(255 * 0.4706), (255 * 0.7647), (255 * 0.8824), (255 * 0.1176)},
				{(255 * 0.1765), (255 * 0.5294), (255 * 0.2941), (255 * 0.6471)}
			};
			//grayscaleA = new long double[width * height];
			/*for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					std::cout << mask[i][j] << ' ';
				}
				std::cout << std::endl;
			}*/

			//int count = 0;
			for (int i = 0; i < height; i++)
			{
				for (int j = 0; j < width; j++) {
					int index = (i * width + j) * 4;
					unsigned char   rgbGray[3];

					RGBA_To_RGB(data + index, rgbGray);
					double grayscale = 0.299 * (double)rgbGray[0] + 0.587 * (double)rgbGray[1] + 0.114 * (double)rgbGray[2];//grayscale function
					//count++;
					data[index] = data[index + 1] = data[index + 2] = (unsigned char)thresholdFunc(grayscale, mask[i % 4][j % 4]);//I[x][y] >= mask[x % 4][y % 4]
					data[index + 3] = 255;
				}
			}
			return true;
		}
	}
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
	if ((width == 0) && (height == 0))
	{
		//Dither_Bright before load image
		ClearToBlack();
		cout << "Dither_FS: no image\n";
		return false;
	}// if
	else
	{
		double* rgb = new double[height * width * 3];

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				int dataIndex = (i * width + j);
				rgb[dataIndex * 3] = data[dataIndex * 4];
				rgb[dataIndex * 3 + 1] = data[dataIndex * 4 + 1];
				rgb[dataIndex * 3 + 2] = data[dataIndex * 4 + 2];
			}
		}

		for (int i = 0; i < height; i++)
		{
			int j = (i % 2 == 0) ? 0 : width - 1;
			for (; (j < width) && (j >= 0);)
			{
				int index = (i * width + j) * 3;
				int downIndex = ((i + 1) * width + j) * 3;
				double rgbError[3];
				rgbError[0] = rgb[index] - Quantthreshold(rgb[index], 0);//oldpixel-newpixel
				rgbError[1] = rgb[index + 1] - Quantthreshold(rgb[index + 1], 1);//oldpixel-newpixel
				rgbError[2] = rgb[index + 2] - Quantthreshold(rgb[index + 2], 2);//oldpixel-newpixel

				int rightIndex, downRightIndex, downLeftIndex;
				if (i % 2 == 0)
				{
					rightIndex = index + 3;
					downRightIndex = downIndex + 3;
					downLeftIndex = downIndex - 3;
				}
				else
				{
					rightIndex = index - 3;
					downRightIndex = downIndex - 3;
					downLeftIndex = downIndex + 3;
				}

				if (i % 2 == 0)
				{
					//[row][col+1]->7/16
					if (j + 1 < width)
					{
						rgb[rightIndex] += rgbError[0] * (7.0 / 16.0);
						rgb[rightIndex + 1] += rgbError[1] * (7.0 / 16.0);
						rgb[rightIndex + 2] += rgbError[2] * (7.0 / 16.0);
					}

					//[row+1][]
					if (i + 1 < height)
					{
						if (j - 1 >= 0)
						{
							//[row+1][col-1]->3/16
							rgb[downLeftIndex] += rgbError[0] * (3.0 / 16.0);
							rgb[downLeftIndex + 1] += rgbError[1] * (3.0 / 16.0);
							rgb[downLeftIndex + 2] += rgbError[2] * (3.0 / 16.0);
						}

						//[row+1][col]->5/16
						rgb[downIndex] += rgbError[0] * (5.0 / 16.0);
						rgb[downIndex + 1] += rgbError[1] * (5.0 / 16.0);
						rgb[downIndex + 2] += rgbError[2] * (5.0 / 16.0);

						if (j + 1 < width)
						{
							//[row+1][col+1]->1/16
							rgb[downRightIndex] += rgbError[0] * (1.0 / 16.0);
							rgb[downRightIndex + 1] += rgbError[1] * (1.0 / 16.0);
							rgb[downRightIndex + 2] += rgbError[2] * (1.0 / 16.0);
						}
					}
				}
				else
				{
					//[row][col+1]->7/16
					if (j - 1 >= 0)
					{
						rgb[rightIndex] += rgbError[0] * (7.0 / 16.0);
						rgb[rightIndex + 1] += rgbError[1] * (7.0 / 16.0);
						rgb[rightIndex + 2] += rgbError[2] * (7.0 / 16.0);
					}

					//[row+1][]
					if (i + 1 < height)
					{
						if (j + 1 < width)
						{
							//[row+1][col-1]->3/16
							rgb[downLeftIndex] += rgbError[0] * (3.0 / 16.0);
							rgb[downLeftIndex + 1] += rgbError[1] * (3.0 / 16.0);
							rgb[downLeftIndex + 2] += rgbError[2] * (3.0 / 16.0);
						}

						//[row+1][col]->5/16
						rgb[downIndex] += rgbError[0] * (5.0 / 16.0);
						rgb[downIndex + 1] += rgbError[1] * (5.0 / 16.0);
						rgb[downIndex + 2] += rgbError[2] * (5.0 / 16.0);

						if (j - 1 >= 0)
						{
							//[row+1][col+1]->1/16
							rgb[downRightIndex] += rgbError[0] * (1.0 / 16.0);
							rgb[downRightIndex + 1] += rgbError[1] * (1.0 / 16.0);
							rgb[downRightIndex + 2] += rgbError[2] * (1.0 / 16.0);
						}
					}
				}
				if (i % 2 == 0)
				{
					j++;
				}
				else
				{
					j--;
				}
			}
		}

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				int dataIndex = (i * width + j);

				data[dataIndex * 4] = Quantthreshold(rgb[dataIndex * 3], 0);
				data[dataIndex * 4 + 1] = Quantthreshold(rgb[dataIndex * 3 + 1], 1);
				data[dataIndex * 4 + 2] = Quantthreshold(rgb[dataIndex * 3 + 2], 2);
				data[dataIndex * 4 + 3] = 255;
			}
		}
		return true;
	}
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Over: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_In: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Out: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Atop: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Xor: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
	if (!pImage)
		return false;

	if (width != pImage->width || height != pImage->height)
	{
		cout << "Difference: Images not the same size\n";
		return false;
	}// if
	//long double mask[4][4] = // threshold matrix
	//{
	//    {(255 * 0.7059), (255 * 0.3529), (255 * 0.5882), (255 * 0.2353)},
	//    {(255 * 0.0588), (255 * 0.9412), (255 * 0.8235), (255 * 0.4118)},
	//    {(255 * 0.4706), (255 * 0.7647), (255 * 0.8824), (255 * 0.1176)},
	//    {(255 * 0.1765), (255 * 0.5294), (255 * 0.2941), (255 * 0.6471)}
	//};
	//int count = 0;
	//for (int i = 0; i < height; i++)
	//{
	//    for (int j = 0; j < width; j++)
	//    {
	//        int index = (i * width + j) * 4;
	//        unsigned char        rgb1[3];
	//        unsigned char        rgb2[3];

	//        RGBA_To_RGB(data + index, rgb1);
	//        RGBA_To_RGB(pImage->data + index, rgb2);

	//        data[index] = abs(rgb1[0] - rgb2[0]);
	//        data[index + 1] = abs(rgb1[1] - rgb2[1]);
	//        data[index + 2] = abs(rgb1[2] - rgb2[2]);
	//        data[index + 3] = 255;
	//        if ((data[index] != 0) && (data[index + 1] != 0) && (data[index + 2] != 0))
	//        {
	//            count++;
	//            std::cout<< i<< " "<< j <<": " << grayscaleA[(i * width + j)] << " " << mask[i % 4][j % 4] << std::endl;
	//        }
	//    }
	//}
	//std::cout << count << std::endl;
	for (int i = 0; i < width * height * 4; i += 4)
	{
		unsigned char        rgb1[3];
		unsigned char        rgb2[3];

		RGBA_To_RGB(data + i, rgb1);
		RGBA_To_RGB(pImage->data + i, rgb2);

		data[i] = abs(rgb1[0] - rgb2[0]);
		data[i + 1] = abs(rgb1[1] - rgb2[1]);
		data[i + 2] = abs(rgb1[2] - rgb2[2]);
		data[i + 3] = 255;
	}

	return true;
}// Difference


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Box()
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Box before load image
		ClearToBlack();
		cout << "Filter_Box: no image\n";
		return false;
	}// if
	else
	{
		unsigned char* temp = new unsigned char[width * height * 4];
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++) {
				//find the range limit
				int left = j - 2;
				int right = j + 2;
				int up = i - 2;
				int down = i + 2;
				if (left < 0)    left = 0;
				if (right >= width)    right = width - 1;
				if (up < 0)     up = 0;
				if (down >= height)     down = height - 1;;


				int rgbTotal[3] = { 0,0,0 };//add the neer together
				for (int l = up; l <= down; l++)
				{
					for (int m = left; m <= right; m++)
					{
						int ind = ((l * width) + m) * 4;
						unsigned char   rgbNeer[3];

						RGBA_To_RGB(data + ind, rgbNeer);

						for (int k = 0; k < 3; k++)
						{
							rgbTotal[k] += (int)rgbNeer[k];
						}
					}
				}

				int ind = ((i * width) + j) * 4;
				for (int k = 0; k < 3; k++)
				{
					rgbTotal[k] /= 25;//average
					temp[ind + k] = (unsigned char)rgbTotal[k];//assign to the data
				}
				temp[ind + 3] = 255;
			}
		}
		for (int i = 0; i < width * height * 4; i++)
		{
			data[i] = temp[i];
		}
		return true;
	}
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Bartlett before load image
		ClearToBlack();
		cout << "Filter_Bartlett: no image\n";
		return false;
	}// if
	else
	{
		int bartlett[5][5];
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				bartlett[i][j] = bartlett[4 - i][j] = bartlett[i][4 - j] = bartlett[4 - i][4 - j] = (i + 1) * (j + 1);
			}
		}

		unsigned char* temp = new unsigned char[width * height * 4];
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++) {
				//find the matrix limit
				int LLimit = 0;
				int uLimit = 0;
				//find the range limit
				int left = j - 2;
				int right = j + 2;
				int up = i - 2;
				int down = i + 2;
				if (left < 0)
				{
					LLimit = -left;
					left = 0;
				}
				if (right >= width)
				{
					right = width - 1;
				}
				if (up < 0)
				{
					uLimit = -up;
					up = 0;
				}
				if (down >= height)
				{
					down = height - 1;
				}


				int rgbTotal[3] = { 0, 0, 0 };//add the neer together

				int BU = uLimit;
				for (int l = up; l <= down; l++)
				{
					int BL = LLimit;
					for (int m = left; m <= right; m++)
					{
						int ind = ((l * width) + m) * 4;
						unsigned char   rgbNeer[3];

						RGBA_To_RGB(data + ind, rgbNeer);

						for (int k = 0; k < 3; k++)
						{
							rgbTotal[k] += ((int)rgbNeer[k] * bartlett[BU][BL]);
						}
						BL++;
					}
					BU++;
				}
				int ind = ((i * width) + j) * 4;
				for (int k = 0; k < 3; k++)
				{
					rgbTotal[k] /= 81;//average
					temp[ind + k] = (unsigned char)rgbTotal[k];//assign to the data
				}
				temp[ind + 3] = 255;
			}
		}
		for (int i = 0; i < width * height * 4; i++)
		{
			data[i] = temp[i];
		}
		return true;
	}
}// Filter_Bartlett


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Gaussian before load image
		ClearToBlack();
		cout << "Filter_Gaussian: no image\n";
		return false;
	}// if
	else
	{
		int guassian[5][5];
		guassian[0][0] = 1; guassian[0][1] = 4; guassian[0][2] = 6;
		guassian[1][0] = 4; guassian[1][1] = 16; guassian[1][2] = 24;
		guassian[2][0] = 6; guassian[2][1] = 24; guassian[2][2] = 36;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				guassian[4 - i][j] = guassian[i][4 - j] = guassian[4 - i][4 - j] = guassian[i][j];
			}
		}
		/*for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				std::cout << guassian[i][j] << ' ';
			}
			std::cout << std::endl;
		}*/

		unsigned char* temp = new unsigned char[width * height * 4];
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++) {
				int LLimit = 0;
				int uLimit = 0;
				//find the range limit
				int left = j - 2;
				int right = j + 2;
				int up = i - 2;
				int down = i + 2;
				if (left < 0)
				{
					LLimit = -left;
					left = 0;
				}
				if (right >= width)
				{
					right = width - 1;
				}
				if (up < 0)
				{
					uLimit = -up;
					up = 0;
				}
				if (down >= height)
				{
					down = height - 1;
				}


				int rgbTotal[3] = { 0, 0, 0 };//add the neer together

				int BU = uLimit;
				for (int l = up; l <= down; l++)
				{
					int BL = LLimit;
					for (int m = left; m <= right; m++)
					{
						int ind = ((l * width) + m) * 4;
						unsigned char   rgbNeer[3];

						RGBA_To_RGB(data + ind, rgbNeer);

						for (int k = 0; k < 3; k++)
						{
							rgbTotal[k] += ((int)rgbNeer[k] * guassian[BU][BL]);
						}
						BL++;
					}
					BU++;
				}
				int ind = ((i * width) + j) * 4;
				for (int k = 0; k < 3; k++)
				{
					rgbTotal[k] /= 256;//average
					temp[ind + k] = (unsigned char)rgbTotal[k];//assign to the data
				}
				temp[ind + 3] = 255;
			}
		}
		for (int i = 0; i < width * height * 4; i++)
		{
			data[i] = temp[i];
		}
		return true;
	}
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N(unsigned int N)
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Gaussian_N before load image
		ClearToBlack();
		cout << "Filter_Gaussian_N: no image\n";
		return false;
	}// if
	else
	{
		int count = 0;
		int** pascalRow = new int* [N];
		for (int i = 0; i < N; i++) {
			pascalRow[i] = new int[N];
		}

		//calculate pascal triangle
		for (int i = 0; i < N; i++) {
			pascalRow[0][i] = 1;
			for (int j = N - 1; j > N - 1 - i; j--)
			{
				pascalRow[0][i] *= j;
			}
			int divisor = 1;
			for (int j = 1; j <= i; j++)
			{
				divisor *= j;
			}
			pascalRow[0][i] /= divisor;
			pascalRow[i][0] = pascalRow[0][i];
			count += pascalRow[i][0];
			//C(N¨úi) - > (row)*(row-1)*..*(row-i+1)
			//pascal[i]=
		}
		count = pow(count, 2);//divisor

		for (int i = 0; i < N; i++) {
			for (int j = 0; j < N; j++) {
				pascalRow[i][j] = pascalRow[i][0] * pascalRow[0][j];
			}
		}


		unsigned char* temp = new unsigned char[width * height * 4];
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++) {
				int LLimit = 0;
				int uLimit = 0;
				//find the range limit
				int left = j - 2;
				int right = j + 2;
				int up = i - 2;
				int down = i + 2;
				if (left < 0)
				{
					LLimit = -left;
					left = 0;
				}
				if (right >= width)
				{
					right = width - 1;
				}
				if (up < 0)
				{
					uLimit = -up;
					up = 0;
				}
				if (down >= height)
				{
					down = height - 1;
				}


				int rgbTotal[3] = { 0, 0, 0 };//add the neer together

				int BU = uLimit;
				for (int l = up; l <= down; l++)
				{
					int BL = LLimit;
					for (int m = left; m <= right; m++)
					{
						int ind = ((l * width) + m) * 4;
						unsigned char   rgbNeer[3];

						RGBA_To_RGB(data + ind, rgbNeer);

						for (int k = 0; k < 3; k++)
						{
							if ((BL < N)&&(BU<N))
							{
								rgbTotal[k] += ((int)rgbNeer[k] * pascalRow[BU][BL]);
							}
						}
							BL++;
					}
						BU++;
				}
				int ind = ((i * width) + j) * 4;
				for (int k = 0; k < 3; k++)
				{
					rgbTotal[k] /= count;//average
					temp[ind + k] = (unsigned char)rgbTotal[k];//assign to the data
				}
				temp[ind + 3] = 255;
			}
		}
		for (int i = 0; i < width * height * 4; i++)
		{
			data[i] = temp[i];
		}
		return true;
	}
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Bartlett before load image
		ClearToBlack();
		cout << "Filter_Enhance: no image\n";
		return false;
	}// if
	else
	{
		int* enhanceData = new int[width * height * 4];

		for (int i = 0; i < height*width*4; i++)
		{
			enhanceData[i] = data[i];
		}
		Filter_Gaussian();

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				for (int k = 0; k < 3; k++)
				{
					int doubleIndex = (i * width + j) * 4 + k;
					if (enhanceData[doubleIndex]- data[doubleIndex]>0)
					{
						data[doubleIndex] = enhanceData[doubleIndex] - data[doubleIndex];
					}
					else
					{
						data[doubleIndex] = 0;
					}
				}
			}
		}
		return true;
	}
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Bartlett before load image
		ClearToBlack();
		cout << "Filter_Enhance: no image\n";
		return false;
	}// if
	else
	{
		int* enhanceData = new int[width * height * 4];

		for (int i = 0; i < height * width * 4; i++)
		{
			enhanceData[i] = data[i];
		}
		Filter_Edge();

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				for (int k = 0; k < 3; k++)
				{
					int doubleIndex = (i * width + j) * 4 + k;
					if (enhanceData[doubleIndex] + data[doubleIndex] < 255)
					{
						data[doubleIndex] = enhanceData[doubleIndex] + data[doubleIndex];
					}
					else
					{
						data[doubleIndex] = 255;
					}
				}
			}
		}
	}
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint()
{
	ClearToBlack();
	return false;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Bartlett before load image
		ClearToBlack();
		cout << "Half_Size: no image\n";
		return false;
	}// if
	else
	{
		int bartlett[3][3] = { {1,2,1},{2,4,2},{1,2,1} };

		unsigned char* temp = new unsigned char[width * height * 4];
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++) {
				//find the matrix limit
				int LLimit = 0;
				int uLimit = 0;
				//find the range limit
				int left = j - 1;
				int right = j + 1;
				int up = i - 1;
				int down = i + 1;
				if (left < 0)
				{
					LLimit = -left;
					left = 0;
				}
				if (right >= width)
				{
					right = width - 1;
				}
				if (up < 0)
				{
					uLimit = -up;
					up = 0;
				}
				if (down >= height)
				{
					down = height - 1;
				}


				int rgbTotal[3] = { 0, 0, 0 };//add the neer together

				int BU = uLimit;
				for (int l = up; l <= down; l++)
				{
					int BL = LLimit;
					for (int m = left; m <= right; m++)
					{
						int ind = ((l * width) + m) * 4;
						unsigned char   rgbNeer[3];

						RGBA_To_RGB(data + ind, rgbNeer);

						for (int k = 0; k < 3; k++)
						{
							rgbTotal[k] += ((int)rgbNeer[k] * bartlett[BU][BL]);
						}
						BL++;
					}
					BU++;
				}
				int ind = ((i * width) + j) * 4;
				for (int k = 0; k < 3; k++)
				{
					rgbTotal[k] /= 16;//average
					temp[ind + k] = (unsigned char)rgbTotal[k];//assign to the data
				}
				temp[ind + 3] = 255;
			}
		}
		delete[] temp;
		unsigned char* half = new unsigned char[(height / 2) * (width / 2) * 4];
		for (int i = 0; i < height / 2; i++)
		{
			for (int j = 0; j < width / 2; j++)
			{
				int halfIndex = (i * (width / 2) + j) * 4;
				int index = ((i * 2) * width + (j * 2)) * 4;
				half[halfIndex] = data[index];
				half[halfIndex + 1] = data[index + 1];
				half[halfIndex + 2] = data[index + 2];
				half[halfIndex + 3] = data[index + 3];
			}
		}

		for (int i = 0; i < height / 2; i++)
		{
			for (int j = 0; j < width / 2; j++)
			{
				int halfIndex = (i * (width / 2) + j) * 4;
				data[halfIndex] = half[halfIndex];
				data[halfIndex + 1] = half[halfIndex + 1];
				data[halfIndex + 2] = half[halfIndex + 2];
				data[halfIndex + 3] = half[halfIndex + 3];
			}
		}
		delete[] half;
		height /= 2;
		width /= 2;
		return true;
	}
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Bartlett before load image
		ClearToBlack();
		cout << "Half_Size: no image\n";
		return false;
	}// if
	else
	{
		int bartlettEven[3][3] = { {1,2,1},{2,4,2},{1,2,1} };
		int bartlettOdd[4][4] = { {1,3,3,1},{3,9,9,3},{3,9,9,3},{1,3,3,1 } };
		int bartlettOther[4][3] = { {1,2,1},{3,6,3},{3,6,3},{1,2,1 } };

		unsigned char* doubleData = new unsigned char[(width * 2) * (height * 2) * 4];

		for (int i = 0; i < (height * 2); i++)
		{
			for (int j = 0; j < (width * 2); j++)
			{
				int doubleIndex = (i * width * 2 + j) * 4;
				int Urow = i / 2 - 1;
				int Drow = i / 2 + 1;
				int Lcol = j / 2 - 1;
				int Rcol = j / 2 + 1;
				//std::cout << Urow << " " << Drow << " " << Lcol << " " << Rcol << " ";
				if ((i % 2 == 0) && (j % 2 == 0))
				{

				}
				else if ((i % 2 == 1) && (j % 2 == 1))
				{
					Drow++;
					Rcol++;
				}
				else
				{
					Drow++;
				}
				int rgbTotal[3] = { 0, 0, 0 };//add the neer together
				for (int k = Urow; k <= Drow; k++)
				{
					for (int l = Lcol; l <= Rcol; l++)
					{

						if ((k >= 0) && (k < height) && (l >= 0) && (l < width))
						{
							int ind = ((k * width) + l) * 4;
							//std::cout << k << " " << l << " ";
							unsigned char   rgbNeer[3];

							RGBA_To_RGB(data + ind, rgbNeer);

							if ((i % 2 == 0) && (j % 2 == 0))
							{
								for (int m = 0; m < 3; m++)
								{
									rgbTotal[m] += ((int)rgbNeer[m] * bartlettEven[k - Urow][l - Lcol]);
								}
							}
							else if ((i % 2 == 1) && (j % 2 == 1))
							{
								for (int m = 0; m < 3; m++)
								{
									rgbTotal[m] += ((int)rgbNeer[m] * bartlettOdd[k - Urow][l - Lcol]);
								}
							}
							else
							{
								for (int m = 0; m < 3; m++)
								{
									rgbTotal[m] += ((int)rgbNeer[m] * bartlettOther[k - Urow][l - Lcol]);
								}
							}
						}
					}
					//std::cout << std::endl;
				}
				if ((i % 2 == 0) && (j % 2 == 0))
				{
					for (int m = 0; m < 3; m++)
					{
						doubleData[doubleIndex + m] = rgbTotal[m] / 16;
					}
				}
				else if ((i % 2 == 1) && (j % 2 == 1))
				{
					for (int m = 0; m < 3; m++)
					{
						doubleData[doubleIndex + m] = rgbTotal[m] / 64;
					}
				}
				else
				{
					for (int m = 0; m < 3; m++)
					{
						doubleData[doubleIndex + m] = rgbTotal[m] / 32;
					}
				}
				doubleData[doubleIndex + 3] = 255;
			}
		}

		data = new unsigned char[height * width * 16];
		for (int i = 0; i < height * 2; i++)
		{
			for (int j = 0; j < width * 2; j++)
			{
				int doubleIndex = (i * width * 2 + j) * 4;
				data[doubleIndex] = doubleData[doubleIndex];
				data[doubleIndex + 1] = doubleData[doubleIndex + 1];
				data[doubleIndex + 2] = doubleData[doubleIndex + 2];
				data[doubleIndex + 3] = 255;
			}
		}
		height *= 2;
		width *= 2;
		return true;
	}
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale)
{
	ClearToBlack();
	return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
	if ((width == 0) && (height == 0))
	{
		//Filter_Bartlett before load image
		ClearToBlack();
		cout << "Rotate: no image\n";
		return false;
	}// if
	else
	{
		int bartlettHighPass[4][4] = { {1,2,2,1},{2,4,4,2},{2,4,4,2},{1,2,2,1} };

		unsigned char* enhanceData = new unsigned char[width * height * 4];

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				int doubleIndex = (i * width + j) * 4;
				int Urow = i - 1;
				int Drow = i + 1;
				int Lcol = j - 1;
				int Rcol = j + 1;
				//std::cout << Urow << " " << Drow << " " << Lcol << " " << Rcol << " ";

				int rgbTotal[3] = { 0, 0, 0 };//add the neer together
				for (int k = Urow; k <= Drow; k++)
				{
					for (int l = Lcol; l <= Rcol; l++)
					{

						if ((k >= 0) && (k < height) && (l >= 0) && (l < width))
						{
							int ind = ((k * width) + l) * 4;
							//std::cout << k << " " << l << " ";
							unsigned char   rgbNeer[3];

							RGBA_To_RGB(data + ind, rgbNeer);


							for (int m = 0; m < 3; m++)
							{
								rgbTotal[m] += ((int)rgbNeer[m] * bartlettHighPass[k - Urow][l - Lcol]);
							}
						}
					}
					//std::cout << std::endl;
				}

				for (int m = 0; m < 3; m++)
				{
					enhanceData[doubleIndex + m] = rgbTotal[m] / 16;
				}
			}
		}

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				int doubleIndex = (i * width + j) * 4;
				data[doubleIndex] = enhanceData[doubleIndex];
				data[doubleIndex + 1] = enhanceData[doubleIndex + 1];
				data[doubleIndex + 2] = enhanceData[doubleIndex + 2];
				data[doubleIndex + 3] = enhanceData[doubleIndex + 3];
			}
		}
		return true;
	}
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char* rgba, unsigned char* rgb)
{
	const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

	unsigned char  alpha = rgba[3];

	if (alpha == 0)
	{
		rgb[0] = BACKGROUND[0];
		rgb[1] = BACKGROUND[1];
		rgb[2] = BACKGROUND[2];
	}
	else
	{
		float	alpha_scale = (float)255 / (float)alpha;
		int	val;
		int	i;

		for (i = 0; i < 3; i++)
		{
			val = (int)floor(rgba[i] * alpha_scale);
			if (val < 0)
				rgb[i] = 0;
			else if (val > 255)
				rgb[i] = 255;
			else
				rgb[i] = val;
		}
	}
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
	unsigned char* dest = new unsigned char[width * height * 4];
	TargaImage* result;
	int 	        i, j;

	if (!data)
		return NULL;

	for (i = 0; i < height; i++)
	{
		int in_offset = (height - i - 1) * width * 4;
		int out_offset = i * width * 4;

		for (j = 0; j < width; j++)
		{
			dest[out_offset + j * 4] = data[in_offset + j * 4];
			dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
			dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
			dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
		}
	}

	result = new TargaImage(width, height, dest);
	delete[] dest;
	return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
	memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
	int radius_squared = (int)s.radius * (int)s.radius;
	for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
		for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
			int x_loc = (int)s.x + x_off;
			int y_loc = (int)s.y + y_off;
			// are we inside the circle, and inside the image?
			if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
				int dist_squared = x_off * x_off + y_off * y_off;
				if (dist_squared <= radius_squared) {
					data[(y_loc * width + x_loc) * 4 + 0] = s.r;
					data[(y_loc * width + x_loc) * 4 + 1] = s.g;
					data[(y_loc * width + x_loc) * 4 + 2] = s.b;
					data[(y_loc * width + x_loc) * 4 + 3] = s.a;
				}
				else if (dist_squared == radius_squared + 1) {
					data[(y_loc * width + x_loc) * 4 + 0] =
						(data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
					data[(y_loc * width + x_loc) * 4 + 1] =
						(data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
					data[(y_loc * width + x_loc) * 4 + 2] =
						(data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
					data[(y_loc * width + x_loc) * 4 + 3] =
						(data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
	unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
	radius(iradius), x(ix), y(iy), r(ir), g(ig), b(ib), a(ia)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//      Test whether the rbg are the same 
//
///////////////////////////////////////////////////////////////////////////////
const bool populoData::operator==(const populoData& last)
{
	bool result = true;
	for (int i = 0; i < 3; i++)
	{
		if (rgb[i] != last.rgb[i])
		{
			result = false;
			break;
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Test which is bigger 
//
///////////////////////////////////////////////////////////////////////////////
const bool biggerCount(const populoData& first, const populoData& last)
{
	return first.count > last.count;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Calulate the euclidean distance between the origin color and the 256
//  popular color(sqrt((r1-r2)^2 + (g1-g2)^2 + (b1-b2)^2))
///////////////////////////////////////////////////////////////////////////////
const double euclideanDistance(const populoData& popular, const unsigned char* origin, const unsigned int ind)
{
	double answer = 0;
	for (int i = 0; i < 3; i++) {
		answer += pow(((int)origin[ind + i] - (int)popular.rgb[i]), 2);
	}
	answer = sqrtf(answer);
	return answer;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Calulate dither shredhold
///////////////////////////////////////////////////////////////////////////////
const double thresholdFunc(const double gray, const double threshold)
{
	if (gray >= threshold)
	{
		return 255;
	}
	else
	{
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//      Calulate quant shredhold
///////////////////////////////////////////////////////////////////////////////
const double Quantthreshold(const double rgb, const int color)
{
	double resultColor = rgb;
	switch (color)
	{
	case 0:
	case 1:
		resultColor = resultColor / 32 * 32;
		if (rgb >= 237)
		{
			resultColor = 255;
		}
		else if (rgb >= 200.5)
		{
			resultColor = 219;
		}
		else if (rgb >= 164)
		{
			resultColor = 182;
		}
		else if (rgb >= 127.5)
		{
			resultColor = 146;
		}
		else if (rgb >= 91)
		{
			resultColor = 109;
		}
		else if (rgb >= 54.5)
		{
			resultColor = 73;
		}
		else if (rgb >= 18)
		{
			resultColor = 36;
		}
		else
		{
			resultColor = 0;
		}
		break;
	case 2:
		resultColor = resultColor / 64 * 64;
		if (rgb >= 212.5)
		{
			resultColor = 255;
		}
		else if (rgb >= 127.5)
		{
			resultColor = 170;
		}
		else if (rgb >= 29)
		{
			resultColor = 85;
		}
		else
		{
			resultColor = 0;
		}
		break;
	default:
		break;
	}
	return resultColor;
}
