#include <main/php.h>
#include <main/php_output.h>
#include <ext/standard/info.h>
#include <Zend/zend_API.h>
#include <Zend/zend_exceptions.h>

#include <memory>
#include <string>

#include "backends/llvm/llvm_backend.hpp"
#include "parser/parser.hpp"

#include "php_template.h"
#include "php_bindings.hpp"

#include "ast/passes/coalesce_rawblocks_pass.hpp"
#include "ast/passes/convert_literal_printblock_to_rawblock_pass.hpp"
#include "ast/passes/fold_constant_expressions_pass.hpp"
#include "ast/passes/pass_manager.hpp"
#include "ast/passes/resolve_includes_pass.hpp"

#define B2_VERSION_STRING "0.0.1-dev"

namespace {

// class entries
static zend_class_entry* b2_engine_class_entry;
static zend_class_entry* b2_template_class_entry;
static zend_class_entry* b2_syntaxerror_class_entry;

// internal structures
struct Engine_object {
    zend_object zo;
    std::string basePath;
	llvm::LLVMContext llvmContext;
	llvm::IRBuilder<> irBuilder;
	b2::PHPBindings bindings;
	b2::LLVMBackend backend;
	b2::Parser parser;

	Engine_object() :
		irBuilder(llvmContext),
		bindings(irBuilder),
		backend(irBuilder, bindings)
	{
		bindings.linkInFunctions(backend.getModule());
	}
};

struct Template_object {
    zend_object zo;
    size_t estimatedBufferSize;
    template_fn renderFunc;
};

/* {{{ Engine_object_create */
static zend_object_value Engine_object_create(zend_class_entry *ce TSRMLS_DC)
{
    Engine_object* engine;
    zend_object_value retval;

    try {
        engine = new Engine_object();
    } catch (std::exception& ex) {
        zend_throw_exception(nullptr, (char*) ex.what(), 0);

        zend_object* obj;
        return zend_objects_new(&obj, ce);
    }

    zend_object_std_init(&engine->zo, ce TSRMLS_CC);
    object_properties_init(&engine->zo, ce);

    auto dtor = [](void *object, zend_object_handle handle TSRMLS_DC) {
        zend_objects_destroy_object((zend_object*) object, handle TSRMLS_CC);
    };

    auto freer = [](void *object TSRMLS_DC) {
        Engine_object* engine = (Engine_object*) object;

        zend_object_std_dtor(&engine->zo TSRMLS_CC);
        delete engine;
    };

    retval.handle = zend_objects_store_put(engine, dtor, freer, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}
/* }}} */

static PHP_METHOD(Engine, __construct)
{
    char* basePath = NULL;
    int basePathLen = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &basePath, &basePathLen) == FAILURE) {
        return;
    }

    Engine_object* engine = (Engine_object*) zend_object_store_get_object(getThis() TSRMLS_CC);

    engine->basePath = std::string(basePath, basePathLen);
}

static b2::AST* optimizeAST(b2::AST* ast, const std::string& basePath)
{
    b2::PassManager passManager;

    passManager.addPass(new b2::ResolveIncludesPass(basePath));
    passManager.addPass(new b2::FoldConstantExpressionsPass());
    passManager.addPass(new b2::ConvertLiteralPrintBlockToRawBlockPass());
    passManager.addPass(new b2::CoalesceRawBlocksPass());
    return passManager.run(ast);
}

static PHP_METHOD(Engine, parseTemplate)
{
    char* input = NULL;
    int input_len = 0;

    // parse parameters
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &input, &input_len) == FAILURE) {
        RETURN_NULL();
    }

    Engine_object* engine = (Engine_object*) zend_object_store_get_object(getThis() TSRMLS_CC);

	std::string path(input, input_len);
	if (path[0] != '/') {
		// TODO: this is UNIX-specific
		path = engine->basePath + "/" + path;
	}
	
    std::unique_ptr<b2::AST> ast;
    try {
        ast.reset(engine->parser.parse(path));
    } catch (b2::SyntaxError& err) {
        // TODO: pass line number and filename
        zend_throw_exception(b2_syntaxerror_class_entry, (char*) err.what(), 0);
        return;
    } catch (std::exception& ex) {
        zend_throw_exception(nullptr, (char*) ex.what(), 0);
        return;
    }

    try {
        ast.reset(optimizeAST(ast.release(), engine->basePath));
    } catch (std::exception& ex) {
        zend_throw_exception(nullptr, (char*) ex.what(), 0);
        return;
    }

    template_fn func;
    try {
        std::string templateName(input, input_len);
        func = (template_fn) engine->backend.createFunction(templateName, ast.get());
    } catch (std::exception& ex) {
        zend_throw_exception(nullptr, (char*) ex.what(), 0);
        return;
    }

    // create template object
    object_init_ex(return_value, b2_template_class_entry);

    // fill internal properties
    Template_object* templ = (Template_object*) zend_object_store_get_object(return_value TSRMLS_CC);
    templ->estimatedBufferSize = 200; // TODO
    templ->renderFunc = func;

    // add engine reference to template
    zend_update_property(b2_template_class_entry, return_value, "engine", strlen("engine"), getThis());
	// zend_update_property() will increase the refcount
}

static PHP_METHOD(Engine, addFunction)
{
    // TODO
}

/* {{{ b2_functions[] : Engine class */
ZEND_BEGIN_ARG_INFO_EX(engine_constructor, 0, 0, 1)
    ZEND_ARG_INFO(0, basePath)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(engine_parseTemplate, 0, 0, 1)
    ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(engine_addFunction, 0, 0, 1)
    ZEND_ARG_INFO(0, callable)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

