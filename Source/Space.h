#pragma once

#include "ArchetypeMap.h"
#include "Containers/RecycledCounter.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Query.h"

namespace X
{
class Space
{
public:
    Space() : spaceId(spaceCounter.GetNextId()), archetypeMap(this, spaceId), entityManager(archetypeMap)
    {
        spaces.emplace_back(this);
    }

    void DestroyAllEntities()
    {
        for (Archetype* archetype : Internal::Query<>::Get()->GetArchetypes(spaceId))
        {
            X::Internal::Pool& pool = archetype->GetPool();
            // Using a 'while' here because an entity's destructor can destroy another entity
            while (pool.GetSize() > 0)
                entityManager.DestroyEntity<true>(pool.GetParent(0));
        }
    }

    ~Space()
    {
        DestroyAllEntities();

        for (Internal::QueryInterface* queryInterface : Internal::QueryInterface::GetQueries())
            queryInterface->UnregisterSpace(spaceId);

        spaceCounter.RecycleId(spaceId);
        MoveLastInAndRemove(spaces, this);
    }

    // Create a new entity
    [[nodiscard]] Entity CreateEntity()
    {
        return entityManager.CreateEntity();
    }

#ifdef ALLOY_DEFER_DESTRUCTION
    // Defer destruction of a given entity
    template<bool DestroyFromLookup = true>
    void DestroyEntity(const Entity entity)
    {
        ALLOY_ASSERT(entityManager.IsValid(entity), "Given entity is not valid and therefore cannot be destroyed.");
        toDestroy.emplace_back(entity);
    }

    // Apply all deferred destruction requests
    void ApplyDestructionQueue()
    {
        for (int index = 0; index < toDestroy.size(); ++index)
            entityManager.DestroyEntity<true>(toDestroy[index]);
        toDestroy.clear();
    }
#else
    // Destroy a given entity
    template<bool DestroyFromLookup = true>
    void DestroyEntity(const Entity entity)
    {
        ALLOY_ASSERT(entityManager.IsValid(entity), "Given entity is not valid and therefore cannot be destroyed.");
        entityManager.DestroyEntity<DestroyFromLookup>(entity);
    }
#endif

    template<typename Component, typename... Args>
    void EmplaceComponent(const Entity entity, Args&&... args)
    {
        entityManager.template EmplaceComponent<Component>(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    void InsertComponent(const Entity entity, const Component& component = {})
    {
        entityManager.template InsertComponent(entity, component);
    }

    template<typename... Components>
    void InsertComponents(const Entity entity)
    {
        (entityManager.template InsertComponent<Components>(entity), ...);
    }

    template<typename... Components, typename Function>
    auto SetComponent(const Entity entity, Function&& function)
    {
        return function(entityManager.template EmplaceComponent<Components>(entity)...);
    }

    template<typename Component, typename Function, typename... Args>
    auto SetComponent(const Entity entity, Function&& function, Args&&... args)
    {
        return function(entityManager.template EmplaceComponent<Component>(entity, std::forward<Args>(args)...));
    }

    //    // TODO: Make this faster
    //    template<typename... Components>
    //    void InsertComponents(const Entity entity, const Components&&... component)
    //    {
    //        (entityManager.template InsertComponent(entity, component), ...);
    //    }

    // Returns true/false depending on if the given entity has a component or not
    template<typename Component>
    [[nodiscard]] bool HasComponent(const Entity entity)
    {
        return entityManager.HasComponent<Component>(entity);
    }

    template<typename Component, typename Function>
    auto GetComponent(const Entity entity, Function&& function)
    {
        return function(entityManager.GetComponent<Component>(entity));
    }

    // Returns a given component type from a given entity
    // Reference is only valid until the parent space is edited in ANY way
    template<typename Component>
    Component& GetComponentTemporary(const Entity entity)
    {
        ALLOY_ASSERT(IsValid(entity), "Given entity is not valid.");
        ALLOY_ASSERT(HasComponent<Component>(entity), "Entity does not have the requested component.");
        return entityManager.GetComponent<Component>(entity);
    }

    // Returns multiple component types from a given entity
    template<typename... Components, typename Function>
    void GetComponents(const Entity entity, Function&& function)
    {
        entityManager.GetComponents<Components...>(entity, std::forward<Function>(function));
    }

    // Removes a given component type from a given entity
    template<typename Component>
    void RemoveComponent(const Entity entity)
    {
        entityManager.RemoveComponent<Component>(entity);
    }

    [[nodiscard]] bool IsValid(const X::Entity entity)
    {
        return entityManager.IsValid(entity);
    }

    // Update a component system from a given type given an update function and arguments
    template<typename... Components, typename UpdateFunction, typename... Args>
    void Update(UpdateFunction&& updateFunction, Args&&... args)
    {
        //        PRO_CATEGORY("Space::Update", Profiler::Category::Scene);
        std::vector<Archetype*>& archetypes = Internal::Query<Components...>::Get()->GetArchetypes(spaceId);

        // TODO: Build an iterator (?)
        for (Archetype* archetype : archetypes)
            archetype->GetPool().Update<Components...>(
                std::forward<UpdateFunction>(updateFunction),
                std::forward<Args>(args)...);
    }

    static void RegisterQuery(Internal::QueryInterface* queryInterface)
    {
        for (Space* space : spaces)
            space->archetypeMap.RegisterQuery(queryInterface);
    }

    static size_t GetCurrentSpaceCount()
    {
        return spaceCounter.GetCurrentCount();
    }

    static std::vector<Space*>& GetSpaces()
    {
        return spaces;
    }

#ifdef ALLOY_EXPOSE_INTERNALS
    [[nodiscard]] size_t GetSpaceId() const
    {
        return spaceId;
    }

    [[nodiscard]] const Internal::ArchetypeMap& GetArchetypeMap() const
    {
        return archetypeMap;
    }

    [[nodiscard]] const Internal::EntityManager& GetEntityManager() const
    {
        return entityManager;
    }

    [[nodiscard]] bool HasComponentById(const Entity entity, size_t componentId)
    {
        return entityManager.HasComponentById(entity, componentId);
    }
#endif

private:
    size_t spaceId;
    Internal::ArchetypeMap archetypeMap;
    Internal::EntityManager entityManager;

    inline static RecycledCounter<size_t> spaceCounter;
    inline static std::vector<Space*> spaces;
#ifdef ALLOY_DEFER_DESTRUCTION
    std::vector<X::Entity> toDestroy{};
#endif
};

} // namespace X
