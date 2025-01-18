// internal
#include "physics_system.hpp"
#include "world_init.hpp"
#include "ecs/physics_components.hpp"

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	const float r_squared = max(other_r_squared, my_r_squared);
	if (dist_squared < r_squared) {
        return true;
    }
	return false;
}

bool pointInsidePoly(const vec2& point, const std::vector<vec2>& polygon) {
    // only works if vertices saved in counterclockwise order!
	for (std::size_t i = 0; i < polygon.size(); ++i) {
		const vec2 p0 = polygon[i];
		const vec2 p1 = polygon[(i + 1) % polygon.size()];
		// Calculate if point is on the left of the line
		//if (const auto result = point.x * (p1.y - p0.y) + point.y * (p0.x - p1.x) + p0.x * (p1.y - p0.y) - p0.y * (p1.x - p0.y);
		//	result <= 0) {
		//	return true;
		//}
        const vec2 affine_segment = p1 - p0;
        const vec2 affine_point = point - p0;
        const float cosine_sign = affine_segment.x * affine_point.y - affine_segment.y * affine_point.x; //only works if vertices saved in counterclockwise order
        if (cosine_sign > 0) { // true when point on right side
            return false;
        }
	}
	//return false;
    return true;
}

// This assumes that both polys are convex
bool collidesPoly(const Motion& motion1, const Motion& motion2, const std::vector<vec2>& poly1, const std::vector<vec2>& poly2) {
	Transform tf1, tf2;
	tf1.translate(motion1.position);
	tf2.translate(motion2.position);
	tf1.rotate(motion1.angle);
	tf2.rotate(motion2.angle);
	tf1.scale(motion1.scale);
	tf2.scale(motion2.scale);
	// Transform both polys
	auto poly1TF = std::vector<vec2>(poly1.size());
	auto poly2TF = std::vector<vec2>(poly2.size());
	for (std::size_t i = 0; i < poly1.size(); ++i) {
		poly1TF[i] = tf1 * poly1[i];
	}
	for (std::size_t i = 0; i < poly2.size(); ++i) {
		poly2TF[i] = tf2 * poly2[i];
	}
	// Check if point of poly2 is inside poly1
	for (const auto& poly2_pos : poly2TF) {
		if (pointInsidePoly(poly2_pos, poly1TF)) {
			return true;
		}
	}
	// Check if point of poly1 is inside poly2
	for (const auto& poly1_pos : poly1TF) {
		if (pointInsidePoly(poly1_pos, poly2TF)) {
			return true;
		}
	}
	return false;
}

bool enemyInRange(const Motion& tower_motion, const float input_range, const Motion& enemy_motion) {
	const vec2 d_p = tower_motion.position - enemy_motion.position;
	const float dist_squared = dot(d_p, d_p);
	const vec2 enemy_bounding_box = get_bounding_box(enemy_motion);
	const float enemy_r_squared = dot(enemy_bounding_box, enemy_bounding_box);
	const float input_r_squared = input_range * input_range;
	const float r_squared = max(enemy_r_squared, input_r_squared);
	if (dist_squared < r_squared)
		return true;
	return false;
}

