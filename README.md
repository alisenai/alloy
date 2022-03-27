# Alloy
Alloy is an extremely fast ECS library.

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
## Concept: Spaces
Fundamentally, an `X::Space` is just a grouping of entities. A space manages entities and the storage of components associated with them. It's also, in a way, a lifetime scope for those entities - the scope of the space is the scope for those entities.

The reason for this feature, versus global functions, is to allow separation of unrelated entities. For example, in the context of a game, there would be an engine space and a scene space.

Conceptually, an `X::Space` should be treated as an `std::unordered_set`. Don't assume iterators (component references) will always be valid. Don't remove items (destroy entities/components) while updating over the structure. Items (entities/components) are not in a guaranteed order.

### Using a Space
```c++
X::Space space{};
```
The destructor of a space will automatically destroy all associated entities and their components.

Main Space Functions:
- `CreateEntity` | Creates an entity
- `DestroyEntity` | Destroys an entity
- `Update` | Updates over all entities with given components
- `EmplaceComponent` | Adds a component to an entity using emplacement
- `InsertComponent` | Adds a component to an entity using insertion
- `GetComponent` | Calls given function with a reference to a component from the entity
- `GetComponentTemporary` | Returns a TEMPORARY component reference
- `HasComponent` | Returns if an entity has a component
- `RemoveComponent` | Removes a component from an entity
- `IsValid` | Returns if an entity is valid
- `~Space` | Destroys all entities in the space

### Space Performance
Spaces are somewhat cheap to construct, but constructing them should be done at a minimum. For reference:
```
Creating 10 Spaces  | ~6654 ns
Creating 10 Vectors |    ~6 ns
```
That is, creating an `X::Space` is over 1000x slower than creating a `std::vector`. This is not to mention the cost of destroying a space which comes with the cost of destroying all of its entities.

## Concept: Entities
An `X::Entity` is just an ID (and a generation) used as an index by its owning space.

Conceptually, an entity "holds" components which can be added, gotten, modified, and removed by using the entity as an index into an `X::Space`. See `Concept: Components` below.

### Using an X::Entity
```c++
// Create an entity to live in 'space'
X::Entity entity = space.CreateEntity();
// Destroy an entity
space.DestroyEntity(entity);
```

Main Entity Functions:
- `GetID` | Returns the entity's ID
- `operator==` and `operator!=` | Compares two entities

### Entity Performance
As an `X::Entity` is just some `uint`s, it is cheap to copy and can be stored. \
While not extremely cheap, creating entities can be done many thousands of times before it is even noticeable.
```
Creating 10 Entities | 349 ns
```

## Concept: Components
A component is a user-defined structure which is made up of aggregate, non-managed, data. These components can be defined anywhere and do _not_ need to be registered.

### Using Your Components
```c++
struct Component
{
    int var1;
    int var2;
};

// Forwards arguments to placement new for construction into pool
space.EmplaceComponent<Component>(entity, 1, 2);

// Return true/false if the entity has a component
space.HasComponent<Component>(entity); // True

// Removes given component from the given entity
// Calls ~Component(), if non-trivial
space.RemoveComponent<Component>(entity);

// Assigns given component to index into pool
space.InsertComponent<Component>(entity, Component{ 1, 2 });

// --- Remove 'Component' from 'entity' here ---

// Adds a default-constructed component to the given entity
// Calls given function passing the component for editing
space.AddComponent<Component>(entity, [](Component& component) {
    // Finish constructing the component here
});

// Calls a given function for a component from the given entity
space.GetComponent<Component>(entity, [](Component& component) {
    // Use component here
});

// Returns a temporary reference to the component
// Only valid until the parent space is used in any way
Component& tempComponent = space.GetComponentTemporary<Component>(entity);
```

### Component Performance
```
Emplace and Remove 1 Component |  ~138 ns
Insert and Remove 1 Component  |  ~139 ns
Get 1000 Components Off Entity | ~1558 ns
```
While adding and removing components is somewhat expensive, getting a component only takes ~1.5 nanoseconds and is extremely cheap.

