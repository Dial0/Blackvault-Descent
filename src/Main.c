#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

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

    iVec2 cursTilePos;
    RenderTexture2D mapRendTex;
    RenderTexture2D uiRendTex;
    RenderParams renderParams;

    int path[200];
    int pathsize;
    int movePathIdx;
    int totalPathCost;
    int curTileTurnsTraversed;

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
    int tileYOffset = param.mapViewPortHeight / param.tileSize;
    int scrollOffset = (param.smoothScrollY / param.scale);

    return (struct iVec2) { MapX* param.tileSize, (-MapY + tileYOffset)* param.tileSize + mapRenderOffset % param.tileSize + scrollOffset };
}

iVec2 mapWorldXYtoScreenXY(float MapX, float MapY, RenderParams param) {
    int mapRenderOffset = -(param.mapHeight - param.mapViewPortHeight);
    int tileYOffset = param.mapViewPortHeight / param.tileSize;
    int scrollOffset = (param.smoothScrollY / param.scale);

    return (struct iVec2) { MapX* (float)(param.tileSize), (-MapY + tileYOffset)* (float)(param.tileSize) + mapRenderOffset % param.tileSize + scrollOffset };
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


    float col_length = 1.0f;
    
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

int main(void)
{




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

    state.mapData = LoadFileData("resources/respitetest.rspb", &state.mapDataSize);
    state.mapSizeX = 27;
    state.mapSizeY = 19;

    InitWindow(state.screenWidth, state.screenHeight, "raylib [core] example - basic window");


    state.map = LoadTexture("resources/map.png");
    state.ui = LoadTexture("resources/UI.png");

    font = LoadFontEx("resources/rainyhearts.ttf", fontSize, NULL, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

    state.mapRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);
    state.uiRendTex = LoadRenderTexture(state.baseSizeX, state.baseSizeY);
   
    state.renderParams = (RenderParams){ state.baseSizeX ,state.baseSizeY,state.map.width,state.map.height,state.tileSize,state.smoothScrollY,state.scale };

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

