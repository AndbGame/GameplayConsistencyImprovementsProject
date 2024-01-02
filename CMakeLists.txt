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

# ----- Copy and merge across the po files that come with the source.

message("Compile Papyrus scripts...")

file(GLOB papyrusPathProject "Papyrus/*")

foreach(pPath ${papyrusPathProject})
  # Get the language name, like en_US or zh_CN from the name of the po file, so
  # 'en_US.po' or 'zh_CN.po' become 'en_US' or 'zh_CN.po'
#  get_filename_component(langName ${file} NAME_WE)

#    message(STATUS "${pPath}")
    file(GLOB pscFile "${pPath}/source/*.psc")
    foreach(file ${pscFile})
#        message(STATUS "${pscFile}")
        get_filename_component(pScriptName ${pscFile} NAME_WLE)
        get_filename_component(pScriptSourceDir ${pscFile} DIRECTORY)
#        message(STATUS "${pScriptName}")
        set(pCompilledScriptFile "${pPath}/${pScriptName}.pex")
#        message(STATUS "${pCompilledScriptFile}")

      if(NOT EXISTS ${pCompilledScriptFile} OR ${pscFile} IS_NEWER_THAN ${pCompilledScriptFile})
        message(STATUS "Recomplie ${pscFile}")
        execute_process(COMMAND "${CMAKE_SOURCE_DIR}/Papyrus/PAPYRUS/PapyrusCompiler.exe" 
            "${pscFile}"
            "-o=${pPath}" 
            "-i=${pScriptSourceDir};${CMAKE_SOURCE_DIR}/Papyrus/scripts/source;SRC;SRC_UD;SRC_OTHER;SRC_LAL;SRC_LLOTHER;SRC_ZAZ;SRC_PO3;SRC_DD;SRC_SKYUI;SRC_UIEXT;SRC_PAPUTIL;SRC_SXL;SRC_MFG;SRC_RM;SRC_FNIS;SRC_SLA;SRC_IWW;SRC_XPMSE;SRC_ST"
            "-optimize"
            "-f=Flags.flg"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Papyrus/PAPYRUS"
            OUTPUT_QUIET ERROR_VARIABLE error RESULT_VARIABLE ret)

        if(ret) # Have to do this hack as msgmerge prints to stderr.
            message(WARNING "${ret} - ${error}")
        endif()
      endif()

        set(pscOriginalFile "${pPath}/source_original/${pScriptName}.psc")
        set(pscPatchFile "${pPath}/${pScriptName}.psc.patch.txt")
        if(EXISTS ${pscOriginalFile} AND (NOT EXISTS ${pscPatchFile} OR ${pscFile} IS_NEWER_THAN ${pscPatchFile}))
            execute_process( COMMAND ${CMAKE_COMMAND} -E compare_files ${pscOriginalFile} ${pscFile}
                     RESULT_VARIABLE compare_result)
            if( compare_result EQUAL 1)
                message("Create patch file for ${pscFile}")
                execute_process(COMMAND "git" "diff" "--patch" "--no-index"
                    "--output=${pscPatchFile}"
                    ${pscOriginalFile} ${pscFile}
                    OUTPUT_QUIET ERROR_VARIABLE error RESULT_VARIABLE ret)

                if(ret) # Have to do this hack as msgmerge prints to stderr.
                    message(WARNING "${ret} - ${error}")
                endif()
            endif()
        endif()

    endforeach()
endforeach()

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
