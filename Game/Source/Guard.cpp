#include "Input.h"
#include "Audio.h"
#include "DungeonScene.h"
#include "Pathfinding.h"
#include "Guard.h"
#include "Collider.h"

#include "Animation.h"

Guard::Guard(fPoint position, bool flip, Player* parent) : Enemy(EntityType::GUARD, position, flip, parent)
{
	resting = false;

	chasing = false;
	falling = false;
	jumping = false;

	timeOnAir = 0.0f;

	fPoint velocity = { 0.0f,0.0f };

	texture = app->tex->Load("Assets/textures/guard.png");

	walk = new Animation(4, true, 0.3f);
	walk->PushBack(0, 0, 16, 16);
	walk->PushBack(16, 0, 16, 16);
	walk->PushBack(32, 0, 16, 16);
	walk->PushBack(16, 0, 16, 16);

	chase = new Animation(4, true, 0.1f);
	chase->PushBack(0, 16, 16, 16);
	chase->PushBack(16, 16, 16, 16);
	chase->PushBack(32, 16, 16, 16);
	chase->PushBack(16, 16, 16, 16);

	currentAnimation = walk;
}

Guard::~Guard()
{
	app->tex->UnLoad(texture);

	if (path)
		delete path;

	delete walk;
	delete chase;
}

bool GuardDecoder(int id)
{
	if (id == 2)
		return true;
	return false;
}

bool ChaseDecoder(int id)
{
	if (id == 2 || id == 3)
		return true;
	return false;
}

bool Guard::Update(float dt)
{
	float speed = 25.0f * dt;

	iPoint visionSize = { 100,200 };
	fPoint visionPos = { position.x, position.y - visionSize.y / 2 };
	if(flip)
		visionPos.x = visionPos.x - visionSize.x;

	if(chasing)
	{
		if(currentAnimation != chase)
			currentAnimation = chase;
	}
	else
	{
		if(currentAnimation != walk)
			currentAnimation = walk;
	}

	if(app->dungeonscene->player)
	{
		if(!falling)
		{
			if(visionPos.x <= app->dungeonscene->player->position.x && visionPos.x + visionSize.x >= app->dungeonscene->player->position.x &&
				visionPos.y <= app->dungeonscene->player->position.y && visionPos.y + visionSize.y >= app->dungeonscene->player->position.y)
			{
				pathTimer += dt;
				if(pathTimer >= pathColdown)
				{
					pathTimer = 0.0f;
					if(path)
					{
						delete path;
						path = nullptr;
					}

					if(!path && !jumping)
						path = app->paths->PathTo(position, app->dungeonscene->player->position, ChaseDecoder, true);
				}

				if(!chasing)
				{
					app->audio->PlayFx(app->dungeonscene->guardAlertSound);
					delete path;
					path = nullptr;
					path = app->paths->PathTo(position, app->dungeonscene->player->position, ChaseDecoder, true);
				}

				chasing = true;
			}
			else
			{
				if(chasing)
				{
					app->audio->PlayFx(app->dungeonscene->guardStopChaseSound);
					delete path;
					path = nullptr;
					chasing = false;
					flip = !flip;
				}
				if(!path)
				{
					float distance = 50.0f;
					if(flip)
						distance = distance * -1;
					path = app->paths->PathTo(position, { position.x - distance, position.y }, GuardDecoder, true);
				}
			}
		}
		velocity.x = 0.0f;
		if(path)
		{
			if (app->pathDebug)
				path->DrawPath();
			bool end;
			fPoint destination = path->NextPoint(position, end);
			dest = destination;

			if(falling)
			{
				Walk(speed * 2, destination.x);
				if (position.y < destination.y)
				{
					velocity.y += timeOnAir * dt;
					timeOnAir += 5000.0f * dt;
					if (position.y + (velocity.y * dt) > destination.y)
					{
						position.y = destination.y;
						velocity.y = 0.0f;
					}
				}
				else
				{
					position.y = destination.y;
					falling = false;
					velocity.y = 0.0f;
					timeOnAir = 0.0f;
				}
			}

			if(end)
			{
				delete path;
				path = nullptr;
				flip = !flip;
			}
			else
			{
				if(!chasing)
				{
					Walk(speed, destination.x);
				}
				else
				{ 
					jumping = false;
					MoveType type = MoveType::NONE;
					fPoint specialDest = GetSpecialDestination(type);

					switch (type)
					{
					case MoveType::NONE:
						Walk(speed * 2, destination.x);
						break;
					case MoveType::JUMP:
						dest = position;
						jumping = true;
						break;
					case MoveType::FALL:
						falling = true;
						break;
					case MoveType::ERROR:
						flip = !flip;
						break;
					}
				}
			}
		}
	}

	position.x += velocity.x;
	position.y += velocity.y * dt;

	fPoint dPosition = GetDrawPosition(size);
	body->SetPosition((int)dPosition.x, (int)dPosition.y);

	app->render->SetTextureEvent(5, texture, dPosition, currentAnimation->GetFrame(dt), flip);

	if(app->pathDebug)
	{
		app->render->SetRectangleEvent(10, visionPos, visionSize, 255, 0, 0, 255, true, false);
	}

	return true;
}

fPoint* GetPathInfo(const Path& path, int& size, int& current)
{
	size = path.size;
	current = path.current;

	return path.path;
}

void Guard::Walk(const float& speed, const float& destination)
{
	if (position.x < destination)
	{
		velocity.x = speed;
		if (position.x + velocity.x > destination)
			velocity.x = destination - position.x;
		flip = false;
	}
	else if (position.x > destination)
	{
		velocity.x = -speed;
		if (position.x + velocity.x < destination)
			velocity.x = -(position.x - destination);
		flip = true;
	}
}

void SetCurrent(Path* p, int newCurrent)
{
	p->current = newCurrent;
}

fPoint Guard::GetSpecialDestination(MoveType& type)
{
	int size = 0;
	int current = 0;
	fPoint* points = GetPathInfo(*path, size, current);
	current--;

	if(!points)
		return position;

	if(current + 1 >= size)
		return points[current++];

	if(current < 0 && points[0].y == points[1].y)
		return points[0];
	else if(current < 0 && position.x == points[0].x)
		current = 0;
	else if(current < 0)
		if(size == 1)
			current = 0;
		else
			return points[0];

	if(points[current].y < points[current + 1].y || position.y < points[current].y) //FALL 
	{
		type = MoveType::FALL;
		for(uint i = current; i < size; i++)
		{
			if(i + 1 != size)
			{
				if(points[i + 1].y <= points[i].y)
				{
					SetCurrent(path, i);
					return points[i];
				}
				continue;
			}
			SetCurrent(path, i);
			return points[i];
		}
	}
	else if(points[current].y > points[current + 1].y || position.y > points[current].y) //JUMP
	{
		type = MoveType::JUMP;
		for(uint i = current; i < size; i++)
		{
			if(i + 1 != size)
			{
				if(points[i + 1].y >= points[i].y)
					return points[i];
				continue;
			}
			return points[i];
		}
	}

	return points[current++];
}
