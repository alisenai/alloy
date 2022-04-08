#pragma once
#include "Alloy/Alloy.h"
#include "benchmark/benchmark.h"
#include "gtest/gtest.h"
#include <vector>

#ifndef COMPONENT_COUNt_REF
    #define COMPONENT_COUNT_REF 1000
#endif
#ifndef COMPONENT_COUNt_REF
    #define LOOPS 1000
#endif

struct CFirst
{
    long unsigned e;
    long unsigned c;
};

struct CSecond
{
    long unsigned e;
    long unsigned c;
};

struct CThird
{
    long unsigned a;
    long unsigned b;
};

struct PositionComponent
{
    float x = 0.0f;
    float y = 0.0f;
};

struct DirectionComponent
{
    float x = 0.0f;
    float y = 0.0f;
};

struct ComflabulationComponent
{
    float thingy = 0.0;
    long unsigned dingy = 0;
    bool mingy = false;
};

TEST(Alloy, BasicComponentUsage)
{
    X::Space space1{};
    X::Entity e0 = space1.CreateEntity();
    ASSERT_TRUE(e0 == 0) << "Entity was not destroyed or ID was not recycled.";
    X::Entity e1 = space1.CreateEntity();
    ASSERT_TRUE(e1 == 1) << "Entity was not destroyed or ID was not recycled.";
    space1.EmplaceComponent<CFirst>(e0, CFirst{ 0, 1 });
    ASSERT_TRUE(space1.HasComponent<CFirst>(e0)) << "Component not added to correctly";
    space1.GetComponent<CFirst>(e0, [](CFirst& cFirst) { ALLOY_ASSERT(cFirst.c == 1, "Component data is incorrect"); });
    space1.EmplaceComponent<CSecond>(e0, 0ul, 2ul);
    ALLOY_ASSERT(space1.HasComponent<CSecond>(e0), "Component not added to correctly");
    space1.GetComponent<CSecond>(e0, [](CSecond& cSecond) { ALLOY_ASSERT(cSecond.c == 2, "Component data is incorrect"); });

    space1.EmplaceComponent<CFirst>(e1, 1ul, 1ul);
    ALLOY_ASSERT(space1.HasComponent<CFirst>(e1), "Component not added to correctly");
    space1.GetComponent<CFirst>(e1, [](CFirst& cFirst) { ALLOY_ASSERT(cFirst.c == 1, "Component data is incorrect"); });

    space1.RemoveComponent<CFirst>(e0);
    ASSERT_TRUE(space1.HasComponent<CFirst>(e0) == false) << "Component not removed correctly";
    space1.RemoveComponent<CSecond>(e0);
    ASSERT_TRUE(space1.HasComponent<CSecond>(e0) == false) << "Component not removed correctly";
    space1.RemoveComponent<CFirst>(e1);
    ASSERT_TRUE(space1.HasComponent<CFirst>(e1) == false) << "Component not removed correctly";
    space1.DestroyEntity(e0);
    space1.DestroyEntity(e1);
    space1.Update<CFirst>([](const auto e, auto& c) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });

    space1.Update<CSecond>([](const auto e, auto& c) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });

    space1.Update<>([](const auto e) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });
}

