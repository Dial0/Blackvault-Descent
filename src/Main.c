#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <stdlib.h>

#if defined(PLATFORM_WEB)
#define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
#include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

Font font;
int fontSize = 15;

typedef struct iVec2 {
	int x;
	int y;
} iVec2;

enum EntityState { IDLE, MOVING, ATTACKING };

typedef struct Entity {
	iVec2 tilePos;
	Vector2 worldPos;
	iVec2 targetTilePos;
	float moveSpeed;
	enum EntityState eState;
	int path[200];
	int pathsize;
	int movePathIdx;
	int aniFrame;
	Rectangle baseTexSource;
} Entity;

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

	Texture2D map;
	Texture2D ui;
	Texture2D ent;

	iVec2 cursTilePos;
	RenderTexture2D mapRendTex;
	RenderTexture2D uiRendTex;
	RenderParams renderParams;

	Entity playerEnt;

	int mapDataSize;
	unsigned char* mapData;
	int mapSizeX;
	int mapSizeY;

	int curTurn;
	int prevTurn;


}State;

//Returns the screenXY of the top left pixel of the tile
iVec2 mapTileXYtoScreenXY(int MapX, int MapY, RenderParams param) {
	int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
	int tileYOffset = (param.mapViewPortHeight - 1) / param.tileSize;
	int scrollOffset = (param.smoothScrollY / param.scale);

	return (struct iVec2) { MapX* param.tileSize, (-MapY + tileYOffset)* param.tileSize + mapRenderOffset % param.tileSize + scrollOffset };
}

iVec2 mapWorldXYtoScreenXY(float MapX, float MapY, RenderParams param) {
	int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
	int tileYOffset = (param.mapViewPortHeight - 1) / param.tileSize;
	int scrollOffset = (param.smoothScrollY / param.scale);

	return (struct iVec2) { MapX* (float)(param.tileSize), (-MapY + tileYOffset)* (float)(param.tileSize) + mapRenderOffset % param.tileSize + scrollOffset };
}

static inline int floordiv(int a, int b) {
	if (a >= 0) { return a / b; }
	return (a - b + 1) / b;
}

iVec2 screenXYtoMapTileXY(int screenX, int screenY, RenderParams param) {
	// screenX/screenY already divided by scale before calling

	int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
	int tileYOffset = (param.mapViewPortHeight - 1) / param.tileSize;
	int scrollOffset = param.smoothScrollY / param.scale;

	// Fix negative modulo
	int moduloPart = mapRenderOffset % param.tileSize;
	if (moduloPart < 0) moduloPart += param.tileSize;

	int constantY = moduloPart + scrollOffset;

	int MapX = screenX / param.tileSize;

	int adjustedY = screenY - constantY;

	//use floor division instead of regular int div
	int tilePart = floordiv(adjustedY, param.tileSize);

	int MapY = tileYOffset - tilePart;

	return (iVec2) { MapX, MapY };
}

Vector2 screenXYtoMapWorldXY(float screenX, float screenY, RenderParams param) { //UNTESTED
	float scrollY = (float)param.smoothScrollY / param.scale;

	int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
	int moduloPart = mapRenderOffset % param.tileSize;
	if (moduloPart < 0) moduloPart += param.tileSize;

	float constantY = (float)moduloPart + scrollY;

	float tileX = screenX / (float)param.tileSize;

	float adjustedY = screenY - constantY;
	float tilePart = adjustedY / (float)param.tileSize;

	float tileY = (float)(param.mapViewPortHeight - 1) / (float)param.tileSize - tilePart;

	return (Vector2) { tileX, tileY };
}



int calcTileMoveCost(unsigned int tileData) {

	int moveCost = 0;
	return moveCost;
}

int mapXYtoIdx(unsigned char x, unsigned char y, unsigned char* mapData) {
	unsigned char width = mapData[0];
	return y * width + x;
}

iVec2 mapIdxToXY(int tileIdx, int mapSizeX) {
	int x = tileIdx % mapSizeX;
	int y = tileIdx / mapSizeX;
	return (struct iVec2) { x, y };
}

int hDistNodes(int start, int end, int mapSizeX) {
	iVec2 startPos = mapIdxToXY(start, mapSizeX);
	iVec2 endPos = mapIdxToXY(end, mapSizeX);
	return abs(endPos.x - startPos.x) + abs(endPos.y - startPos.y);
}

