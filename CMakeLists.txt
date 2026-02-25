cmake_minimum_required(VERSION 3.31)
project(Bond_Sequence_Optimiser)

set(CMAKE_CXX_STANDARD 23)

add_executable(Bond_Sequence_Optimiser
    main.cpp
    src/app/cli/OutputMessages.cpp
    src/app/cli/Prompts.cpp
    src/app/counter/PathCounter.cpp
    src/app/domain/BondReturnData.cpp
    src/app/io/CSVLoader.cpp
    src/app/io/ExportOptions.cpp
    src/app/io/ResultsOutput.cpp
    src/app/optimiser/DynamicOptimiser.cpp
    src/helpers/Filesystem.cpp
    src/helpers/Strings.cpp
    src/helpers/Output.cpp
    src/helpers/Quit.cpp
    src/helpers/printing/Print.cpp
    src/helpers/printing/StyledPrint.cpp
)

target_include_directories(Bond_Sequence_Optimiser
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_compile_definitions(your_target PRIVATE NOMINMAX)