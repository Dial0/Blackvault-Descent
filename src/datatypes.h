Font font;
int fontSize = 15;

typedef struct iVec2 {
	int x;
	int y;
} iVec2;

//=======
// ENTITY
//=======

typedef enum { PLAYER, ENEMY } EntityType;

typedef enum { IDLE, MOVING, ATTACKING } EntityActionState;

typedef enum {ANIM_IDLE, ANIM_MOVING, ANIM_ATTACKING} EntityAnimType;

typedef struct {
	int dummy;
} IDLE_ANIMATION;

typedef struct {
	iVec2 start;
	iVec2 end;
} MOVING_ANIMATION;

typedef struct {
	iVec2 start;
	iVec2 end;
} ATTACKING_ANIMATION;

typedef union {
	IDLE_ANIMATION idle;
	MOVING_ANIMATION moving;
	ATTACKING_ANIMATION attacking;
} AnimationDataUnion;

typedef struct Animation {
	EntityAnimType type;
	AnimationDataUnion data;
}Animation; 

typedef struct Entity {
	int id;
	EntityType type;

	char name[32];
	iVec2 tilePos;
	Vector2 renderWorldPos;
	iVec2 moveTargetTilePos;

	iVec2 combatTargetTilePos;

	EntityActionState currentState;
	EntityActionState nextTurnState;
	int path[200];
	int pathsize;
	int movePathIdx;
	Animation animation;
	int aniFrame;
	Rectangle baseTexSource;
} Entity;

