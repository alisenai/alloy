#include "Query.h"
#include "Space.h"

template<typename... Components>
X::Internal::Query<Components...>::Query() : QueryInterface(BitSetMap<Components...>::Get().GetComponentMask())
{
    AssureSpaceSize(Space::GetCurrentSpaceCount());
    Space::RegisterQuery(this);
}
