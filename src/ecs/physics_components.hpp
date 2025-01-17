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

const std::vector spike_left_collision_poly {
        vec2{0.29, 0.62} - grid_shift,
        vec2{0.43, 0.56} - grid_shift,
        vec2{0.29, 0.41} - grid_shift,
        vec2{0.22, 0.56} - grid_shift
};

const std::vector spike_middle_collision_poly {
        vec2{0.47, 0.50} - grid_shift,
        vec2{0.56, 0.44} - grid_shift,
        vec2{0.47, 0.25} - grid_shift,
        vec2{0.38, 0.44} - grid_shift
};

const std::vector spike_right_collision_poly {
        vec2{0.53, 0.59} - grid_shift,
        vec2{0.72, 0.56} - grid_shift,
        vec2{0.63, 0.34} - grid_shift,
};

inline const std::vector<vec2>& getCollisionMeshOfTexture(const TEXTURE_ASSET_ID id, const unsigned int atlas_id = 0) {
    switch (id) {
        case TEXTURE_ASSET_ID::SLIME: //TODO: Slime poly currently not used, since other asset ids used for different textures
            return slime_collision_poly;
        case TEXTURE_ASSET_ID::SLIME_BIG:
            return slime_collision_poly;
        case TEXTURE_ASSET_ID::BOW:
            return arrow_collision_poly;
        case TEXTURE_ASSET_ID::SWORD:
            return sword_collision_poly;
        case TEXTURE_ASSET_ID::SPIKES:
            switch (atlas_id) {
                case static_cast<unsigned int>(SPIKES_SPRITE::SPIKE_LEFT):
                    return spike_left_collision_poly;
                case static_cast<unsigned int>(SPIKES_SPRITE::SPIKE_MIDDLE):
                    return spike_middle_collision_poly;
                case static_cast<unsigned int>(SPIKES_SPRITE::SPIKE_RIGHT):
                    return spike_right_collision_poly;
                default:
                    return basic_bounding_box;
            }
        default: // This is just the bounding box of the texture
            return basic_bounding_box;
    }
}
