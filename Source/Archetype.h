#pragma once

#include <utility>

#include "ComponentTypeTraits.h"
#include "Containers/BitSet.h"
#include "Pool.h"

namespace X
{

class Archetype
{
public:
    explicit Archetype() : pool(),
                           componentMask(Internal::ComponentTypeTraits::CurrentCount()),
                           forwardArchetypes(Internal::ComponentTypeTraits::CurrentCount()),
                           backwardArchetypes(Internal::ComponentTypeTraits::CurrentCount())
    {}

    ~Archetype() = default;

    // For the default archetype
    void RegisterEmptyEntity(const Entity entity)
    {
        EntityMapAssureSizeAndSet(entity, pool.GetNextIndex(entity));
    }

    // Returns a new ALLOCATED archetype grown to hold the new component
    template<typename Component>
    [[nodiscard]] static Archetype* AddComponentToArchetype(const Archetype* templateArchetype)
    {
        return CreateNewRuntimeArchetype<Component, true>(templateArchetype);
    }

    // Returns a new ALLOCATED archetype shrunk to no longer hold the old component
    template<typename Component>
    [[nodiscard]] static Archetype*
    RemoveComponentFromArchetype(const Archetype* templateArchetype)
    {
        return CreateNewRuntimeArchetype<Component, false>(templateArchetype);
    }

    // Transfer to this archetype while adding a component
    template<typename NewComponent, typename... Args>
    NewComponent& TransferEntityEmplace(Archetype* oldArchetype, const Entity entity, Args&&... args)
    {
        Internal::Pool::IndexType oldIndex = oldArchetype->entityMap[entity];
        Internal::Pool::IndexType nextIndex = pool.GetNextIndex(entity);
        // Copy existing components over
        for (size_t i = 0; i < oldArchetype->componentMask.GetSize(); ++i)
            if (oldArchetype->componentMask[i] == BitSet::True)
                pool.CopyComponentToPool(nextIndex, i, oldArchetype->pool.GetComponentRaw(oldIndex, i));
        // Register new entity link
        EntityMapAssureSizeAndSet(entity, nextIndex);
        // Add the new component
        NewComponent& newComponent = pool.EmplaceComponent<NewComponent>(nextIndex, Internal::GetComponentId<NewComponent>::value, std::forward<Args>(args)...);
        // Remove from old archetype
        oldArchetype->RemoveEntity<false>(entity);
        return newComponent;
    }

    // Transfer to this archetype while adding a component
    template<typename NewComponent>
    void TransferEntityInsert(Archetype* oldArchetype, const Entity entity, const NewComponent& component)
    {
        Internal::Pool::IndexType oldIndex = oldArchetype->entityMap[entity];
        Internal::Pool::IndexType nextIndex = pool.GetNextIndex(entity);
        // Copy existing components over
        for (size_t i = 0; i < oldArchetype->componentMask.GetSize(); ++i)
            if (oldArchetype->componentMask[i] == BitSet::True)
                pool.CopyComponentToPool(nextIndex, i, oldArchetype->pool.GetComponentRaw(oldIndex, i));
        // Register new entity link
        EntityMapAssureSizeAndSet(entity, nextIndex);
        // Add the new component
        pool.InsertComponent<NewComponent>(nextIndex, Internal::GetComponentId<NewComponent>::value, component);
        // Remove from old archetype
        oldArchetype->RemoveEntity<false>(entity);
    }

    // Transfer to this archetype while removing a component
    template<typename Component>
    void TransferEntityRemove(Archetype* oldArchetype, const Entity entity)
    {
        Internal::Pool::IndexType oldIndex = oldArchetype->entityMap[entity];
        Internal::Pool::IndexType nextIndex = pool.GetNextIndex(entity);
        // Copy existing components over, minus one
        for (size_t i = 0; i < componentMask.GetSize(); ++i)
        {
            if (componentMask[i] == BitSet::True)
                pool.CopyComponentToPool(nextIndex, i, oldArchetype->pool.GetComponentRaw(oldIndex, i));
        }
        // Register new entity link
        EntityMapAssureSizeAndSet(entity, nextIndex);
        // Remove from old archetype
        // Don't destroy because the transfer above already did
        oldArchetype->RemoveEntity<false>(entity);
    }

    // Entity -> Pool -> Component
    template<typename Component>
    [[nodiscard]] Component& GetComponent(const Entity entity)
    {
        ALLOY_ASSERT(entityMap[entity] != InvalidIndex, "Archetype does not hold the given entity.");
        return pool.GetComponent<Component>(entityMap[entity]);
    }

