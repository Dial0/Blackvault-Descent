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
#include "entity_actions.c"
#include "game_loop.c"

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
		entityArr[i].renderWorldPos = (struct Vector2){ (float)randPos.x,(float)randPos.y};

		entityArr[i].atkStr = 1;
		entityArr[i].hitPoints = 4;
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
	state.turnPhase = TURN_PHASE_WAITING;

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
		updateGameLoop(&state);
}
#endif
	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}

