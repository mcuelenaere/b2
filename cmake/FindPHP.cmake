find_program(PHP_CONFIG_EXECUTABLE php-config DOC "path to the php-config executable")
mark_as_advanced(PHP_CONFIG_EXECUTABLE)

if(PHP_CONFIG_EXECUTABLE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --version
        OUTPUT_VARIABLE PHP_CONFIG_version_output
        ERROR_VARIABLE PHP_CONFIG_version_error
        RESULT_VARIABLE PHP_CONFIG_version_result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT ${PHP_CONFIG_version_result} EQUAL 0)
        message(SEND_ERROR "Command \"${PHP_CONFIG_EXECUTABLE} --version\" failed with output:\n${PHP_CONFIG_version_error}")
    else()
        set(PHP_VERSION ${PHP_CONFIG_version_output})
    endif()

    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --includes
        RESULT_VARIABLE PHP_CONFIG_includes_result
        OUTPUT_VARIABLE PHP_CONFIG_includes_output
        ERROR_VARIABLE PHP_CONFIG_includes_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT ${PHP_CONFIG_includes_result} EQUAL 0)
        message(SEND_ERROR "Command \"${PHP_CONFIG_EXECUTABLE} --includes\" failed with output: ${PHP_CONFIG_includes_error}")
    else()
        set(PHP_DEFINITIONS ${PHP_CONFIG_includes_output})
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PHP REQUIRED_VARS PHP_CONFIG_EXECUTABLE PHP_DEFINITIONS VERSION_VAR PHP_VERSION)

if(APPLE)
  #set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} -Wl,-flat_namespace -Wl,-undefined,dynamic_lookup")
  #set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -Wl,-flat_namespace -Wl,-undefined,dynamic_lookup")

  set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -Wl,-flat_namespace")
  foreach(symbol
    __efree
    __emalloc
    __estrdup
    __object_init_ex
    __zval_ptr_dtor
    __zval_ptr_dtor_wrapper
    _display_ini_entries
    _object_properties_init
    _php_info_print_table_end
    _php_info_print_table_row
    _php_info_print_table_start
    _php_output_write
    _zend_declare_property_null
    _zend_exception_get_default
    _zend_get_std_object_handlers
    _zend_new_interned_string
    _zend_object_std_dtor
    _zend_object_std_init
    _zend_object_store_get_object
    _zend_objects_new
    _zend_objects_destroy_object
    _zend_objects_store_put
    _zend_parse_parameters
    _zend_register_internal_class
    _zend_register_internal_class_ex
    _zend_strndup
    _zend_throw_exception
    _zend_update_property
    __zend_hash_init
    _zend_hash_destroy
    __zend_hash_add_or_update
    _zend_is_callable
    _zend_read_property
    )
    set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS},-U,${symbol}")
  endforeach()
endif()
