# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

# Set your project name. This will be the name of your SKSE .dll file.
project(GCIP VERSION 0.1 LANGUAGES CXX)

include("CMakeUser.txt" OPTIONAL)

# Otherwise, you can set OUTPUT_FOLDER to any place you'd like :)
# set(OUTPUT_FOLDER "C:/path/to/any/folder")

# Get the .cpp files from your src/ directory
# to be passed into add_commonlibsse_plugin
#
# This helper automatically finds all of your .cpp files for you
#
# Please note: the official recommendation is to list our every .cpp
# file individually after SOURCES
set(source_files ${source_files}
    src/GCIPModuleManager.cpp
    src/GCIPConfig.cpp
    src/GCIPHooks.cpp
    src/modules/EncounterPlanner.cpp
)

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${source_files} plugin.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h) # <--- PCH.h is required!
target_include_directories(${PROJECT_NAME} PRIVATE include)

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED OUTPUT_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
    set(PAPYRUS_FOLDER "${OUTPUT_FOLDER}/scripts")
    set(PAPYRUS_SOURCE_FOLDER "${OUTPUT_FOLDER}/scripts/source")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Papyrus/scripts" "${PAPYRUS_FOLDER}"
        VERBATIM
    )

    # If you perform a "Debug" build, also copy .pdb file (for debug symbols)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()

    if(NOT EXISTS "${DLL_FOLDER}/GCIP.ini")
      execute_process(COMMAND ${CMAKE_COMMAND}
      -E copy "${CMAKE_SOURCE_DIR}/SKSE/Plugins/GCIP.ini" "${DLL_FOLDER}/GCIP.ini")
    endif()

    if(DEFINED OUTPUT_PAPYRUS_PATHES)
        foreach(OUTPUT_PAPYRUS_PATH IN LISTS OUTPUT_PAPYRUS_PATHES)
            message("Copy Patch for: ${OUTPUT_PAPYRUS_PATH}")
            add_custom_command(
                TARGET "${PROJECT_NAME}"
                POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Papyrus/${OUTPUT_PAPYRUS_PATH}" "${PAPYRUS_FOLDER}"
                VERBATIM
            )
        endforeach()
    endif()

endif()