TEST(Alloy, UpdateSpaceTest)
{
    float dt = 1 / 60.0f;
    X::Space space1{};

    for (long unsigned _ = 0; _ < 2; ++_)
    {
        std::vector<X::Entity> entities1;
        entities1.reserve(COMPONENT_COUNT_REF);

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
            entities1.emplace_back(space1.CreateEntity());

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
            space1.EmplaceComponent<PositionComponent>(entities1[i], 0.0f, 0.0f);
        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
            space1.EmplaceComponent<DirectionComponent>(entities1[i], static_cast<float>(i), static_cast<float>(i));

        long unsigned count = 0;
        space1.Update<PositionComponent, DirectionComponent>(
            [&count](const auto parent, const auto& position, const auto& direction) { ++count; });
        ASSERT_TRUE(count == COMPONENT_COUNT_REF) << "Update count not met.";

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; i += 2)
            space1.RemoveComponent<PositionComponent>(entities1[i]);
        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; i += 2)
            space1.RemoveComponent<DirectionComponent>(entities1[i]);

        count = 0;
        space1.Update<PositionComponent, DirectionComponent>(
            [&count](const auto parent, const auto& position, const auto& direction) { ++count; });
        ASSERT_TRUE(count == COMPONENT_COUNT_REF / 2) << "Update count not met.";

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; i += 2)
            space1.DestroyEntity(entities1[i]);

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; i += 2)
            entities1[i] = space1.CreateEntity();

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; i += 2)
        {
            space1.EmplaceComponent<PositionComponent>(entities1[i], 0.0f, 0.0f);
            space1.EmplaceComponent<DirectionComponent>(entities1[i], static_cast<float>(i), static_cast<float>(i));
        }

        count = 0;
        space1.Update<PositionComponent, DirectionComponent>(
            [&count](const auto parent, const auto& position, const auto& direction) { ++count; });
        ASSERT_TRUE(count == COMPONENT_COUNT_REF) << "Update count not met.";

        std::vector<X::Entity> entities2;
        entities2.reserve(COMPONENT_COUNT_REF);

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i) entities2.emplace_back(space1.CreateEntity());

        count = 0;
        space1.Update<PositionComponent, DirectionComponent>(
            [&count](const auto parent, const auto& position, const auto& direction) { ++count; });
        ASSERT_TRUE(count == COMPONENT_COUNT_REF) << "Update count not met.";

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
            space1.EmplaceComponent<ComflabulationComponent>(entities2[i], 0.0f, i, true);

        for (long unsigned i = 0; i < LOOPS; ++i)
        {
            count = 0;
            space1.Update<PositionComponent, DirectionComponent>(
                [dt, &count](const auto parent, auto& position, const auto& direction) {
                    (void) parent;
                    position.x += direction.x * dt;
                    position.y += direction.y * dt;
                    ++count;
                });
            ASSERT_TRUE(count == COMPONENT_COUNT_REF) << "Update count not met.";

            count = 0;
            space1.Update<ComflabulationComponent>([&count](const auto parent, auto& comflab) {
                (void) parent;
                comflab.thingy *= 1.000001f;
                comflab.mingy = !comflab.mingy;
                comflab.dingy++;
                ++count;
            });
            ASSERT_TRUE(count == COMPONENT_COUNT_REF) << "Update count not met.";
        }

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
            space1.DestroyEntity(entities1[i]);

        for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
            space1.DestroyEntity(entities2[i]);
    }

    space1.Update<PositionComponent>([](const auto e, auto& c) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });

    space1.Update<DirectionComponent>([](const auto e, auto& c) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });

    space1.Update<ComflabulationComponent>([](const auto e, auto& c) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });

    space1.Update<>([](const auto e) {
        ASSERT_TRUE(true) << "All entities should be destroyed.";
    });
}

TEST(Alloy, TestComponentValidity)
{
    X::Space space1{};
    std::vector<X::Entity> entities;

    entities.reserve(COMPONENT_COUNT_REF);

    for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
        entities.emplace_back(space1.CreateEntity());

    for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
        space1.EmplaceComponent<CThird>(entities[i], 0ul, 0ul);
    long unsigned counter = 0;
    space1.Update<CThird>([&counter](const auto e, CThird& c) {
        c.a = counter;
        ++c.b;
        ++counter;
    });

    counter = 0;
    for (const auto entity : entities)
    {
        space1.GetComponent<CThird>(entity, [&counter](CThird& cThird) {
            ASSERT_TRUE(cThird.a == counter) << "Component data is incorrect.";
            ASSERT_TRUE(cThird.b == 1) << "Component data is incorrect.";
        });
        ++counter;
    }

    for (long unsigned i = 0; i < COMPONENT_COUNT_REF; ++i)
        space1.DestroyEntity(entities[i]);

    space1.Update<CThird>([](const auto e, auto& c) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });

    space1.Update<>([](const auto e) {
        ASSERT_TRUE(false) << "All entities should be destroyed.";
    });
}

