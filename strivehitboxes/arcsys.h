#pragma once

#include "ue4.h"
#include "struct_util.h"

class AREDGameState_Battle : public AGameState {
public:
	static UClass *StaticClass();

	FIELD(0xB60, class asw_engine*, Engine);
	FIELD(0xB68, class asw_scene*, Scene);
};

class player_block {
	char pad[0x160];
public:
	FIELD(0x8, class asw_entity*, entity);
};

static_assert(sizeof(player_block) == 0x160);

// Used by the shared GG/BB/DBFZ engine code
class asw_engine {
public:
	static constexpr auto coord_scale = .458f;

	static asw_engine *get();

	ARRAY_FIELD(0x0, player_block, players);
	FIELD(0x8A0, int, entity_count);
	ARRAY_FIELD(0xC10, class asw_entity*, entities);
};


class asw_scene {
public:
	static asw_scene *get();

	// "delta" is the difference between input and output position
	// position gets written in place
	// position/angle can be null
	void camera_transform(FVector *delta, FVector *position, FVector *angle) const;
	void camera_transform(FVector *position, FVector *angle) const;
};

class hitbox {
public:
	enum class box_type : int {
		hurt = 0,
		hit = 1,
		grab = 2 // Not used by the game
	};

	box_type type;
	float x, y;
	float w, h;
};

enum class direction : int {
	right = 0,
	left = 1
};

namespace action_flag1 {
	enum {
		counterhit = 256
	};
}

namespace action_flag2 {
	enum {
		strike_invuln = 16,
		throw_invuln = 32,
		wakeup = 64
	};
}

namespace cinematic_flag {
	enum {
		//   _____    ____    _    _   _   _   _______   ______   _____  
		//  / ____|  / __ \  | |  | | | \ | | |__   __| |  ____| |  __ \ 
		// | |      | |  | | | |  | | |  \| |    | |    | |__    | |__) |
		// | |      | |  | | | |  | | | . ` |    | |    |  __|   |  _  / 
		// | |____  | |__| | | |__| | | |\  |    | |    | |____  | | \ \ 
		//  \_____|  \____/   \____/  |_| \_|    |_|    |______| |_|  \_\ 
		counter = 0x4000000
	};
}

class asw_entity {
public:
	FIELD(0x18, bool, is_player);
	FIELD(0x44, unsigned char, player_index);
	FIELD(0x68, hitbox*, hitboxes);
	FIELD(0xFC, int, hurtbox_count);
	FIELD(0x100, int, hitbox_count);
	FIELD(0x198, int, cinematic_flags);
	FIELD(0x2B0, asw_entity*, last_combatted_entity);
	FIELD(0x2C8, asw_entity*, parent);
	FIELD(0x308, asw_entity*, attached);
	FIELD(0x380, int, action_flags1);
	FIELD(0x384, int, action_flags2);
	FIELD(0x388, int, action_flags3);
	FIELD(0x394, direction, facing);
	FIELD(0x398, int, pos_x);
	FIELD(0x39C, int, pos_y);
	FIELD(0x3A0, int, pos_z);
	FIELD(0x3A4, int, angle_x);
	FIELD(0x3A8, int, angle_y);
	FIELD(0x3AC, int, angle_z);
	FIELD(0x3B4, int, scale_x);
	FIELD(0x3B8, int, scale_y);
	FIELD(0x3BC, int, scale_z);
	FIELD(0x4B8, int, vel_x);
	FIELD(0x4BC, int, vel_y);
	FIELD(0x4C0, int, gravity);
	FIELD(0x4EC, int, pushbox_front_offset);
	FIELD(0x714, int, throw_box_top);
	FIELD(0x71C, int, throw_box_bottom);
	FIELD(0x720, int, throw_range);
	FIELD(0x1104, int, backdash_invuln);
	// bbscript
	FIELD(0x11C0, char*, script_base);
	FIELD(0x11C8, char*, next_script_cmd);
	FIELD(0x11D0, char*, first_script_cmd);
	FIELD(0x11F8, int, sprite_frames);
	FIELD(0x11FC, int, sprite_duration);

	bool is_active() const;
	bool is_pushbox_active() const;
	bool is_strike_invuln() const;
	bool is_throw_invuln() const;
	int get_pos_x() const;
	int get_pos_y() const;
	int pushbox_width() const;
	int pushbox_height() const;
	int pushbox_bottom() const;
	void get_pushbox(int *left, int *top, int *right, int *bottom) const;
	asw_entity *get_opponent() const;
};
