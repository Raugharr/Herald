/**
 * Author: David Brotz
 * File: MapGenerator.c
 */


#include "MapGenerator.h"

#include "Tile.h"

#include "../sys/Math.h"
#include "../sys/Log.h"

#include <math.h>
#include <float.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define PI 3.14158265

static uint8_t g_PerlinPerm[] = {
      151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	  190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20,
	  125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220,
	  105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
	  135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255,
	  82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221,
	  153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 
	  251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106,
	  157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93 
};

float Interpolate(float x0, float x1, float alpha) {
		return x0 * (1 - alpha) + alpha * x1;
}

float lerp(float Weight, float x0, float x1) {
	return x0 + Weight * (x1 - x0);
}

double Fade(double x) {
	return x * x * x * (x * (x * 6 - 15) + 10);
}

float PerlinGrad(uint8_t Hash, float x, float y, float z) {
  int h = Hash & 15;
  float u = (h < 8) ? x : y;
  float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
} 

double BrownNoiseGen() {
	return sin(Rand() / (double)(~((uint64_t)0)) / 3);// * (PI / 180));
}

void WhiteNoiseGen(uint16_t Width, uint16_t Height, float* Array) {
	uint32_t Area = Width * Height;

	for(int i = 0; i < Area; ++i) {
		Array[i] = Rand() / (double)(~((uint64_t)0));
	}
}

void SmoothNoise(uint16_t Width, uint16_t Height, float* Array, uint8_t Octave) {
	//uint32_t Area = Width * Height;
	uint32_t SamplePeriod = (1 << Octave);
	uint32_t x0 = 0;
	uint32_t x1 = 0;
	uint32_t y0 = 0;
	uint32_t y1 = 0;
	float SampleFrequency = 1.0f / SamplePeriod;
	float xBlend = 0;
	float yBlend = 0;
	
	for(int x = 0; x < Width; ++x) {
		x0 = (x / SamplePeriod) * SamplePeriod;
		x1 = (x + SamplePeriod) % Width;
		xBlend = (x - x0) * SampleFrequency;

		for(int y = 0; y < Height; ++y) {
			y0 = (y / SamplePeriod) * SamplePeriod;
			y1 = (y + SamplePeriod) % Width;
			yBlend = (y - y0) * SampleFrequency;
			float Top = Interpolate(Array[y0 * Height + x0], Array[y0 * Height + x1], xBlend);
			float Bottom = Interpolate(Array[y1 * Height + x0], Array[y1 * Height + x1], xBlend);
			Array[y * Height + x] = Interpolate(Top, Bottom, yBlend);
		}
	}
}
float PerlinNoise(float x, float y, float z) {
      float fx = floor(x);
      float fy = floor(y);
      float fz = floor(z);

      int gx = (int) fx & 0xFF;
      int gy = (int) fy & 0xFF;
      int gz = (int) fz & 0xFF;

      float u = Fade(x -= fx);
      float v = Fade(y -= fy);
      float w = Fade(z -= fz);

      int a0 = g_PerlinPerm[gx + 0] + gy;
      int b0 = g_PerlinPerm[gx + 1] + gy;
      int aa = g_PerlinPerm[a0 + 0] + gz;
      int ab = g_PerlinPerm[a0 + 1] + gz;
      int ba = g_PerlinPerm[b0 + 0] + gz;
      int bb = g_PerlinPerm[b0 + 1] + gz;

      float a1 = PerlinGrad(g_PerlinPerm[bb + 1], x - 1, y - 1, z - 1);
      float a2 = PerlinGrad(g_PerlinPerm[ab + 1], x - 0, y - 1, z - 1);
      float a3 = PerlinGrad(g_PerlinPerm[ba + 1], x - 1, y - 0, z - 1);
      float a4 = PerlinGrad(g_PerlinPerm[aa + 1], x - 0, y - 0, z - 1);
      float a5 = PerlinGrad(g_PerlinPerm[bb + 0], x - 1, y - 1, z - 0);
      float a6 = PerlinGrad(g_PerlinPerm[ab + 0], x - 0, y - 1, z - 0);
      float a7 = PerlinGrad(g_PerlinPerm[ba + 0], x - 1, y - 0, z - 0);
      float a8 = PerlinGrad(g_PerlinPerm[aa + 0], x - 0, y - 0, z - 0);

      float a2_1 = lerp(u, a2, a1);
      float a4_3 = lerp(u, a4, a3);
      float a6_5 = lerp(u, a6, a5);
      float a8_7 = lerp(u, a8, a7);
      float a8_5 = lerp(v, a8_7, a6_5);
      float a4_1 = lerp(v, a4_3, a2_1);
      float a8_1 = lerp(w, a8_5, a4_1);

      return a8_1;
}

void CreatePerlinNoise(uint16_t Width, uint16_t Height, float* HeightMap) {
	double XFreq = 1.0 / Width * 8;
	double YFreq = 1.0 / Height * 9;
	double Amplitude = 1.0f;

	for(int x = 0; x < Width; ++x) {
		for(int y = 0; y < Height; ++y) {
			HeightMap[y * Width + x] = PerlinNoise(x * XFreq, y * YFreq, 0.8) * Amplitude;
		}
	}
}

