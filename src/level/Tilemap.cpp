#include "Tilemap.hpp"
#include "../gfx/Texture.hpp"
#include "../util/JMath.hpp"
#include "../util/Log.hpp"
#include <iostream>
#include <fstream>	// std::ifstream, ...
#include <sstream>	// std::stringstream

Tilemap::Tilemap(int tileSize, const char* textureFile, const char* metadataFile): width(0), height(0), tileSize(tileSize)
{
	tilemapTexture = new Texture(textureFile);
	loadTileData(metadataFile);
	init();
}

Tilemap::~Tilemap()
{
	delete tilemapTexture;
	delete[] tileData;
	delete[] solidData;
	delete[] foregroundData;
	delete[] tileTextures;
}

void Tilemap::init()
{
	// Load texture for each tile
	int nTiles = (tilemapTexture->width / tileSize) * (tilemapTexture->height / tileSize);
	for(int id = 0; id < nTiles; id++)
	{
		int tilePosX = id % tileSize;
		int tilePosY = id / tileSize; 
		Texture* croppedTile = tilemapTexture->crop(tilePosX * tileSize, tilePosY * tileSize, tileSize, tileSize);
		// Initialize the tileTextures array assuming all tiles are equal size
		if(id == 0) tileTextures = new Texture*[nTiles];
		tileTextures[id] = croppedTile;
	}
}

/*
	Loads tile data from a BMP image
    First 4 rows of image are used to 
    declare colors for each tile ID
*/
void Tilemap::loadData(const char* pathToLevelFile)
{
	std::ifstream readFile(pathToLevelFile);
	if(!readFile.is_open()) error("Error reading tile data from '%s'", pathToLevelFile);
	std::vector<std::string> lines;
	std::string line;
	while(std::getline(readFile, line)) lines.push_back(line);
	log("Loading tilemap '%s'", lines.at(0).c_str());
	width = atoi(lines.at(1).c_str());
	height = atoi(lines.at(2).c_str());
	tileData = new int[width * height];
	for(int i = 3; i < (int) lines.size(); i++) tileData[i - 3] = atoi(lines.at(i).c_str());
}

// Reads file that specifies information about each tile (e.g. solidity)
void Tilemap::loadTileData(const char* pathToTileData)
{
	std::ifstream readFile(pathToTileData);
	if(!readFile.is_open()) error("Error reading tile data from '%s'", pathToTileData);
	std::string line;
	std::vector<bool> solid, foreground;
	while(std::getline(readFile, line))
	{
		std::vector<std::string> tokens;
    	std::stringstream stream(line); 
    	while (stream.good()) 
    	{ 
    		std::string token; 
    	    getline(stream, token, ','); 
    	    tokens.push_back(token); 
    	} 
		solid.push_back(tokens.at(0).compare("true") == 0 ? TILE_SOLID : TILE_PASSABLE);	
		foreground.push_back(tokens.at(1).compare("true") == 0 ? TILE_FOREGROUND : TILE_BACKGROUND);
	} 
	solidData = new bool[solid.size()];
	foregroundData = new bool[foreground.size()];
	std::copy(solid.begin(), solid.end(), solidData);
	std::copy(foreground.begin(), foreground.end(), foregroundData);
}

void Tilemap::render(Graphics* graphics, Camera* camera, bool layer)
{
	// Only render the tiles that are in the view of the camera
	const int padding = 4;
	int nRowTiles = graphics->renderBuffer->width / tileSize;
	int nColTiles = graphics->renderBuffer->height / tileSize;
	int x1 = clamp((int) camera->getFocusX() / tileSize - (nRowTiles / 2) - padding, 0, width);
	int x2 = clamp((int) camera->getFocusX() / tileSize + (nRowTiles / 2) + padding, 0, width);
	int y1 = clamp((int) camera->getFocusY() / tileSize - (nColTiles / 2) - padding, 0, height);
	int y2 = clamp((int) camera->getFocusY() / tileSize + (nColTiles / 2) + padding, 0, height);
	for(int x = x1; x <= x2; x++)
	{
		for(int y = y1; y <= y2; y++)
			if(isInForeground(x, y) == layer) graphics->drawTexture(tileTextures[getTileId(x, y)], x * tileSize, y * tileSize, 0xFF00FF, camera);
	}
}

/* 
	Returns Rectangle objects for the tile (x, y)
	lands on, as well as the 8 surrounding tiles
*/
std::vector<Rectangle*> Tilemap::getRectanglesSurrounding(int x, int y)
{
	int tx = x / tileSize;
	int ty = y / tileSize;
	std::vector<Rectangle*> rectangles;
	#define tryToAdd(a, b) if(isSolid(a, b)) rectangles.push_back(getTileRectangle(a, b));
	tryToAdd(tx, ty);
	tryToAdd(tx - 1, ty - 1); 
	tryToAdd(tx, ty - 1);
	tryToAdd(tx + 1, ty - 1);
	tryToAdd(tx - 1, ty);
	tryToAdd(tx + 1, ty);
	tryToAdd(tx - 1, ty + 1);
	tryToAdd(tx, ty + 1);
	tryToAdd(tx + 1, ty + 1);
	return rectangles;
}

void Tilemap::setTile(int x, int y, int id)
{
	if(tileInRange(x, y) && id >= 0) tileData[x + y * width] = id;
}

int Tilemap::getTileId(int x, int y)
{
	if(tileInRange(x, y)) return tileData[x + y * width];
	return 0;
}

Rectangle* Tilemap::getTileRectangle(int x, int y) 
{ return new Rectangle(x * tileSize, y * tileSize, tileSize, tileSize); }
bool Tilemap::isSolid(int id) { return solidData[id]; }
bool Tilemap::isSolid(int x, int y) { return solidData[getTileId(x, y)]; }
bool Tilemap::isInForeground(int id) { return foregroundData[id]; }
bool Tilemap::isInForeground(int x, int y) { return foregroundData[getTileId(x, y)]; }
bool Tilemap::tileInRange(int x, int y) { return (x >= 0 && y >= 0 && x < width && y < height); }