int getNeighbour(int tileIdx, int dir, int mapSizeX, int mapSizeY) {
	int i = tileIdx;
	if (dir == 0) {
		//North
		if (i + mapSizeX < (mapSizeX * mapSizeY)) {
			return i + mapSizeX;
		}
		else {
			return -1;
		}
	}

	if (dir == 1) {
		//East
		if ((i / mapSizeX) == ((i + 1) / mapSizeX)) {
			return i + 1;
		}
		else {
			return -1;
		}
	}

	if (dir == 2) {
		//West
		if ((i / mapSizeX) == ((i - 1) / mapSizeX)) {
			return i - 1;
		}
		else {
			return -1;
		}

	}

	if (dir == 3) {
		//South
		if (i - mapSizeX >= 0) {
			return i - mapSizeX;
		}
		else {
			return -1;
		}
	}

	return -2;
}

int findAsPath(iVec2 startTile, iVec2 endTile, unsigned char* mapData, int* path, int pathMaxSize) {

	int mapSizeX = 27;// mapData[0];
	int mapSizeY = 19; // mapData[1];

	int start = startTile.y * mapSizeX + startTile.x;
	int end = endTile.y * mapSizeX + endTile.x;

	int mapDataSizeInt = mapSizeX * mapSizeY;
	int mapDataSizeByte = mapDataSizeInt * sizeof(int);
	int totalDataSizeByte = mapDataSizeByte * 4;

	int* aStarData = malloc(totalDataSizeByte);

	int* openSet = aStarData;
	int openSetSize = 0;

	openSet[0] = start;
	openSetSize += 1;

	int* cameFromIdx = aStarData + mapDataSizeInt;
	int* gScore = aStarData + (mapDataSizeInt * 2);
	int* fScore = aStarData + (mapDataSizeInt * 3);

	for (int i = mapDataSizeInt; i < (mapDataSizeInt * 4); i++) {
		aStarData[i] = -1;
	}

	gScore[start] = 0;

	while (openSetSize > 0) {

		//Get the node with the lowest fScore
		int curIdx = 0;
		int fScoreLow = fScore[openSet[0]];

		for (int i = 1; i < openSetSize; i++) {
			if (fScore[openSet[i]] < fScoreLow) {
				fScoreLow = fScore[openSet[i]];
				curIdx = i;
			}
		}

		if (openSet[curIdx] == end) {

			int pathSize = 1;
			int curIdx = end;
			while (curIdx != start) {
				curIdx = cameFromIdx[curIdx];
				pathSize += 1;
			}

			curIdx = end;
			int skip = pathSize - pathMaxSize;
			if (skip < 0) {
				skip = 0;
			}
			int writeIdx = pathSize - 1;
			while (1) {
				if (skip) {
					skip -= 1;
				}
				else {
					path[writeIdx] = curIdx;
				}

				if (curIdx == start) {
					break;
				}
				curIdx = cameFromIdx[curIdx];
				writeIdx -= 1;
			}

			free(aStarData);

			if (pathSize > pathMaxSize) {
				pathSize = pathMaxSize;
			}

			return pathSize; //Return the Path we found
		}

		//check the neightbours of the current node
		int curNode = openSet[curIdx];

		//swap and drop current node
		openSet[curIdx] = openSet[openSetSize - 1];
		openSetSize -= 1;

		for (int i = 0; i < 4; i++) {
			int neighIdx = getNeighbour(curNode, i, mapSizeX, mapSizeY);
			if (neighIdx == -1) {
				continue;
			}


			//unsigned int dataPos = 2 + curNode * 4;
			//unsigned int curTileData;
			//memcpy(&curTileData, mapData + dataPos, 4);

			//dataPos = 2 + neighIdx * 4;
			//unsigned int neighTileData;
			//memcpy(&neighTileData, mapData + dataPos, 4);

			int edgeCost = 1; // (calcTileMoveCost(curTileData) + calcTileMoveCost(neighTileData)) / 2;

			int tentative_gScore = gScore[curNode] + edgeCost;
			int curNeigh_gScore = gScore[neighIdx];

			if ((tentative_gScore < curNeigh_gScore) || curNeigh_gScore == -1) {
				cameFromIdx[neighIdx] = curNode;
				gScore[neighIdx] = tentative_gScore;
				fScore[neighIdx] = tentative_gScore + hDistNodes(neighIdx, end, mapSizeX);

				// if not in openset add
				int add = 1;
				for (int i = 0; i < openSetSize; i++) {
					if (openSet[i] == neighIdx) {
						add = 0;
					}
				}

				if (add) {
					openSet[openSetSize] = neighIdx;
					openSetSize += 1;
				}
			}

		}

	}

	free(aStarData);
	return 0;
}


