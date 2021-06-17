#include "arcsys.h"
#include "sigscan.h"

using StaticClass_t = UClass*(*)();
const auto AREDGameState_Battle_StaticClass = (StaticClass_t)(get_rip_relative(
	sigscan::get().scan("\x30\x01\x00\x00\x48\x85\xFF\x74\x36", "xxxxxxxxx") + 10));

using asw_scene_camera_transform_t = void(*)(const asw_scene*, FVector*, FVector*, FVector*);
const auto asw_scene_camera_transform = (asw_scene_camera_transform_t)(
	sigscan::get().scan("\x4D\x85\xC0\x74\x15\xF2\x41\x0F", "xxxxxxxx") - 0x56);

using asw_entity_is_active_t = bool(*)(const asw_entity*, bool);
const auto asw_entity_is_active = (asw_entity_is_active_t)(
	sigscan::get().scan("\x0F\x85\x86\x00\x00\x00\x8B\x81\x9C\x01", "xxxxxxxxxx") - 0x1C);

using asw_entity_is_pushbox_active_t = bool(*)(const asw_entity*);
const auto asw_entity_is_pushbox_active = (asw_entity_is_pushbox_active_t)(
	sigscan::get().scan("\xF7\x80\xEC\x5C", "xxxx") - 0x1A);

using asw_entity_get_pos_x_t = int(*)(const asw_entity*);
const auto asw_entity_get_pos_x = (asw_entity_get_pos_x_t)(
	sigscan::get().scan("\xEB\x06\x8B\xBB\x98\x03\x00\x00", "xxxxxxxx") - 0x104);

using asw_entity_get_pos_y_t = int(*)(const asw_entity*);
const auto asw_entity_get_pos_y = (asw_entity_get_pos_y_t)(
	sigscan::get().scan("\x3D\x00\x08\x04\x00\x75\x18", "xxxxxxx") - 0x3D);

using asw_entity_pushbox_width_t = bool(*)(const asw_entity*);
const auto asw_entity_pushbox_width = (asw_entity_pushbox_width_t)(
	sigscan::get().scan("\x8B\x81\xE0\x04\x00\x00\x48\x8B\xD9\x85\xC0", "xxxxxxxxxxx") - 6);

using asw_entity_pushbox_height_t = bool(*)(const asw_entity*);
const auto asw_entity_pushbox_height = (asw_entity_pushbox_height_t)(
	sigscan::get().scan("\x8B\x81\xE4\x04\x00\x00\x48\x8B\xD9\x85\xC0", "xxxxxxxxxxx") - 6);

using asw_entity_pushbox_bottom_t = bool(*)(const asw_entity*);
const auto asw_entity_pushbox_bottom = (asw_entity_pushbox_bottom_t)(
	sigscan::get().scan("\x8B\x81\xE8\x04\x00\x00\x48\x8B\xD9\x3D\xFF", "xxxxxxxxxxx") - 6);

using asw_entity_get_pushbox_t = void(*)(const asw_entity*, int*, int*, int*, int*);
const auto asw_entity_get_pushbox = (asw_entity_get_pushbox_t)(
	sigscan::get().scan("\x99\x48\x8B\xCB\x2B\xC2\xD1\xF8\x44", "xxxxxxxxx") - 0x5B);

UClass *AREDGameState_Battle::StaticClass()
{
	return AREDGameState_Battle_StaticClass();
}

asw_engine *asw_engine::get()
{
	if (*GWorld == nullptr)
		return nullptr;

	const auto *GameState = (*GWorld)->GameState;
	if (GameState == nullptr || !GameState->IsA<AREDGameState_Battle>())
		return nullptr;

	return ((AREDGameState_Battle*)GameState)->Engine;
}

asw_scene *asw_scene::get()
{
	if (*GWorld == nullptr)
		return nullptr;

	const auto *GameState = (*GWorld)->GameState;
	if (GameState == nullptr || !GameState->IsA<AREDGameState_Battle>())
		return nullptr;

	return ((AREDGameState_Battle*)GameState)->Scene;
}

void asw_scene::camera_transform(FVector *delta, FVector *position, FVector *angle) const
{
	asw_scene_camera_transform(this, delta, position, angle);
}

void asw_scene::camera_transform(FVector *position, FVector *angle) const
{
	FVector delta;
	asw_scene_camera_transform(this, &delta, position, angle);
}

bool asw_entity::is_active() const
{
	// Otherwise returns false during COUNTER
	if (cinematic_flags & cinematic_flag::counter)
		return true;

	return asw_entity_is_active(this, !is_player);
}

bool asw_entity::is_pushbox_active() const
{
	return asw_entity_is_pushbox_active(this);
}

bool asw_entity::is_invuln() const
{
	return
		(action_flags2 & (action_flag2::invuln | action_flag2::wakeup)) ||
		backdash_invuln > 0;
}

int asw_entity::get_pos_x() const
{
	return asw_entity_get_pos_x(this);
}

int asw_entity::get_pos_y() const
{
	return asw_entity_get_pos_y(this);
}

int asw_entity::pushbox_width() const
{
	return asw_entity_pushbox_width(this);
}

int asw_entity::pushbox_height() const
{
	return asw_entity_pushbox_height(this);
}

int asw_entity::pushbox_bottom() const
{
	return asw_entity_pushbox_bottom(this);
}

void asw_entity::get_pushbox(int *left, int *top, int *right, int *bottom) const
{
	asw_entity_get_pushbox(this, left, top, right, bottom);
}
