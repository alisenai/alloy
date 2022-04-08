//#include <benchmark/benchmark.h>
#include "gtest/gtest.h"

// ===== MODULES =====
#include "Modules/AlloyModule.h"
// ===================

int main(int argc, char** argv)
{
// Force instantiation of these functions
#ifdef ALLOY_EXPOSE_INTERNALS
    using GetEntityInfo = void (*)(X::Space&, const X::Entity, bool);
    using GetSpaceInfo = void (*)(X::Space&, bool);
    volatile GetEntityInfo defineGetEntityInfo = X::GetEntityInfo;
    volatile GetSpaceInfo defineGetSpaceInfo = X::GetSpaceInfo;
#endif

    std::cout << "[Running tests]" << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    int returnValue = RUN_ALL_TESTS();
    std::cout << "[Running tests completed]" << std::endl;
    if (returnValue == 0)
    {
        std::cout << "[Running benchmarks]" << std::endl;
        ::benchmark::Initialize(&argc, argv);
        if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
        ::benchmark::RunSpecifiedBenchmarks();
        ::benchmark::Shutdown();
        std::cout << "[Running benchmarks completed]" << std::endl;
    }
    else
    {
        std::cout << "[Tests failed, not running benchmarks]" << std::endl;
    }

    std::cout << "[Testing completed]" << std::endl;
    return returnValue;
}
