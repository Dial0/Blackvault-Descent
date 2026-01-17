
void handleInput(State* state) {

	if (IsKeyReleased(KEY_RIGHT)) state->cursTilePos.x += 1;
	if (IsKeyReleased(KEY_LEFT)) state->cursTilePos.x -= 1;
	if (IsKeyReleased(KEY_UP)) state->cursTilePos.y += 1;
	if (IsKeyReleased(KEY_DOWN)) state->cursTilePos.y -= 1;

	if (state->cursTilePos.y < 0) state->cursTilePos.y = 0;
	if (state->cursTilePos.y >= state->mapSizeY) state->cursTilePos.y = state->mapSizeY - 1;
	if (state->cursTilePos.x < 0) state->cursTilePos.x = 0;
	if (state->cursTilePos.x >= state->mapSizeX) state->cursTilePos.x = state->mapSizeX - 1;

	Vector2 mousePos = GetMousePosition();

	state->cursTilePos = screenXYtoMapTileXY((int)(mousePos.x / state->renderParams.scale), (int)(mousePos.y / state->renderParams.scale), state->renderParams);

	int playAreaYMin = (int)state->playArea.y;
	int playAreaYMax = (int)(state->playArea.y + state->playArea.height);
	int playAreaXMin = (int)state->playArea.x;
	int playAreaXMax = (int)(state->playArea.x + state->playArea.width);
	if (state->cursTilePos.y < playAreaYMin) { state->cursTilePos.y = playAreaYMin; }
	if (state->cursTilePos.y > playAreaYMax) { state->cursTilePos.y = playAreaYMax; }
	if (state->cursTilePos.x < playAreaXMin) { state->cursTilePos.x = playAreaXMin; }
	if (state->cursTilePos.x > playAreaXMax) { state->cursTilePos.x = playAreaXMax; }

	int cursorYScreen = (int)(mousePos.y / state->renderParams.scale);

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
		int enemyIdxOccupy = tileOccupiedByEnemy(state->cursTilePos, &state->enemies[0], state->enemiesLen);
		if (enemyIdxOccupy != -1 && adjacentTile) {
			//int newEnemiesLen = swapAndDropEnemy(enemyIdxOccupy, &state->enemies, state->enemiesLen);
			//state->enemiesLen = newEnemiesLen;
			state->playerEnt.nextTurnState = ATTACKING;
			//TODO: Set this to next turn target? or find a way to only update the target at the start of next turn?
			//Maybe when we create the animation we make a copy to the target tile and then don't update it till next turn?
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



			iVec2 nextPathTile = mapIdxToXY(state->playerEnt.path[1], state->mapSizeX);
			int enemyIdxOccupy = tileOccupiedByEnemy(nextPathTile, &state->enemies[0], state->enemiesLen);
			if (enemyIdxOccupy == -1) {
				state->playerEnt.movePathIdx = 0;
				state->playerEnt.nextTurnState = MOVING;
			}
			else {
				state->playerEnt.pathsize = 0;
				state->playerEnt.movePathIdx = 0;
				state->playerEnt.nextTurnState = IDLE;
			}



		}
	}
	return;
}

bool playerHasQueuedAction(State* state) {
	return state->playerEnt.nextTurnState != IDLE;
}

void startNewTurn(State* state) {

	state->playerEnt.currentState = state->playerEnt.nextTurnState;

	state->nextTurnTime = state->gameTime + state->turnDuration;
	state->curTurn += 1;

	if (state->playerEnt.currentState == ATTACKING) {
		state->playerEnt.nextTurnState = IDLE;
		state->playerEnt.pathsize = 0;
		state->playerEnt.movePathIdx = 0;
		state->playerEnt.animation.type = ANIM_ATTACKING;
		state->playerEnt.animation.data.attacking.start = state->playerEnt.tilePos;
		state->playerEnt.animation.data.attacking.end = state->playerEnt.combatTargetTilePos;

		for (int i = 0; i < state->enemiesLen; i += 1) {
			iVec2 enemyPos = state->enemies[i].moveTargetTilePos;
			iVec2 playerAtkPos = state->playerEnt.combatTargetTilePos;
			if (enemyPos.x == playerAtkPos.x && enemyPos.y == playerAtkPos.y) {
				state->enemies[i].hitPoints -= state->playerEnt.atkStr;
				if (state->enemies[i].hitPoints <= 0) {
					state->enemies[i].hitPoints = 0;
					state->enemies[i].currentState = DEAD;
				}
			}
		}
	}

	if (state->playerEnt.currentState == MOVING) {
		updateEntityPath(&state->playerEnt, state->mapSizeX);
		state->playerEnt.animation.type = ANIM_MOVING;
		state->playerEnt.animation.data.moving.start = state->playerEnt.tilePos;
		state->playerEnt.animation.data.moving.end = state->playerEnt.moveTargetTilePos;
	}

	//All non-player entity turn logic happens here

	//Probably want to do the same as in the player logic where we sync all the entities to their prev target as the turn has ended

	for (int i = 0; i < state->enemiesLen; i += 1) {

		if (state->enemies[i].currentState == DEAD) {
			continue;
		}

		calculateEnemyTurn(&state->enemies[i], state->playerEnt, &state->enemies[0], state->enemiesLen, state->playArea, state->mapSizeX);
	}

	return;
}

