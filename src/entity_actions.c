
typedef enum {
	ACT_MOVE_PATH,
	ACT_MELEE_ATK,
}ActionType;

typedef struct { State* state; iVec2 targetTile; } ActionMovePath; //Maybe update this to take an entity ID? to be more general?
typedef struct { State* state; iVec2 combatTargetTile; } ActionMelee;

typedef struct {
	ActionType type;
	union {
		ActionMovePath actionMovePath;
		ActionMelee actionMelee;
	};
} EntityAction;

typedef void (*EntityActionHandlerStart)(const EntityAction* cmd);
typedef void (*EntityActionHandlerEnd)(const EntityAction* cmd);

static void actionMovePathStart(const EntityAction* v_data) {
	ActionMovePath data = v_data->actionMovePath;
	State* state = data.state;


	updateEntityPath(&state->playerEnt, state->mapSizeX);
	state->playerEnt.animation.type = ANIM_MOVING;
	state->playerEnt.animation.data.moving.start = state->playerEnt.tilePos;
	state->playerEnt.animation.data.moving.end = state->playerEnt.moveTargetTilePos;
}

static void actionMovePathEnd(const EntityAction* v_data) {
	ActionMovePath data = v_data->actionMovePath;
	State* state = data.state;

	bool pathEndReached = state->playerEnt.movePathIdx >= (state->playerEnt.pathsize - 1);

	//If we didn't idle from reaching the end of the path, check if there is an obstruction in the path
	//If there is, set to idle, so player can react
	iVec2 nextTile = mapIdxToXY(state->playerEnt.path[state->playerEnt.movePathIdx + 1], state->mapSizeX);
	int enemyIdxOccupy = tileOccupiedByEnemy(nextTile, &state->enemies[0], state->enemiesLen);

	bool nextTileOccupied = enemyIdxOccupy != -1;

	if (pathEndReached || nextTileOccupied) {
		state->playerEnt.pathsize = 0;
		state->playerEnt.movePathIdx = 0;
		state->playerEnt.nextTurnState = IDLE;
		state->playerEnt.animation.type = ANIM_IDLE;
	}
}

static void actionMeleeStart(const EntityAction* v_data) {
	ActionMelee data = v_data->actionMelee;
	State* state = data.state;

	state->playerEnt.combatTargetTilePos = data.combatTargetTile;
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

static void actionMeleeEnd(const EntityAction* v_data) {
	ActionMelee data = v_data->actionMelee;
	State* state = data.state;

	state->playerEnt.nextTurnState = IDLE;
	state->playerEnt.animation.type = ANIM_IDLE;
}


static const EntityActionHandlerStart startActionHandlers[] = {
	[ACT_MOVE_PATH]	= actionMovePathStart,
	[ACT_MELEE_ATK] = actionMeleeStart
};

static const EntityActionHandlerEnd endActionHandlers[] = {
	[ACT_MOVE_PATH] = actionMovePathEnd,
	[ACT_MELEE_ATK] = actionMeleeEnd
};


void processEntityActionStart(const EntityAction* action) {
	if (action->type >= sizeof(startActionHandlers) / sizeof(startActionHandlers[0]) || !startActionHandlers[action->type]) {
		return;
	}
	startActionHandlers[action->type](action);
}


void processEntityActionEnd(const EntityAction* action) {
	if (action->type >= sizeof(endActionHandlers) / sizeof(endActionHandlers[0]) || !endActionHandlers[action->type]) {
		return;
	}
	endActionHandlers[action->type](action);
}



int ActionExample(State* state) {



	EntityAction commands[] = {
		{.type = ACT_MOVE_PATH,  {state,{ 1.0f, -2.0f}}},
		{.type = ACT_MELEE_ATK ,  {state,{ 1.0f, -2.0f} }}
	};

	for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		processEntityActionStart(&commands[i]);
	}

	return 0;
}