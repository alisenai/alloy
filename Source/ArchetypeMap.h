#pragma once

#include <algorithm>
#include <unordered_map>

#include "Alloy/AlloyConfig.h"
#include "Archetype.h"
#include "Containers/HeatVector.h"
#include "Entity.h"
#include "QueryInterface.h"
#include "Utilities.h"

namespace X
{
class Space;
}

namespace X::Internal
{

#ifdef ALLOY_ARCHETYPES_TYPE_ORDERED_VECTOR
template<class Other>
using BitSetPair = std::pair<BitSet, Other>;

struct BitSetPairHash
{
    template<class Other>
    inline std::size_t operator()(const BitSetPair<Other>& pair) const
    {
        return BitSet::BitSetHash()(pair.first);
    }
};

bool CompareBitSet(const BitSet& a, const BitSet& b)
{
    return BitSet::BitSetHash()(a) > BitSet::BitSetHash()(b);
}

template<class Other>
bool CompareBitSetPair(const BitSetPair<Other>& a, const BitSetPair<Other>& b)
{
    return BitSet::BitSetHash()(a.first) > BitSet::BitSetHash()(b.first);
}

template<class Other>
bool CompareBitSetPairEqual(const BitSetPair<Other>& a, const BitSetPair<Other>& b)
{
    return BitSet::BitSetHash()(a.first) == BitSet::BitSetHash()(b.first);
}

template<class Other>
struct CompareBitSetAndPair
{
    inline bool operator()(const BitSetPair<Other>& a, const BitSet& b)
    {
        return CompareBitSet(a.first, b);
    }

    inline bool operator()(const BitSet& a, const BitSetPair<Other>& b)
    {
        return CompareBitSet(a, b.first);
    }
};

template<typename T>
typename std::vector<T>::iterator
InsertSortedPair(std::vector<T>& vec, T const& item)
{
    return vec.insert(
        std::upper_bound(vec.begin(), vec.end(), item, CompareBitSetPair<Archetype*>),
        item);
}

using ArchetypeMapsCompareBitSetAndPair = CompareBitSetAndPair<Archetype*>;
using ArchetypesMapType = std::vector<BitSetPair<Archetype*>>;
#elif defined(ALLOY_ARCHETYPES_TYPE_UNORDERED_MAP)
using ArchetypesMapType = std::unordered_map<BitSet, Archetype*, BitSet::BitSetHash>;
#elif defined(ALLOY_ARCHETYPES_TYPE_UNORDERED_VECTOR)
template<class Other>
using BitSetPair = std::pair<BitSet, Other>;

template<class Other>
bool CompareBitSetPairEqual(const BitSetPair<Other>& a, const BitSetPair<Other>& b)
{
    return BitSet::BitSetHash()(a.first) == BitSet::BitSetHash()(b.first);
}

using ArchetypesMapType = std::vector<BitSetPair<Archetype*>>;
#elif defined(ALLOY_ARCHETYPES_TYPE_HEAT_VECTOR)
template<class Other>
using BitSetPair = std::pair<BitSet, Other>;

using ArchetypesMapType = HeatVector<BitSetPair<Archetype*>>;
#endif

class ArchetypeMap
{
    // Needs to be here to deduce auto :(
private:
    [[nodiscard]] auto FindArchetype(const BitSet& componentMask)
    {
#ifdef ALLOY_ARCHETYPES_TYPE_UNORDERED_MAP
        return ArchetypesMapType.find(componentMask);
#elif defined(ALLOY_ARCHETYPES_TYPE_ORDERED_VECTOR)
        auto val = std::lower_bound(
            ArchetypesMapType.begin(), ArchetypesMapType.end(), componentMask, ArchetypeMapsCompareBitSetAndPair() //
        );
        if (BitSet::BitSetHash()(val->first) == BitSet::BitSetHash()(componentMask))
            return val;
        return ArchetypesMapType.end();
#elif defined(ALLOY_ARCHETYPES_TYPE_UNORDERED_VECTOR)
        return std::find_if(
            ArchetypesMapType.begin(),
            ArchetypesMapType.end(),
            [&componentMask](const BitSetPair<Archetype*>& item) {
                return BitSet::BitSetHash()(item.first) == BitSet::BitSetHash()(componentMask);
            });
#elif defined(ALLOY_ARCHETYPES_TYPE_HEAT_VECTOR)
        return archetypesMap.Find([=](const auto& item) {
            return (BitSet::BitSetHash()(item.first) == BitSet::BitSetHash()(componentMask));
        });
#endif
    }

public:
    // Registers a base, no component, archetype
    explicit ArchetypeMap(Space* parentSpacePtr, size_t parentSpace)
        : parentSpacePtr(parentSpacePtr),
          parentSpaceId(parentSpace),
          baseArchetype(new Archetype())
    {
        RegisterRuntimeArchetype(baseArchetype);
    }

