#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <stdlib.h>

#include "datatypes.h"
#include "state.h"

#if defined(PLATFORM_WEB)
#define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
#include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#include "utils.c"
#include "entities.c"
#include "render.c"
#include "enemy_ai.c"


int tileOccupiedByEnemy(iVec2 tile, const  Entity* const OtherEnemies, int enemiesLen) {
	for (int i = 0; i < enemiesLen; i+=1) {
		if (OtherEnemies[i].moveTargetTilePos.x == tile.x && OtherEnemies[i].moveTargetTilePos.y == tile.y) {
			return i;
		}
	}
	return -1;
}

double getTurnElapsedTime(double gameTime, double nextTurnTime, double TurnDuration) {
	double remainingTime = nextTurnTime - gameTime;
	double elapsedTime = TurnDuration - remainingTime;
	if (elapsedTime > TurnDuration) { elapsedTime = TurnDuration; }
	return elapsedTime;
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
		bool adjacentTile = cardinallyAdjacent(state->cursTilePos, state->playerEnt.tilePos);
		int enemyIdxOccupy = tileOccupiedByEnemy(state->cursTilePos, &state->enemies, state->enemiesLen);
		if (enemyIdxOccupy != -1 && adjacentTile) {
			//int newEnemiesLen = swapAndDropEnemy(enemyIdxOccupy, &state->enemies, state->enemiesLen);
			//state->enemiesLen = newEnemiesLen;
			state->playerEnt.nextTurnState = ATTACKING;
			state->playerEnt.combatTargetTilePos = state->cursTilePos;
		}
		else {

			if (state->playerEnt.movePathIdx < state->playerEnt.pathsize) {
				state->playerEnt.pathsize = findAsPath(state->playerEnt.moveTargetTilePos, state->cursTilePos, state->mapData, state->playerEnt.path, 200);
			}
			else {
				state->playerEnt.pathsize = findAsPath(state->playerEnt.tilePos, state->cursTilePos, state->mapData, state->playerEnt.path, 200);
				state->playerEnt.moveTargetTilePos = mapIdxToXY(state->playerEnt.path[0], state->mapSizeX);
			}
			state->playerEnt.movePathIdx = 0;
			state->playerEnt.nextTurnState = MOVING;

		}
	}

	//-------------------------------------------------------------------------------------------
	//ABOVE: Check all the above input, to see if we kick the player out of IDLE and start a turn
	//		 from initaiting moving or attacking etc
	//-------------------------------------------------------------------------------------------


	if (state->gameTime > state->nextTurnTime) {


		state->playerEnt.currentState = state->playerEnt.nextTurnState;

		//-----------------------------------------------------------------------
		//BELOW: All the logic to run to end the previous turn
		//		 Using this section to finilise any movement and 
		//		 set states back to IDLE if nothing in the actions/movement queue
		//------------------------------------------------------------------------

		//try to set player to Idle if and stop turns incrementing

		if (state->playerEnt.currentState == MOVING && !setEntityIdleIfPathEnd(&state->playerEnt)) {
			//If we didn't idle from reaching the end of the path, check if there is an obstruction in the path
			//If there is, set to idle, so player can react
			iVec2 nextTile = mapIdxToXY(state->playerEnt.path[state->playerEnt.movePathIdx + 1], state->mapSizeX);
			int enemyIdxOccupy = tileOccupiedByEnemy(nextTile, &state->enemies, state->enemiesLen);
			if (enemyIdxOccupy != -1) {
				state->playerEnt.currentState = IDLE;
				state->playerEnt.pathsize = 0;
				state->playerEnt.movePathIdx = 0;
			}
		}

		else if (state->playerEnt.currentState == ATTACKING) {
			state->playerEnt.nextTurnState = IDLE;
			state->playerEnt.pathsize = 0;
			state->playerEnt.movePathIdx = 0;
		}

		//Make sure the player and enemies are the target tile from the end of last turn
		state->playerEnt.tilePos = state->playerEnt.moveTargetTilePos;
		state->playerEnt.renderWorldPos.x = state->playerEnt.moveTargetTilePos.x;
		state->playerEnt.renderWorldPos.y = state->playerEnt.moveTargetTilePos.y;

		for (int i = 0; i < state->enemiesLen; i += 1) {

			state->enemies[i].tilePos = state->enemies[i].moveTargetTilePos;
			state->enemies[i].renderWorldPos.x = state->enemies[i].moveTargetTilePos.x;
			state->enemies[i].renderWorldPos.y = state->enemies[i].moveTargetTilePos.y;
			state->enemies[i].currentState = IDLE;
		}

		//-----------------------------------------------------------------
		//BELOW: Logic to determine if we automatically start the next turn
		//-----------------------------------------------------------------

		if (state->playerEnt.currentState != IDLE) {
			state->nextTurnTime = state->gameTime + state->turnDuration;
			state->curTurn += 1;

			//TODO: Maybe we can just do all the turn update here??
		}
	}

	if (state->curTurn != state->prevTurn) {
		state->prevTurn = state->curTurn;


		
		//if (updateEntityMovement(&state->playerEnt, state->mapSizeX)) {
		if (state->playerEnt.pathsize > 0 && state->playerEnt.currentState == MOVING) {
			updateEntityPath(&state->playerEnt, state->mapSizeX);
		}




		//All non-player entity turn logic happens here

		//Probably want to do the same as in the player logic where we sync all the entities to their prev target as the turn has ended

		for (int i = 0; i < state->enemiesLen; i += 1) {
			calculateEnemyTurn(&state->enemies[i], state->playerEnt,&state->enemies,state->enemiesLen, state->playArea,state->mapSizeX);
		}

	}

	//NON GAME LOGIC UPDATES
	//UPDATE ENTITY WORLD POSITIONS AND ANIMATION FOR RENDERING


	updateEntityRenderPos(&state->playerEnt, state->turnDuration, getTurnElapsedTime(state->gameTime, state->nextTurnTime, state->turnDuration));

	for (int i = 0; i < state->enemiesLen; i += 1) {
		updateEntityRenderPos(&state->enemies[i], state->turnDuration, getTurnElapsedTime(state->gameTime, state->nextTurnTime, state->turnDuration));
	}


	drawFrame(state);
}



void populateEnemies(Rectangle playArea, Entity* entityArr, int entityArrLen) {

	for (int i = 0; i < entityArrLen; i+=1) {
		entityArr[i].aniFrame = 0;
		entityArr[i].baseTexSource = (struct Rectangle){ 0.0f, 16.0f, 16.0f, 16.0f }; //ORC
		entityArr[i].currentState = IDLE;
		entityArr[i].movePathIdx = 0;
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
		entityArr[i].moveTargetTilePos = entityArr[i].tilePos;
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
	InitWindow(state.screenWidth, state.screenHeight, "BlackVault - Descent");

	state.cursTilePos = (iVec2){ 13,8 };
	state.playerEnt = initEntity(PLAYER, "Player", sizeof("Player"), (struct iVec2) { 13, 8 }, (struct Rectangle) { 0.0f, 0.0f, 16.0f, 16.0f });
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
	state.turnDuration = 0.6f;

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