bool enemyPolyInBombRange(const Motion& bomb_motion, const float range, const Motion& enemy_motion, const std::vector<vec2>& poly) {
    Transform tf;
    tf.translate(enemy_motion.position);
    tf.rotate(enemy_motion.angle);
    tf.scale(enemy_motion.scale);
    auto polyTF = std::vector<vec2>(poly.size());
    for (std::size_t i = 0; i < poly.size(); ++i) {
        polyTF[i] = tf * poly[i];
    }
    for (const auto& poly_pos : polyTF) {
        if (distance(poly_pos, bomb_motion.position) <= range) {
            return true;
        }
    }
    return false;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_container = registry.motions;
	for(uint i = 0; i < motion_container.size(); i++)
	{
		Motion& motion = motion_container.components[i];
		const float step_seconds = elapsed_ms / 1000.f;
		motion.position += step_seconds * motion.velocity;
	}

    auto& map_container = registry.maps;
	//printf("Active maps: %lu\n", active_maps.size());
	//printf("Map count: %lu\n", map_container.size());
    if (map_container.size() == 1) {
	    const Map& active_map = map_container.components[0];

    	auto& enemy_container = registry.enemies;
    	for (uint i = 0; i < enemy_container.size(); i++) {
    		Enemy& enemy = enemy_container.components[i];
            if(enemy.spawned) {
                Motion &motion = registry.motions.get(enemy_container.entities[i]);
                const float step_seconds = elapsed_ms / 1000.f;
            	motion.position = calculate_enemy_position(enemy, enemy_container.entities[i], active_map, step_seconds, true);
                //printf("%f %f\n", motion.position[0], motion.position[0]);
                //printf("%f, %f\n", enemy.enemy_progress, enemy.section_progress);
            }
    	}
    }

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Check for collisions between all moving entities
	for(uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];

		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for(uint j = i+1; j < motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			Entity entity_j = motion_container.entities[j];

            if ((registry.towers.has(entity_i) && registry.enemies.has(entity_j))){
                if(registry.enemies.get(entity_j).spawned) {
                    if (enemyInRange(motion_i, registry.towers.get(entity_i).range, motion_j)) {
                        registry.collisions.emplace_with_duplicates(entity_i, entity_j);
                        registry.collisions.emplace_with_duplicates(entity_j, entity_i);
                    }
                }
			} else if (registry.towers.has(entity_j) && registry.enemies.has(entity_i)) {
                if(registry.enemies.get(entity_i).spawned) {
                    if (enemyInRange(motion_j, registry.towers.get(entity_j).range, motion_i)) {
                        registry.collisions.emplace_with_duplicates(entity_i, entity_j);
                        registry.collisions.emplace_with_duplicates(entity_j, entity_i);
                    }
                }
            } else if (registry.bombs.has(entity_i) && registry.enemies.has(entity_j)) {
                if(registry.enemies.get(entity_j).spawned) {
                    if (enemyInRange(motion_i, registry.consumables.get(entity_i).range, motion_j)) {
                        const RenderRequest& request_j = registry.renderGameLayer.get(entity_j);
                        auto& poly_j = getCollisionMeshOfTexture(request_j.used_texture, request_j.atlas_ids[0]);
                        if (enemyPolyInBombRange(motion_i, registry.consumables.get(entity_i).range, motion_j, poly_j)) {
                            registry.collisions.emplace_with_duplicates(entity_i, entity_j);
                            registry.collisions.emplace_with_duplicates(entity_j, entity_i);
                        }
                    }
                }
            } else if (!(registry.enemies.has(entity_i) && registry.enemies.has(entity_j)) // ignore collision between enemies
                        && !(registry.towers.has(entity_i) || registry.towers.has(entity_j)) // ignore collision with towers
                        && !(registry.consumables.has(entity_i) && registry.consumables.has(entity_j)) // ignore collisions between consumables
                        && collides(motion_i, motion_j)) {
            	// Check if coarse collision is an actual collision
            	// TODO Think about Entities with multiple render requests
            	const RenderRequest& request_i = registry.renderGameLayer.get(entity_i);
            	const RenderRequest& request_j = registry.renderGameLayer.get(entity_j);
            	auto& poly_i = getCollisionMeshOfTexture(request_i.used_texture, request_i.atlas_ids[0]);
            	auto& poly_j = getCollisionMeshOfTexture(request_j.used_texture, request_j.atlas_ids[0]);
                if (collidesPoly(motion_i, motion_j, poly_i, poly_j)) {
            		// Create a collisions event
            		// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
            		registry.collisions.emplace_with_duplicates(entity_i, entity_j);
            		registry.collisions.emplace_with_duplicates(entity_j, entity_i);
            	}
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE PEBBLE collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

vec2 PhysicsSystem::calculate_enemy_position(Enemy& enemy, Entity enemy_entity, const Map& current_map, const float seconds, const bool update_enemy) {
	vec2 previous_checkpoint = current_map.checkpoints[enemy.next_checkpoint - 1];
	if (enemy.next_checkpoint >= current_map.checkpoints.size()) {
        enemy.enemy_progress = 1.;
		return previous_checkpoint;
	}
	vec2 next_checkpoint = current_map.checkpoints[enemy.next_checkpoint];
	float enemy_progress = enemy.enemy_progress;
    const auto walk_timer = registry.enemyWalkTimers.get(enemy_entity);
    float walk_speed = 0;
    const float walk_interval = walk_timer.start_time / (static_cast<unsigned int>(SLIME_WALK_FRAME::COUNT) * 2 - 1);
    if (walk_timer.time > walk_interval) {
        const float move_time = walk_timer.start_time - walk_interval; // time duration when enemy is moving
        walk_speed = (1.f - pow(abs(walk_timer.time - move_time/2)/move_time/2, 2.f)) * enemy.speed * 1.2f;
    }
    if (registry.sloweds.has(enemy_entity)) {
        walk_speed *= (1.f - registry.sloweds.get(enemy_entity).slow);
    }
	enemy_progress += (walk_speed * seconds) / current_map.path_length;
	if (update_enemy) {
		enemy.enemy_progress = enemy_progress;
	}
	const float section_length = current_map.section_lengths[enemy.next_checkpoint-1];//abs(distance(previous_checkpoint, next_checkpoint)); //TODO maybe already calc this in create_map and save with map
	float section_progress = enemy.section_progress;
    //float* temp = nullptr;
    //section_progress = std::modf(enemy_progress*(current_map.checkpoints.size()-1), temp);
    /* TODO:
     * make section progress go > 1, then mod f and use first part to select section and decimal part to interpolate
     * if section progress over checkpoint size (or maybe size-1) set enemy progress 1 to avoid floating point inaccuracies
     */
	section_progress += (walk_speed * seconds) / section_length;
	if (update_enemy) {
		enemy.section_progress = section_progress;
	}
	if (section_progress >= 1) {
		uint next_checkpoint_index = enemy.next_checkpoint;
		next_checkpoint_index++;
		if (update_enemy) {
			enemy.next_checkpoint = next_checkpoint_index;
		}
		if (next_checkpoint_index >= current_map.checkpoints.size()) {
			return next_checkpoint;
		}
		section_progress -= 1.f;
		if (update_enemy) {
			enemy.section_progress = section_progress;
		}
		previous_checkpoint = current_map.checkpoints[next_checkpoint_index - 1];
		next_checkpoint = current_map.checkpoints[next_checkpoint_index];

        //TODO: change slime angle
        if (update_enemy) {
            auto path_vector = previous_checkpoint - next_checkpoint;
            const float angle = atan2(path_vector.y, path_vector.x);
            Motion& enemy_motion = registry.motions.get(enemy_entity);
            enemy_motion.angle = angle;
            printf("%f\n", angle);
        }
	}
	return previous_checkpoint + (next_checkpoint - previous_checkpoint) * section_progress;
}
