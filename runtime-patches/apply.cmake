# Patches the submodules: N64Recomp-browser's patches/ (its apply.cmake), then the engine's changes to them
# (lib/turok-engine/runtime-patches/), then this game's own (runtime-patches/ here), each <project>-<n>-*.patch in order
# of name, where <project> is a name in N64Recomp-browser's patches/patches.txt, which says the folder (README.md here).
# Doing it again changes nothing.
#
# The engine's and the game's are taken out first and put back after: N64Recomp-browser's check that its patch is in,
# for a checkout it patched with a three-way merge, is that the checkout's changes are what they were when it went in,
# and these change them. What comes out is what went in: a copy of each patch as it was applied, kept in the git dir of
# the checkout it patched (turok-runtime-patches/<n>-<name>, <n> its place in the order), so a patch edited, renamed or
# removed since comes out too.
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
    string(REGEX REPLACE "^[^ ]+ +([^ ]+).*" "\1" dir "${lines}")
    set(${out} "${root}/lib/${dir}" PARENT_SCOPE)
endfunction()

# The folder, in the git dir of the checkout at <dir>, holding the copies of the patches applied to it.
function(t1_applied_dir dir out)
    execute_process(COMMAND git rev-parse --path-format=absolute --git-path turok-runtime-patches
        WORKING_DIRECTORY "${dir}" OUTPUT_VARIABLE path OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${out} "${path}" PARENT_SCOPE)
endfunction()

function(t1_apply_patches)
    get_filename_component(root "${T1_ROOT}" ABSOLUTE)
    file(GLOB engine "${root}/lib/turok-engine/runtime-patches/*.patch")
    file(GLOB game "${root}/runtime-patches/*.patch")
    list(SORT engine)
    list(SORT game)
    set(patches ${engine} ${game})

    # The copies of what went in, from every checkout a patch can be for.
    file(STRINGS "${root}/lib/N64Recomp-browser/patches/patches.txt" rows REGEX "^[^#]")
    set(applied_dirs "")
    foreach (row IN LISTS rows)
        string(REGEX REPLACE " +" ";" row "${row}")
        list(GET row 1 path)
        if (EXISTS "${root}/lib/${path}/.git")
            t1_applied_dir("${root}/lib/${path}" applied_dir)
            list(APPEND applied_dirs "${applied_dir}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES applied_dirs)
    set(applied "")
    foreach (applied_dir IN LISTS applied_dirs)
        file(GLOB copies "${applied_dir}/*.patch")
        foreach (copy IN LISTS copies)
            get_filename_component(copy_name "${copy}" NAME)
            list(APPEND applied "${copy_name}")
            set("copy_${copy_name}" "${copy}")
        endforeach()
    endforeach()

    # Out, last first: the copies, or for a checkout patched before they were kept, the patches as they are now.
    if (applied)
        list(SORT applied)
        list(REVERSE applied)
        foreach (copy_name IN LISTS applied)
            set(copy "${copy_${copy_name}}")
            string(REGEX REPLACE "^[0-9]+-" "" name "${copy_name}")
            t1_patch_dir("${root}" "${name}" dir)
            execute_process(COMMAND git apply --whitespace=nowarn --reverse "${copy}"
                WORKING_DIRECTORY "${dir}" RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
            file(REMOVE "${copy}")
        endforeach()
    else()
        set(reversed ${patches})
        list(REVERSE reversed)
        foreach (patch IN LISTS reversed)
            get_filename_component(name "${patch}" NAME)
            t1_patch_dir("${root}" "${name}" dir)
            execute_process(COMMAND git apply --whitespace=nowarn --reverse "${patch}"
                WORKING_DIRECTORY "${dir}" RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
        endforeach()
    endif()

    n64recomp_browser_apply_patches("${root}/lib")

    set(n 100)
    foreach (patch IN LISTS patches)
        get_filename_component(name "${patch}" NAME)
        t1_patch_dir("${root}" "${name}" dir)
        execute_process(COMMAND git apply --whitespace=nowarn "${patch}"
            WORKING_DIRECTORY "${dir}" RESULT_VARIABLE result ERROR_VARIABLE error)
        if (NOT result EQUAL 0)
            message(FATAL_ERROR "${name} doesn't apply to ${dir}:\n${error}")
        endif()
        t1_applied_dir("${dir}" applied_dir)
        file(MAKE_DIRECTORY "${applied_dir}")
        configure_file("${patch}" "${applied_dir}/${n}-${name}" COPYONLY)
        math(EXPR n "${n} + 1")
    endforeach()
endfunction()

if (CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    t1_apply_patches()
endif()