int moveEntity(Entity* entity) {

	//if we are already at the target return
	if (entity->tilePos.x == entity->targetTilePos.x
		&& entity->tilePos.y == entity->targetTilePos.y) {
		return 1;
	}

	Vector2 target = { entity->targetTilePos.x, entity->targetTilePos.y };
	Vector2 dir = Vector2Normalize(Vector2Subtract(target, entity->worldPos));


	entity->worldPos = Vector2Add(Vector2Scale(dir, entity->moveSpeed), entity->worldPos);

	entity->aniFrame += 1;

	if (Vector2Distance(entity->worldPos, target) <= entity->moveSpeed) {
		entity->tilePos = entity->targetTilePos;
		entity->worldPos.x = entity->tilePos.x;
		entity->worldPos.y = entity->tilePos.y;
		entity->aniFrame = 0;
		return 1;
	}

	return 0;

}

bool updateEntityMovement(Entity* entity, int mapSizeX) {
	bool movedNewTile = false;
	if ((entity->movePathIdx < entity->pathsize)) {
		if (moveEntity(entity)) {
			entity->movePathIdx += 1;
			movedNewTile = true;
			if (entity->movePathIdx == entity->pathsize) {
				// FINISHED MOVING TO NEW LOCATION 
				entity->movePathIdx = 0;
				entity->pathsize = 0;
				entity->eState = IDLE;
			}
			else {
				entity->targetTilePos = mapIdxToXY(entity->path[entity->movePathIdx], mapSizeX);
			}
		}
	}
	return movedNewTile;
}

void RenderPlayerPath(State* state) {


	//int pathRawTileCost = 0;

	for (int i = state->playerEnt.movePathIdx; i < state->playerEnt.pathsize; i++) {


		int nextTile = i + 1;
		if (nextTile >= state->playerEnt.pathsize) { nextTile = -1; }
		int prevTile = i - 1;
		if (prevTile < 0) { prevTile = -1; }

		int north = 0b0000000000001000;
		int east = 0b0000000000000100;
		int west = 0b0000000000000010;
		int south = 0b0000000000000001;

		int neighbors = 0;
		if (nextTile > 0 && prevTile >= 0) { //drawing a path tile, not a start or end
			if (state->playerEnt.path[i] + state->mapSizeX == state->playerEnt.path[nextTile] || state->playerEnt.path[i] + state->mapSizeX == state->playerEnt.path[prevTile]) { //tile to the north
				neighbors |= north;
			}
			if (state->playerEnt.path[i] + 1 == state->playerEnt.path[nextTile] || state->playerEnt.path[i] + 1 == state->playerEnt.path[prevTile]) { //tile to the east
				neighbors |= east;
			}
			if (state->playerEnt.path[i] - 1 == state->playerEnt.path[nextTile] || state->playerEnt.path[i] - 1 == state->playerEnt.path[prevTile]) { //tile to the west
				neighbors |= west;
			}
			if (state->playerEnt.path[i] - state->mapSizeX == state->playerEnt.path[nextTile] || state->playerEnt.path[i] - state->mapSizeX == state->playerEnt.path[prevTile]) { //tile to the south
				neighbors |= south;
			}
		}

		int drawtileID = 0; //start/end tile

		if (neighbors == (north | south)) {
			drawtileID = 1;
		}
		else if (neighbors == (east | west)) {
			drawtileID = 2;
		}
		else if (neighbors == (north | west)) {
			drawtileID = 3;
		}
		else if (neighbors == (south | east)) {
			drawtileID = 4;
		}
		else if (neighbors == (south | west)) {
			drawtileID = 5;
		}
		else if (neighbors == (north | east)) {
			drawtileID = 6;
		}
		else {
			drawtileID = -1;
		}

		Rectangle pathTileSrc = { drawtileID * state->tileSize, 2 * state->tileSize, state->tileSize,state->tileSize };

		iVec2 tileLoc = mapIdxToXY(state->playerEnt.path[i], state->mapSizeX);
		iVec2 pathPos = mapTileXYtoScreenXY(tileLoc.x, tileLoc.y, state->renderParams);

		DrawTextureRec(state->ui, pathTileSrc, (struct Vector2) { pathPos.x, pathPos.y }, WHITE);


	}

}



