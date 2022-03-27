// -----------------------------------------------------------------------------
// \Project Name    GAM400 - SwitchBlade
// \filename        AlloyDebug.h
// \author          Calin Gavriliuc
// \date            11/9/21
// \brief           Contains debugging tools for alloy
//
// Copyright � 2019 DigiPen, All rights reserved.
// -----------------------------------------------------------------------------

#pragma once

#include "Entity.h"
#include "Space.h"

// TODO: Make this either a string return or a data structure??
// NOTE: These WILL get optimized out if not called and won't be available to GDB/LLDB if not called at some point
namespace X
{
inline void GetSpaceInfo(X::Space& space, bool verbose)
{
    std::string tab{ "  " };
    std::cout << "=====" << std::endl;
    std::cout << "SPACE:" << std::endl;
#if defined(ALLOY_DEBUG)
    std::cout << tab << "SIZE: " << space.GetEntityManager().GetRecycledCounter().GetExistingIds().size() << std::endl;
#endif

    auto& archetypes = space.GetArchetypeMap().GetArchetypes();
    for (auto archetype : archetypes)
    {
        std::cout << tab << "ARCHETYPE:" << std::endl;
        std::cout << tab << tab << "MASK:" << std::endl;
        auto& mask = archetype->GetComponentMask();
        bool foundMask = false;
        for (int j = 0; j < X::Internal::ComponentTypeTraits::CurrentCount(); ++j)
        {
            if (mask[j] || verbose)
            {
                foundMask = true;
                // @calin TODO: Print other info from GetComponentInfo
                std::cout << tab << tab << tab << (mask[j] ? "TRUE " : "FALSE") << " : " << X::Internal::ComponentTypeTraits::GetComponentInfo()[j].name << std::endl;
            }
        }

        if (!foundMask)
            std::cout << tab << tab << tab << "DEFAULT MASK (NONE)" << std::endl;

        X::Internal::Pool& pool = archetype->GetPool();
        std::cout << tab << tab << "POOL: " << std::endl;
        std::cout << tab << tab << tab << "SIZE: " << pool.GetSize() << std::endl;
        std::cout << tab << tab << tab << "CAPACITY: " << pool.GetCapacity() << std::endl;
    }
    std::cout << "=====" << std::endl;
}

inline void GetEntityInfo(X::Space& space, const X::Entity entity, bool verbose = false)
{
    std::string tab{ "  " };
    std::cout << "=====" << std::endl;
    std::cout << "ENTITY: " << std::endl;
    std::cout << tab << "ID: " << entity.GetId() << std::endl;
    std::cout << tab << "GENERATION: " << entity.GetGeneration() << std::endl;

    std::cout << "SPACE: " << std::endl;
    std::cout << tab << "ENTITY VALIDITY: ";
    if (space.IsValid(entity))
    {
        std::cout << "VALID"
                  << std::endl;
        std::vector<std::string> existingComponents{};
        std::vector<std::string> missingComponents{};
        for (int i = 0; i < X::Internal::ComponentTypeTraits::CurrentCount(); ++i)
        {
            if (space.HasComponentById(entity, i))
                existingComponents.emplace_back(X::Internal::ComponentTypeTraits::GetComponentInfo()[i].name);
            else if (verbose)
                missingComponents.emplace_back(X::Internal::ComponentTypeTraits::GetComponentInfo()[i].name);
        }
        std::cout << tab << "EXISTING COMPONENTS:";
        if (existingComponents.empty())
            std::cout << "NONE";
        std::cout << std::endl;
        for (auto& existingComponent : existingComponents)
            std::cout << tab << tab << existingComponent << std::endl;
        if (verbose)
        {
            std::cout << tab << "MISSING COMPONENTS:";
            if (missingComponents.empty())
                std::cout << "NONE";
            std::cout << std::endl;
            for (auto& missingComponent : missingComponents)
                std::cout << tab << tab << missingComponent << std::endl;
        }
    }
    else
    {
        std::cout << "INVALID [!]"
                  << std::endl;
        bool foundValid = false;
        for (X::Space* space : X::Space::GetSpaces())
        {
            if (space->IsValid(entity))
            {
                std::cout << tab << tab << "NOTE: ENTITY IS VALID FOR A DIFFERENT SPACE [?]"
                          << std::endl;
                foundValid = true;
            }
        }

        if (!foundValid)
            std::cout << tab << tab << "NOTE: ENTITY IS NOT VALID FOR ANY SPACE [!]"
                      << std::endl;
    }

    std::cout << "=====" << std::endl;
}

inline void PrintDotInfo()
{
    for (Space* space : Space::GetSpaces())
    {
        std::cout << "====" << std::endl;
        std::cout << "Space: " << space->GetSpaceId() << std::endl;
        for (Archetype* archetype : space->GetArchetypeMap().GetArchetypes())
        {
            std::cout << "(\"";
            for (::Internal::BitSetValue bitSetValue : archetype->GetComponentMask().GetRawData())
                std::cout << bitSetValue;
            std::cout << "\", ";
            std::cout << "(";
            auto& forw = archetype->GetForwardArchetypes();
            for (int i = 0; i < forw.size(); ++i)
            {
                auto& forward = forw[i];
                if (forward != nullptr)
                {
                    std::cout << "(\"";
                    std::cout << X::Internal::ComponentTypeTraits::GetComponentInfo()[i].name;
                    std::cout << "\",\"";
                    for (::Internal::BitSetValue bitSetValue : forward->GetComponentMask().GetRawData())
                        std::cout << bitSetValue;
                    std::cout << "\"),";
                }
            }
            std::cout << "), ";
            std::cout << "(";
            auto& back = archetype->GetBackwardArchetypes();
            for (int i = 0; i < back.size(); ++i)
            {
                auto& backward = back[i];
                if (backward != nullptr)
                {
                    std::cout << "(\"";
                    std::cout << X::Internal::ComponentTypeTraits::GetComponentInfo()[i].name;
                    std::cout << "\",\"";
                    for (::Internal::BitSetValue bitSetValue : backward->GetComponentMask().GetRawData())
                        std::cout << bitSetValue;
                    std::cout << "\"),";
                }
            }
            std::cout << "))," << std::endl;
        }
        std::cout << "====" << std::endl;
    }
}

} // namespace X
