// Alloy ECS
// By: Calin Gavriliuc
//
// Notes:
// - Prefer 'GetComponents' over multiple 'GetComponent' calls
//
// @calin TODO:
// - Future functions
//   - Add multiple components at a time (fast version)
//   - Debug check that a user isn't adding/removing components during an update

#pragma once

#include "../Source/ArchetypeMap.h"
#include "../Source/ComponentWrapper.h"
#include "../Source/Entity.h"
#include "../Source/EntityManager.h"
#include "../Source/Query.h"
#include "../Source/Space.h"
#include "../Source/SpaceQueryCoupler.h"
#include "AlloyConfig.h"
#if defined(ALLOY_EXPOSE_INTERNALS)
    #include "AlloyDebug.h"
#endif

namespace X
{
namespace Internal
{
template<class F>
auto BindSpace(F&& f, Space& space)
{
    return [f = std::forward<F>(f), &space](auto&&... args) {
        return f(space, decltype(args)(args)...);
    };
}
} // namespace Internal

template<typename... Components, typename UpdateFunction, typename... Args>
void Update(UpdateFunction&& updateFunction, Args&&... args)
{
    for (Space* space : Space::GetSpaces())
        space->Update<Components...>(Internal::BindSpace<UpdateFunction>(std::forward<UpdateFunction>(updateFunction), *space), std::forward<Args>(args)...);
}

template<typename UpdateFunction, typename... Args>
void UpdateSpaces(UpdateFunction&& updateFunction, Args&&... args)
{
    for (Space* space : Space::GetSpaces())
        updateFunction(*space, std::forward<Args>(args)...);
}

#ifdef ALLOY_DEFER_DESTRUCTION
inline void ApplyDestructionQueues()
{
    X::UpdateSpaces([](X::Space& space) {
        space.ApplyDestructionQueue();
    });
}
#endif
} // namespace X
