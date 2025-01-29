#pragma once

#include <variant>

#include "common.hpp"
#include <vector>
#include <map>

// Player component
class Player {
	int health = 100;
	int food = 10;
	int coins = 0;

	public:
    int score = 0;
    float game_speed = 1.f;
    int maxHealth = 100;
	int baseFoodGain = 5;
    std::vector<Entity> owned_cards;
    int won_battles = 0;
    int defeated_enemies = 0;
	std::vector<Entity> status_bar_entities;
	std::function<void()> status_bar_cleanup_func = []{};
    std::vector<Entity> placement_marker;
    Entity cardStatsWindow;

	std::function<void(int new_hp, int old_hp)> health_update_callback = [](int ...){};
	std::function<void(int new_food, int old_food)> food_update_callback = [](int ...){};
	std::function<void(int new_coins, int old_coins)> coins_update_callback = [](int ...){};

	int getHealth() const { return health; }
	int getFood() const { return food; }
	int getCoins() const { return coins; }

	void updateHealth(const int new_hp) {
		health_update_callback(new_hp, health);
		health = new_hp;
	}

	void updateFood(const int new_food) {
		food_update_callback(new_food, food);
		food = new_food;
	}

	void updateCoins(const int new_coins) {
		coins_update_callback(new_coins, coins);
		coins = new_coins;
	}

	~Player() {
		status_bar_cleanup_func();
	}
    //std::map<float, Entity> animation_timers; // TODO: map movement speed to animation timer
};

// Enemy components
enum class EnemyType {
    SLIME = 0,
    SLIME_BIG,
    ENEMY_TYPE_COUNT
};

class Enemy {
	int health = 0;
	std::function<void()> damage_callback = []{};
public:
	float enemy_progress = 0.0f;
    float speed = 0.f;
    float spawn_time = 0.f; // spawn time after combat started(in ms)
    bool spawned = false;
    uint next_checkpoint = 1; // checkpoint 0 is start position
    float section_progress = 0.f;
    bool alive = true;
	int damage = 10; // damage to player if enemy completes path
	int coin_gain = 1;
    std::vector<Entity> spawns_enemies; // spawns these enemies (eg. on death)

	explicit Enemy(const std::function<void()> &damage_callback) : damage_callback(damage_callback) {}

	int getHealth() const { return health; }

	void addDamage(const int damage) {
		health -= damage;
		printf("health = %d\n", health);
		printf("alive = %d\n", alive);
		damage_callback();
	}

	void addHealth(const int new_health) { health += new_health; }

    void setPlayerDamage(const int player_damage) {
        damage = player_damage;
    }
};

struct Slime {
};

struct SlimeBig {
};

struct Slowed {
    float slow = 1.f; // percentage, by which unit is slowed
    Entity origin;
};

// Items
enum class TowerType {
	ARCHER = 0,
    KNIGHT,
	TOWER_TYPE_COUNT
};

enum class ConsumableType {
    BOMB = 0,
    SPIKES,
    BARRIER,
    HEALTH_POTION,
	CONSUMABLE_TYPE_COUNT
};

constexpr unsigned int consumable_type_count = static_cast<unsigned int>(ConsumableType::CONSUMABLE_TYPE_COUNT);
constexpr unsigned int tower_type_count = static_cast<unsigned int>(TowerType::TOWER_TYPE_COUNT);
constexpr unsigned int item_type_count = consumable_type_count + tower_type_count;

using ItemType = std::variant<TowerType, ConsumableType>;

struct Item {
	int gold_cost = 5;
};

// Tower components
enum class EnemyPriority {
	LAST,
	FIRST
};

struct Tower {
	bool placed = false; //TODO: placed even necessary? other towers are cards next to which towers cant be placed either
	float range = 50.0f; //TODO: is range scaled with window? when trying #include "world_init.hpp" compilation fails
	int food_cost = 5;
	int food_gain = 5;
	EnemyPriority priority = EnemyPriority::FIRST;
	bool is_aiming = false;
};

// given to card representation of tower while off field
struct Card {
	Entity item_entity;
    bool selected = false;
	bool selectable = true;
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
    int damage = 50;
    Entity bow;
};

// Arrow
struct Arrow {
    int damage;
	std::size_t max_hitcount = 1;
	std::set<Entity> hit_entities{};
};

// Bow
struct Bow {
    int damage = 50;
};

// Knight
struct Knight {
    float swing_time = 800.0f; //time to complete a full swing
    float cooldown = 1200.0f;
    int damage = 30;
    Entity sword;
};

// Sword
struct Sword { //TODO: maybe outsource hitcound and hit list to Weapon and make Arrow Weapon, too.
    int damage;
    bool has_collision = false; // activate collision when sword is swung
    std::set<Entity> hit_entities{};
};

// Weapon
struct Weapon { //TODO: give to bows, swords or any other future weapon types

};

//Consumable components
struct Consumable {
    bool placed = false;
    float range = 100;
};

struct Bomb {
    int damage = 9999;
    //float range = 100;
    bool exploding = false;
    std::set<Entity> hit_entities{};
};

struct BombTimer {
    float burn_time = 1500;
    float explosion_time = 300.0f; //explodes when timer below this
    float time = burn_time + explosion_time;//1800.0f;
};

struct Spike {
    int damage = 200;
    std::size_t max_hitcount = 3;
    std::set<Entity> hit_entities{};
};

struct Barrier {
    int health = 3;
};

struct BarrierTimer {
    float hold_time = 1000.0f; // loses 1 health after holding enemies for this amount of time
    float time = hold_time;
};

struct HealthPotion {
    int health = 40;
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

struct PlacementMarker {
};

struct Map
{
    std::vector<vec2> checkpoints = {vec2(0.f,0.f)};
    float path_length = 0;
    std::vector<float> section_lengths = {};
    std::vector<Entity> enemies = {};
    float combat_time = 0.f; // time passed since combat started (in ms)
	Entity map_decoration;
    bool active = false;
	// This is some wild shit, but should work to help cleaning up the map_decoration
	// (maybe there is a better way, but including the ecs registry in this header is not possible)
	std::function<void()> destruction_lambda = [] {};

	~Map() {
		destruction_lambda();
	}
};

enum class LocationType {
	FIGHT = 0,
	GARRISON,
	MERCHANT,
};

struct OverviewMapLocation {
	std::vector<Entity> next_locations{};
	std::vector<Entity> previous_locations{};
	Entity overview_selection;
	LocationType type;
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

struct HitTimer {
	float timer_ms = 200.f;
};

struct ScoreTimer {
    float start_time = 500.f;
    float timer_ms = start_time;
};

enum class ScoreStep {
    START = 0,
    BATTLES,
    KILLS,
    HEALTH,
    FOOD,
    COINS,
    CARDS,
    FACTORS,
    FINAL_TEXT,
    FINAL_SCORE,
    CONTINUE,
    WAIT,
};

enum class StatusType {
    HEALTH = 0,
    FOOD,
	COINS
};

struct StatusTextTimer {
	float timer_ms = 2000.f;
    StatusType type = StatusType::HEALTH;
};

struct CardStatsWindow {
    Entity text_entity;
    Entity background_entity;
    size_t text_rows = 0;
    size_t text_cols = 0;
};

struct Button {
	std::vector<std::function<void(bool)>> on_click_listeners{};
	std::function<void()> cleanup_func = []{};

	void click(const bool pressing) {
		for (const auto & on_click_listener : on_click_listeners) {
			on_click_listener(pressing);
		}
	}

	~Button() {
		cleanup_func();
	}
};

enum class Music {
    OVERVIEW = 0,
    FIGHT,
    SHOP,
    END,
};