#pragma once

#include "Archetype.h"
#include "BitSetMap.h"
#include "ComponentTypeTraits.h"
#include "Containers/BitSet.h"
#include "QueryInterface.h"

#include <vector>

namespace X
{
class Space;
}

namespace X::Internal
{

template<typename... Components>
class Query final : public QueryInterface
{
public:
    void RegisterArchetype(Space* spacePtr, size_t spaceId, Archetype* archetype) final
    {
        this->GetArchetypes(spaceId).emplace_back(archetype);
    }

    void UnregisterSpace(size_t parentSpace) final
    {
        this->GetArchetypes(parentSpace).clear();
    }

    // Make sure there's room for the given space
    void AssureSpaceSize(size_t spaceId) final
    {
        if (spaceId >= spaceArchetypeMap.size())
            spaceArchetypeMap.resize(spaceId + 1, std::vector<Archetype*>{});
    }

    [[nodiscard]] std::vector<Archetype*>& GetArchetypes(size_t spaceId) final
    {
        return spaceArchetypeMap[spaceId];
    }

    [[nodiscard]] static Query* Get()
    {
        static Query query;
        return &query;
    }

private:
    Query();

    ~Query() final = default;

    // Space ID -> Archetypes Vector
    std::vector<std::vector<Archetype*>> spaceArchetypeMap;
};

} // namespace X::Internal