    // Entity -> Pool -> Components
    template<typename... Components, typename Function>
    void GetComponents(const Entity entity, Function&& function)
    {
        ALLOY_ASSERT(entityMap[entity] != InvalidIndex, "Archetype does not hold the given entity.");
        Internal::Pool::IndexType entityIndex = entityMap[entity];
        return function(pool.GetComponent<Components>(entityIndex)...);
    }

    template<bool Destroy = true>
    void RemoveEntity(const Entity entity)
    {
        ALLOY_ASSERT(entityMap[entity] != InvalidIndex, "Archetype does not hold the given entity.");
        Archetype::RemoveEntityInternal<Destroy>(entity);
    }

    [[nodiscard]] BitSet& GetComponentMask()
    {
        return componentMask;
    }

    [[nodiscard]] Internal::Pool& GetPool()
    {
        return pool;
    }

    [[nodiscard]] std::vector<Archetype*>& GetForwardArchetypes()
    {
        return forwardArchetypes;
    }

    [[nodiscard]] std::vector<Archetype*>& GetBackwardArchetypes()
    {
        return backwardArchetypes;
    }

protected:
    static constexpr Internal::Pool::IndexType InvalidIndex = std::numeric_limits<X::Internal::Pool::IndexType>::max();

    template<bool Destroy>
    void RemoveEntityInternal(X::Entity entity)
    {
        // Get the pool index for the entity using the map
        size_t poolIndex = entityMap[entity];

        if constexpr (Destroy)
        {
            auto& componentIds = pool.GetComponentIds();
            for (X::Internal::ComponentTypeTraits::ComponentId componentId : componentIds)
            {
                uint8_t* c = pool.Get(componentId, poolIndex);
                Internal::ComponentTypeTraits::GetComponentInfo()[componentId].destructorCaller(c);
                poolIndex = entityMap[entity];
            }
        }

        //poolIndex = entityMap[entity];

        size_t lastIndex = pool.GetSize() - 1;

        // Old entity data is not at the end, copy last item in
        if (poolIndex != lastIndex)
        {
            // Swap the last archetype in, getting the last index's parent
            IdType lastIndexParent = pool.CopyLastIn<false>(poolIndex);
            // Update the pool index in case a component destructor also destroyed another entity
            entityMap[lastIndexParent] = poolIndex;
#ifdef ALLOY_DEBUG
            entityMap[entity] = InvalidIndex;
#endif
        }
        pool.PopBack<false>();
        // Remove entity from archetype sparse set
#ifdef ALLOY_DEBUG
        entityMap[entity] = InvalidIndex;
#endif
    }

    void EntityMapAssureSizeAndSet(IdType entity, Internal::Pool::IndexType index)
    {
        if (entityMap.size() <= entity)
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
            entityMap.resize(static_cast<size_t>(v), InvalidIndex);
#else
            entityMap.resize(static_cast<size_t>(v));
#endif
        }

        entityMap[entity] = index;
    }

    template<typename Component, bool AddComponent>
    [[nodiscard]] static Archetype* CreateNewRuntimeArchetype(const Archetype* templateArchetype)
    {
        auto* newArchetype = new Archetype();
        // Update the component mask with the added component
        newArchetype->componentMask = templateArchetype->componentMask;
        //        std::destroy_at(&newArchetype->pool);
        if constexpr (AddComponent)
        {
            newArchetype->componentMask.Set(Internal::GetComponentId<Component>::value, BitSet::True);
            Internal::Pool::CreateIncreasedPool<Component>(&templateArchetype->pool, &newArchetype->pool);
        }
        else
        {
            newArchetype->componentMask.Set(Internal::GetComponentId<Component>::value, BitSet::False);
            Internal::Pool::CreateDecreasedPool<Component>(&templateArchetype->pool, &newArchetype->pool);
        }
        return newArchetype;
    }

    // Graph of how to find the next archetype
    std::vector<Archetype*> forwardArchetypes;
    std::vector<Archetype*> backwardArchetypes;

    // This archetype's components
    Internal::Pool pool;
    // Entity ID -> Pool Index
    // Starts with a size of 1, so it doubles in size as it grows
    SparseSetContainer<X::Internal::Pool::IndexType> entityMap{ 0 };

    // Stores components the archetype holds
    BitSet componentMask;
};

} // namespace X
