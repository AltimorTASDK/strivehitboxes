#define _USE_MATH_DEFINES
#include "sigscan.h"
#include "ue4.h"
#include "arcsys.h"
#include "math_util.h"
#include <cmath>
#include <array>
#include <vector>
#include <Windows.h>
#undef min
#undef max

constexpr auto AHUD_PostRender_index = 214;

// Actually AREDHUD_Battle
const auto **AHUD_vtable = (const void**)get_rip_relative(sigscan::get().scan(
	"\x48\x8D\x05\x00\x00\x00\x00\xC6\x83\x18\x03", "xxx????xxxx") + 3);

using AHUD_PostRender_t = void(*)(AHUD*);
AHUD_PostRender_t orig_AHUD_PostRender;

struct drawn_hitbox {
	hitbox::box_type type;

	// Unclipped corners of filled box
	std::array<FVector2D, 4> corners;

	// Boxes to fill, clipped against other boxes
	std::vector<std::array<FVector2D, 4>> fill;

	// Outlines
	std::vector<std::array<FVector2D, 2>> lines;

	drawn_hitbox(const hitbox &box) :
		type(box.type),
		corners {
			FVector2D(box.x, box.y),
			FVector2D(box.x + box.w, box.y),
			FVector2D(box.x + box.w, box.y + box.h),
			FVector2D(box.x, box.y + box.h) }
	{
		for (auto i = 0; i < 4; i++)
			lines.push_back(std::array { corners[i], corners[(i + 1) % 4] });

		fill.push_back(corners);
	}

	// Clip outlines against another hitbox
	void clip_lines(const drawn_hitbox &other)
	{
		const auto &&old_lines = std::move(lines);

		for (auto &line : old_lines) {
			float entry_fraction, exit_fraction;
			auto intersected = line_box_intersection(
				other.corners[0], other.corners[2],
				line[0], line[1],
				&entry_fraction, &exit_fraction);

			if (!intersected) {
				lines.push_back(line);
				continue;
			}

			const auto delta = line[1] - line[0];

			if (entry_fraction != 0.f)
				lines.push_back(std::array { line[0], line[0] + delta * entry_fraction });

			if (exit_fraction != 1.f)
				lines.push_back(std::array { line[0] + delta * exit_fraction, line[1] });
		}
	}

	// Clip filled rectangle against another hitbox
	void clip_fill(const drawn_hitbox &other)
	{
		const auto &&old_fill = std::move(fill);

		for (const auto &box : old_fill) {
			const auto &box_min = box[0];
			const auto &box_max = box[2];
			
			const auto clip_min = FVector2D(
				max(box_min.X, other.corners[0].X),
				max(box_min.Y, other.corners[0].Y));

			const auto clip_max = FVector2D(
				min(box_max.X, other.corners[2].X),
				min(box_max.Y, other.corners[2].Y));

			if (clip_min.X > clip_max.X || clip_min.Y > clip_max.Y) {
				// No intersection
				fill.push_back(box);
				continue;
			}

			if (clip_min.X > box_min.X) {
				// Left box
				fill.push_back(std::array {
					FVector2D(box_min.X, box_min.Y),
					FVector2D(clip_min.X, box_min.Y),
					FVector2D(clip_min.X, box_max.Y),
					FVector2D(box_min.X, box_max.Y) });
			}

			if (clip_max.X < box_max.X) {
				// Right box
				fill.push_back(std::array {
					FVector2D(clip_max.X, box_min.Y),
					FVector2D(box_max.X, box_min.Y),
					FVector2D(box_max.X, box_max.Y),
					FVector2D(clip_max.X, box_max.Y) });
			}

			if (clip_min.Y > box_min.Y) {
				// Top box
				fill.push_back(std::array {
					FVector2D(clip_min.X, box_min.Y),
					FVector2D(clip_max.X, box_min.Y),
					FVector2D(clip_max.X, clip_min.Y),
					FVector2D(clip_min.X, clip_min.Y) });
			}

			if (clip_max.Y < box_max.Y) {
				// Bottom box
				fill.push_back(std::array {
					FVector2D(clip_min.X, clip_max.Y),
					FVector2D(clip_max.X, clip_max.Y),
					FVector2D(clip_max.X, box_max.Y),
					FVector2D(clip_min.X, box_max.Y) });
			}
		}
	}
};

