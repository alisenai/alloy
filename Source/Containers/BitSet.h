#pragma once

#include <vector>


namespace Internal
{
using BitSetValue = size_t;
using BitSetContainer = std::vector<BitSetValue>;
} // namespace Internal

class BitSet
{
public:
    static constexpr Internal::BitSetValue True = 1;
    static constexpr Internal::BitSetValue False = 0;

    struct BitSetHash
    {
        inline std::size_t operator()(BitSet const& bitSet) const noexcept;
    };

    explicit BitSet(size_t size) : bitSet(size, False)
    {
        RecomputeHash();
    }

    BitSet(const BitSet& other)
    {
        *this = other;
    }

    [[nodiscard]] const Internal::BitSetValue& operator[](size_t index) const
    {
        return bitSet[index];
    }

    [[nodiscard]] bool operator==(const BitSet& rhs) const
    {
        return bitSet == rhs.bitSet;
    }

    [[nodiscard]] bool operator!=(const BitSet& rhs) const
    {
        return !(rhs == *this);
    }

    [[nodiscard]] size_t GetSize()
    {
        return bitSet.size();
    }

    [[nodiscard]] const Internal::BitSetContainer& GetRawData() const
    {
        return bitSet;
    }

    void SetNoRecompute(const size_t index, const Internal::BitSetValue value)
    {
        bitSet[index] = value;
    }

    void RecomputeHash()
    {
        std::hash<Internal::BitSetValue> hasher;
        hash = 0;
        for (Internal::BitSetValue i : bitSet) hash ^= hasher(i) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    void Set(size_t index, Internal::BitSetValue value)
    {
        SetNoRecompute(index, value);
        RecomputeHash();
    }

private:
    Internal::BitSetContainer bitSet;
    size_t hash{ 0 };
};


inline std::size_t BitSet::BitSetHash::operator()(const BitSet& bitSet) const noexcept
{
    return bitSet.hash;
}
