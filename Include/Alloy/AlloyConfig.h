#pragma once

#include <cstdint>
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

// ==== Custom Setup ====
// Custom defines may be places here
// ======================

// ==== Alloy Deferring ====
// Just define: ALLOY_DEFER_DESTRUCTION
// =========================

// ==== Alloy Debugging ====
// Just define: ALLOY_DEBUG
// =========================

// ==== Alloy Expose Internal Members ====
// Just define: ALLOY_EXPOSE_INTERNALS
// =========================

// ==== How to allocate components ====
#define ALLOY_CHUNK_ALLOCATOR_MULTI_VECTOR_MODE
//#define ALLOY_CHUNK_ALLOCATOR_MULTI_DEQUE_MODE
// ====================================

// ==== How to store runtimeArchetypesMap ====
#define ALLOY_ARCHETYPES_TYPE_HEAT_VECTOR
// #define ALLOY_ARCHETYPES_TYPE_UNORDERED_MAP
// #define ALLOY_ARCHETYPES_TYPE_ORDERED_VECTOR
// #define ALLOY_ARCHETYPES_TYPE_UNORDERED_VECTOR
// =================================

// ==== Type to use for entity sparse sets ====
#define ALLOY_SPARSE_SET_TYPE_VECTOR
// vvv Is, generally, SIGNIFICANTLY slower
//#define ALLOY_SPARSE_SET_TYPE_DEQUE
// ============================================

#ifdef ALLOY_DEBUG
    #define ALLOY_ASSERT(condition, message)                                                                                              \
        do                                                                                                                                \
        {                                                                                                                                 \
            if (!(condition))                                                                                                             \
            {                                                                                                                             \
                std::cerr << "Assertion `" #condition "` failed in " << __FILE__ << " line " << __LINE__ << ": " << message << std::endl; \
                std::terminate();                                                                                                         \
            }                                                                                                                             \
        } while (false)
#else // Not ALLOY_DEBUG
    #define ALLOY_ASSERT(condition, message)
#endif // ALLOY_DEBUG

namespace X
{

#ifdef ALLOY_SPARSE_SET_TYPE_VECTOR
template<typename T>
using SparseSetContainer = std::vector<T>;
#elif defined(ALLOY_SPARSE_SET_TYPE_DEQUE)
template<typename T>
using SparseSetContainer = std::deque<T>;
#endif

//constexpr size_t KV_TO_B = 8192;
//constexpr size_t CHUNK_SIZE_KB = 16;

// ==== Forward declarations ====
template<typename ComponentType>
struct ComponentWrapper;
// ==============================

// ===== CONFIG =====
// Type to use for entity IDs
using IdType = uint32_t;
// Type used for entity generations
using GenerationType = uint32_t;
// Invalid entity ID
constexpr IdType INVALID_ENTITY_ID = std::numeric_limits<IdType>::max();
// Container to store Component(Wrapper)s in
template<typename ComponentType>
using ComponentStorage = std::deque<ComponentWrapper<ComponentType>>;
// Container to store [Entity -> Container] mappings in
using EntityStorage = std::deque<size_t>;
// Invalid component index
constexpr size_t INVALID_COMPONENT_INDEX = std::numeric_limits<size_t>::max();

namespace Internal
{

template<class T>
constexpr T ExponentBaseTwo(T exponent)
{
    return (T(1) << exponent);
}

// TODO: Tune these numbers
// How runtimeArchetypesMap are stored in the chunk allocator
inline const constexpr size_t ArchetypesPerChunkPower = 7; // 128
inline const constexpr size_t ArchetypesPerChunk = ExponentBaseTwo(ArchetypesPerChunkPower);

} // namespace Internal
// ==================
} // namespace X
