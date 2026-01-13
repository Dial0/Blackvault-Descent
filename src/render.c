
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

		Rectangle pathTileSrc = { (float)(drawtileID * state->tileSize), (float)(2 * state->tileSize), (float)state->tileSize,(float)state->tileSize };

		iVec2 tileLoc = mapIdxToXY(state->playerEnt.path[i], state->mapSizeX);
		iVec2 pathPos = mapTileXYtoScreenXY(tileLoc.x, tileLoc.y, state->renderParams);

		DrawTextureRec(state->ui, pathTileSrc, (struct Vector2) { (float)pathPos.x, (float)pathPos.y }, WHITE);


	}

}


void renderEntity(Entity entity, RenderParams renderParams, Texture2D tex) {
	iVec2 playerEntityPixelPos = mapWorldXYtoScreenXY(entity.renderWorldPos.x, entity.renderWorldPos.y, renderParams);

	bool alternateAnimation = ((int)entity.aniFrame / 10) % 2;


	if (alternateAnimation) {

		Rectangle newTexSource = entity.baseTexSource;
		newTexSource.x += 16;
		DrawTextureRec(tex, newTexSource, (struct Vector2) { (float)playerEntityPixelPos.x, (float)playerEntityPixelPos.y }, WHITE);

	}
	else {
		DrawTextureRec(tex, entity.baseTexSource, (struct Vector2) { (float)playerEntityPixelPos.x, (float)playerEntityPixelPos.y }, WHITE);

	}
}


void drawFrame(State* state) {

	BeginDrawing();

	ClearBackground(RAYWHITE);



	BeginTextureMode(state->mapRendTex);

	ClearBackground(RAYWHITE);

	int scrollOffset = (state->smoothScrollY / state->scale);

	int mapRenderOffset = -(state->map.height - state->mapRendTex.texture.height);

	DrawTexture(state->map, 0, mapRenderOffset + scrollOffset, WHITE);

	Rectangle cursorRec = { 0.0f,0.0f,18.0f,18.0f };
	int centOff = (int)((cursorRec.width - state->tileSize) / 2);
	iVec2 cursPixel = mapTileXYtoScreenXY(state->cursTilePos.x, state->cursTilePos.y, state->renderParams);

	DrawTextureRec(state->ui, cursorRec, (struct Vector2) { (float)(cursPixel.x - centOff), (float)(cursPixel.y - centOff )}, WHITE);

	if (state->playerEnt.pathsize) {
		RenderPlayerPath(state);
		Rectangle pathEndRec = { 19.0f,0.0f,18.0f,18.0f };
		iVec2 tilePathEnd = mapIdxToXY(state->playerEnt.path[state->playerEnt.pathsize - 1], state->mapSizeX);
		iVec2 pathEndPixel = mapTileXYtoScreenXY(tilePathEnd.x, tilePathEnd.y, state->renderParams);
		DrawTextureRec(state->ui, pathEndRec, (struct Vector2) { (float)(pathEndPixel.x - centOff), (float)(pathEndPixel.y - centOff )}, WHITE);
	}

	//PLAYER
	renderEntity(state->playerEnt, state->renderParams, state->ent);


	for (int i = 0; i < state->enemiesLen; i += 1) {
		renderEntity(state->enemies[i], state->renderParams, state->ent);
	}


	EndTextureMode();

	BeginTextureMode(state->uiRendTex);
	ClearBackground(BLANK);

	EndTextureMode();


	Rectangle mapRendTexSrc = { 0, 0, (float)state->mapRendTex.texture.width, (float)(- state->mapRendTex.texture.height)};
	DrawTexturePro(state->mapRendTex.texture, mapRendTexSrc,
		(struct Rectangle) { 0, (float)( - state->scale + state->smoothScrollY % state->scale), (float)(state->baseSizeX* state->scale), (float)(state->baseSizeY* state->scale) },
		(struct Vector2) { 0.0f, 0.0f }, 0.0f, WHITE);

	EndDrawing();
}