
Entity initEntity(enum EntityType type, char* name, int namelen, iVec2 tilePos, Rectangle baseTexSource) {

	Entity newEntity;

	newEntity.id = getNextID();

	newEntity.type = type;

	if (namelen > 32) { namelen = 32; }
	for (int i = 0; i < namelen; i++) {
		newEntity.name[i] = name[i];
	}

	newEntity.tilePos = tilePos;
	newEntity.moveTargetTilePos = tilePos;
	newEntity.renderWorldPos.x = tilePos.x;
	newEntity.renderWorldPos.y = tilePos.y;

	newEntity.currentState = IDLE;
	newEntity.nextTurnState = IDLE;

	newEntity.pathsize = 0;
	newEntity.movePathIdx = 0;
	newEntity.aniFrame = 0;

	newEntity.baseTexSource = baseTexSource;

	return newEntity;

}

bool updateEntityAnimation2(Entity* entity, double turnDuration, double turnElapsed) {

	if (entity->animation.type == ATTACKING) {
		double t = turnElapsed / turnDuration;
		iVec2 iStart = entity->animation.data.attacking.start;
		iVec2 iEnd = entity->animation.data.attacking.end;
		entity->renderWorldPos = interpolatePingpong(iVec2ToVector2(iStart), iVec2ToVector2(iEnd), t);
		entity->aniFrame += 1;
	}

	if (entity->animation.type == MOVING) {

		//if (entity->tilePos.x == entity->moveTargetTilePos.x
		//	&& entity->tilePos.y == entity->moveTargetTilePos.y) {
		//	return false; //Stationary no update required
		//	//TODO: Can this be set to check if the Entity State is IDLE?
		//}
		double t = turnElapsed / turnDuration;
		iVec2 iStart = entity->animation.data.moving.start;
		iVec2 iEnd = entity->animation.data.moving.end;
		entity->renderWorldPos = Vector2Lerp(iVec2ToVector2(iStart) , iVec2ToVector2(iEnd), t); 
		entity->aniFrame += 1;
	}



	return true;
}

bool updateEntityAnimation(Entity* entity, double turnDuration, double turnElapsed) {

	if (entity->currentState == ATTACKING) {
		double t = turnElapsed / turnDuration;
		Vector2 start = { entity->tilePos.x, entity->tilePos.y };
		Vector2 end = { entity->combatTargetTilePos.x, entity->combatTargetTilePos.y };
		entity->renderWorldPos = interpolatePingpong(start, end, t);
		entity->aniFrame += 1;
	}

	if (entity->currentState == MOVING)  {
		if (entity->tilePos.x == entity->moveTargetTilePos.x
			&& entity->tilePos.y == entity->moveTargetTilePos.y) {
			return false; //Stationary no update required
			//TODO: Can this be set to check if the Entity State is IDLE?
		}

		Vector2 start = { entity->tilePos.x, entity->tilePos.y };
		Vector2 end = { entity->moveTargetTilePos.x, entity->moveTargetTilePos.y };
		double t = turnElapsed / turnDuration;

		entity->renderWorldPos = Vector2Lerp(start, end, t);

		entity->aniFrame += 1;
	}



	return true;
}

int swapAndDropEnemy(int dropIdx, Entity* Enemies, int enemiesLen) {
	Enemies[dropIdx] = Enemies[enemiesLen - 1];
	enemiesLen -= 1;
	return enemiesLen;
}

updateEntityPath(Entity* entity, int mapSizeX) {
	entity->movePathIdx += 1;
	entity->moveTargetTilePos = mapIdxToXY(entity->path[entity->movePathIdx], mapSizeX);
}


bool setEntityIdleIfPathEnd(Entity* entity) {
	if (entity->movePathIdx >= (entity->pathsize - 1)) {
		entity->movePathIdx = 0;
		entity->pathsize = 0;
		entity->currentState = IDLE;
		return true;
	}
	return false;
}