static void Create10Spaces(benchmark::State& state)
{
    for (auto _ : state)
    {
        for (int i = 0; i < 10; ++i)
        {
            X::Space space1{};
            benchmark::DoNotOptimize(space1);
        }
    }
}
BENCHMARK(Create10Spaces);

static void Create10Entities(benchmark::State& state)
{
    X::Space space{};
    for (auto _ : state)
    {
        for (int i = 0; i < 10; ++i)
        {
            X::Entity e = space.CreateEntity();
            benchmark::DoNotOptimize(e);
        }
    }
}
BENCHMARK(Create10Entities);

static void Create10Vectors(benchmark::State& state)
{
    for (auto _ : state)
    {
        for (int i = 0; i < 10; ++i)
        {
            std::vector<int> vector{};
            benchmark::DoNotOptimize(vector);
        }
    }
}
BENCHMARK(Create10Vectors);


static void EmplaceRemoveComponent(benchmark::State& state)
{
    struct Component
    {
        int var;
    };

    X::Space space{};
    X::Entity entity = space.CreateEntity();
    for (auto _ : state)
    {
        space.EmplaceComponent<Component>(entity, 1);
        space.RemoveComponent<Component>(entity);
    }
}
BENCHMARK(EmplaceRemoveComponent);

static void InsertRemoveComponent(benchmark::State& state)
{
    struct Component
    {
        int var;
    };

    X::Space space{};
    X::Entity entity = space.CreateEntity();
    for (auto _ : state)
    {
        space.InsertComponent<Component>(entity, Component{ 1 });
        space.RemoveComponent<Component>(entity);
    }
}
BENCHMARK(InsertRemoveComponent);


static void Get1KComponent(benchmark::State& state)
{
    struct Component
    {
        int var;
    };

    X::Space space{};
    X::Entity entity = space.CreateEntity();
    space.EmplaceComponent<Component>(entity, 1);
    for (auto _ : state)
    {
        for (int i = 0; i < 1000; ++i)
        {
            space.GetComponent<Component>(entity, [](Component& component) { benchmark::DoNotOptimize(component); });
        }
    }
}
BENCHMARK(Get1KComponent);


static void Get1KComponentTemporary(benchmark::State& state)
{
    struct Component
    {
        int var;
    };

    X::Space space{};
    X::Entity entity = space.CreateEntity();
    space.EmplaceComponent<Component>(entity, 1);
    for (auto _ : state)
    {
        for (int i = 0; i < 1000; ++i)
        {
            benchmark::DoNotOptimize(space.GetComponentTemporary<Component>(entity));
        }
    }
}
BENCHMARK(Get1KComponentTemporary);

static void Create10EntitiesSpace(benchmark::State& state)
{
    X::Space space{};
    for (auto _ : state)
    {
        for (int i = 0; i < 10; ++i)
        {
            X::Entity e = space.CreateEntity();
            benchmark::DoNotOptimize(e);
        }
    }
}
BENCHMARK(Create10EntitiesSpace);

static void Create10EntitiesSpaceWithAComponent(benchmark::State& state)
{
    X::Space space{};
    for (auto _ : state)
    {
        for (int i = 0; i < 10; ++i)
        {
            X::Entity e = space.CreateEntity();
            space.EmplaceComponent<CFirst>(e, 1ul, 1ul);
            benchmark::DoNotOptimize(e);
        }
    }
}
BENCHMARK(Create10EntitiesSpaceWithAComponent);

static void Create10EntitiesSpaceWithTwoComponents(benchmark::State& state)
{
    X::Space space{};
    for (auto _ : state)
    {
        for (int i = 0; i < 10; ++i)
        {
            X::Entity e = space.CreateEntity();
            space.EmplaceComponent<CFirst>(e, 1ul, 1ul);
            space.EmplaceComponent<CSecond>(e, 1ul, 1ul);
            benchmark::DoNotOptimize(e);
        }
    }
}
BENCHMARK(Create10EntitiesSpaceWithTwoComponents);

