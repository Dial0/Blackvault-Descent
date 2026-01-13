
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

	if (openSet != NULL) {
		openSet[0] = start;
	}
	else {
		return 0;
	}
	
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

void calculateEnemyTurn(Entity* enemyEntity, Entity player, Entity* OtherEnemies, int enemiesLen, Rectangle playArea, int mapSizeX) {


	if (cardinallyAdjacent(player.moveTargetTilePos, enemyEntity->tilePos)) {
		enemyEntity->currentState = ATTACKING;
		enemyEntity->combatTargetTilePos = player.moveTargetTilePos;
		return;
	}

	// Choose the adjacent tile closest to the player
	float minDist = iVec2fDistance(player.moveTargetTilePos, enemyEntity->tilePos);
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
		candidate.x = (int)Clamp((float)candidate.x, playArea.x, playArea.x + playArea.width);
		candidate.y = (int)Clamp((float)candidate.y, playArea.y, playArea.y + playArea.height);

		//check here is there is already a entity in the target tile
		//if there is continue

		//TODO: update this logic to use a pathing algorithm to determine which tile will be best to move towards to get access to the player for attacking
		if (player.moveTargetTilePos.x == candidate.x && player.moveTargetTilePos.y == candidate.y) {
			continue;
		}

		bool collision = false;
		for (int i = 0; i < enemiesLen; i++) {
			if (OtherEnemies[i].moveTargetTilePos.x == candidate.x && OtherEnemies[i].moveTargetTilePos.y == candidate.y) {
				collision = true;
			}
		}

		if (collision == true) {
			continue;
		}

		float dist = iVec2fDistance(player.moveTargetTilePos, candidate);

		if (dist < minDist) {
			minDist = dist;
			bestTarget = candidate;
			enemyEntity->currentState = MOVING;
		}
	}

	// If no better move found, bestTarget remains current position (stays put)
	enemyEntity->moveTargetTilePos = bestTarget;
	enemyEntity->pathsize = 1;  // Move to this single target tile
	enemyEntity->path[0] = mapXYtoIdx(bestTarget.x, bestTarget.y, mapSizeX);

}