void renderEntity(Entity entity, RenderParams renderParams, Texture2D tex) {
	iVec2 playerEntityPixelPos = mapWorldXYtoScreenXY(entity.worldPos.x, entity.worldPos.y, renderParams);

	bool alternateAnimation = ((int)entity.aniFrame / 10) % 2;


	if (alternateAnimation) {

		Rectangle newTexSource = entity.baseTexSource;
		newTexSource.x += 16;
		DrawTextureRec(tex, newTexSource, (struct Vector2) { playerEntityPixelPos.x, playerEntityPixelPos.y }, WHITE);

	}
	else {
		DrawTextureRec(tex, entity.baseTexSource, (struct Vector2) { playerEntityPixelPos.x, playerEntityPixelPos.y }, WHITE);

	}
}

void UpdateDrawFrame(void* v_state) {




	State* state = (State*)v_state;




	if (IsKeyReleased(KEY_RIGHT)) state->cursTilePos.x += 1;
	if (IsKeyReleased(KEY_LEFT)) state->cursTilePos.x -= 1;
	if (IsKeyReleased(KEY_UP)) state->cursTilePos.y += 1;
	if (IsKeyReleased(KEY_DOWN)) state->cursTilePos.y -= 1;

	if (state->cursTilePos.y < 0) state->cursTilePos.y = 0;
	if (state->cursTilePos.y >= state->mapSizeY) state->cursTilePos.y = state->mapSizeY - 1;
	if (state->cursTilePos.x < 0) state->cursTilePos.x = 0;
	if (state->cursTilePos.x >= state->mapSizeX) state->cursTilePos.x = state->mapSizeX - 1;

	Vector2 mousePos = GetMousePosition();

	state->cursTilePos = screenXYtoMapTileXY(mousePos.x / state->renderParams.scale, mousePos.y / state->renderParams.scale, state->renderParams);

	int playAreaYMin = 6;
	int playAreaYMax = 16;
	if (state->cursTilePos.y < playAreaYMin) state->cursTilePos.y = playAreaYMin;
	if (state->cursTilePos.y > playAreaYMax) state->cursTilePos.y = playAreaYMax;
	int playAreaXMin = 1;
	int playAreaXMax = 25;
	if (state->cursTilePos.x < playAreaXMin) state->cursTilePos.x = playAreaXMin;
	if (state->cursTilePos.x > playAreaXMax) state->cursTilePos.x = playAreaXMax;

	int cursorYScreen = mapTileXYtoScreenXY(state->cursTilePos.x, state->cursTilePos.y, state->renderParams).y;
	int screenCenter = (state->baseSizeY / 2);
	int cursorScreenCenterDist = abs(screenCenter - cursorYScreen);
	int scrollSpeed = 4;
	if (cursorScreenCenterDist < scrollSpeed) {
		scrollSpeed = cursorScreenCenterDist;
	}

	if (cursorYScreen < screenCenter) {
		state->smoothScrollY += scrollSpeed;
	}
	if (cursorYScreen > screenCenter) {
		state->smoothScrollY -= scrollSpeed;
	}

	int maxScroll = (state->map.height - state->baseSizeY) * state->scale;
	if (state->smoothScrollY > maxScroll) {
		state->smoothScrollY = maxScroll;
	}
	if (state->smoothScrollY < 0) {
		state->smoothScrollY = 0;
	}

	state->renderParams.smoothScrollY = state->smoothScrollY;

	if (state->curTurn != state->prevTurn) {
		state->prevTurn = state->curTurn;

		//Do the AI stuff here
	}

	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {

		if (state->playerEnt.movePathIdx < state->playerEnt.pathsize) {
			state->playerEnt.pathsize = findAsPath(state->playerEnt.targetTilePos, state->cursTilePos, state->mapData, state->playerEnt.path, 200);
		}
		else {
			state->playerEnt.pathsize = findAsPath(state->playerEnt.tilePos, state->cursTilePos, state->mapData, state->playerEnt.path, 200);
			state->playerEnt.targetTilePos = mapIdxToXY(state->playerEnt.path[0], state->mapSizeX);
		}
		state->playerEnt.eState = MOVING;
		state->playerEnt.movePathIdx = 0;
	}
	


	if (updateEntityMovement(&state->playerEnt, state->mapSizeX)) {
		state->curTurn += 1;
	}
	

	BeginDrawing();

	ClearBackground(RAYWHITE);



	BeginTextureMode(state->mapRendTex);

	ClearBackground(RAYWHITE);

	int scrollOffset = (state->smoothScrollY / state->scale);

	int mapRenderOffset = -(state->map.height - state->mapRendTex.texture.height);

	DrawTexture(state->map, 0, mapRenderOffset + scrollOffset, WHITE);

	Rectangle cursorRec = { 0.0f,0.0f,18.0f,18.0f };
	int centOff = (cursorRec.width - state->tileSize) / 2;
	iVec2 cursPixel = mapTileXYtoScreenXY(state->cursTilePos.x, state->cursTilePos.y, state->renderParams);

	DrawTextureRec(state->ui, cursorRec, (struct Vector2) { cursPixel.x - centOff, cursPixel.y - centOff }, WHITE);

	if (state->playerEnt.pathsize) {
		RenderPlayerPath(state);
		Rectangle pathEndRec = { 19.0f,0.0f,18.0f,18.0f };
		iVec2 tilePathEnd = mapIdxToXY(state->playerEnt.path[state->playerEnt.pathsize-1], state->mapSizeX);
		iVec2 pathEndPixel = mapTileXYtoScreenXY(tilePathEnd.x, tilePathEnd.y, state->renderParams);
		DrawTextureRec(state->ui, pathEndRec, (struct Vector2) { pathEndPixel.x - centOff, pathEndPixel.y - centOff }, WHITE);
	}


	renderEntity(state->playerEnt, state->renderParams, state->ent);


	EndTextureMode();

	BeginTextureMode(state->uiRendTex);
	ClearBackground(BLANK);

	EndTextureMode();


	Rectangle mapRendTexSrc = { 0, 0, state->mapRendTex.texture.width, -state->mapRendTex.texture.height };
	DrawTexturePro(state->mapRendTex.texture, mapRendTexSrc,
		(struct Rectangle) {
		0, -state->scale + state->smoothScrollY % state->scale, state->baseSizeX* state->scale, state->baseSizeY* state->scale
	},
		(struct Vector2) {
		0, 0
	}, 0.0f, WHITE);


	EndDrawing();
}

