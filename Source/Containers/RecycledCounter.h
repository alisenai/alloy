#pragma once

#include <stack>
// CounterType must be convertable to T
template<typename T>
class RecycledCounter
{
public:
    [[nodiscard]] T GetNextId()
    {
        T nextId;
        // Try to pop a recycled ID
        if (!freeIds.empty())
        {
            nextId = freeIds.top();
            freeIds.pop();
        }
        else // No recycled IDs available, create a new one
        {
            nextId = currentId;
            currentId++;
        }

#if defined(ALLOY_DEBUG)
        existingIds.emplace(nextId);
#endif

        return nextId;
    }

    void RecycleId(T id)
    {
        // Add to the free list
        freeIds.emplace(id);
#if defined(ALLOY_DEBUG)
        // Remove from the live list
        existingIds.erase(id);
#endif
    }

    // Current ID is always the next one to give
    // Thus it is returning the count
    [[nodiscard]] T GetCurrentCount() const
    {
        return currentId;
    }

#if defined(ALLOY_DEBUG)
    [[nodiscard]] const std::unordered_set<T>& GetExistingIds() const
    {
        return existingIds;
    }
#endif

#ifdef ALLOY_EXPOSE_INTERNALS
    // Returns all the free entity IDs
    [[nodiscard]] const std::stack<T>& GetFreeIds() const
    {
        return freeIds;
    }

#endif

private:
    // If there are no recycled entities, used to create a new, unique, id
    T currentId{};
    // Entity recycling
    std::stack<T> freeIds{};
#if defined(ALLOY_DEBUG)
    // Tracking existing Ids
    std::unordered_set<T> existingIds{};
#endif
};
