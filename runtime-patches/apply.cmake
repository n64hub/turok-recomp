# Patches the submodules: N64Recomp-browser's patches/ (its apply.cmake), then the engine's changes to them
# (lib/turok-engine/runtime-patches/), then this game's own (runtime-patches/ here), each <project>-<n>-*.patch in order
# of name, where <project> is a name in N64Recomp-browser's patches/patches.txt, which says the folder (README.md here).
# Doing it again changes nothing.
#
# The engine's and the game's are taken out first and put back after: N64Recomp-browser's check that its patch is in,
# for a checkout it patched with a three-way merge, is that the checkout's changes are what they were when it went in,
# and these change them.
#
#   cmake -P runtime-patches/apply.cmake          from the repo's root (tools/build_n64recomp.sh)
#   include(runtime-patches/apply.cmake)          and t1_apply_patches() (CMakeLists.txt)

set(T1_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
include("${T1_ROOT}/lib/N64Recomp-browser/patches/apply.cmake")

# The folder under lib/ a patch's project is in, from N64Recomp-browser's patches.txt.
function(t1_patch_dir root name out)
    string(REGEX REPLACE "-[0-9]+-.*" "" project "${name}")
    file(STRINGS "${root}/lib/N64Recomp-browser/patches/patches.txt" lines REGEX "^${project} ")
    if (NOT lines)
        message(FATAL_ERROR "${name}: ${project} isn't in lib/N64Recomp-browser/patches/patches.txt")
    endif()
    string(REGEX REPLACE "^[^ ]+ +([^ ]+).*" "\\1" dir "${lines}")
    set(${out} "${root}/lib/${dir}" PARENT_SCOPE)
endfunction()

function(t1_apply_patches)
    get_filename_component(root "${T1_ROOT}" ABSOLUTE)
    file(GLOB engine "${root}/lib/turok-engine/runtime-patches/*.patch")
    file(GLOB game "${root}/runtime-patches/*.patch")
    list(SORT engine)
    list(SORT game)
    set(patches ${engine} ${game})
    set(reversed ${patches})
    list(REVERSE reversed)
    foreach (patch IN LISTS reversed)
        get_filename_component(name "${patch}" NAME)
        t1_patch_dir("${root}" "${name}" dir)
        execute_process(COMMAND git apply --whitespace=nowarn --reverse "${patch}"
            WORKING_DIRECTORY "${dir}" RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
    endforeach()

    n64recomp_browser_apply_patches("${root}/lib")

    foreach (patch IN LISTS patches)
        get_filename_component(name "${patch}" NAME)
        t1_patch_dir("${root}" "${name}" dir)
        execute_process(COMMAND git apply --whitespace=nowarn "${patch}"
            WORKING_DIRECTORY "${dir}" RESULT_VARIABLE result ERROR_VARIABLE error)
        if (NOT result EQUAL 0)
            message(FATAL_ERROR "${name} doesn't apply to ${dir}:\n${error}")
        endif()
    endforeach()
endfunction()

if (CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    t1_apply_patches()
endif()
