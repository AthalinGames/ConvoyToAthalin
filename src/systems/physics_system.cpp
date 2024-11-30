// internal
#include "physics_system.hpp"
#include "world_init.hpp"

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
	for (std::size_t i = 0; i < polygon.size(); ++i) {
		const vec2 p0 = polygon[i];
		const vec2 p1 = polygon[(i + 1) % polygon.size()];
		// Calculate if point is on the left of the line
		if (const auto result = point.x * (p1.y - p0.y) + point.y * (p0.x - p1.x) + p0.x * (p1.y - p0.y) - p0.y * (p1.x - p0.y);
			result <= 0) {
			return true;
		}
	}
	return false;
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

bool enemyInTowerRange(const Motion& tower_motion, const Tower& tower, const Motion& enemy_motion) {
	const vec2 d_p = tower_motion.position - enemy_motion.position;
	const float dist_squared = dot(d_p, d_p);
	const vec2 enemy_bounding_box = get_bounding_box(enemy_motion);
	const float enemy_r_squared = dot(enemy_bounding_box, enemy_bounding_box);
	const float tower_r_squared = tower.range * tower.range;
	const float r_squared = max(enemy_r_squared, tower_r_squared);
	if (dist_squared < r_squared)
		return true;
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
    std::vector<Map> active_maps;
    for(Map& map : map_container.components) {
        if(map.active)
            active_maps.push_back(map);
    }
	//printf("Active maps: %lu\n", active_maps.size());
	//printf("Map count: %lu\n", map_container.size());
    if (active_maps.size() == 1) {
	    const Map& active_map = active_maps[0];

    	auto& enemy_container = registry.enemies;
    	for (uint i = 0; i < enemy_container.size(); i++) {
    		Enemy& enemy = enemy_container.components[i];
            if(enemy.spawned) {
                Motion &motion = registry.motions.get(enemy_container.entities[i]);
                const float step_seconds = elapsed_ms / 1000.f;
            	motion.position = calculate_enemy_position(enemy, active_map, step_seconds, true);
                //printf("%f %f\n", motion.position[0], motion.position[0]);
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
			if ((registry.towers.has(entity_i) && !registry.cards.has(entity_i) && registry.enemies.has(entity_j))){
                if(registry.enemies.get(entity_j).spawned) {
                    if (enemyInTowerRange(motion_i, registry.towers.get(entity_i), motion_j)) {
                        registry.collisions.emplace_with_duplicates(entity_i, entity_j);
                        registry.collisions.emplace_with_duplicates(entity_j, entity_i);
                    }
                }
			} else if (registry.towers.has(entity_j) && !registry.cards.has(entity_j) && registry.enemies.has(entity_i)) {
                if(registry.enemies.get(entity_i).spawned) {
                    if (enemyInTowerRange(motion_j, registry.towers.get(entity_j), motion_i)) {
                        registry.collisions.emplace_with_duplicates(entity_i, entity_j);
                        registry.collisions.emplace_with_duplicates(entity_j, entity_i);
                    }
                }
            } else if (collides(motion_i, motion_j)) {
            	// Check if coarse collision is an actual collision
            	auto& poly_i = getCollisionMeshOfTexture(registry.renderRequests.get(entity_i).used_texture);
            	auto& poly_j = getCollisionMeshOfTexture(registry.renderRequests.get(entity_j).used_texture);
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

vec2 PhysicsSystem::calculate_enemy_position(Enemy& enemy, const Map& current_map, const float seconds, const bool update_enemy) {
	vec2 previous_checkpoint = current_map.checkpoints[enemy.next_checkpoint - 1];
	if (enemy.next_checkpoint >= current_map.checkpoints.size()) {
		return previous_checkpoint;
	}
	vec2 next_checkpoint = current_map.checkpoints[enemy.next_checkpoint];
	float enemy_progress = enemy.enemy_progress;
	enemy_progress += (enemy.speed * seconds) / current_map.path_length;
	if (update_enemy) {
		enemy.enemy_progress = enemy_progress;
	}
	const float section_length = abs(distance(previous_checkpoint,
										next_checkpoint)); //TODO maybe already calc this in create_map and save with map
	float section_progress = enemy.section_progress;
	section_progress += (enemy.speed * seconds) / section_length;
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
	}
	return previous_checkpoint + (next_checkpoint - previous_checkpoint) * section_progress;
}
