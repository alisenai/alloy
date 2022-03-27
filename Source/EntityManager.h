#pragma once

#include "Archetype.h"
#include "ArchetypeMap.h"
#include "Entity.h"

#include "Containers/RecycledCounter.h"
#include <iostream>
#include <stack>
#include <unordered_set>

namespace X::Internal
{
class EntityManager
{
public:
    explicit EntityManager(ArchetypeMap& archetypeMap) : archetypeMap(archetypeMap)
    {}

    // Creates an entity
    // Attempts to use a recycled entity ID
    // Uses the next available ID if no recycled entity ID exists
    [[nodiscard]] Entity CreateEntity()
    {
        IdType nextId = recycledCounter.GetNextId();
        Entity newEntity{ nextId, GetGeneration(nextId) };
        Archetype* baseArchetype = archetypeMap.GetBaseArchetype();
        baseArchetype->RegisterEmptyEntity(newEntity);
        archetypeLookupAssureSizeAndSet(nextId, baseArchetype);
        return newEntity;
    }

    // Destroys an entity
    // All components are removed
    // The entity ID is then recycled
    template<bool DestroyFromLookup>
    void DestroyEntity(const Entity entity)
    {
        // Check to assure invalid entities aren't being destroyed
        ALLOY_ASSERT(IsValid(entity), "Invalid entity was destroyed");

        if constexpr (DestroyFromLookup)
        {
            ALLOY_ASSERT(entity < archetypeLookup.size(), "Invalid entity was destroyed");
            // Entity ID -> Archetype -> Destroy
            archetypeLookup[entity]->RemoveEntity<true>(entity);
#ifdef ALLOY_DEBUG
            archetypeLookup[entity] = InvalidArchetype;
#endif
        }
        InvalidateEntity(entity);
        recycledCounter.RecycleId(entity);
    }

    void InvalidateEntity(const Entity entity)
    {
        ++generationLookup[entity];
    }

    template<typename Component, typename... Args>
    Component& EmplaceComponent(const Entity entity, Args&&... args)
    {
        ALLOY_ASSERT(HasComponent<Component>(entity) == false, "Entity already has the component that is being added.");
        // Get the current, now old, archetype
        Archetype* oldArchetype = archetypeLookup[entity];
        // Try to find an archetype given the old archetype
        Archetype* newArchetype = archetypeMap.GetForwardArchetype<Component>(oldArchetype);

        //#ifdef ALLOY_DEBUG
        //        archetypeLookup[entity] = InvalidArchetype;
        //#endif

        archetypeLookup[entity] = newArchetype;
        // Transfer the entity over
        Component& newComponent = newArchetype->TransferEntityEmplace<Component>(
            oldArchetype,
            entity,
            std::forward<Args>(args)...);
        return newComponent;
    }

    template<typename Component>
    void InsertComponent(const Entity entity, const Component& component = {})
    {
        ALLOY_ASSERT(HasComponent<Component>(entity) == false, "Entity already has the component that is being added.");
        // Get the current, now old, archetype
        Archetype* oldArchetype = archetypeLookup[entity];
        // Try to find an archetype given the old archetype
        Archetype* newArchetype = archetypeMap.GetForwardArchetype<Component>(oldArchetype);

        archetypeLookup[entity] = newArchetype;
        // Transfer the entity over
        newArchetype->TransferEntityInsert<Component>(
            oldArchetype,
            entity,
            component);
    }

    template<typename Component>
    void RemoveComponent(const Entity entity)
    {
        ALLOY_ASSERT(HasComponent<Component>(entity) == true, "Entity does not have the component that is being removed.");
        // Get the current, now old, archetype
        Archetype* oldArchetype = archetypeLookup[entity];
        // Try to find an archetype given the old archetype
        Archetype* newArchetype = archetypeMap.template GetBackwardArchetype<Component>(oldArchetype);

        archetypeLookup[entity] = newArchetype;
        // Transfer the entity over
        newArchetype->TransferEntityRemove<Component>(oldArchetype, entity);
    }

    // Entity ID -> ComponentMask -> Archetype -> Component
    template<typename Component>
    [[nodiscard]] Component& GetComponent(const Entity entity)
    {
        ALLOY_ASSERT(HasComponent<Component>(entity), "Entity does not hold the requested component.");
        return archetypeLookup[entity]->GetComponent<Component>(entity);
    }

    // Entity ID -> ComponentMask -> Archetype -> Components
    template<typename... Components, typename UpdateFunction>
    void GetComponents(const Entity entity, UpdateFunction&& updateFunction)
    {
        archetypeLookup[entity]->GetComponents<Components...>(entity, std::forward<UpdateFunction>(updateFunction));
    }

    // Entity ID -> ComponentMask -> Archetype -> Check
    template<typename Component>
    [[nodiscard]] bool HasComponent(const Entity entity)
    {
        ALLOY_ASSERT(entity < archetypeLookup.size(), "Given bad entity.");
        ALLOY_ASSERT(archetypeLookup[entity] != nullptr, "Given bad entity.");
        return archetypeLookup[entity]->GetComponentMask()[GetComponentId<Component>::value] == BitSet::True;
    }

    [[nodiscard]] bool IsValid(const Entity entity)
    {
        return entity.id != INVALID_ENTITY_ID &&
               entity.id < generationLookup.size() &&
               generationLookup[entity] == entity.GetGeneration();
    }

#ifdef ALLOY_EXPOSE_INTERNALS
    const RecycledCounter<IdType>& GetRecycledCounter() const
    {
        return recycledCounter;
    }

    [[nodiscard]] bool HasComponentById(const Entity entity, size_t componentId)
    {
        ALLOY_ASSERT(entity < archetypeLookup.size(), "Given bad entity.");
        ALLOY_ASSERT(archetypeLookup[entity] != nullptr, "Given bad entity.");
        return archetypeLookup[entity]->GetComponentMask()[componentId] == BitSet::True;
    }
#endif

private:
    static constexpr Archetype* InvalidArchetype = nullptr;

    void archetypeLookupAssureSizeAndSet(IdType entity, Archetype* RuntimeArchetype)
    {
        if (archetypeLookup.size() <= entity)
        {
            IdType v = entity + 1;
            --v;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            ++v;
#ifdef ALLOY_DEBUG
            archetypeLookup.resize(v, InvalidArchetype);
#else
            archetypeLookup.resize(v);
#endif
        }
        archetypeLookup[entity] = RuntimeArchetype;
    }

    GenerationType GetGeneration(IdType entity)
    {
        if (generationLookup.size() <= entity)
        {
            IdType v = entity + 1;
            --v;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            ++v;
            generationLookup.resize(v, 0);
        }
        return generationLookup[entity];
    }

    RecycledCounter<IdType> recycledCounter{};
    // Entity ID -> Archetype
    SparseSetContainer<Archetype*> archetypeLookup{ nullptr };
    ArchetypeMap& archetypeMap;
    SparseSetContainer<GenerationType> generationLookup{ 0 };
};

} // namespace X::Internal
