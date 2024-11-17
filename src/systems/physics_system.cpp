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
	    Map& active_map = active_maps[0];

    	auto& enemy_container = registry.enemies;
    	for (uint i = 0; i < enemy_container.size(); i++) {
    		Enemy& enemy = enemy_container.components[i];
            if(enemy.spawned) {
                Motion &motion = registry.motions.get(enemy_container.entities[i]);
                float step_seconds = elapsed_ms / 1000.f;
                vec2 previous_checkpoint = active_map.checkpoints[enemy.next_checkpoint - 1];
                if (enemy.next_checkpoint >= active_map.checkpoints.size()) {
                    break;
                }
                vec2 next_checkpoint = active_map.checkpoints[enemy.next_checkpoint];
                enemy.enemy_progress += (enemy.speed * step_seconds) / active_map.path_length;
                float section_length = abs(distance(previous_checkpoint,
                                                    next_checkpoint)); //TODO maybe already calc this in create_map and save with map
                enemy.section_progress += (enemy.speed * step_seconds) / section_length;
                //printf("%f\n", enemy.section_progress);
                if (enemy.section_progress >= 1) {
                    enemy.next_checkpoint++;
                    if (enemy.next_checkpoint >= active_map.checkpoints.size()) {
                        break;
                    }
                    enemy.section_progress = 0.f;
                    previous_checkpoint = active_map.checkpoints[enemy.next_checkpoint - 1];
                    next_checkpoint = active_map.checkpoints[enemy.next_checkpoint];
                }
                motion.position = previous_checkpoint + (next_checkpoint - previous_checkpoint) * enemy.section_progress;
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
            }else if (collides(motion_i, motion_j)) {
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE PEBBLE collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}
