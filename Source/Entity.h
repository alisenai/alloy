#pragma once

#include <set>
#include <unordered_set>

#include "Alloy/AlloyConfig.h"


namespace X
{

class Entity;

namespace Internal
{

class EntityManager;

} // namespace Internal

struct Entity
{
public:
    // Will create an INVALID entity
    Entity() : id(INVALID_ENTITY_ID), generation(INVALID_ENTITY_ID)
    {}

    operator IdType() const
    {
        return id;
    }

    [[nodiscard]] IdType GetId() const
    {
        return id;
    }

    [[nodiscard]] GenerationType GetGeneration() const
    {
        return generation;
    }

    // Compare two entities
    [[nodiscard]] bool operator==(const Entity& other) const
    {
        return id == other.id && generation == other.generation;
    }

    [[nodiscard]] bool operator!=(const Entity& other) const
    {
        return id != other.id || generation != other.generation;
    }

    [[nodiscard]] bool operator<(const Entity& other) const
    {
        return id == other.id ? generation < other.generation : id < other.id;
    }

private:
    // Private so only copies of the entity can be made to avoid creation of entities "out of thin air"
    IdType id;
    GenerationType generation;

    friend class Internal::EntityManager;

    // Private so Entities can't be created "out of thin air"
    explicit Entity(const IdType id, const GenerationType generation) : id(id), generation(generation)
    {}
};


namespace Internal
{

// Entity hashing util
struct EntityHash
{
    inline std::size_t operator()(const Entity& entity) const noexcept
    {
        return std::hash<X::IdType>{}(entity);
    }
};

} // namespace Internal

} // namespace X

namespace std
{
template<>
struct hash<X::Entity>
{
    std::size_t operator()(X::Entity e) const
    {
        return std::hash<X::IdType>{}(e.GetId()) ^ (std::hash<X::GenerationType>{}(e.GetGeneration()) + 1);
    }
};
} // namespace std