void updateTurnProgress(State* state) {
	return;
}

void endTurn(State* state) {

	if (state->playerEnt.currentState == MOVING && !setEntityIdleIfPathEnd(&state->playerEnt)) {
		//If we didn't idle from reaching the end of the path, check if there is an obstruction in the path
		//If there is, set to idle, so player can react
		iVec2 nextTile = mapIdxToXY(state->playerEnt.path[state->playerEnt.movePathIdx + 1], state->mapSizeX);
		int enemyIdxOccupy = tileOccupiedByEnemy(nextTile, &state->enemies[0], state->enemiesLen);
		if (enemyIdxOccupy != -1) {
			state->playerEnt.nextTurnState = IDLE;
			state->playerEnt.pathsize = 0;
			state->playerEnt.movePathIdx = 0;
		}
	}

	if (state->playerEnt.currentState == IDLE || state->playerEnt.nextTurnState == IDLE) {
		state->playerEnt.animation.type = ANIM_IDLE;
	}

	//Make sure the player and enemies are the target tile from the end of last turn
	state->playerEnt.tilePos = state->playerEnt.moveTargetTilePos;
	state->playerEnt.renderWorldPos.x = (float)state->playerEnt.moveTargetTilePos.x;
	state->playerEnt.renderWorldPos.y = (float)state->playerEnt.moveTargetTilePos.y;

	for (int i = 0; i < state->enemiesLen; i += 1) {

		state->enemies[i].tilePos = state->enemies[i].moveTargetTilePos;
		state->enemies[i].renderWorldPos.x = (float)state->enemies[i].moveTargetTilePos.x;
		state->enemies[i].renderWorldPos.y = (float)state->enemies[i].moveTargetTilePos.y;

		if (state->enemies[i].currentState != DEAD) {
			state->enemies[i].currentState = IDLE;
		}
	}

	return;
}

void updateRender(State* state) {
	updateEntityAnimation2(&state->playerEnt, state->turnDuration, getTurnElapsedTime(state->gameTime, state->nextTurnTime, state->turnDuration));

	for (int i = 0; i < state->enemiesLen; i += 1) {
		updateEntityAnimation(&state->enemies[i], state->turnDuration, getTurnElapsedTime(state->gameTime, state->nextTurnTime, state->turnDuration));
	}
	return;
}

void UpdateDrawFrameTest(void* v_state) {
	State* state = (State*)v_state;
	state->gameTime = GetTime();

	// ---------------------------------------------------------------
	// A. Always - frame-rate independent input / UI
	// ---------------------------------------------------------------

	handleInput(state);           // cursor movement, mouse hover, scrolling etc.

	// ---------------------------------------------------------------
	// B. Turn start gate - only when waiting AND time is ready AND action queued
	// ---------------------------------------------------------------
	if (state->turnPhase == TURN_PHASE_WAITING) {
		if (state->gameTime > state->nextTurnTime && playerHasQueuedAction(state)) {
			state->turnPhase = TURN_PHASE_IN_PROGRESS;
			startNewTurn(state);
		}
	}

	// ---------------------------------------------------------------
	// C. During active turn - smooth progress & per-frame updates
	// ---------------------------------------------------------------
	if (state->turnPhase == TURN_PHASE_IN_PROGRESS) {
		updateTurnProgress(state);          // lerp positions, advance animations, etc.

		// Check if the 600 ms window has ended
		if (state->gameTime >= state->nextTurnTime) {
			endTurn(state);
			state->turnPhase = TURN_PHASE_WAITING;
		}
	}

	// ---------------------------------------------------------------
	// D. Always - prepare rendering
	// ---------------------------------------------------------------
	updateRender(state);
	drawFrame(state);
}