#define _USE_MATH_DEFINES
#include "sigscan.h"
#include "ue4.h"
#include "arcsys.h"
#include <cmath>
#include <array>
#include <Windows.h>

constexpr auto AHUD_PostRender_index = 214;

// Actually AREDHUD_Battle
const auto **AHUD_vtable = (const void**)get_rip_relative(sigscan::get().scan(
	"\x48\x8D\x05\x00\x00\x00\x00\xC6\x83\x18\x03", "xxx????xxxx") + 3);

using AHUD_PostRender_t = void(*)(AHUD*);
AHUD_PostRender_t orig_AHUD_PostRender;

void asw_coords_to_screen(UCanvas *canvas, FVector2D *pos)
{
	pos->X *= asw_engine::coord_scale / 1000.F;
	pos->Y *= asw_engine::coord_scale / 1000.F;

	FVector pos3d(pos->X, 0.f, pos->Y);
	asw_scene::get()->camera_transform(&pos3d, nullptr);

	const auto proj = canvas->K2_Project(pos3d);
	*pos = FVector2D(proj.X, proj.Y);
}

void draw_rect(
	UCanvas *canvas,
	const std::array<FVector2D, 2> &corners,
	const FLinearColor &color,
	float rotation)
{
	FVector2D min, max;

	if (corners[0].X < corners[1].X) {
		min.X = corners[0].X;
		max.X = corners[1].X;
	}

	if (corners[0].Y < corners[1].Y) {
		min.Y = corners[0].Y;
		max.Y = corners[1].Y;
	}

	const auto center = (min + max) / 2;

	std::array<FVector2D, 4> points = {
		FVector2D(max.X, max.Y),
		FVector2D(min.X, max.Y),
		FVector2D(min.X, min.Y),
		FVector2D(max.X, min.Y)
	};

	if (rotation != 0.f) {
		for (auto &pos : points)
			pos = (pos - center).Rotate(rotation) + center;
	}

	for (auto i = 0; i < 4; i++)
		canvas->K2_DrawLine(points[i], points[(i + 1) % 4], 2.F, color);

	canvas->K2_DrawTexture(
		nullptr,
		min,
		max - min,
		FVector2D(),
		FVector2D(1.f, 1.f),
		color,
		BLEND_Translucent,
		rotation);
}

void draw_hitbox(UCanvas *canvas, const asw_entity *entity, const hitbox &box)
{
	std::array<FVector2D, 2> corners = {
		FVector2D(box.x, box.y),
		FVector2D(box.x + box.w, box.y + box.h)
	};

	for (auto &pos : corners) {
		pos.X *= entity->scale_x;
		pos.Y *= entity->scale_y;

		if (entity->facing == direction::right)
			pos.X *= -1.F;

		pos.X += entity->get_pos_x();
		pos.Y += entity->get_pos_y();

		asw_coords_to_screen(canvas, &pos);
	}

	FLinearColor color;
	if (box.type == hitbox::type::hit)
		color = FLinearColor(1.f, 0.f, 0.f, .1f);
	else if (entity->action_flags1 & action_flag1::counterhit)
		color = FLinearColor(0.f, 1.f, 1.f, .1f);
	else
		color = FLinearColor(0.f, 1.f, 0.f, .1f);

	const auto rotation = (float)entity->angle_x / 1000.f;
	if (entity->facing == direction::right)
		draw_rect(canvas, corners, color, -rotation);
	else
		draw_rect(canvas, corners, color, rotation);
}

void draw_pushbox(UCanvas *canvas, const asw_entity *entity)
{
	int left, top, right, bottom;
	entity->get_pushbox(&left, &top, &right, &bottom);

	std::array<FVector2D, 2> corners = {
		FVector2D(left, top),
		FVector2D(right, bottom)
	};

	for (auto &pos : corners)
		asw_coords_to_screen(canvas, &pos);

	draw_rect(canvas, corners, FLinearColor(1.f, 1.f, 0.f, .1f), 0.f);
}

void draw_hitboxes(UCanvas *canvas, const asw_entity *entity)
{
	const auto count = entity->hitbox_count + entity->hurtbox_count;
	const auto *parent = entity->parent != nullptr ? entity->parent : entity;
	const auto active = parent->is_active();
	const auto invuln = entity->is_invuln();

	for (auto boxidx = 0; boxidx < count; boxidx++) {
		const auto &box = entity->hitboxes[boxidx];

		// Don't show inactive hitboxes
		if (box.type == hitbox::type::hit && !active)
			continue;
		else if (box.type == hitbox::type::hurt && invuln)
			continue;

		draw_hitbox(canvas, entity, box);
	}
}

void draw_display(UCanvas *canvas)
{
	const auto *engine = asw_engine::get();
	if (engine == nullptr)
		return;

	// Loop through entities backwards because the player that most
	// recently landed a hit is at index 0
	for (auto entidx = engine->entity_count - 1; entidx >= 0; entidx--) {
		const auto *entity = engine->entities[entidx];

		if (entity->is_pushbox_active())
			draw_pushbox(canvas, entity);

		draw_hitboxes(canvas, entity);
	}
}

void hook_AHUD_PostRender(AHUD *hud)
{
	draw_display(hud->Canvas);
	orig_AHUD_PostRender(hud);
}

const void *vtable_hook(const void **vtable, const int index, const void *hook)
{
	DWORD old_protect;
	VirtualProtect(&vtable[index], sizeof(void*), PAGE_READWRITE, &old_protect);
	const auto *orig = vtable[index];
	vtable[index] = hook;
	VirtualProtect(&vtable[index], sizeof(void*), old_protect, &old_protect);
	return orig;
}

void install_hooks()
{
	// AHUD::PostRender
	orig_AHUD_PostRender = (AHUD_PostRender_t)
		vtable_hook(AHUD_vtable, AHUD_PostRender_index, hook_AHUD_PostRender);
}

void uninstall_hooks()
{
	// AHUD::PostRender
	vtable_hook(AHUD_vtable, AHUD_PostRender_index, orig_AHUD_PostRender);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
		install_hooks();
	else if (reason == DLL_PROCESS_DETACH)
		uninstall_hooks();
	else
		return false;

	return true;
}