### Notes
- Components are assumed to be aggregate types in most cases.
- Do _not_ hold onto references to components. The component storage system may invalidate them. You can re-grab components off the parent entity when needed.
- Try to keep components small, if possible.

## Concept: Systems (Updates)
A system in Alloy is just a function called on every requested entity - This action is called an update. Systems in Alloy are not limited to iterating over one component either.

### Using Updates
to use an updatee, call `Update` on an `X::Space` and specify the components you would like from entities in that space. The update _will run immediately_ where `Update` is called.

The following examples will use lambdas for simplicity and to show the many ways `Update` can be called, but nothing stops you from passing in function pointers or even member function pointers if you would like to create a system function/class.

```c++
// Iterate over all entities that have AT LEAST <Component1> in 'space'
space.Update<Component1>([](X::Entity entity, Component1& component1) {
    // Use entity and component here
});

// Iterate over all entities that have AT LEAST <Component1, Component2> in 'space'
space.Update<Component1, Component2>([](X::Entity entity, Component1& component1, Component2& component2) {
    // Use entity and component here
});

// Iterate over all entities that have AT LEAST <Component1> in 'space'
// Lambda will be passed 1.0f as the last parameter
space.Update<Component1>([](X::Entity entity, Component1& component1, float f) {
    // Use entity, component, and f here
}, 1.0f);

// Iterate over all entities that have AT LEAST <Component1> in 'space'
// Lambda will capture f, which can then be used
float f = 1.0f;;
space.Update<Component1>([f](X::Entity entity, Component1& component1) {
    // Use entity, component, and f here
}, 1.0f);
```

For more advanced usage, there is also the global update, `X::Update` which will iterate over _all_ spaces and not just a specified one.

```c++
X::Update<Component>([](X::Space& space, X::Entity entity, Component component) {
    // Use space, entity, and component here
});
```

### Updates Performance
```
Updating Over 1000 Entities Requesting One Component  | ~259 ns
Updating Over 1000 Entities Requesting Two Components | ~265 ns
```
Updating over 1000 entities and requesting one component is ~6x times faster than calling `GetComponent` on 1000 entities. \
It should also be noted that `GetComponent` incurs cost for each time it is called for getting a different component while `Update` does not incur a significant cost when requesting more components.

### Notes
- **As of currently, you cannot destroy an entity or remove a component from a space while calling `Update` on that space!**
- The entirety of Alloy was created to optimize `Update` calls and thus they are fast
- The order of the template component list does not matter for an update call as long as the component parameters are in the same order - you will get the same entities/components no matter the order
- Please use `const` on components you are not using

## Concept: Archetypes
// TODO :)

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

## How Alloy Works
### Why it's worth knowing
Answers questions such as:
- What is a space?
- Why can't you store component references?
- Why is updating so fast?
- Why is adding/removing components so slow?
- etc

### Rationale
The most minimal ECS-like structure is to have an array of components, using entities as indexes into those structures.

Consider:
```c++
struct Entity
{
    size_t vectorIndex;
};

size_t currentVectorSize{};
std::vector<bool> hasTransform{};
std::vector<Transform> transformComponents{};
std::vector<bool> hasPhysics{};
std::vector<Physics> physicsComponents{};
```
Great, you have basic ECS. You know which components your entity has and can add/remove components.

Obvious problems with this implementation:
- No use of templates.
- Have to keep all 4 vectors the same length.
- Have to keep the `bool` vectors updated.
- Invalidation of component references on vector grow.
- Components need to be "registered" somewhere

Consider attempting to update over all entities with a transform:
```c++
for (Entity entity from 0 to currentVectorSize):
    if (hasTransform(entity.vectorIndex)):
        transformComponents[entity.vectorIndex]; // Use the Transform component here
```

Why is this bad? What happens if you only have one entity with a transform?
```
                           v
Vector: <---|X|X|X|X|X|X|X|T|X|X|X|X|X|X|X|X|X|X|--->
```
Obviously, in the case of a transform this is unlikely but with some component types it is not. The issue is that you have to iterate over many entities (indexes) that _may or may not_ have the component. We can do better.

