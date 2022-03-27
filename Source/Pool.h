#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

#include "Alloy/AlloyConfig.h"
#include "ComponentTypeTraits.h"
#include "Entity.h"

// TODO: @calin move entities to the end of the pool's vector list for perf?
// TODO: @calin merge Pool.h and ChunkAllocator.h
// TODO: Track end pointer for setting something at the back would be MUCH faster
// See X::Internal::Pool::GetNext

namespace X::Internal
{
class Pool
{
public:
#if defined(ALLOY_CHUNK_ALLOCATOR_MULTI_DEQUE_MODE)
    using DataType = std::vector<std::vector<uint8_t>>;
    using ChunkAllocatorType = std::vector<DataType>;
    static constexpr size_t StartCapacity = 0;
#elif defined(ALLOY_CHUNK_ALLOCATOR_MULTI_VECTOR_MODE)
    using DataType = uint8_t*;
    using DataTypeNoPtr = uint8_t;
    using ChunkAllocatorType = std::vector<DataType>;
    static constexpr size_t StartCapacity = 8;
#endif
    using IndexType = size_t;

    Pool() : archetypeSize(sizeof(Entity)), capacity(StartCapacity), componentIds{ GetComponentId<Entity>::value }
    {
        RawDataConstructHelper();
    }

    void RawDataConstructHelper()
    {
        rawData = ChunkAllocatorType{ X::Internal::ComponentTypeTraits::CurrentCount() };
#if defined(ALLOY_CHUNK_ALLOCATOR_MULTI_VECTOR_MODE)
        // Using the "this" pointer because the parameter was moved
        for (X::Internal::ComponentTypeTraits::ComponentId componentId : componentIds)
            rawData[componentId] = new DataTypeNoPtr[StartCapacity * X::Internal::ComponentTypeTraits::GetComponentInfo()[componentId].size];
#endif
    }

    void RawDataDestructHelper()
    {
        // @calin TODO: undo this change
        // Call component destructors
        for (X::Internal::ComponentTypeTraits::ComponentId componentId : componentIds)
        {
            X::Internal::ComponentTypeTraits::ComponentInfo& componentInfo = X::Internal::ComponentTypeTraits::GetComponentInfo()[componentId];
            DataType& componentIdVector = rawData[componentId];
            for (int j = 0; j < size; ++j)
                componentInfo.destructorCaller(&componentIdVector[componentInfo.size * j]);
            delete[] rawData[componentId];
        }
    }

    ~Pool()
    {
        RawDataDestructHelper();
    }

    IndexType GetNextIndex(const Entity indexParent)
    {
        // Grow, if needed
        if (size == capacity)
        {
            for (X::Internal::ComponentTypeTraits::ComponentId componentId : componentIds)
            {
                X::Internal::ComponentTypeTraits::ComponentInfo& componentInfo = X::Internal::ComponentTypeTraits::GetComponentInfo()[componentId];
                // Allocate double-sized buffer
                auto newRawData = new DataTypeNoPtr[capacity * 2 * componentInfo.size];
                // Copy everything to the new buffer
                for (int j = 0; j < size; ++j)
                {
                    unsigned offset = j * componentInfo.size;
                    componentInfo.transferCaller(rawData[componentId] + offset, newRawData + offset);
                }
                delete[] rawData[componentId];
                rawData[componentId] = newRawData;
            }
            capacity *= 2;
        }

        X::Internal::ComponentTypeTraits::ComponentId entityComponentId = X::Internal::GetComponentId<X::Entity>::value;
        *reinterpret_cast<X::Entity*>(&rawData[entityComponentId][sizeof(X::Entity) * size]) = indexParent;
        return size++;
    }

    [[nodiscard]] Entity GetParent(const IndexType index) const
    {
        return *reinterpret_cast<const Entity*>(Get(GetComponentId<Entity>::value, index));
    }

    void CopyComponentToPool(const IndexType index, const size_t componentId, uint8_t* component)
    {
        // Not worth branching for a "speed copy"
        ComponentTypeTraits::GetComponentInfo()[componentId].transferCaller(
            component,
            Get(componentId, index) //
        );
    }

    template<typename Component, typename... Args>
    Component& EmplaceComponent(const IndexType index, const size_t componentId, Args&&... args)
    {
        if constexpr (std::is_aggregate_v<Component>)
        {
            // SSO (Small String Optimizations) cannot be trivially disabled and will cause strings to point to stack memory
            // which will only be valid until removed from the stack and thus should not be put into ECS.
            // Use a vector<uint8_t>, const uint8_t *, or a custom string class instead.
            // static_assert(!(std::is_same_v<std::string, Args> || ...), "Small String Optimizations will cause invalid reads. Please do not hold strings in your ");
            return *(new (Get(componentId, index)) Component{ std::forward<Args>(args)... });
        }
        else
        {
            return *(new (Get(componentId, index)) Component(std::forward<Args>(args)...));
        }
    }

    template<typename Component>
    void InsertComponent(const IndexType index, const size_t componentId, const Component& component)
    {
        *reinterpret_cast<Component*>(Get(componentId, index)) = component;
    }

    template<bool Destroy>
    void PopBack()
    {
        --size;
        if constexpr (Destroy)
        {
            // Call component destructors
            for (X::Internal::ComponentTypeTraits::ComponentId componentId : componentIds)
            {
                X::Internal::ComponentTypeTraits::ComponentInfo& componentInfo = X::Internal::ComponentTypeTraits::GetComponentInfo()[componentId];
                DataType& componentTypeVector = rawData[componentId];
                componentInfo.destructorCaller(&componentTypeVector[componentInfo.size * size]);
            }
        }
    }

