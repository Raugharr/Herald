/**
 * Author: David Brotz
 * File: MapGenerator.h
 */

#ifndef __MAPGENERATOR_H
#define __MAPGENERATOR_H

#include <stdint.h>

typedef struct SDL_Surface SDL_Surface;

void WhiteNoiseGen(uint16_t Width, uint16_t Length, float* Array);
void SmoothNoise(uint16_t Width, uint16_t Length, float* Array, uint8_t Octave);

void Fault(uint16_t Width, uint16_t Length, float* Array, float Disp);
void Erode(uint16_t Width, uint16_t Length, float* Input, float Split);
void BrownNoise(uint16_t Width, uint16_t Length, float* Input);

void CreateHeightMap(uint16_t Width, uint16_t Length, float* HeightMap, float DispStart, float DispEnd, uint16_t FaultTimes);
void HeightMapTexture(const char* Name, uint16_t Width, uint16_t Length, float* HeightMap);
#endif

