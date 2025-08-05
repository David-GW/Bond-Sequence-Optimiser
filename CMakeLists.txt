cmake_minimum_required(VERSION 3.31)
project(BondSequenceOptimiser)

set(CMAKE_CXX_STANDARD 23)

add_executable(BondSequenceOptimiser
    main.cpp
    src/helpers/GeneralHelpers.cpp
    src/helpers/InputHelpers.cpp
    src/helpers/OutputHelpers.cpp
    src/optimisers/DynamicOptimiser.cpp
    src/optimisers/RecursiveOptimiser.cpp
    src/types/BondReturnData.cpp
)

target_include_directories(BondSequenceOptimiser
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
