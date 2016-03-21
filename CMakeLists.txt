cmake_minimum_required(VERSION 2.8)

project(task-pool)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

###
# 3rd party
###
find_package(googletest)

###
# config (modified from qartar/qflags)
###

set(C_FLAGS CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
set(CXX_FLAGS CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
              CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
if(MSVC)
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
        # /W4       Warning level 4
        # /WX       Warnings as errors
        # /MP       Multi-process compilation
        # /sdl      Additional security checks
        set(compiler_flags "/W4 /WX /MP /sdl")
        set(cxx_flags ${compiler_flags})
        set(c_flags ${compiler_flags})

    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
        # -Wall             Enable (almost) all warnings
        # -Wextra           Enable additional warnings
        # -Werror           Warnings as errors
        # -pedantic         Warn when using unstandard language extensions
        # /MP               Multi-process compilation
        # /sdl              Additional security checks
        set(compiler_flags "-Wall -Wextra -Werror -pedantic -Wtype-limits /MP /sdl
                            -Wno-unused-value
                            -fstack-protector-all
                            -Wno-missing-field-initializers
                            -Wlanguage-extension-token")
        # -fno-rtti         Disable RTTI
        # -fno-exceptions   Disable exceptions
        set(cxx_flags "-fno-rtti -fno-exceptions -Woverloaded-virtual")
        set(c_flags "")

        # Set gmock and gtest as system headers in order to suppress warnings.
        set(compiler_flags "${compiler_flags} --system-header-prefix=gtest/")
        set(compiler_flags "${compiler_flags} --system-header-prefix=gmock/")
    endif()


    # For MSVC, CMake sets certain flags to defaults we want to override.
    # This replacement code is taken from sample in the CMake Wiki at
    # http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace.
    foreach (flag_var ${C_FLAGS} ${CXX_FLAGS})

        # Set base compiler flags
        set(${flag_var} "${${flag_var}} ${compiler_flags}")

        # Disable dynamic linking to C++ runtime libraries
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")

        # Disable RTTI
        string(REPLACE "/GR" "/GR-" ${flag_var} "${${flag_var}}")

        # Disable exceptions
        string(REGEX REPLACE "/EH[a-z]+" "" ${flag_var} "${${flag_var}}")
        add_definitions(-D_HAS_EXCEPTIONS=0)

    endforeach()

    # Add C specific flags
    foreach (flag_var ${C_FLAGS})
        # Set base compiler flags
        set(${flag_var} "${${flag_var}} ${c_flags}")
    endforeach()

    # Add C++ specific flags
    foreach (flag_var ${CXX_FLAGS})
        # Set base compiler flags
        set(${flag_var} "${${flag_var}} ${cxx_flags}")
    endforeach()

    # Enable Unicode
    add_definitions(-D_UNICODE -DUNICODE)

    # Enable automatic strcpy => strcpy_s conversion
    # FIXME: Doesn't work with Microsoft's clang
    #add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
    #                -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1
    #)

    # Enable edit-and-continue support
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")

elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU OR ${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)

    # -Wall         Enable (almost) all warnings
    # -Wextra       Enable additional warnings
    # -Werror       Warnings as errors
    # -pedantic     Warn when using unstandard language extensions
    # -std=c++11    Use C++11 language features
    set(compiler_flags "-Wall -Wextra -Werror -pedantic -Wtype-limits -Wunused-parameter -Wno-missing-field-initializers")
    # -fno-rtti         Disable RTTI
    # -fno-exceptions   Disable exceptions
    set(cxx_flags "-fno-rtti -fno-exceptions -Woverloaded-virtual -std=c++11")
    set(c_flags "-std=c99")

    foreach (flag_var ${C_FLAGS} ${CXX_FLAGS})
        # Set base compiler flags
        set(${flag_var} "${${flag_var}} ${compiler_flags}")
    endforeach()

    # Add C specific flags
    foreach (flag_var ${C_FLAGS})
        # Set base compiler flags
        set(${flag_var} "${${flag_var}} ${c_flags}")
    endforeach()

    # Add C++ specific flags
    foreach (flag_var ${CXX_FLAGS})
        # Set base compiler flags
        set(${flag_var} "${${flag_var}} ${cxx_flags}")
    endforeach()

endif()

###
# source
###
set(SOURCES
    include/task-pool/task-pool.h
    src/stdatomic.h
    src/stdatomic.cpp
    src/task-queue.h
    src/task-pool.c
)

add_library(task-pool STATIC ${SOURCES})
target_include_directories(task-pool PUBLIC include)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    target_compile_options(task-pool PUBLIC /analyze /wd6326)
endif()

###
# test
###
if(TARGET gtest)
    set(TEST_SOURCES
        test/pool_test.cpp
        test/task-queue_test.cpp
    )

    add_executable(task-pool-test ${TEST_SOURCES})
    target_link_libraries(task-pool-test gtest_main task-pool)

    add_custom_command(TARGET task-pool-test
                       POST_BUILD
                       COMMAND task-pool-test)

    if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
        target_compile_options(task-pool-test PUBLIC /analyze /wd6326)
    endif()

endif()