void Fault(uint16_t Width, uint16_t Length, float* Array, float Disp) {
	uint64_t Val = Rand();
	float a = sin(Val);
	float b = cos(Val);
	float d = sqrt(Width * Width + Length * Length);
	//float c = 1;
	double r = (double)Rand();
	double c = ((r / ((uint64_t)~(0LL)))) * d - (d / 2);
	double Dist = 0;
	//double Freq = Width * Length / 360;

	for(int x = 0; x < Width; ++x) {
		for(int y = 0; y < Length; ++y) {
			Dist = a * x + b * y - c;
			if(Dist > 0) {
				Array[y * Width + x] += Disp * cos(Dist / 2);
//					if(Array[y * Length + x] > 1)
//						Array[y * Length + x] = 1;
			} else {
				Array[y * Width + x] -= Disp * cos(Dist / 2);
//			if(Array[y * Length + x] < 0)
//				Array[y * Length + x] = 0;
			}
		}
	}
}

void BrownNoise(uint16_t Width, uint16_t Height, float* Input) {
	float* Array = calloc(Width * Height, sizeof(float));

	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Height; ++y) {
			Array[y * Width + x] = Input[y * Width + x];
		}
	}
	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Height; ++y) {
			uint8_t Neighbors = 0;
			uint32_t Origin = y * Width + x;

			if(x > 0) {
				Array[Origin] += Input[y * Width + (x - 1)];
				++Neighbors;
			}
			if(y > 0) {
				Array[Origin] += Input[(y - 1) * Width + x];
				++Neighbors;
			}
			if((x + 1) < Height ) {
				 Array[Origin] += Input[y * Width + (x + 1)];
				++Neighbors;
			}
			if((y + 1) < Width) {
				Array[Origin] += Input[(y + 1) * Width + x];
			}
			Array[Origin] /= Neighbors;
		}
	}
	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Height; ++y) {
			Input[y * Width + x] = Array[y * Width + x];
		}
	}
	free(Array);
}

void Erode(uint16_t Width, uint16_t Height, float* Input, float Split) {
	uint8_t* Array = calloc(Width * Height, sizeof(uint8_t));

	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Height; ++y) {
			if(Input[y * Width + x] >= Split) {
				Array[y * Width + x] = 1;
			} else {
				Array[y * Width + x] = 0;
			}
		}
	}
	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Height; ++y) {
			if(Array[y * Width + x] == 1) {
				if(x > 0 && Array[y * Width + (x - 1)] == 0) Array[y * Width + (x - 1)] = 2;
				if(y > 0 && Array[(y - 1) * Width + x] == 0) Array[(y - 1) * Width + x] = 2;
				if((x + 1) < Height && Array[y * Width + (x + 1)] == 0) Array[y * Width + (x + 1)] = 2;
				if((y + 1) < Width && Array[(y + 1) * Width + x] == 0) Array[(y + 1) * Width + x]  = 2;
			}
		}
	}
	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Height; ++y) {
			if(Array[y * Width + x] == 2)
				Input[y * Width + x] = Split;
		}
	}
	free(Array);
}

void CreateHeightMap(uint16_t Width, uint16_t Height, float* HeightMap, float DispStart, float DispEnd, uint16_t FaultTimes) {
	float Disp = DispStart;
	float Min = FLT_MAX;
	float Max = FLT_MIN;

//	for(int i = 0; i < Width * Height; ++i)
//			HeightMap[i] = 0.5f;
	for(int i = 0; i < FaultTimes; ++i) {
		Fault(Width, Height, HeightMap, Disp);
		Disp = DispStart + (i / ((float)FaultTimes)) * (DispEnd - DispStart);
	}
	for(int i = 0; i < Width * Height; ++i) {
		if(HeightMap[i] < Min)
			Min = HeightMap[i];	
		if(HeightMap[i] > Max)
			Max = HeightMap[i];
	}
	for(int i = 0; i < Width * Height; ++i) {
		//HeightMap[i] = (HeightMap[i] - Min) / (Max - Min);
//		if(HeightMap[i] < 0)
//			HeightMap[i] = 0;
//		else if(HeightMap[i] > 1.0)
//			HeightMap[i] = 1.0;
	}
}

void HeightMapTexture(const char* Name, uint16_t Width, uint16_t Height, float* HeightMap) {
	SDL_Surface* Surface = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Height; ++y) {
			uint32_t Val = ((uint8_t)(HeightMap[y * Width + x] * 0xFF)) & 0xFF;
			uint32_t Pixel = (Val << 16) | (Val << 8) | Val; 
			//uint32_t Pixel = Val & (Val << 8) & (Val << 16);

			((uint32_t*)Surface->pixels)[y * Surface->w + x] = Pixel;
		}
	}
	if(SDL_SaveBMP(Surface, Name) != 0) {
		Log(ELOG_ERROR, "%s", SDL_GetError());
	}
	SDL_FreeSurface(Surface);
}
