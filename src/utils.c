int getNextID() {
	static int util_incrementing_id = 0;
	return util_incrementing_id++;
}

static inline int floordiv(int a, int b) {
	if (a >= 0) { return a / b; }
	return (a - b + 1) / b;
}

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

int mapXYtoIdx(unsigned char x, unsigned char y, int mapWidth) {
	unsigned char width = mapWidth;
	return y * width + x;
}

iVec2 mapIdxToXY(int tileIdx, int mapSizeX) {
	int x = tileIdx % mapSizeX;
	int y = tileIdx / mapSizeX;
	return (struct iVec2) { x, y };
}

float iVec2fDistance(iVec2 v1, iVec2 v2) {
	float result = sqrtf(((float)v1.x - (float)v2.x) * ((float)v1.x - v2.x) + ((float)v1.y - (float)v2.y) * ((float)v1.y - (float)v2.y));
	return result;
}

Vector2 iVec2ToVector2(iVec2 v) {
	return (struct Vector2) { v.x,v.y };
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


Vector2 interpolatePingpong(Vector2 start, Vector2 end, float t) {
	Vector2 result;
	float abs_progress = fabsf(2.0f * t - 1.0f);
	result.x = (start.x + end.x) * 0.5f + (start.x - end.x) * 0.5f * abs_progress;
	result.y = (start.y + end.y) * 0.5f + (start.y - end.y) * 0.5f * abs_progress;
	return result;
}


bool cardinallyAdjacent(iVec2 Tile1, iVec2 Tile2) {
	return (abs(Tile1.x - Tile2.x) + abs(Tile1.y - Tile2.y) == 1);
}

