#pragma once
#include "raylib.h"

Font font;
int fontSize = 15;

typedef struct RenderParams {
	int mapViewPortWidth;
	int mapViewPortHeight;
	int mapWidth;
	int mapHeight;
	int tileSize;
	int smoothScrollY;
	int scale;
} RenderParams;

typedef struct State {
	int tileSize;
	int baseSizeX;
	int baseSizeY;
	int scale;
	int screenWidth;
	int screenHeight;
	int smoothScrollY;

	Rectangle playArea;
	//int playAreaYMin;
	//int playAreaYMax;
	//int playAreaXMin;
	//int playAreaXMax;

	Texture2D map;
	Texture2D ui;
	Texture2D ent;

	iVec2 cursTilePos;
	RenderTexture2D mapRendTex;
	RenderTexture2D uiRendTex;
	RenderParams renderParams;

	Entity playerEnt;

	int enemiesLen;
	Entity enemies[32];

	int mapDataSize;
	unsigned char* mapData;
	int mapSizeX;
	int mapSizeY;

	int curTurn;
	int prevTurn;


}State;