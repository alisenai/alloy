#pragma once
#include "Entity.h"
#include "Space.h"

namespace X
{
class ComponentWrapperGeneric
{
public:
    ComponentWrapperGeneric(X::Space& Space) : space(&Space) // constructs invalid wrapper
    {}

    ComponentWrapperGeneric(X::Space& Space, X::Entity Entity) : space(&Space), entity(Entity)
    {}

    ComponentWrapperGeneric(X::Space* Space, X::Entity Entity) : space(Space), entity(Entity)
    {}

    template<typename Component>
    [[nodiscard]] Component& GetComponent() const
    {
        return space->GetComponentTemporary<Component>(entity);
    }

    template<typename Component>
    [[nodiscard]] bool HasComponent() const
    {
        return space->HasComponent<Component>(entity);
    }

    [[nodiscard]] Space& GetSpace() const
    {
        return *space;
    }

    [[nodiscard]] Entity GetEntity() const
    {
        return entity;
    }

    [[nodiscard]] bool IsValid() const
    {
        return space->IsValid(entity);
    }

    void Invalidate()
    {
        SetEntity(X::Entity());
    }

    void SetEntity(X::Entity newEntity)
    {
        entity = newEntity;
    }

    void DestroyEntity()
    {
        space->DestroyEntity(entity);
    }

protected:
    X::Space* space{ nullptr };
    X::Entity entity{};
};

template<typename Component>
class ComponentWrapper : public ComponentWrapperGeneric
{
public:
    ComponentWrapper(X::Space& space, X::Entity entity) : ComponentWrapperGeneric(space, entity)
    {}

    ComponentWrapper(X::Space* space, X::Entity entity) : ComponentWrapperGeneric(space, entity)
    {}

    ComponentWrapper(ComponentWrapperGeneric& generic) : ComponentWrapperGeneric(generic)
    {}

    [[nodiscard]] Component* operator->() const
    {
        return &space->GetComponentTemporary<Component>(entity);
    }

    [[nodiscard]] Component& GetComponent() const
    {
        return space->GetComponentTemporary<Component>(entity);
    }

    [[nodiscard]] bool HasComponent() const
    {
        return space->HasComponent<Component>(entity);
    }
};
} // namespace X