int main(void) {

	State state;

	state.tileSize = 16;
	state.baseSizeX = 432;
	state.baseSizeY = 240;
	state.scale = 4;
	state.smoothScrollY = 0;// state.tileSize* state.scale * 4;
	// Initialization
	//--------------------------------------------------------------------------------------
	state.screenWidth = state.baseSizeX * state.scale;
	state.screenHeight = state.baseSizeY * state.scale - state.scale;

	state.cursTilePos = (iVec2){ 13,8 };
	state.playerEnt = (Entity){ (struct iVec2) { 13,8 },(struct Vector2) { 13.0f,8.0f },(struct iVec2) { 13,8 }, 0.05f };
	state.playerEnt.pathsize = 0;
	state.playerEnt.movePathIdx = 0;
	state.playerEnt.baseTexSource = (struct Rectangle){ 0.0f, 0.0f, 16.0f, 16.0f };

	state.mapData = LoadFileData("resources/respitetest.rspb", &state.mapDataSize);
	state.mapSizeX = 27;
	state.mapSizeY = 19;

	InitWindow(state.screenWidth, state.screenHeight, "BlackVault - Descent");


	state.map = LoadTexture("resources/map.png");
	state.ui = LoadTexture("resources/UI.png");
	state.ent = LoadTexture("resources/entities.png");

	font = LoadFontEx("resources/rainyhearts.ttf", fontSize, NULL, 0);
	SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

	state.mapRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);
	state.uiRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);

	state.renderParams = (RenderParams){ state.baseSizeX ,state.baseSizeY,state.map.width,state.map.height,state.tileSize,state.smoothScrollY,state.scale };

	state.playerEnt.eState = IDLE;

	SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------

#if defined(PLATFORM_WEB)
	emscripten_set_main_loop_arg(UpdateDrawFrame, &state, 60, 1);
#else 
	SetTargetFPS(60);
	// Main game loop
	while (!WindowShouldClose()) {
		UpdateDrawFrame(&state);
}
#endif
	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}

