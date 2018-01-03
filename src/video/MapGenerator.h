/**
 * Author: David Brotz
 * File: MapGenerator.h
 */

#ifndef __MAPGENERATOR_H
#define __MAPGENERATOR_H

#include <stdint.h>

typedef struct SDL_Surface SDL_Surface;

struct QuadTree;
struct Tile;

void WhiteNoiseGen(uint16_t Width, uint16_t Length, float* Array);
void SmoothNoise(uint16_t Width, uint16_t Length, float* Array, uint8_t Octave);

void CreatePerlinNoise(uint16_t Width, uint16_t Height, float* HeightMap);
void Fault(uint16_t Width, uint16_t Length, float* Array, float Disp);
void Erode(uint16_t Width, uint16_t Length, float* Input, float Split);
void BrownNoise(uint16_t Width, uint16_t Length, float* Input);

void CreateHeightMap(uint16_t Width, uint16_t Length, float* HeightMap, float DispStart, float DispEnd, uint16_t FaultTimes);
void HeightMapTexture(const char* Name, uint16_t Width, uint16_t Length, float* HeightMap);
void CreateMiniMap(const struct Tile* Tiles, uint32_t TileLength, const struct QuadTree* Tree, uint32_t SetListMax);
#endif