TEST(Alloy, TestGenerations)
{
    X::Space space{};

    X::Entity entity1 = space.CreateEntity();
    X::Entity entity2 = space.CreateEntity();

    X::GenerationType generation1 = entity1.GetGeneration();
    X::GenerationType generation2 = entity2.GetGeneration();
    ASSERT_TRUE(entity1 != entity2) << "Generation should differ.";
    ASSERT_TRUE(entity2 != entity1) << "Generation should differ.";
    ASSERT_TRUE(space.IsValid(entity1)) << "Entity should be valid.";
    ASSERT_TRUE(space.IsValid(entity2)) << "Entity should be valid.";

    space.DestroyEntity(entity1);
    ASSERT_FALSE(space.IsValid(entity1)) << "Destroyed entity is still valid";
    ASSERT_TRUE(space.IsValid(entity2)) << "Living entity should still be valid.";

    space.DestroyEntity(entity2);
    ASSERT_FALSE(space.IsValid(entity2)) << "Destroyed entity is still valid";

    X::Entity entity3 = space.CreateEntity();

    if (entity1.GetId() == entity3.GetId())
        ASSERT_TRUE(generation1 != entity3.GetGeneration()) << "Generations should not be reused.";
    if (entity2.GetId() == entity3.GetId())
        ASSERT_TRUE(generation2 != entity3.GetGeneration()) << "Generations should not be reused.";

    benchmark::DoNotOptimize(entity1);
}

static void UpdateOn1KEntities(benchmark::State& state)
{
    struct Component
    {
        int ivar;
    };

    X::Space space{};

    for (int i = 0; i < 1000; ++i)
    {
        X::Entity entity = space.CreateEntity();
        space.EmplaceComponent<Component>(entity, 1);
    }

    for (auto _ : state)
    {
        space.Update<Component>([](X::Entity e, Component& component) {
            benchmark::DoNotOptimize(component);
        });
    }
}
BENCHMARK(UpdateOn1KEntities);

static void UpdateOn1KEntities2Components(benchmark::State& state)
{
    struct Component
    {
        int ivar;
    };

    struct Component2
    {
        int ivar;
    };

    X::Space space{};

    for (int i = 0; i < 1000; ++i)
    {
        X::Entity entity = space.CreateEntity();
        space.EmplaceComponent<Component>(entity, 1);
        space.EmplaceComponent<Component2>(entity, 1);
    }

    for (auto _ : state)
    {
        space.Update<Component, Component2>([](X::Entity e, Component& component1, Component2& component2) {
            benchmark::DoNotOptimize(component1);
            benchmark::DoNotOptimize(component2);
        });
    }
}
BENCHMARK(UpdateOn1KEntities2Components);

static void CreateDestroy100EntitiesInOrderWithOneComponent(benchmark::State& state)
{
    struct Component
    {
        int a;
        int b;
    };

    X::Space space{};
    std::vector<X::Entity> entities{};
    entities.resize(100);


    for (auto _ : state)
    {
        for (int i = 0; i < 100; ++i)
        {
            X::Entity entity = space.CreateEntity();
            space.EmplaceComponent<Component>(entity);
            entities[i] = entity;
        }

        for (int i = 0; i < 100; ++i)
        {
            space.DestroyEntity(entities[i]);
        }
    }
}
BENCHMARK(CreateDestroy100EntitiesInOrderWithOneComponent);


static void CreateDestroy10EntitiesReverseOrderWithOneComponent(benchmark::State& state)
{
    struct Component
    {
        int a;
        int b;
    };

    X::Space space{};
    std::vector<X::Entity> entities{};
    entities.resize(100);

    for (auto _ : state)
    {
        for (int i = 0; i < 100; ++i)
        {
            X::Entity entity = space.CreateEntity();
            space.EmplaceComponent<Component>(entity);
            entities[i] = entity;
        }

        for (int i = 100; i > 0; --i)
        {
            space.DestroyEntity(entities[i - 1]);
        }
    }
}
BENCHMARK(CreateDestroy10EntitiesReverseOrderWithOneComponent);