    // Deletes all existing ArchetypesMapType and queries
    ~ArchetypeMap()
    {
        archetypesMap.Each([](auto& pair) {
            delete pair.second;
        });
    }

    template<typename AddedComponent>
    [[nodiscard]] Archetype* GetForwardArchetype(Archetype* currentArchetype)
    {
        return GetArchetypeHelper<AddedComponent, true>(currentArchetype);
    }

    template<typename RemovedComponent>
    [[nodiscard]] Archetype* GetBackwardArchetype(Archetype* currentArchetype)
    {
        return GetArchetypeHelper<RemovedComponent, false>(currentArchetype);
    }

    void RegisterQuery(QueryInterface* query)
    {
        for (Archetype* archetype : archetypes)
            if (query->DoesBitSetMatch(archetype->GetComponentMask()))
                query->RegisterArchetype(parentSpacePtr, parentSpaceId, archetype);
    }

    [[nodiscard]] Archetype* GetBaseArchetype()
    {
        return baseArchetype;
    }

    void RegisterRuntimeArchetype(Archetype* archetype)
    {
#ifdef ALLOY_ARCHETYPES_TYPE_UNORDERED_MAP
        ArchetypesMapType.insert({ bitMask, archetype });
#elif defined(ALLOY_ARCHETYPES_TYPE_ORDERED_VECTOR)
        InsertSortedPair(ArchetypesMapType, { bitMask, archetype });
#elif defined(ALLOY_ARCHETYPES_TYPE_UNORDERED_VECTOR)
        ArchetypesMapType.emplace_back(std::make_pair(bitMask, archetype));
#elif defined(ALLOY_ARCHETYPES_TYPE_HEAT_VECTOR)
        archetypesMap.EmplaceBack(std::make_pair(archetype->GetComponentMask(), archetype));
#endif

        RegisterArchetypeHelper(archetype);
    }

#ifdef ALLOY_EXPOSE_INTERNALS
    [[nodiscard]] const std::vector<Archetype*>& GetArchetypes() const
    {
        return archetypes;
    }
#endif

private:
    template<typename Component, bool AddComponent>
    [[nodiscard]] Archetype* GetArchetypeHelper(Archetype* currentArchetype)
    {
        // Getting the forward archetype is a 3 step process

        Archetype** nextArchetype;
        if constexpr (AddComponent)
            nextArchetype = &currentArchetype->GetForwardArchetypes()[GetComponentId<Component>::value];
        else
            nextArchetype = &currentArchetype->GetBackwardArchetypes()[GetComponentId<Component>::value];

        // 1) Check if the next archetype is already assigned before searching ALL registered archetypes
        if (*nextArchetype != nullptr)
            return *nextArchetype;

        // 2) Try to search ALL registered archetypes before creating a new one
        BitSet componentMask = currentArchetype->GetComponentMask();
        if constexpr (AddComponent)
            componentMask.Set(GetComponentId<Component>::value, BitSet::True);
        else
            componentMask.Set(GetComponentId<Component>::value, BitSet::False);

        auto foundArchetype = FindArchetype(componentMask);

        // Found a suitable archetype
        if (foundArchetype != archetypesMap.End())
        {
            // Save it for faster lookup next time
            *nextArchetype = foundArchetype->second;
            return foundArchetype->second;
        }

        // 3) Create a new archetype, after trying to look for an existing one
        if constexpr (AddComponent)
            *nextArchetype = Archetype::AddComponentToArchetype<Component>(currentArchetype);
        else
            *nextArchetype = Archetype::RemoveComponentFromArchetype<Component>(currentArchetype);
        RegisterRuntimeArchetype(*nextArchetype);
        return *nextArchetype;
    }

    void RegisterArchetypeHelper(Archetype* archetype)
    {
        archetypes.emplace_back(archetype);
        for (QueryInterface* query : QueryInterface::GetQueries())
        {
            query->AssureSpaceSize(parentSpaceId);
            if (query->DoesBitSetMatch(archetype->GetComponentMask()))
                query->RegisterArchetype(parentSpacePtr, parentSpaceId, archetype);
        }
    }

    size_t parentSpaceId;
    Space* parentSpacePtr;
    std::vector<Archetype*> archetypes;
    Archetype* baseArchetype;
    ArchetypesMapType archetypesMap{};
};

} // namespace X::Internal
