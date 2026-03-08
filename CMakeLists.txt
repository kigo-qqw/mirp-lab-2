cmake_minimum_required(VERSION 3.31)

project(mirp-lab-2 VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(SANITIZER "" CACHE STRING "Use sanitizers: one of Address, Leak, Thread, Memory, Undefined")

if (SANITIZER)
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Compiling the code with sanitizers in non-debug mode"
                " is misleading and should be avoided"
                " (CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})")
    endif ()

    if (SANITIZER STREQUAL "Address")
        # AddressSanitizer (detects addressability issues)
        # Note that: https://clang.llvm.org/docs/HardwareAssistedAddressSanitizerDesign.html is more efficient
        # See:
        # - https://github.com/google/sanitizers/wiki/AddressSanitizerFlags
        # - https://clang.llvm.org/docs/AddressSanitizer.html
        set(
                ASAN_OPTIONS
                symbolize=1
        )
        message("Set ASAN_OPTIONS to: ${ASAN_OPTIONS}")

        find_program(LLVM_SYMB_PATH llvm-symbolizer)
        set(
                ASAN_SYMBOLIZER_PATH
                ${LLVM_SYMB_PATH}
        )
        message("Set ASAN_SYMBOLIZER_PATH to: ${ASAN_SYMBOLIZER_PATH}")

        add_compile_options(-O1 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls)
        add_link_options(-fsanitize=address)
    elseif (SANITIZER STREQUAL "Leak")
        # LeakSanitizer (detects memory leaks)
        # See: https://clang.llvm.org/docs/LeakSanitizer.html
        set(
                ASAN_OPTIONS
                detect_leaks=1
        )
        message("Set ASAN_OPTIONS to: ${ASAN_OPTIONS}")

        add_compile_options(-g -fsanitize=leak)
        add_link_options(-fsanitize=leak)
    elseif (SANITIZER STREQUAL "Thread")
        # ThreadSanitizer (detects data races and deadlocks)
        add_compile_options(-O1 -g -fsanitize=thread)
        add_link_options(-fsanitize=thread)
    elseif (SANITIZER STREQUAL "Memory")
        # MemorySanitizer (detects use of uninitialized memory)
        # See:
        #  - https://clang.llvm.org/docs/MemorySanitizer.html
        find_program(LLVM_SYMB_PATH llvm-symbolizer)
        set(
                MSAN_SYMBOLIZER_PATH
                ${LLVM_SYMB_PATH}
        )
        message("Set MSAN_SYMBOLIZER_PATH to: ${ASAN_SYMBOLIZER_PATH}")

        add_compile_options(-O1 -g -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -fno-optimize-sibling-calls)
        add_link_options(-fsanitize=memory)
    elseif (SANITIZER STREQUAL "Undefined")
        # UndefinedBehaviorSanitizer (detects undefined behaviors)
        # See:
        #  - https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
        set(
                UBSAN_OPTIONS
                print_stacktrace=1
        )
        message("Set UBSAN_OPTIONS to: ${UBSAN_OPTIONS}")

        add_compile_options(-O1 -g -fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=undefined)
    else ()
        message(FATAL_ERROR "Unknown sanitizer. Aborting...")
    endif ()
endif ()

execute_process(
        COMMAND git log -1 --format=%H
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

enable_testing()

add_subdirectory(lib)

find_package(OpenMP REQUIRED)

add_library(openmp_wrapper INTERFACE)

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(openmp_wrapper INTERFACE -Xpreprocessor -fopenmp)
    target_link_options(openmp_wrapper INTERFACE -lomp)
    target_include_directories(openmp_wrapper INTERFACE ${OpenMP_C_INCLUDE_DIRS})
else()
    target_compile_options(openmp_wrapper INTERFACE ${OpenMP_C_FLAGS})
    target_link_libraries(openmp_wrapper INTERFACE OpenMP::OpenMP_C)
endif()

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.c")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE lib::common lib::string lib::vector openmp_wrapper)

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Weverything

            -Wno-extra-semi
            -Wno-gnu-zero-variadic-macro-arguments
            -Wno-declaration-after-statement
            -Wno-unsafe-buffer-usage
            -Wno-strict-prototypes
            -Wno-padded
            -Wno-implicit-void-ptr-cast
            -Wno-switch-default
            -Wno-pre-c11-compat
            -Wno-cast-qual
    )
endif ()

target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unknown-warning-option

        -O0 -g
)

target_compile_definitions(
        ${PROJECT_NAME}
        PRIVATE
        VERSION="${GIT_HASH}"
        PROJECT_NAME="${PROJECT_NAME}"
)

add_test(
        NAME test-${PROJECT_NAME}-trace-none
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        10000
        --trace=none
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
        NAME test-${PROJECT_NAME}-trace-result
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        10000
        --trace
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
        NAME test-${PROJECT_NAME}-trace-all
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        10000
        --trace=all
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