TEST(Alloy, EmplaceIsValid)
{
    struct Component1
    {
        int a{ 1 };
    };

    struct Component2
    {
        Component2(X::Space& space, X::Entity entity)
        {
            if (!space.IsValid(entity))
            {
                exit(1);
            }

            if (!space.HasComponent<Component1>(entity))
            {
                exit(1);
            }
        }
        int b{ 2 };
    };

    X::Space space{};
    X::Entity entity = space.CreateEntity();

    space.EmplaceComponent<Component1>(entity);
    space.EmplaceComponent<Component2>(entity, space, entity);
}

TEST(Alloy, ComponentDataValidation)
{
    struct Component1
    {
        std::string string{};
        std::vector<int> vector{};

        Component1() {}

        //        int& a;

        //        Component1(const Component1& other) : string(other.string), vector(other.vector), a(other.a)
        //        {}
        //
        Component1(const std::string& string, const std::vector<int> vector, int& a) : string(string), vector(vector) /*, a(a)*/
        {}

        Component1(const Component1& other) = delete;
        Component1& operator=(const Component1& other)
        {
            string = other.string;
            vector = other.vector;
            //            a = other.a;
            return *this;
        }
    };

    struct Component2
    {
        int a;
    };

    X::Space space{};
    X::Entity entity = space.CreateEntity();

    //    bool functionWasCalled = false;
    //    auto function = [&](X::Entity e){
    //      EXPECT_TRUE(space.IsValid(e));
    //      functionWasCalled = true;
    //    };

    int b = 1;

    // Artificial scope to force any stack objects to be popped
    {
        space.EmplaceComponent<Component1>(entity, "hello world", std::vector<int>{ 1, 2, 3 }, b);
        // Force Component1 to be copied/moved
        space.EmplaceComponent<Component2>(entity, 1);
    }

    Component1& component1 = space.GetComponentTemporary<Component1>(entity);

    // Try to modify the vector with range-based
    //    for (int& i : component1.vector) i += 1;
    //    std::cout << component1.string << std::endl;
    // Copy and compare string
    // Call the std::function
    //    component1.stdFunction(entity);
}


TEST(Alloy, ManySpaces)
{
    for (int i = 0; i < 10; ++i)
    {
        X::Space space{};
        benchmark::DoNotOptimize(space);
    }

    X::Space space1{};
    X::Space space2{};
    X::Space space3{};
    X::Space space4{};
    X::Space space5{};
    X::Space space6{};
    X::Space space7{};
    X::Space space8{};
    X::Space space9{};
    X::Space space10{};
    benchmark::DoNotOptimize(space1);
    benchmark::DoNotOptimize(space2);
    benchmark::DoNotOptimize(space3);
    benchmark::DoNotOptimize(space4);
    benchmark::DoNotOptimize(space5);
    benchmark::DoNotOptimize(space6);
    benchmark::DoNotOptimize(space7);
    benchmark::DoNotOptimize(space8);
    benchmark::DoNotOptimize(space9);
    benchmark::DoNotOptimize(space10);
}
TEST(Alloy, Random)
{
    X::Space space1{};
    X::Entity entityInvalid{};
    X::Entity entity1 = space1.CreateEntity();
    X::Entity entity2 = space1.CreateEntity();
    X::Entity entityDestroyed = space1.CreateEntity();
    space1.DestroyEntity(entityDestroyed);

    struct DebugComponent
    {
    };

    space1.EmplaceComponent<DebugComponent>(entity2);
    //    X::GetInfo(space1, entityInvalid);
    //    X::GetInfo(space1, entity1);
    //    X::GetInfo(space1, entity2);
    //    X::GetInfo(space1, entityDestroyed);
    //    X::GetInfo(space1);
}
