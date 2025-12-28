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
	Vector2 renderWorldPos;
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

	double gameTime;
	double nextTurnTime;
	double turnDuration;

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

int mapXYtoIdx(unsigned char x, unsigned char y, int mapWidth) {
	unsigned char width = mapWidth;
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

static int getNeighbour(int node, int dir, int mapSizeX, int mapSizeY) {

	//TODO: update function to handle inbounds, but inaccessible tiles

	int x = node % mapSizeX;
	int y = node / mapSizeX;

	if (dir == 0) {      // North
		y++;
	}
	else if (dir == 1) { // East
		x++;
	}
	else if (dir == 2) { // South
		y--;
	}
	else if (dir == 3) { // West
		x--;
	}
	else {
		return -1;  // Invalid direction
	}

	// Check bounds
	if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY) {
		return -1;
	}

	return y * mapSizeX + x;
}

int findAsPath(iVec2 startTile, iVec2 endTile, unsigned char* mapData, int* path, int pathMaxSize) {

	int mapSizeX = 27;// mapData[0];
	int mapSizeY = 19; // mapData[1];

	int start = startTile.y * mapSizeX + startTile.x;
	int end = endTile.y * mapSizeX + endTile.x;

	int mapDataSizeInt = mapSizeX * mapSizeY;
	int mapDataSizeByte = mapDataSizeInt * sizeof(mapDataSizeInt);
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
			int writeIdx = pathSize - 1; //Additional -1 because we don't want the start tile
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


bool updateEntityRenderPos(Entity* entity, double turnDuration, double turnElapsed){

	if (entity->tilePos.x == entity->targetTilePos.x
		&& entity->tilePos.y == entity->targetTilePos.y) {
		return false; //Stationary no update required
		//TODO: Can this be set to check if the Entity State is IDLE?
	}

	Vector2 start = { entity->tilePos.x, entity->tilePos.y };
	Vector2 end = { entity->targetTilePos.x, entity->targetTilePos.y };
	double t = turnElapsed/ turnDuration;
	entity->renderWorldPos = Vector2Lerp(start, end, t);

	entity->aniFrame += 1;

	return true;
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
	iVec2 playerEntityPixelPos = mapWorldXYtoScreenXY(entity.renderWorldPos.x, entity.renderWorldPos.y, renderParams);

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

float iVec2fDistance(iVec2 v1, iVec2 v2) {
	float result = sqrtf(((float)v1.x - (float)v2.x) * ((float)v1.x - v2.x) + ((float)v1.y - (float)v2.y) * ((float)v1.y - (float)v2.y));
	return result;
}

void calculateEnemyTurn(Entity* enemyEntity, Entity player, const Entity * const OtherEnemies,int enemiesLen, Rectangle playArea, int mapSizeX) {
	

	// Choose the adjacent tile closest to the player
	float minDist = iVec2fDistance(player.targetTilePos, enemyEntity->tilePos);
	iVec2 bestTarget = enemyEntity->tilePos;

	const iVec2 offsets[4] = {
		{  0,  1 },  // North
		{  1,  0 },  // East
		{  0, -1 },  // South
		{ -1,  0 }   // West
	};

	for (int i = 0; i < 4; i++) {
		iVec2 candidate = {
			enemyEntity->tilePos.x + offsets[i].x,
			enemyEntity->tilePos.y + offsets[i].y
		};

		// Clamp to play area bounds (assuming playArea is a rectangle: x, y, width, height)
		candidate.x = Clamp(candidate.x, playArea.x, playArea.x + playArea.width);
		candidate.y = Clamp(candidate.y, playArea.y, playArea.y + playArea.height);

		//check here is there is already a entity in the target tile
		//if there is continue

		//TODO: update this logic to use a pathing algorithm to determine which tile will be best to move towards to get access to the player for attacking
		if (player.targetTilePos.x == candidate.x && player.targetTilePos.y == candidate.y) {
			continue;
		}

		bool collision = false;
		for (int i = 0; i < enemiesLen; i++) {
			if (OtherEnemies[i].targetTilePos.x == candidate.x && OtherEnemies[i].targetTilePos.y == candidate.y) {
				collision = true;
			}
		}

		if (collision == true) {
			continue;
		}

		float dist = iVec2fDistance(player.targetTilePos, candidate);

		if (dist < minDist) {
			minDist = dist;
			bestTarget = candidate;
		}
	}

	// If no better move found, bestTarget remains current position (stays put)
	enemyEntity->targetTilePos = bestTarget;
	enemyEntity->pathsize = 1;  // Move to this single target tile
	enemyEntity->path[0] = mapXYtoIdx(bestTarget.x, bestTarget.y, mapSizeX);

}

int tileOccupiedByEnemy(iVec2 tile, const  Entity* const OtherEnemies, int enemiesLen) {
	for (int i = 0; i < enemiesLen; i+=1) {
		if (OtherEnemies[i].targetTilePos.x == tile.x && OtherEnemies[i].targetTilePos.y == tile.y) {
			return i;
		}
	}
	return -1;
}

int swapAndDropEnemy(int dropIdx, Entity* Enemies, int enemiesLen) {
	Enemies[dropIdx] = Enemies[enemiesLen - 1];
	enemiesLen -= 1;
	return enemiesLen;
}

updateEntityPath(Entity* entity, int mapSizeX) {
	entity->movePathIdx += 1;
	entity->targetTilePos = mapIdxToXY(entity->path[entity->movePathIdx], mapSizeX);
}

double getTurnElapsedTime(double gameTime, double nextTurnTime, double TurnDuration) {
	double remainingTime = nextTurnTime - gameTime;
	double elapsedTime = TurnDuration - remainingTime;
	if (elapsedTime > TurnDuration) { elapsedTime = TurnDuration; }
	return elapsedTime;
}

void setEntityIdleIfPathEnd(Entity* entity) {
	if (entity->movePathIdx >= (entity->pathsize - 1)) {
		entity->movePathIdx = 0;
		entity->pathsize = 0;
		entity->eState = IDLE;
	}
}

void UpdateDrawFrame(void* v_state) {




	State* state = (State*)v_state;

	state->gameTime = GetTime();

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

	int playAreaYMin = state->playArea.y;
	int playAreaYMax = state->playArea.y + state->playArea.height;
	int playAreaXMin = state->playArea.x;
	int playAreaXMax = state->playArea.x + state->playArea.width;
	if (state->cursTilePos.y < playAreaYMin) { state->cursTilePos.y = playAreaYMin; }
	if (state->cursTilePos.y > playAreaYMax) { state->cursTilePos.y = playAreaYMax; }
	if (state->cursTilePos.x < playAreaXMin) { state->cursTilePos.x = playAreaXMin; }
	if (state->cursTilePos.x > playAreaXMax) { state->cursTilePos.x = playAreaXMax; }

	int cursorYScreen = mousePos.y / state->renderParams.scale;

	int scrollUpThreshold = state->baseSizeY / 8;
	int scrollDownThreshold = state->baseSizeY - scrollUpThreshold;

	int basescrollSpeed = 6;
	int scrollSpeed = 0;
	if (cursorYScreen < scrollUpThreshold) {
		int cursorthresholdDist = abs(scrollUpThreshold - cursorYScreen);
		if (cursorthresholdDist < basescrollSpeed) {
			scrollSpeed = cursorthresholdDist;
		}
		else {
			scrollSpeed = basescrollSpeed;
		}
	}
	if (cursorYScreen > scrollDownThreshold) {
		int cursorthresholdDist = abs(scrollDownThreshold - cursorYScreen);
		if (cursorthresholdDist < basescrollSpeed) {
			scrollSpeed = -cursorthresholdDist;
		}
		else {
			scrollSpeed = -basescrollSpeed;
		}

	}

	state->smoothScrollY += scrollSpeed;

	int maxScroll = (state->map.height - state->baseSizeY) * state->scale;
	if (state->smoothScrollY > maxScroll) {
		state->smoothScrollY = maxScroll;
	}
	if (state->smoothScrollY < 0) {
		state->smoothScrollY = 0;
	}

	state->renderParams.smoothScrollY = state->smoothScrollY;

	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {

		//TODO: Check if the clicked square contains an enemy, if it does set that as the attack target
		//if the enemy is adjacent then attack, otherwise move towards it
		int enemyIdxOccupy = tileOccupiedByEnemy(state->cursTilePos, &state->enemies, state->enemiesLen);
		if (enemyIdxOccupy != -1) {
			int newEnemiesLen = swapAndDropEnemy(enemyIdxOccupy, &state->enemies, state->enemiesLen);
			state->enemiesLen = newEnemiesLen;
		}

		if (state->playerEnt.movePathIdx < state->playerEnt.pathsize) {
			state->playerEnt.pathsize = findAsPath(state->playerEnt.targetTilePos, state->cursTilePos, state->mapData, state->playerEnt.path, 200);
		}
		else {
			state->playerEnt.pathsize = findAsPath(state->playerEnt.tilePos, state->cursTilePos, state->mapData, state->playerEnt.path, 200);
			state->playerEnt.targetTilePos = mapIdxToXY(state->playerEnt.path[0], state->mapSizeX);
		}
		state->playerEnt.movePathIdx = 0;
		state->playerEnt.eState = MOVING;
		
	}

	//-------------------------------------------------------------------------------------------
	//ABOVE: Check all the above input, to see if we kick the player out of IDLE and start a turn
	//		 from initaiting moving or attacking etc
	//-------------------------------------------------------------------------------------------


	if (state->gameTime > state->nextTurnTime) {

		//-----------------------------------------------------------------------
		//BELOW: All the logic to run to end the previous turn
		//		 Using this section to finilise any movement and 
		//		 set states back to IDLE if nothing in the actions/movement queue
		//------------------------------------------------------------------------

		//try to set player to Idle if and stop turns incrementing
		setEntityIdleIfPathEnd(&state->playerEnt);

		//Make sure the player and enemies are the target tile from the end of last turn
		state->playerEnt.tilePos = state->playerEnt.targetTilePos;
		state->playerEnt.renderWorldPos.x = state->playerEnt.targetTilePos.x;
		state->playerEnt.renderWorldPos.y = state->playerEnt.targetTilePos.y;

		for (int i = 0; i < state->enemiesLen; i += 1) {

			state->enemies[i].tilePos = state->enemies[i].targetTilePos;
			state->enemies[i].renderWorldPos.x = state->enemies[i].targetTilePos.x;
			state->enemies[i].renderWorldPos.y = state->enemies[i].targetTilePos.y;
		}

		//-----------------------------------------------------------------
		//BELOW: Logic to determine if we automatically start the next turn
		//-----------------------------------------------------------------

		if (state->playerEnt.eState != IDLE) {
			state->nextTurnTime = state->gameTime + state->turnDuration;
			state->curTurn += 1;

			//TODO: Maybe we can just do all the turn update here??
		}
	}

	if (state->curTurn != state->prevTurn) {
		state->prevTurn = state->curTurn;


		updateEntityPath2(&state->playerEnt, state->mapSizeX);

		//All non-player entity turn logic happens here

		//Probably want to do the same as in the player logic where we sync all the entities to their prev target as the turn has ended

		for (int i = 0; i < state->enemiesLen; i += 1) {
			calculateEnemyTurn(&state->enemies[i], state->playerEnt,&state->enemies,state->enemiesLen, state->playArea,state->mapSizeX);
		}

	}

	//NON GAME LOGIC UPDATES
	//UPDATE ENTITY WORLD POSITIONS AND ANIMATION FOR RENDERING

	

	updateEntityRenderPos(&state->playerEnt, state->turnDuration, getTurnElapsedTime(state->gameTime, state->nextTurnTime, state->turnDuration));

	//if (updateEntityMovement(&state->playerEnt, state->mapSizeX)) {

		//int enemyIdxOccupy = tileOccupiedByEnemy(state->playerEnt.targetTilePos, &state->enemies, state->enemiesLen);
		//if (enemyIdxOccupy != -1) {
		//	state->playerEnt.eState = IDLE;
		//	state->playerEnt.pathsize = 0;
		//	state->playerEnt.movePathIdx = 0;
		//	state->playerEnt.targetTilePos = state->playerEnt.tilePos;
		//}
	//}

	for (int i = 0; i < state->enemiesLen; i += 1) {
		updateEntityRenderPos(&state->enemies[i], state->turnDuration, getTurnElapsedTime(state->gameTime, state->nextTurnTime, state->turnDuration));
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

	//PLAYER
	renderEntity(state->playerEnt, state->renderParams, state->ent);


	for (int i = 0; i < state->enemiesLen; i+=1) {
		renderEntity(state->enemies[i], state->renderParams, state->ent);
	}


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

iVec2 getRandPosInPlayArea(Rectangle playArea) {

	int playAreaXMin = playArea.x;
	int playAreaXMax = playArea.x + playArea.width;
	int playAreaYMin = playArea.y;
	int playAreaYMax = playArea.y + playArea.height;

	int randX = GetRandomValue(playAreaXMin, playAreaXMax);
	int randY = GetRandomValue(playAreaYMin, playAreaYMax);
	
	return (struct iVec2) { randX, randY };
}

void populateEnemies(Rectangle playArea, Entity* entityArr, int entityArrLen) {

	for (int i = 0; i < entityArrLen; i+=1) {
		entityArr[i].aniFrame = 0;
		entityArr[i].baseTexSource = (struct Rectangle){ 0.0f, 16.0f, 16.0f, 16.0f }; //ORC
		entityArr[i].eState = IDLE;
		entityArr[i].movePathIdx = 0;
		entityArr[i].moveSpeed = 1.0f/60.0f;
		entityArr[i].pathsize = 0;

		iVec2 randPos = getRandPosInPlayArea(playArea);
		bool collision = true;
		while (collision) {
			int j;
			for (j = 0; j < i; j+=1) {
				if (randPos.x == entityArr[j].tilePos.x && randPos.y == entityArr[j].tilePos.y) {
					break;
				}
			}
			if (j == i) {
				collision = false;
			}
			else {
				randPos = getRandPosInPlayArea(playArea);
			}
			
		}

		entityArr[i].tilePos = randPos;
		entityArr[i].targetTilePos = entityArr[i].tilePos;
		entityArr[i].renderWorldPos = (struct Vector2){ randPos.x,randPos.y};
	}
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
	state.playerEnt = (Entity){ (struct iVec2) { 13,8 },(struct Vector2) { 13.0f,8.0f },(struct iVec2) { 13,8 }, 1.0f / 60.0f };
	state.playerEnt.pathsize = 0;
	state.playerEnt.movePathIdx = 0;
	state.playerEnt.baseTexSource = (struct Rectangle){ 0.0f, 0.0f, 16.0f, 16.0f };
	state.playerEnt.eState = IDLE;
	state.curTurn = 0;
	state.prevTurn = 0;

	state.mapData = LoadFileData("resources/respitetest.rspb", &state.mapDataSize);
	state.mapSizeX = 27;
	state.mapSizeY = 19;

	state.playArea.y = 6;
	state.playArea.height = 10;
	state.playArea.x = 1;
	state.playArea.width = 24;

	state.enemiesLen = 10;

	state.nextTurnTime = 0.0f;
	state.turnDuration = 1.0f;
	
	InitWindow(state.screenWidth, state.screenHeight, "BlackVault - Descent");

	populateEnemies(state.playArea, state.enemies, state.enemiesLen);

	SetRandomSeed(100);
	state.map = LoadTexture("resources/map.png");
	state.ui = LoadTexture("resources/UI.png");
	state.ent = LoadTexture("resources/entities.png");

	font = LoadFontEx("resources/rainyhearts.ttf", fontSize, NULL, 0);
	SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

	state.mapRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);
	state.uiRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);

	state.renderParams = (RenderParams){ state.baseSizeX ,state.baseSizeY,state.map.width,state.map.height,state.tileSize,state.smoothScrollY,state.scale };

	
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

