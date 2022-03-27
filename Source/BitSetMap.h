#pragma once

namespace X::Internal
{

// Maps Components -> BitSet
template<typename... Components>
class BitSetMap
{
public:
    [[nodiscard]] static const BitSet& GetComponentMask()
    {
        return Get().componentMask;
    }

    [[nodiscard]] static BitSetMap& Get()
    {
        static BitSetMap componentsToMask;
        return componentsToMask;
    }

private:
    BitSetMap() : componentMask(Internal::ComponentTypeTraits::CurrentCount())
    {
        ((void) componentMask.SetNoRecompute(Internal::GetComponentId<Components>::value, BitSet::True), ...);
        componentMask.RecomputeHash();
    }

    BitSet componentMask;
};

} // namespace X::Internal
