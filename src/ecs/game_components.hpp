#pragma once

#include <variant>

#include "common.hpp"
#include <vector>

// Player component
struct Player {
    int health = 100;
    int maxHealth = 100;
    int coins = 0;
    std::vector<Entity> owned_cards;
    int won_battles = 0;
};

// Enemy components
struct Enemy {
    int health = 100;
	float enemy_progress = 0.0f;
    float speed = 0.f;
    float spawn_time = 0.f; // spawn time after combat started(in ms)
    bool spawned = false;
    uint next_checkpoint = 1; // checkpoint 0 is start position
    float section_progress = 0.f;
    bool alive = true;
	int damage = 20;
};

struct Slime {

};

// Items
enum class TowerType {
	ARCHER = 0,
	TOWER_TYPE_COUNT
};

enum class ConsumableType {
	CONSUMABLE_TYPE_COUNT
};

constexpr unsigned int consumable_type_count = static_cast<unsigned int>(ConsumableType::CONSUMABLE_TYPE_COUNT);
constexpr unsigned int tower_type_count = static_cast<unsigned int>(TowerType::TOWER_TYPE_COUNT);
constexpr unsigned int item_type_count = consumable_type_count + tower_type_count;

using ItemType = std::variant<TowerType, ConsumableType>;

struct Item {};

// Tower components
enum class EnemyPriority {
	LAST,
	FIRST
};

struct Tower {
	bool placed = false;
	float range = 50.0f;
	EnemyPriority priority = EnemyPriority::FIRST;
	bool is_aiming = false;
};

// given to card representation of tower while off field
struct Card{
	Entity item_entity;
    bool selected = false;
    bool dragged = false; //TODO: follow mouse pointer when true
    // TODO: add int parameter for position from left to right on screen?
};

struct TowerAimingAt {
	Entity aimed_entity;
};

struct ShotTimer {
	float time = 1000.0f;
};

// Archer
struct Archer {
	float arrow_speed = 1000.0f;
    Entity bow;
};

// Arrow
struct Arrow {
    int damage = 50;
	std::size_t max_hitcount = 1;
	std::set<Entity> hit_entities{};
};

// Bow
struct Bow {

};

// Weapon
struct Weapon { //TODO: give to bows, swords or any other future weapon types

};

// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0.f, 0.f };
	float angle = 0.f;
    bool use_direction_sprite = false; // TODO: maybe param in render request instead?
	vec2 velocity = { 0.f, 0.f };
	vec2 scale = { 10.f, 10.f };
};

// Stucture to store collision information
struct Collision
{
	// Note, the first object is stored in the ECS container.entities
	Entity other_entity; // the second object involved in the collision
	explicit Collision(const Entity other_entity) : other_entity(other_entity) {};
};

// Defines attributes of stationary entity
struct Stationary
{
    vec2 position = { 0.f, 0.f };
    float angle = 0.f;
    bool use_direction_sprite = false; // TODO: maybe param in render request instead?
    vec2 scale = { 100.f, 100.f };
};

struct Map
{
    std::vector<vec2> checkpoints = {vec2(0.f,0.f)};
    float path_length = 0;
    std::vector<float> section_lengths = {};
    std::vector<Entity> enemies = {};
    float combat_time = 0.f; // time passed since combat started (in ms)
    bool active = false;
};

struct OverviewMapLocation {
	std::vector<Entity> next_locations{};
	std::vector<Entity> previous_locations{};
	Entity overview_selection;
	bool selectable = false;
	bool active = false;
};

struct Invisible {
};

struct Clickable {
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = false;
	bool in_freeze_mode = false;
};
extern Debug debugging;

// Sets the brightness of the screen
struct ScreenState
{
	float screen_darken_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer
{
	float timer_ms = 3000.f;
};

// Text line that is rendered at a specific position and scale
struct Text {
	std::string text;
	vec2 position;
	float size;
};