static const zend_function_entry engine_functions[] = {
    PHP_ME(Engine, __construct,   engine_constructor,   ZEND_ACC_PUBLIC | ZEND_ACC_CTOR | ZEND_ACC_FINAL)
    PHP_ME(Engine, parseTemplate, engine_parseTemplate, ZEND_ACC_PUBLIC)
    PHP_ME(Engine, addFunction,   engine_addFunction,   ZEND_ACC_PUBLIC)
    PHP_FE_END
};
/* }}} */

/* {{{ Normalizer2_object_create */
static zend_object_value Template_object_create(zend_class_entry *ce TSRMLS_DC)
{
    Template_object* templ = new Template_object();

    zend_object_std_init(&templ->zo, ce TSRMLS_CC);
    object_properties_init(&templ->zo, ce);

    auto dtor = [](void *object, zend_object_handle handle TSRMLS_DC) {
        zend_objects_destroy_object((zend_object*) object, handle TSRMLS_CC);
    };

    auto freer = [](void *object TSRMLS_DC) {
        Template_object* templ = (Template_object*) object;

        zend_object_std_dtor(&templ->zo TSRMLS_CC);
        delete templ;
    };

    zend_object_value retval;
    retval.handle = zend_objects_store_put(templ, dtor, freer, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}
/* }}} */

static PHP_METHOD(Template, __construct)
{
    zend_throw_exception(NULL, "An object of this type cannot be created with the new operator", 0 TSRMLS_CC);
}

static void render_template(HashTable* assignments, Template_object* templ, zval* dest_buffer)
{
    // init buffer
    template_buffer* buffer = (template_buffer*) emalloc(sizeof(template_buffer));
    buffer->ptr = (char*) emalloc(templ->estimatedBufferSize);
    buffer->allocated_length = templ->estimatedBufferSize;
    buffer->str_length = 0;

    // run template
    templ->renderFunc(assignments, buffer);

    // fill dest_buffer
    ZVAL_STRINGL(dest_buffer, buffer->ptr, buffer->str_length, false);

    // free buffer
    efree(buffer);
}

static PHP_METHOD(Template, render)
{
    HashTable* assignments;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "H", &assignments) == FAILURE) {
        RETURN_NULL();
    }

    Template_object* templ = (Template_object*) zend_object_store_get_object(getThis() TSRMLS_CC);

    // run template
    render_template(assignments, templ, return_value);
}

static PHP_METHOD(Template, display)
{
    HashTable* assignments;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "H", &assignments) == FAILURE) {
        return;
    }

    Template_object* templ = (Template_object*) zend_object_store_get_object(getThis() TSRMLS_CC);

    // allocate buffer
    zval* buf;
    MAKE_STD_ZVAL(buf);

    // render template
    render_template(assignments, templ, buf);

    // write buffer to stdout and destroy it
    PHPWRITE(Z_STRVAL_P(buf), Z_STRLEN_P(buf));
    zval_ptr_dtor(&buf);
}

/* {{{ b2_functions[] : Template class */
ZEND_BEGIN_ARG_INFO_EX(template_constructor, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(template_render, 0, 0, 1)
    ZEND_ARG_INFO(0, assignments)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(template_display, 0, 0, 1)
    ZEND_ARG_INFO(0, assignments)
ZEND_END_ARG_INFO()

static const zend_function_entry template_functions[] = {
    PHP_ME(Template, __construct, template_constructor, ZEND_ACC_PRIVATE | ZEND_ACC_CTOR | ZEND_ACC_FINAL)
    PHP_ME(Template, render,      template_render,      ZEND_ACC_PUBLIC)
    PHP_ME(Template, display,     template_display,     ZEND_ACC_PUBLIC)
    PHP_FE_END
};
/* }}} */

static PHP_MINIT_FUNCTION(b2) /* {{{ */
{
    // TODO: implement clone handler

    zend_class_entry engine_ce;
    INIT_NS_CLASS_ENTRY(engine_ce, "b2", "Engine", engine_functions);
    engine_ce.create_object = Engine_object_create;
    b2_engine_class_entry = zend_register_internal_class(&engine_ce TSRMLS_CC);

    zend_class_entry syntaxerror_ce;
    INIT_NS_CLASS_ENTRY(syntaxerror_ce, "b2", "SyntaxError", NULL);
    b2_syntaxerror_class_entry = zend_register_internal_class_ex(&syntaxerror_ce, zend_exception_get_default(), NULL TSRMLS_CC);

    zend_class_entry template_ce;
    INIT_NS_CLASS_ENTRY(template_ce, "b2", "Template", template_functions);
    template_ce.create_object = Template_object_create;
    b2_template_class_entry = zend_register_internal_class(&template_ce TSRMLS_CC);

    zend_declare_property_null(b2_template_class_entry, "engine", strlen("engine"), ZEND_ACC_PRIVATE);

    return SUCCESS;
}
/* }}} */

static PHP_MSHUTDOWN_FUNCTION(b2) /* {{{ */
{
    return SUCCESS;
}
/* }}} */

static PHP_MINFO_FUNCTION(b2) /* {{{ */
{
    php_info_print_table_start();
    php_info_print_table_row(2, "B2 support", "enabled");
    php_info_print_table_row(2, "Version", B2_VERSION_STRING);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ b2_module_entry */
static zend_module_entry b2_module_entry = {
    STANDARD_MODULE_HEADER,
    "b2",
    NULL,
    PHP_MINIT(b2),
    PHP_MSHUTDOWN(b2),
    NULL,
    NULL,
    PHP_MINFO(b2),
    B2_VERSION_STRING,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

// module entry point
ZEND_GET_MODULE(b2)

} // anon namespace
