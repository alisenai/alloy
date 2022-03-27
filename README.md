# Alloy
Alloy is an extremely fast ECS library providing a simple and clean API.

Pull requests and new issues welcome.

## Minimal Example
```c++
#include "Alloy/Alloy.h"

...

// Define a component
struct Component
{
    int var;
};

// Create a space for entities to live within
X::Space space{};

// Create a new entity
X::Entity entity = space.CreateEntity();

// Give the entity a Component that is constructed with { 1 }
space.EmplaceComponent<Component>(entity, 1);

// Update over all entities in the space that have Component
space.Update<Component>([](X::Entity entity, Component& component) {
    // Edit the component's values
    ++component.var;
});

// Remove Component from the entity
space.RemoveComponent<Component>(entity);

// Destroy the entity
space.DestroyEntity(entity);
```

## Quick Reference / Examples
### Creating a space:
```c++
X::Space space{};
```

### Creating an entity
```c++
X::Entity entity = space.CreateEntity();
```

### Emplacing a component
For aggregate types, Alloy constructs `Component1` in-place using an initializer list, such as `Component1{1ul, 1ul}`.

For non-aggregate types, Alloy constructs `Component1` in-place using the component's constructor, such as `Component1(1ul, 1ul)`.
```c++
space.EmplaceComponent<Component1>(entity, 1ul, 1ul);
```

### Inserting a component
Uses given, constructed, `Component2`.
```c++
space.EmplaceComponent<Component2>(entity, Component2{1ul, 1ul});
```

### Adding a component
Alloy constructs a default-constructed `Component3`, letting you edit the component within a given lambda.

Do not:
- Save the given reference to the added component. It will be invalidated eventually.
- Add components within the passed function or your reference will be invalidated.
- Remove components within the passed function or your reference will be invalidated.
- Destroy the entity within the passed function or your reference will be invalidated.
```c++
space.AddComponent<Component3>(entity, [](Component3& component3) {
    // Modify component3 here
});
```

### Removing a component
Component destructor will be called on removal if it is non-trivial.
```c++
space.RemoveComponent<Component1>(entity);
```

### Getting a component
Calls a given function, passing the requested component.

Do not:
- Save the given reference to the given component. It will be invalidated eventually.
- Add components within the passed function or your reference will be invalidated.
- Remove components within the passed function or your reference will be invalidated.
- Destroy the entity within the passed function or your reference will be invalidated.
```c++
space.GetComponent<Component1>(entity, [](Component1& component1) {
    // Modify and complete the construction of component1
});
```
Or, which is only valid until the space is used in any way, like so:
```c++
Component1& tempComponent1 = space.GetComponentTemporary<Component1>(entity);
```

### Updating over a space (A few examples)
Calls given function passing the entity and its components in the order the `Update`'s template parameters specifies.

Do not:
- Save the given reference to the given component. It will be invalidated eventually.
- Add components within the passed function.
- Remove components within the passed function.
- Destroy the entity within the passed function.

Update over one space, requesting one component:
```c++
space.Update<Component1>([](Component1& component1) {
    // Use component1 here
});
```

Update over one space, grabbing two components
```c++
space.Update<Component1, Component2>([](Component1& component1, Component2& component2) {
    // Use component1 and component2 here
});
```

Update over one space, requesting one component, passing one argument:
```c++
space.Update<Component1>([](Component1& component1, int i) {
    // Use component1 and i here
}, 1);
```

Update over all spaces, requesting one component:
```c++
X::Update<Component1>([](X::Space& space, Component1& component1) {
    // Use space, component1, and i here
});
```
