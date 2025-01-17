#pragma once

#include "common.hpp"
#include <vector>

#include "render_components.hpp"

// Collision Definitions
// Positions are defined by percentages of texture positions (top left is 0, 0 and bottom right is 1, 1)
// For proper positions this grid must be shifted, so that 0, 0 is in the center
// Final Transformations are done when calculating the collision
// Remember that even if a texture-atlas is used, only the percentages for the single texture should be used
// Vertices have to be defined in counterclockwise order for collision detection to work (physics_system::pointInsidePoly)
constexpr vec2 grid_shift{0.5, 0.5};

const std::vector basic_bounding_box {
    vec2{0, 0} - grid_shift,
    vec2{0, 1} - grid_shift,
    vec2{1, 1} - grid_shift,
    vec2{1, 0} - grid_shift
};

const std::vector slime_collision_poly {
        vec2{0.22, 0.59} - grid_shift,
        vec2{0.78, 0.59} - grid_shift,
        vec2{0.91, 0.47} - grid_shift,
        vec2{0.88, 0.18} - grid_shift,
        vec2{0.75, 0.06} - grid_shift,
        vec2{0.59, 0.} - grid_shift,
        vec2{0.41, 0.} - grid_shift,
        vec2{0.25, 0.06} - grid_shift,
        vec2{0.13, 0.18} - grid_shift,
        vec2{0.09, 0.47} - grid_shift
};

const std::vector slime_down_collision_poly {
        vec2{0.18, 0.54} - grid_shift,
        vec2{0.38, 0.60} - grid_shift,
        vec2{0.53, 0.60} - grid_shift,
        vec2{0.81, 0.54} - grid_shift,
        vec2{0.85, 0.43} - grid_shift,
        vec2{0.73, 0.19} - grid_shift,
        vec2{0.53, 0.07} - grid_shift,
        vec2{0.46, 0.07} - grid_shift,
        vec2{0.25, 0.19} - grid_shift,
        vec2{0.13, 0.43} - grid_shift,
};

const std::vector slime_left_collision_poly {
        vec2{0.35, 0.57} - grid_shift,
        vec2{0.76, 0.57} - grid_shift,
        vec2{0.92, 0.44} - grid_shift,
        vec2{0.76, 0.25} - grid_shift,
        vec2{0.40, 0.11} - grid_shift,
        vec2{0.30, 0.10} - grid_shift,
        vec2{0.19, 0.27} - grid_shift,
        vec2{0.19, 0.44} - grid_shift,
};

const std::vector slime_up_collision_poly {
        vec2{0.25, 0.64} - grid_shift,
        vec2{0.46, 0.76} - grid_shift,
        vec2{0.53, 0.76} - grid_shift,
        vec2{0.73, 0.64} - grid_shift,
        vec2{0.85, 0.44} - grid_shift,
        vec2{0.75, 0.31} - grid_shift,
        vec2{0.56, 0.22} - grid_shift,
        vec2{0.43, 0.22} - grid_shift,
        vec2{0.24, 0.31} - grid_shift,
        vec2{0.13, 0.44} - grid_shift,
};

const std::vector slime_right_collision_poly {
        vec2{0.22, 0.57} - grid_shift,
        vec2{0.64, 0.57} - grid_shift,
        vec2{0.79, 0.40} - grid_shift,
        vec2{0.79, 0.27} - grid_shift,
        vec2{0.68, 0.10} - grid_shift,
        vec2{0.57, 0.11} - grid_shift,
        vec2{0.22, 0.25} - grid_shift,
        vec2{0.07, 0.44} - grid_shift,
};

const std::vector arrow_collision_poly { //TODO: check if hitbox still rotates correctly with texture
        vec2{0.33, 0.61} - grid_shift,
        vec2{0.38, 0.66} - grid_shift,
        vec2{0.67, 0.37} - grid_shift,
        vec2{0.62, 0.32} - grid_shift,
};

const std::vector sword_collision_poly { //TODO: check if hitbox still rotates correctly with texture
        vec2{0.20, 0.68} - grid_shift,
        vec2{0.31, 0.80} - grid_shift,
        vec2{0.78, 0.35} - grid_shift,
        vec2{0.85, 0.15} - grid_shift,
        vec2{0.66, 0.22} - grid_shift
};

inline const std::vector<vec2>& getCollisionMeshOfTexture(const TEXTURE_ASSET_ID id, const unsigned int atlas_id = 0) {
    switch (id) {
        case TEXTURE_ASSET_ID::SLIME:
            if (atlas_id < static_cast<unsigned int>(SLIME_WALK_FRAME::COUNT)) {
                return slime_down_collision_poly;
            } else if (atlas_id < 2 * static_cast<unsigned int>(SLIME_WALK_FRAME::COUNT)) {
                return slime_left_collision_poly;
            } else if (atlas_id < 3 * static_cast<unsigned int>(SLIME_WALK_FRAME::COUNT)) {
                return slime_up_collision_poly;
            } else if (atlas_id < 4 * static_cast<unsigned int>(SLIME_WALK_FRAME::COUNT)) {
                return slime_right_collision_poly;
            } else {
                return slime_collision_poly;
            }
        case TEXTURE_ASSET_ID::BOW:
            return arrow_collision_poly;
        case TEXTURE_ASSET_ID::SWORD:
            return sword_collision_poly;
        default: // This is just the bounding box of the texture
            return basic_bounding_box;
    }
}
