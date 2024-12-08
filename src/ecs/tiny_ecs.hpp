#pragma once

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional>
#include <iterator>
#include <assert.h>

// Unique identifyer for all entities
class Entity
{
	unsigned int id = 0;
	static unsigned int id_count; // starts from 1, entity 0 is the default initialization
public:
	Entity()
	{
		id = id_count++;
		// Note, indices of already deleted entities aren't re-used in this simple implementation.
	}
	operator unsigned int() const { return id; } // this enables automatic casting to int
};

// Common interface to refer to all containers in the ECS registry
struct ContainerInterface
{
	virtual ~ContainerInterface() = default;

	virtual void clear() = 0;
	virtual size_t size() = 0;
    // the default parameter here is ignored since it is defined at the original method at compile time, still needs to be here to work in code
	virtual void remove(Entity e, bool keep_order = false) = 0;
	virtual bool has(Entity entity) = 0;
};

// A container that stores components of type 'Component' and associated entities
template <typename Component> // A component can be any class
class ComponentContainer final : public ContainerInterface
{
	// The hash map from Entity -> array index.
	std::unordered_map<unsigned int, unsigned int> map_entity_componentID; // the entity is cast to uint to be hashable.
	bool registered = false;
public:
	// Container of all components of type 'Component'
	std::vector<Component> components;

	// The corresponding entities
	std::vector<Entity> entities;

	// Constructor that registers the type
	ComponentContainer()
	{
	}

	// Inserting a component c associated to entity e
	inline Component& insert(const Entity e, Component c, const bool check_for_duplicates = true)
	{
		// Usually, every entity should only have one instance of each component type
		assert(!(check_for_duplicates && has(e)) && "Entity already contained in ECS registry");

		map_entity_componentID[e] = static_cast<unsigned int>(components.size());
		components.push_back(std::move(c)); // the move enforces move instead of copy constructor
		entities.push_back(e);
		return components.back();
	};

	// The emplace function takes the provided arguments Args, creates a new object of type Component, and inserts it into the ECS system
	template<typename... Args>
	Component& emplace(const Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...));
	};
	template<typename... Args>
	Component& emplace_with_duplicates(const Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...), false);
	};

	// A wrapper to return the component of an entity
	Component& get(const Entity e) {
		assert(has(e) && "Entity not contained in ECS registry");
		return components[map_entity_componentID[e]];
	}

	// Check if entity has a component of type 'Component'
	bool has(const Entity entity) override {
		return map_entity_componentID.contains(entity);
	}

	// Remove a component and pack the container to re-use the empty space
	void remove(const Entity e, const bool keep_order = false) override {
		if (has(e))
		{
            if (!keep_order) {
                // Get the current position
                int cID = map_entity_componentID[e];

                // Move the last element to position cID using the move operator
                // Note, components[cID] = components.back() would trigger the copy instead of move operator
                components[cID] = std::move(components.back());
                entities[cID] = entities.back(); // the entity is only a single index, copy it.
                map_entity_componentID[entities.back()] = cID;

                // Erase the old component and free its memory
                map_entity_componentID.erase(e);
                components.pop_back();
                entities.pop_back();
                // Note, one could mark the id for re-use
            } else {
                // Get the current position
                const int cID = map_entity_componentID[e];
                for (std::size_t i = cID; i < components.size()-1; ++i) { //remove doesnt seem to work
                    components[i] = std::move(components[i+1]);
                    entities[i] = entities[i+1]; // the entity is only a single index, copy it.
                    map_entity_componentID[entities[i+1]] = i;
                }
                // Erase the old component and free its memory
                map_entity_componentID.erase(e);
                components.pop_back();
                entities.pop_back();
            }
		}
	};

	// Remove all components of type 'Component'
	void clear() override {
		map_entity_componentID.clear();
		components.clear();
		entities.clear();
	}

	// Report the number of components of type 'Component'
	size_t size() override {
		return components.size();
	}

	// Sort the components and associated entity assignment structures by the comparisonFunction, see std::sort
	template <class Compare>
	void sort(Compare comparisonFunction)
	{
		// First sort the entity list as desired
		std::sort(entities.begin(), entities.end(), comparisonFunction);
		// Now re-arrange the components (Note, creates a new vector, which may be slow! Not sure if in-place could be faster: https://stackoverflow.com/questions/63703637/how-to-efficiently-permute-an-array-in-place-using-stdswap)
		std::vector<Component> components_new; components_new.reserve(components.size());
		std::transform(entities.begin(), entities.end(), std::back_inserter(components_new), [&](Entity e) { return std::move(get(e)); }); // note, the get still uses the old hash map (on purpose!)
		components = std::move(components_new); // note, we use move operations to not create unneccesary copies of objects, but memory is still allocated for the new vector
		// Fill the new hashmap
		for (unsigned int i = 0; i < entities.size(); i++)
			map_entity_componentID[entities[i]] = i;
	}
};