    [[nodiscard]] size_t GetSize() const
    {
        return size;
    }

    [[nodiscard]] size_t GetCapacity() const
    {
        return capacity;
    }

#ifdef ALLOY_DEBUG

    [[nodiscard]] size_t GetArchetypeSize() const
    {
        return archetypeSize;
    }

#endif

    [[nodiscard]] uint8_t* GetComponentRaw(const IndexType index, const size_t componentId)
    {
        return Get(componentId, index);
    }

    template<typename Component>
    [[nodiscard]] Component& GetComponent(const IndexType index)
    {
        return *reinterpret_cast<Component*>(
            Get(GetComponentId<Component>::value, index) //
        );
    }

    // Moves the last component into the given index, returning the affected index's parent
    template<bool Destroy>
    unsigned int CopyLastIn(const IndexType index)
    {
        // Move the last objects in
        for (ComponentTypeTraits::ComponentId componentId : componentIds)
        {
            uint8_t* componentAtIndex = Get(componentId, index);
            uint8_t* lastComponent = Get(componentId, size - 1);
            ComponentTypeTraits::GetComponentInfo()[componentId].transferCaller(lastComponent, componentAtIndex);
        }
        return GetParent(index);
    }

    template<typename Component>
    static void CreateIncreasedPool(const Pool* oldPool, Pool* newPool)
    {
        // Increase the archetype size stored
        newPool->archetypeSize = oldPool->archetypeSize + sizeof(Component);

        // Copy the old component data
        newPool->componentIds = oldPool->componentIds;

        // Add a record for the new component data
        newPool->componentIds.emplace_back(GetComponentId<Component>::value);

        // Create a new chunk allocator with the correct sizes
        newPool->RawDataDestructHelper();
        newPool->RawDataConstructHelper();
    }

    template<typename Component>
    static void CreateDecreasedPool(const Pool* oldPool, Pool* newPool)
    {
        // Decrease the archetype size stored
        newPool->archetypeSize = oldPool->archetypeSize - sizeof(Component);

        // Copy the old component data
        newPool->componentIds = oldPool->componentIds;

        // Remove size info about the component
        auto& vec = newPool->componentIds;
        // TODO: make this faster?
        vec.erase(std::remove(vec.begin(), vec.end(), GetComponentId<Component>::value), vec.end());

        // Create a new chunk allocator with the correct sizes
        newPool->RawDataDestructHelper();
        newPool->RawDataConstructHelper();
    }

#if defined(ALLOY_DEBUG) || defined(ALLOY_EXPOSE_INTERNALS)

    [[nodiscard]] const auto& GetComponentData() const
    {
        return componentIds;
    }

#endif

    template<typename... Components, size_t... Is, size_t N, typename UpdateFunction, typename... Args>
    void UpdateHelper(size_t loops, std::index_sequence<Is...>,
                      const std::array<DataType, N>& vectors,
                      DataType entityVector,
                      UpdateFunction&& updateFunction, Args&&... args)
    {
        for (size_t i = 0; i < loops; ++i)
        {
            updateFunction(
                reinterpret_cast<Entity*>(entityVector)[i],
                (reinterpret_cast<Components*>(vectors[Is])[i])...,
                std::forward<Args>(args)...);
        }
    }

    template<typename... Components, typename UpdateFunction, typename... Args>
    void Update(UpdateFunction&& updateFunction, Args&&... args)
    {
        UpdateHelper<Components...>(
            size,
            std::index_sequence_for<Components...>{},
            std::array<DataType, sizeof...(Components)>{ rawData[GetComponentId<Components>::value]... },
            rawData[GetComponentId<Entity>::value],
            std::forward<UpdateFunction>(updateFunction),
            std::forward<Args>(args)...);
    }

    std::vector<ComponentTypeTraits::ComponentId>& GetComponentIds()
    {
        return componentIds;
    }

    // Returns a pointer to data with the size of dataSize
    [[nodiscard]] uint8_t* Get(const X::Internal::ComponentTypeTraits::ComponentId dataIndex, const size_t index)
    {
        size_t componentSize = X::Internal::ComponentTypeTraits::GetComponentInfo()[dataIndex].size;
        ALLOY_ASSERT(componentSize, "Allocator does not hold the given data index.");
        ALLOY_ASSERT(size > index, "Allocator does not hold the given index.");
        return &rawData[dataIndex][index * componentSize];
    }

    // Returns a pointer to data with the size of dataSize
    [[nodiscard]] const uint8_t* Get(const X::Internal::ComponentTypeTraits::ComponentId dataIndex, const size_t index) const
    {
        size_t componentSize = X::Internal::ComponentTypeTraits::GetComponentInfo()[dataIndex].size;
        ALLOY_ASSERT(componentSize, "Allocator does not hold the given data index.");
        ALLOY_ASSERT(size > index, "Allocator does not hold the given index.");
        return &rawData[dataIndex][index * componentSize];
    }

private:
    // Component IDs
    std::vector<ComponentTypeTraits::ComponentId> componentIds;
    // Size of the archetype stored (includes entity ID)
    IndexType archetypeSize;
    // Component data
    size_t size{ 0 };
    size_t capacity{ 0 };
    ChunkAllocatorType rawData{};
};

} // namespace X::Internal
