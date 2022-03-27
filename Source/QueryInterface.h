#ifndef QUERY_INTERFACE_H
#define QUERY_INTERFACE_H

#pragma once

#include <utility>

#include "Archetype.h"
#include "Containers/BitSet.h"

namespace X
{
class Space;
}

namespace X::Internal
{

class QueryInterface
{
public:
    explicit QueryInterface(BitSet bitMaskIn) : bitMask(std::move(bitMaskIn))
    {
        queries.emplace_back(this);
    }

    // No need to remove "this" from "queries" as it's all static time
    virtual ~QueryInterface() = default;

    virtual void RegisterArchetype(Space* space, size_t parentSpace, Archetype* archetype) = 0;

    virtual void UnregisterSpace(size_t parentSpace) = 0;

    virtual void AssureSpaceSize(size_t spaceId) = 0;

    [[nodiscard]] virtual std::vector<Archetype*>& GetArchetypes(size_t spaceId) = 0;

    [[nodiscard]] const BitSet& GetBitMask() const
    {
        return bitMask;
    }

    [[nodiscard]] static const std::vector<QueryInterface*>& GetQueries()
    {
        return queries;
    }

    // Does NOT check if the bitsets are equal but if all "TRUE" from component mask equal true on the other
    // The reverse of the previous statement might not be true!
    [[nodiscard]] bool DoesBitSetMatch(const BitSet& other) const
    {
        ::Internal::BitSetContainer bitSetData = bitMask.GetRawData();
        for (int i = 0; i < bitSetData.size(); ++i)
            if (bitSetData[i] == BitSet::True && other[i] == BitSet::False)
                return false;
        return true;
    }

protected:
    BitSet bitMask;

    inline static std::vector<QueryInterface*> queries;
};
} // namespace X::Internal
#endif
