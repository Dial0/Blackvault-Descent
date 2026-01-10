
Entity initEntity(enum EntityType type, char* name, int namelen, iVec2 tilePos, Rectangle baseTexSource) {

	Entity newEntity;

	newEntity.id = getNextID();

	newEntity.type = type;

	if (namelen > 32) { namelen = 32; }
	for (int i = 0; i < namelen; i++) {
		newEntity.name[i] = name[i];
	}

	newEntity.tilePos = tilePos;
	newEntity.targetTilePos = tilePos;
	newEntity.renderWorldPos.x = tilePos.x;
	newEntity.renderWorldPos.y = tilePos.y;

	newEntity.eState = IDLE;

	newEntity.pathsize = 0;
	newEntity.movePathIdx = 0;
	newEntity.aniFrame = 0;

	newEntity.baseTexSource = baseTexSource;

	return newEntity;

}

bool updateEntityRenderPos(Entity* entity, double turnDuration, double turnElapsed) {

	if (entity->tilePos.x == entity->targetTilePos.x
		&& entity->tilePos.y == entity->targetTilePos.y) {
		return false; //Stationary no update required
		//TODO: Can this be set to check if the Entity State is IDLE?
	}

	Vector2 start = { entity->tilePos.x, entity->tilePos.y };
	Vector2 end = { entity->targetTilePos.x, entity->targetTilePos.y };
	double t = turnElapsed / turnDuration;
	entity->renderWorldPos = Vector2Lerp(start, end, t);

	entity->aniFrame += 1;

	return true;
}

int swapAndDropEnemy(int dropIdx, Entity* Enemies, int enemiesLen) {
	Enemies[dropIdx] = Enemies[enemiesLen - 1];
	enemiesLen -= 1;
	return enemiesLen;
}

updateEntityPath(Entity* entity, int mapSizeX) {
	entity->movePathIdx += 1;
	entity->targetTilePos = mapIdxToXY(entity->path[entity->movePathIdx], mapSizeX);
}


bool setEntityIdleIfPathEnd(Entity* entity) {
	if (entity->movePathIdx >= (entity->pathsize - 1)) {
		entity->movePathIdx = 0;
		entity->pathsize = 0;
		entity->eState = IDLE;
		return true;
	}
	return false;
}