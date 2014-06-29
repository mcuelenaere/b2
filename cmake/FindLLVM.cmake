find_program(LLVM_CONFIG_EXECUTABLE llvm-config DOC "path to the llvm-config executable")
mark_as_advanced(LLVM_CONFIG_EXECUTABLE)

if(LLVM_CONFIG_EXECUTABLE)
    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
        OUTPUT_VARIABLE LLVM_CONFIG_version_output
        ERROR_VARIABLE LLVM_CONFIG_version_error
        RESULT_VARIABLE LLVM_CONFIG_version_result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT ${LLVM_CONFIG_version_result} EQUAL 0)
        message(SEND_ERROR "Command \"${LLVM_CONFIG_EXECUTABLE} --version\" failed with output:\n${LLVM_CONFIG_version_error}")
    else()
        set(LLVM_VERSION ${LLVM_CONFIG_version_output})
    endif()

    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
        RESULT_VARIABLE LLVM_CONFIG_cppflags_result
        OUTPUT_VARIABLE LLVM_CONFIG_cppflags_output
        ERROR_VARIABLE LLVM_CONFIG_cppflags_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT ${LLVM_CONFIG_cppflags_result} EQUAL 0)
        message(SEND_ERROR "Command \"${LLVM_CONFIG_EXECUTABLE} --cppflags\" failed with output: ${LLVM_CONFIG_cppflags_error}")
    else()
        set(LLVM_DEFINITIONS ${LLVM_CONFIG_cppflags_output})
    endif()

    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs ${COMPONENTS}
        RESULT_VARIABLE LLVM_CONFIG_libs_result
        OUTPUT_VARIABLE LLVM_CONFIG_libs_output
        ERROR_VARIABLE LLVM_CONFIG_libs_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT ${LLVM_CONFIG_libs_result} EQUAL 0)
        message(SEND_ERROR "Command \"${LLVM_CONFIG_EXECUTABLE} --libs\" failed with output: ${LLVM_CONFIG_libs_error}")
    else()
        set(LLVM_LIBS ${LLVM_CONFIG_libs_output})
    endif()

    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
        RESULT_VARIABLE LLVM_CONFIG_ldflags_result
        OUTPUT_VARIABLE LLVM_CONFIG_ldflags_output
        ERROR_VARIABLE LLVM_CONFIG_ldflags_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT ${LLVM_CONFIG_ldflags_result} EQUAL 0)
        message(SEND_ERROR "Command \"${LLVM_CONFIG_EXECUTABLE} --ldflags\" failed with output: ${LLVM_CONFIG_ldflags_error}")
    else()
        set(LLVM_LIBS "${LLVM_CONFIG_ldflags_output} ${LLVM_LIBS}")
    endif()

    execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --bindir ${COMPONENTS}
        RESULT_VARIABLE LLVM_CONFIG_bindir_result
        OUTPUT_VARIABLE LLVM_CONFIG_bindir_output
        ERROR_VARIABLE LLVM_CONFIG_bindir_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT ${LLVM_CONFIG_bindir_result} EQUAL 0)
        message(SEND_ERROR "Command \"${LLVM_CONFIG_EXECUTABLE} --bindir\" failed with output: ${LLVM_CONFIG_bindir_error}")
    else()
        set(LLVM_BIN ${LLVM_CONFIG_bindir_output})
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVM REQUIRED_VARS LLVM_CONFIG_EXECUTABLE LLVM_DEFINITIONS LLVM_LIBS LLVM_BIN VERSION_VAR LLVM_VERSION)

macro(emit_llvm_target Input Output)
    set(Definitions "${ARGV2}")
    separate_arguments(Definitions)
    add_custom_command(
        OUTPUT ${Output}
        COMMAND ${CMAKE_C_COMPILER} -emit-llvm -c ${Definitions} -o ${Output} ${Input}
        DEPENDS ${Input}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_source_files_properties(${Input} PROPERTIES HEADER_FILE_ONLY TRUE)
endmacro()