void asw_coords_to_screen(const UCanvas *canvas, FVector2D *pos)
{
	pos->X *= asw_engine::coord_scale / 1000.F;
	pos->Y *= asw_engine::coord_scale / 1000.F;

	FVector pos3d(pos->X, 0.f, pos->Y);
	asw_scene::get()->camera_transform(&pos3d, nullptr);

	const auto proj = canvas->K2_Project(pos3d);
	*pos = FVector2D(proj.X, proj.Y);
}

// Corners must be in CW or CCW order
void fill_rect(
	UCanvas *canvas,
	const std::array<FVector2D, 4> &corners,
	const FLinearColor &color)
{
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

// Corners must be in CW or CCW order
void draw_rect(
	UCanvas *canvas,
	const std::array<FVector2D, 4> &corners,
	const FLinearColor &color)
{
	fill_rect(canvas, corners, color);

	for (auto i = 0; i < 4; i++)
		canvas->K2_DrawLine(corners[i], corners[(i + 1) % 4], 2.F, color);
}

// Transform entity local space to screen space
void transform_hitbox_point(const UCanvas *canvas, const asw_entity *entity, FVector2D *pos)
{
	pos->X *= -entity->scale_x;
	pos->Y *= -entity->scale_y;

	*pos = pos->Rotate((float)entity->angle_x * (float)M_PI / 180000.f);

	if (entity->facing == direction::left)
		pos->X *= -1.f;

	pos->X += entity->get_pos_x();
	pos->Y += entity->get_pos_y();

	asw_coords_to_screen(canvas, pos);
}

void draw_hitbox(UCanvas *canvas, const asw_entity *entity, const drawn_hitbox &box)
{
	FLinearColor color;
	if (box.type == hitbox::box_type::hit)
		color = FLinearColor(1.f, 0.f, 0.f, .25f);
	else if (entity->action_flags1 & action_flag1::counterhit)
		color = FLinearColor(0.f, 1.f, 1.f, .25f);
	else
		color = FLinearColor(0.f, 1.f, 0.f, .25f);

	for (auto fill : box.fill) {
		for (auto &pos : fill)
			transform_hitbox_point(canvas, entity, &pos);

		fill_rect(canvas, fill, color);
	}

	for (const auto &line : box.lines) {
		auto start = line[0];
		auto end = line[1];
		transform_hitbox_point(canvas, entity, &start);
		transform_hitbox_point(canvas, entity, &end);
		canvas->K2_DrawLine(start, end, 2.F, color);
	}
}

void draw_hitboxes(UCanvas *canvas, const asw_entity *entity, bool active)
{
	const auto count = entity->hitbox_count + entity->hurtbox_count;

	std::vector<drawn_hitbox> hitboxes;

	// Collect hitbox info
	for (auto i = 0; i < count; i++) {
		const auto &box = entity->hitboxes[i];

		// Don't show inactive hitboxes
		if (box.type == hitbox::box_type::hit && !active)
			continue;
		else if (box.type == hitbox::box_type::hurt && entity->is_invuln())
			continue;

		hitboxes.push_back(drawn_hitbox(box));
	}

	for (auto i = 0; i < hitboxes.size(); i++) {
		// Clip outlines
		for (auto j = 0; j < hitboxes.size(); j++) {
			if (i != j && hitboxes[i].type == hitboxes[j].type)
				hitboxes[i].clip_lines(hitboxes[j]);
		}

		// Clip fill against every hitbox after, since two boxes
		// shouldn't both be clipped against each other
		for (auto j = i + 1; j < hitboxes.size(); j++) {
			if (hitboxes[i].type == hitboxes[j].type)
				hitboxes[i].clip_fill(hitboxes[j]);
		}

		draw_hitbox(canvas, entity, hitboxes[i]);
	}
}

void draw_pushbox(UCanvas *canvas, const asw_entity *entity)
{
	int left, top, right, bottom;
	entity->get_pushbox(&left, &top, &right, &bottom);

	std::array corners = {
		FVector2D(left, top),
		FVector2D(right, top),
		FVector2D(right, bottom),
		FVector2D(left, bottom)
	};

	for (auto &pos : corners)
		asw_coords_to_screen(canvas, &pos);

	draw_rect(canvas, corners, FLinearColor(1.f, 1.f, 0.f, .1f));
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
