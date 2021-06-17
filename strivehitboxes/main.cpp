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

// Corners must be in CW or CCW order
void draw_rect(
	UCanvas *canvas,
	const std::array<FVector2D, 4> &corners,
	const FLinearColor &color)
{
	for (auto i = 0; i < 4; i++)
		canvas->K2_DrawLine(corners[i], corners[(i + 1) % 4], 2.F, color);

	// Fill in rectangle
	FCanvasUVTri triangles[2];
	triangles[0].V0_Color = triangles[0].V1_Color = triangles[0].V2_Color = color;
	triangles[1].V0_Color = triangles[1].V1_Color = triangles[1].V2_Color = color;

	triangles[0].V0_Pos = corners[0];
	triangles[0].V1_Pos = corners[1];
	triangles[0].V2_Pos = corners[2];

	triangles[1].V0_Pos = corners[2];
	triangles[1].V1_Pos = corners[3];
	triangles[1].V2_Pos = corners[0];

	FCanvasTriangleItem item(
		FVector2D(0.f, 0.f),
		FVector2D(0.f, 0.f),
		FVector2D(0.f, 0.f),
		*GWhiteTexture);

	item.TriangleList = TArray(triangles);
	item.BlendMode = SE_BLEND_Translucent;
	item.Draw(canvas->Canvas);
}

void draw_hitbox(UCanvas *canvas, const asw_entity *entity, const hitbox &box)
{
	std::array<FVector2D, 4> corners = {
		FVector2D(box.x, box.y),
		FVector2D(box.x + box.w, box.y),
		FVector2D(box.x + box.w, box.y + box.h),
		FVector2D(box.x, box.y + box.h)
	};

	for (auto &pos : corners) {
		pos.X *= -entity->scale_x;
		pos.Y *= -entity->scale_y;

		pos = pos.Rotate((float)entity->angle_x * (float)M_PI / 180000.f);

		if (entity->facing == direction::left)
			pos.X *= -1.f;

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

	if (entity->facing == direction::right)
		draw_rect(canvas, corners, color);
	else
		draw_rect(canvas, corners, color);
}

void draw_pushbox(UCanvas *canvas, const asw_entity *entity)
{
	int left, top, right, bottom;
	entity->get_pushbox(&left, &top, &right, &bottom);

	std::array<FVector2D, 4> corners = {
		FVector2D(left, top),
		FVector2D(right, top),
		FVector2D(right, bottom),
		FVector2D(left, bottom)
	};

	for (auto &pos : corners)
		asw_coords_to_screen(canvas, &pos);

	draw_rect(canvas, corners, FLinearColor(1.f, 1.f, 0.f, .1f));
}

void draw_hitboxes(UCanvas *canvas, const asw_entity *entity, bool active)
{
	const auto count = entity->hitbox_count + entity->hurtbox_count;

	for (auto boxidx = 0; boxidx < count; boxidx++) {
		const auto &box = entity->hitboxes[boxidx];

		// Don't show inactive hitboxes
		if (box.type == hitbox::type::hit && !active)
			continue;
		else if (box.type == hitbox::type::hurt && entity->is_invuln())
			continue;

		draw_hitbox(canvas, entity, box);
	}
}

void draw_display(UCanvas *canvas)
{
	const auto *engine = asw_engine::get();
	if (engine == nullptr)
		return;

	if (canvas->Canvas == nullptr)
		return;

	// Loop through entities backwards because the player that most
	// recently landed a hit is at index 0
	for (auto entidx = engine->entity_count - 1; entidx >= 0; entidx--) {
		const auto *entity = engine->entities[entidx];

		if (entity->is_pushbox_active())
			draw_pushbox(canvas, entity);

		const auto active = entity->is_active();
		draw_hitboxes(canvas, entity, active);

		const auto *attached = entity->attached;
		while (attached != nullptr) {
			draw_hitboxes(canvas, attached, active);
			attached = attached->attached;
		}
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
