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

float Interpolate(float x0, float x1, float alpha) {
		return x0 * (1 - alpha) + alpha * x1;
}

double Fade(double x) {
	return x * x * x * (x * (x * 6 - 15) + 10);
}

double BrownNoiseGen() {
	return sin(Rand() / (double)(~((uint64_t)0)) / 3);// * (PI / 180));
}

void WhiteNoiseGen(uint16_t Width, uint16_t Length, float* Array) {
	uint32_t Area = Width * Length;

	for(int i = 0; i < Area; ++i) {
		Array[i] = Rand() / (double)(~((uint64_t)0));
	}
}

void SmoothNoise(uint16_t Width, uint16_t Length, float* Array, uint8_t Octave) {
	uint32_t Area = Width * Length;
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

		for(int y = 0; y < Length; ++y) {
			y0 = (y / SamplePeriod) * SamplePeriod;
			y1 = (y + SamplePeriod) % Width;
			yBlend = (y - y0) * SampleFrequency;
			float Top = Interpolate(Array[y0 * Length + x0], Array[y0 * Length + x1], xBlend);
			float Bottom = Interpolate(Array[y1 * Length + x0], Array[y1 * Length + x1], xBlend);
			Array[y * Length + x] = Interpolate(Top, Bottom, yBlend);
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
	double Freq = Width * Length / 360;

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

void BrownNoise(uint16_t Width, uint16_t Length, float* Input) {
	float* Array = calloc(Width * Length, sizeof(float));

	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Length; ++y) {
			Array[y * Width + x] = Input[y * Width + x];
		}
	}
	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Length; ++y) {
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
			if((x + 1) < Length ) {
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
		for(uint16_t y = 0; y < Length; ++y) {
			Input[y * Width + x] = Array[y * Width + x];
		}
	}
	free(Array);
}

void Erode(uint16_t Width, uint16_t Length, float* Input, float Split) {
	uint8_t* Array = calloc(Width * Length, sizeof(uint8_t));

	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Length; ++y) {
			if(Input[y * Width + x] >= Split) {
				Array[y * Width + x] = 1;
			} else {
				Array[y * Width + x] = 0;
			}
		}
	}
	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Length; ++y) {
			if(Array[y * Width + x] == 1) {
				if(x > 0 && Array[y * Width + (x - 1)] == 0) Array[y * Width + (x - 1)] = 2;
				if(y > 0 && Array[(y - 1) * Width + x] == 0) Array[(y - 1) * Width + x] = 2;
				if((x + 1) < Length && Array[y * Width + (x + 1)] == 0) Array[y * Width + (x + 1)] = 2;
				if((y + 1) < Width && Array[(y + 1) * Width + x] == 0) Array[(y + 1) * Width + x]  = 2;
			}
		}
	}
	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Length; ++y) {
			if(Array[y * Width + x] == 2)
				Input[y * Width + x] = Split;
		}
	}
	free(Array);
}

void CreateHeightMap(uint16_t Width, uint16_t Length, float* HeightMap, float DispStart, float DispEnd, uint16_t FaultTimes) {
	float Disp = DispStart;
	float Min = FLT_MAX;
	float Max = FLT_MIN;

//	for(int i = 0; i < Width * Length; ++i)
//			HeightMap[i] = 0.5f;
	for(int i = 0; i < FaultTimes; ++i) {
		Fault(Width, Length, HeightMap, Disp);
		Disp = DispStart + (i / ((float)FaultTimes)) * (DispEnd - DispStart);
	}
	for(int i = 0; i < Width * Length; ++i) {
		if(HeightMap[i] < Min)
			Min = HeightMap[i];	
		if(HeightMap[i] > Max)
			Max = HeightMap[i];
	}
	for(int i = 0; i < Width * Length; ++i) {
		//HeightMap[i] = (HeightMap[i] - Min) / (Max - Min);
//		if(HeightMap[i] < 0)
//			HeightMap[i] = 0;
//		else if(HeightMap[i] > 1.0)
//			HeightMap[i] = 1.0;
	}
}

void HeightMapTexture(const char* Name, uint16_t Width, uint16_t Length, float* HeightMap) {
	SDL_Surface* Surface = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Length, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

	for(uint16_t x = 0; x < Width; ++x) {
		for(uint16_t y = 0; y < Length; ++y) {
			uint32_t Pixel = ((uint8_t)(HeightMap[y * Width + x] * 0xFF)) & 0xFF;
			//uint32_t Pixel = Val & (Val << 8) & (Val << 16);

			((uint32_t*)Surface->pixels)[y * Surface->w + x] = Pixel;
		}
	}
	if(SDL_SaveBMP(Surface, Name) != 0) {
		Log(ELOG_ERROR, "%s", SDL_GetError());
	}
	SDL_FreeSurface(Surface);
}
