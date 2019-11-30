#include "j1App.h"
#include "j1Player.h"
#include "j1Render.h"
#include "j1RectSprites.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Input.h"
#include "j1Animation.h"
#include "p2Log.h"
#include "j1Collision.h"
#include "j1Map.h"
#include "j1Scene.h"
#include "j1Fade.h"
#include "Brofiler/Brofiler.h"
#include "j1FlyingEnemy.h"
#include "j1PathFinding.h"
#include "j1GroundEnemy.h"

j1GroundEnemy::j1GroundEnemy() : j1Entity(entityTypes::GROUND_ENEMY)
{
	name.create("goundEnemy");

	float speed = 0.5f;

	idle.PushBack({ 23,165,50,29 }, speed);
	idle.PushBack({ 75,165,50,29 }, speed);
	idle.PushBack({ 127,165,50,29 }, speed);
	idle.PushBack({ 179,165,50,29 }, speed);
	idle.PushBack({ 232,165,50,29 }, speed);
	idle.PushBack({ 284,165,50,29 }, speed);
	idle.PushBack({ 335,165,50,29 }, speed);
	idle.PushBack({ 388,165,50,29 }, speed);
	idle.PushBack({ 440,165,50,29 }, speed);
	idle.PushBack({ 492,165,50,29 }, speed);
	idle.PushBack({ 544,165,50,29 }, speed);

	run.PushBack({ 24,198,50,30 }, speed);
	run.PushBack({ 76,198,50,30 }, speed);
	run.PushBack({ 128,198,50,30 }, speed);
	run.PushBack({ 180,198,50,30 }, speed);
	run.PushBack({ 223,198,50,30 }, speed);
	run.PushBack({ 284,198,50,30 }, speed);

	stunning.PushBack({ 24,234,44,36 }, speed);
	stunning.PushBack({ 74,234,44,36 }, speed);
	stunning.PushBack({ 125,234,44,36 }, speed);
	stunning.PushBack({ 176,234,44,36 }, speed);

	hit.PushBack({ 24,268,52,29 }, speed);
	hit.PushBack({ 76,268,52,29 }, speed);
	hit.PushBack({ 128,268,52,29 }, speed);
	hit.PushBack({ 180,268,52,29 }, speed);
	hit.PushBack({ 232,268,52,29 }, speed);
}

bool j1GroundEnemy::Awake(pugi::xml_node& config)
{
	config = App->GetConfig();
	gravity = config.child("entityManager").child("gravity").attribute("value").as_float();
	config = config.child("entityManager").child("groundEnemy");

	//variables definition
	velocity = config.child("velocity").attribute("value").as_float();

	return true;
}

bool j1GroundEnemy::Start()
{
	spritesheet_entity = App->tex->Load("character/enemies_spritesheet.png");
	spritesheet = App->tex->Load("character/enemies_spritesheet.png");
	debug_tex = App->tex->Load("maps/pathRect.png");

	state = entityStates::IDLE;	

	position.x = 500;
	position.y = 180;

	current_animation = &idle;

	collider_entity = App->collision->AddCollider(current_animation->GetCurrentFrame(), COLLIDER_GROUND_ENEMY, "rino", (j1Module*)App->groundEnemy); //a collider to start

	return true;
}

bool j1GroundEnemy::PreUpdate()
{
	switch (state)
	{
	case entityStates::IDLE:
		if (moving_left)
			state = entityStates::RUN_BACKWARD;
		if (moving_right)
			state = entityStates::RUN_FORWARD;

		break;
	case entityStates::RUN_FORWARD:
		if(stun)
			state = entityStates::STUNNED;
		if (moving_left)
			state = entityStates::RUN_BACKWARD;

		break;
	case entityStates::RUN_BACKWARD:
		if (stun)
			state = entityStates::STUNNED;
		if (moving_right)
			state = entityStates::RUN_FORWARD;

		break;
	case entityStates::STUNNED:
		if (!stun)
			state = entityStates::IDLE;

		break;
	case entityStates::HIT:
		if(!being_hit)
			state = entityStates::IDLE;

		break;
	}




	return true;
}

bool j1GroundEnemy::Update(float dt)
{
	switch (state)
	{
	case entityStates::IDLE:
		current_animation = &idle;

		break;
	case entityStates::RUN_FORWARD:
		current_animation = &run;
		reversed = true;
		moving_left = false;
		moving_right = true;
		position.x += 2;
		break;
	case entityStates::RUN_BACKWARD:
		current_animation = &run;
		reversed = false;
		moving_left = true;
		moving_right = false;
		position.x -= 2;
		break;
	case entityStates::STUNNED:
		current_animation = &stunning;
		break;
	case entityStates::HIT:
		current_animation = &hit;
		break;
	}
	
	//BLIT
	if(reversed)
		App->render->Blit(spritesheet_entity, position.x, position.y, &current_animation->GetCurrentFrame(),1,2)	;
	else
		App->render->Blit(spritesheet_entity, position.x, position.y, &current_animation->GetCurrentFrame());

	collider_entity->SetPos(position.x, position.y);

	calculate_path();
	blit_path();

	return true;
}

bool j1GroundEnemy::CleanUp()
{
	App->tex->UnLoad(spritesheet_entity);
	App->tex->UnLoad(spritesheet);
	App->tex->UnLoad(debug_tex);

	return true;
}

void j1GroundEnemy::OnCollision(Collider* c1, Collider* c2)
{

}

void j1GroundEnemy::calculate_path()
{
	iPoint origin = App->map->WorldToMap(App->player->position.x, App->player->position.y);

	iPoint p = App->render->ScreenToWorld(position.x, position.y);
	p = App->map->WorldToMap(position.x, position.y);
	if (App->player->position.x - position.x >= -160 && position.x - App->player->position.x >= -160) {
		App->pathfinding->CreatePath(origin, p);
		if (set_path == true) {
			check_path_toMove();
		}
	}
}

void j1GroundEnemy::blit_path()
{
	const p2DynArray<iPoint>* path = App->pathfinding->GetLastPath();

	for (uint i = 0; i < path->Count(); ++i)
	{
		iPoint pos = App->map->MapToWorld(path->At(i)->x, path->At(i)->y);
		App->render->Blit(debug_tex, pos.x, pos.y);
	}
}

void j1GroundEnemy::check_path_toMove()
{
	const p2DynArray<iPoint>* path = App->pathfinding->GetLastPath();
	iPoint pos = App->map->MapToWorld(path->At(0)->x, path->At(0)->y);

	if (pos.x < position.x) {
		moving_left = true;
		moving_right = false;
	}
	if (pos.x > position.x) {
		moving_right = true;
		moving_left = false;
	}


}
