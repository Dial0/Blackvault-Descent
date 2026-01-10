Font font;
int fontSize = 15;

typedef struct iVec2 {
	int x;
	int y;
} iVec2;

enum EntityType { PLAYER, ENEMY };

enum EntityState { IDLE, MOVING, INITIATE_ATTACK, ATTACKING };

typedef struct Entity {
	int id;
	enum EntityType type;

	char name[32];
	iVec2 tilePos;
	Vector2 renderWorldPos;
	iVec2 moveTargetTilePos;

	iVec2 combatTargetTilePos;

	enum EntityState currentState;
	enum EntityState nextTurnState;
	int path[200];
	int pathsize;
	int movePathIdx;
	int aniFrame;
	Rectangle baseTexSource;
} Entity;

