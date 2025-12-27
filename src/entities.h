#pragma once
#include "raylib.h"
enum EntityState { IDLE, MOVING, ATTACKING };

typedef struct Entity {
	iVec2 tilePos;
	Vector2 worldPos;
	iVec2 targetTilePos;
	float moveSpeed;
	enum EntityState eState;
	int path[200];
	int pathsize;
	int movePathIdx;
	int aniFrame;
	Rectangle baseTexSource;
} Entity;