Sparse Sets:
- Sparse Set: An entity is used as an index to get the index of the component in the dense set
- Dense Set: Contains all the components in a packed form (no "holes") and the parent entity for that index (for updating the links between the sparse and dense set)

Example:
```
Index:        [E0,E1,E2,E3,E4,E5,E6,E7,E8,E9,-->]
Sparse Set:   [XX,02,00,XX,XX,XX,XX,01,XX,XX,-->]
Dense Set:    [T2,T7,T1,-->]
```

Pros:
- Don't need the `hasTransform` vector. Can just iterate over the dense set.
- No "holes" in the transform storage container and thus no wasted memory.

Cons:
- Long sparse set vector (can be paged/chunked!)
- Entities need to start at 0 (can be recycled!)
- Removing a transform component requires you to:
  - Remove the transform from the dense set and move the last index in (swap remove paradigm) to keep it dense
  - Update the sparse set to reflect the removed transform

Why this isn't the implementation - consider:
```
ECS::Update<Transform, Physics>(...)
```
Issues:
- You lose the cache-efficiency of your dense set because you iterate over `Transforms`, get their parents, check if the parents have a `Physics`, etc...
  - Gets much worse with more components than just `<Transform, Physics>`

There is no way to pack component vectors to remove "holes" - consider:
```
E0: [A0|  |C0]
E1: [A1|B1|  ]
E2: [  |B2|C2]
```

Solution: Archetypes.

What is an archetype? It is a given set of components. For example:
```
Given components A and B:
  Archetype 0: [ | ] -> Entities with no component
                        [  |  |  |--->]
  Archetype 1: [A| ] -> Entities with the A component
                        [A2|A1|A0|--->]
  Archetype 2: [ |B] -> Entities with the B component
                        [B3|B5|B4|--->]
  Archetype 3: [A|B] -> Entities with A and B components
                        [B6|B8|B7|--->]
                        [A6|A8|A7|--->]
...
```

With archetypes consider:
```
ECS::Update<A>(...)
```
It would update over all archetypes with _at least_ an `A` component - Archetypes 1 and 3.
```
ECS::Update<A, B>(...)
```
It would update over all archetypes with _at least_ an `A` and `B` component - Archetype 3.

Pros:
- These archetypes would store components using the sparse set paradigm explained previously and thus would be insanely cache efficient and would not require any indirection nor branching.

Cons:
- Need to move entities between archetypes.

Moving entities between archetypes is logically simple:
- If an archetype with the new component requirements doesn't exist:
  - Create it.
- Transfer from the old archetype to the new one.
  - Remove from current archetype.
  - Add to new archetype.

### Defined Archetypes
In the previous section runtime archetypes were discussed. Defined archetypes are the same except they have a user-specified component list.

### Spaces
A space is just a collection of archetypes. That's it :)

### Answering Common Questions
Questions:
- What is a space?
  - A collection of archetypes.
- Why can't you store component references?
  - Adding or removing a component moves it to a new archetype and thus invalidates it.
  - When using the swap & pop paradigm with sparse sets, a component may be swapped into the place of a removed component.
- Why is updating so fast?
  - For every archetype that matches the query, for every component...
- Why is adding/removing components so slow?
  - If an archetype doesn't exist, it has to be created.
  - The entity's components have to be copied to the new archetype.
  - The entity's components have to be removed from the old archetypes with a new entity's components copied into its old spot.



## Tips
- **As of currently, you cannot destroy an entity or remove a component from a space while calling `Update` on that space!**
- Entities are cheap to copy.
- Don't store component pointers/references.
- The more spaces, the slower global updates are.
- `Update`s are quite cheap as all of Alloy was built to make `Update`s fast.
- If you have a large vector of entities, destroying them in reverse order is a bit faster.
  - Consider `std::vector`'s `remove(index)` vs `pop_back()`.
