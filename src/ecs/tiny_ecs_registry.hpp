#pragma once
#include <vector>

#include "tiny_ecs.hpp"
#include "game_components.hpp"
#include "render_components.hpp"
#include "physics_components.hpp"

class ECSRegistry
{
	// Callbacks to remove a particular or all entities in the system
	std::vector<ContainerInterface*> registry_list;

public:
	// Manually created list of all components this game has
	ComponentContainer<DeathTimer> deathTimers;
	ComponentContainer<Motion> motions;
	ComponentContainer<Collision> collisions;
	ComponentContainer<Player> players;
	ComponentContainer<Item> items;
	ComponentContainer<Tower> towers;
    ComponentContainer<Consumable> consumables;
    ComponentContainer<Card> cards;
	ComponentContainer<TowerAimingAt> aimingAts;
	ComponentContainer<ShotTimer> shotTimers;
	ComponentContainer<Arrow> arrows;
    ComponentContainer<Bow> bows;
	ComponentContainer<Archer> archers;
    ComponentContainer<Knight> knights;
    ComponentContainer<Sword> swords;
    ComponentContainer<Weapon> weapons;
    ComponentContainer<Bomb> bombs;
    ComponentContainer<BombTimer> bombTimers;
    ComponentContainer<Spike> spikes;
    ComponentContainer<Barrier> barriers;
    ComponentContainer<BarrierTimer> barrierTimers;
    ComponentContainer<HealthPotion> healthPotions;
    ComponentContainer<Stationary> stationaries;
    ComponentContainer<PlacementMarker> placementMarkers;
    ComponentContainer<Map> maps;
	ComponentContainer<Enemy> enemies;
    ComponentContainer<Slime> slimes;
    ComponentContainer<SlimeBig> slimesBig;
    ComponentContainer<Slowed> sloweds;
	ComponentContainer<OverviewMapLocation> overviewMapLocations;
	// TODO Think about deleting this or replace this for the collision polygons
	ComponentContainer<Mesh*> meshPtrs;
    ComponentContainer<EnemyWalkTimer> enemyWalkTimers;
	ComponentContainer<RenderRequest> renderBackground;
	ComponentContainer<RenderRequest> renderGameLayer;
	ComponentContainer<RenderRequest> renderForeground;
	ComponentContainer<ScreenState> screenStates;
	ComponentContainer<DebugComponent> debugComponents;
	ComponentContainer<vec4> colors;
	ComponentContainer<Invisible> invisibles;
	ComponentContainer<Clickable> clickables;
	ComponentContainer<Text> texts;
	ComponentContainer<HitTimer> hitTimers;
	ComponentContainer<StatusTextTimer> statusTextTimers;
    ComponentContainer<HitboxVisualization> hitboxVisualizations;
    ComponentContainer<PathVisualization> pathVisualizations;
    ComponentContainer<TowerVisualization> towerVisualizations;

	// constructor that adds all containers for looping over them
	// IMPORTANT: Don't forget to add any newly added containers!
	ECSRegistry()
	{
		registry_list.push_back(&deathTimers);
		registry_list.push_back(&motions);
		registry_list.push_back(&collisions);
		registry_list.push_back(&players);
		registry_list.push_back(&items);
		registry_list.push_back(&towers);
        registry_list.push_back(&cards);
		registry_list.push_back(&aimingAts);
		registry_list.push_back(&shotTimers);
		registry_list.push_back(&arrows);
        registry_list.push_back(&bows);
		registry_list.push_back(&archers);
        registry_list.push_back(&knights);
        registry_list.push_back(&swords);
        registry_list.push_back(&weapons);
        registry_list.push_back(&bombs);
        registry_list.push_back(&bombTimers);
        registry_list.push_back(&spikes);
        registry_list.push_back(&barriers);
        registry_list.push_back(&barrierTimers);
        registry_list.push_back(&healthPotions);
        registry_list.push_back(&stationaries);
        registry_list.push_back(&placementMarkers);
        registry_list.push_back(&maps);
		registry_list.push_back(&enemies);
        registry_list.push_back(&slimes);
        registry_list.push_back(&slimesBig);
        registry_list.push_back(&sloweds);
		registry_list.push_back(&overviewMapLocations);
		registry_list.push_back(&meshPtrs);
        registry_list.push_back(&enemyWalkTimers);
		registry_list.push_back(&renderBackground);
		registry_list.push_back(&renderGameLayer);
		registry_list.push_back(&renderForeground);
		registry_list.push_back(&screenStates);
		registry_list.push_back(&debugComponents);
		registry_list.push_back(&colors);
		registry_list.push_back(&invisibles);
		registry_list.push_back(&clickables);
		registry_list.push_back(&texts);
		registry_list.push_back(&hitTimers);
		registry_list.push_back(&statusTextTimers);
        registry_list.push_back(&hitboxVisualizations);
        registry_list.push_back(&pathVisualizations);
        registry_list.push_back(&towerVisualizations);
	}

	void clear_all_components() {
		for (ContainerInterface* reg : registry_list)
			reg->clear();
	}

	void list_all_components() {
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registry_list)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", static_cast<int>(reg->size()), typeid(*reg).name());
	}

	void list_all_components_of(Entity e) {
		printf("Debug info on components of entity %u:\n", static_cast<unsigned int>(e));
		for (ContainerInterface* reg : registry_list)
			if (reg->has(e))
				printf("type %s\n", typeid(*reg).name());
	}

	void remove_all_components_of(const Entity e) {
		for (ContainerInterface* reg : registry_list)
			reg->remove(e);
	}
};

extern ECSRegistry registry;