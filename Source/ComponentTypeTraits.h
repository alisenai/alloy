#pragma once

#include "Alloy/AlloyConfig.h"

namespace X::Internal
{
struct ComponentTypeTraits
{
    using DestructorCaller = void (*)(uint8_t* object);
    using TransferCaller = void (*)(uint8_t* from, uint8_t* to);
    using ComponentId = size_t;

    struct ComponentInfo
    {
        DestructorCaller destructorCaller;
        TransferCaller transferCaller;
        size_t size;
#ifdef ALLOY_EXPOSE_INTERNALS
        std::string name;
#endif
    };

    // To get the final count, this must ONLY be called during run time
    static ComponentId& CurrentCount()
    {
        return currentId;
    }

    static std::vector<ComponentInfo>& GetComponentInfo()
    {
        return componentInfo;
    }

    template<typename Component>
    static ComponentId RegisterComponent()
    {
#ifdef ALLOY_EXPOSE_INTERNALS
        std::string componentName = typeid(Component).name();
#endif
        size_t componentSize = sizeof(Component);
        DestructorCaller componentDestructorCaller;
        TransferCaller componentTransferCaller;

        //std::cout << "[ALLOY][" << typeid(Component).name() << "]";
        if constexpr (std::is_trivially_destructible_v<Component>)
        {
            //std::cout << "[Trivially destructible]";
            // No-op lambda to prevent nullptr checks
            componentDestructorCaller = +[](uint8_t* obj) {};
        }
        else
        {
            //std::cout << "[Requires destructor call]";
            // Call the destructor for the object
            componentDestructorCaller = +[](uint8_t* obj) {
                std::destroy_at(reinterpret_cast<Component*>(obj));
            };
        }

        if constexpr (std::is_copy_constructible_v<Component> || std::is_move_constructible_v<Component>)
        {
            //std::cout << "[Using std::move with copy constructor to copy]\n";
            componentTransferCaller = +[](uint8_t* from, uint8_t* to) {
                // Placement new with the copy construct the object from the old location to the new one
                new (reinterpret_cast<Component*>(to)) Component(std::move(*reinterpret_cast<Component*>(from)));
                // Call the destructor
                if constexpr (!std::is_trivially_destructible_v<Component>)
                    std::destroy_at(reinterpret_cast<Component*>(from));
            };
        }
        else if constexpr (std::is_default_constructible_v<Component> && (std::is_copy_assignable_v<Component> || std::is_move_assignable_v<Component>) )
        {
            //std::cout << "[Using default construction and std::move with operator= to copy]\n";
            componentTransferCaller = +[](uint8_t* from, uint8_t* to) {
                // Construct new object at the new memory location (required to use operator=)
                new (reinterpret_cast<Component*>(to)) Component();
                // Use operator= to move the component
                (reinterpret_cast<Component*>(to))->operator=(std::move(*reinterpret_cast<Component*>(from)));
                // Call the destructor
                if constexpr (!std::is_trivially_destructible_v<Component>)
                    std::destroy_at(reinterpret_cast<Component*>(from));
            };
        }
        else
        {
            //std::cout << "[Using std::copy to copy]\n";
            componentTransferCaller = +[](uint8_t* from, uint8_t* to) {
                // Copy the object from the previous location to the new location
                std::copy(from, from + sizeof(Component), to);
                // Destructor call is not needed, and shouldn't be called, with a memcpy
            };
        }

        GetComponentInfo().emplace_back(ComponentInfo{
            componentDestructorCaller,
            componentTransferCaller,
            componentSize
#ifdef ALLOY_EXPOSE_INTERNALS
            ,
            componentName //
#endif
        });

        return CurrentCount()++;
    }

    inline static ComponentId currentId = 0;
    inline static std::vector<ComponentInfo> componentInfo{};
};

template<typename Component>
struct GetComponentId
{
    const static ComponentTypeTraits::ComponentId value;
};

template<typename Component>
const ComponentTypeTraits::ComponentId GetComponentId<Component>::value = ComponentTypeTraits::RegisterComponent<Component>();

} // namespace X::Internal
