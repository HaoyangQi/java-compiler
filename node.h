#pragma once
#ifndef __COMPILER_TREE_NODE_H__
#define __COMPILER_TREE_NODE_H__

#include "types.h"
#include "token.h"
#include "expression.h"

/**
 * ast node data query
 *
 * the type will help node determine which type
 * the data is binded to the tree node
*/
typedef enum
{
    JNT_UNIT,
    JNT_NAME_UNIT,
    JNT_NAME,
    JNT_CLASS_TYPE_UNIT,
    JNT_CLASS_TYPE,
    JNT_INTERFACE_TYPE_UNIT,
    JNT_INTERFACE_TYPE,
    JNT_INTERFACE_TYPE_LIST,
    JNT_PKG_DECL,
    JNT_IMPORT_DECL,
    JNT_TOP_LEVEL,
    JNT_CLASS_DECL,
    JNT_INTERFACE_DECL,
    JNT_CLASS_EXTENDS,
    JNT_CLASS_IMPLEMENTS,
    JNT_CLASS_BODY,
    JNT_INTERFACE_EXTENDS,
    JNT_INTERFACE_BODY,
    JNT_CLASS_BODY_DECL,
    JNT_STATIC_INIT,
    JNT_BLOCK,
    JNT_CTOR_DECL,
    JNT_TYPE,
    JNT_METHOD_DECL,
    JNT_FIELD_DECL,
    JNT_FORMAL_PARAM_LIST,
    JNT_FORMAL_PARAM,
    JNT_THROWS,
    JNT_ARGUMENT_LIST,
    JNT_CTOR_BODY,
    JNT_CTOR_INVOCATION,
    JNT_METHOD_BODY,
    JNT_VAR_DECL,
    JNT_ARRAY_INIT,
    JNT_PRIMARY,
    JNT_PRIMARY_SIMPLE,
    JNT_PRIMARY_COMPLEX,
    JNT_PRIMARY_CREATION,
    JNT_PRIMARY_ARR_CREATION,
    JNT_PRIMARY_CLS_CREATION,
    JNT_PRIMARY_METHOD_INVOKE,
    JNT_PRIMARY_ARR_ACCESS,
    JNT_PRIMARY_CLS_LITERAL,
    JNT_EXPRESSION,
    JNT_OPERATOR,
    JNT_STATEMENT,
    JNT_STATEMENT_EMPTY,
    JNT_STATEMENT_SWITCH,
    JNT_STATEMENT_DO,
    JNT_STATEMENT_BREAK,
    JNT_STATEMENT_CONTINUE,
    JNT_STATEMENT_RETURN,
    JNT_STATEMENT_SYNCHRONIZED,
    JNT_STATEMENT_THROW,
    JNT_STATEMENT_TRY,
    JNT_STATEMENT_IF,
    JNT_STATEMENT_WHILE,
    JNT_STATEMENT_FOR,
    JNT_STATEMENT_LABEL,
    JNT_STATEMENT_EXPRESSION,
    JNT_STATEMENT_VAR_DECL,
    JNT_STATEMENT_CATCH,
    JNT_STATEMENT_FINALLY,
    JNT_SWITCH_LABEL,
    JNT_FOR_INIT,
    JNT_FOR_UPDATE,
} java_node_query;

/**
 * Name Unit
 *
 * unit for Name sequence
 * e.g. ID1.ID2.ID3
*/
typedef struct
{
    java_token id;
} node_data_name_unit;

/**
 * Class Type Unit
*/
typedef struct
{
    java_token id;
} node_data_class_type_unit;

/**
 * Interface Type Unit
*/
typedef struct
{
    java_token id;
} node_data_interface_type_unit;

/**
 * Import Declaration
*/
typedef struct
{
    /* on demand import */
    bool on_demand;
} node_data_import_decl;

/**
 * Top Level
*/
typedef struct
{
    /* modifier data */
    lbit_flag modifier;
} node_data_top_level;

/**
 * Class Declaration
*/
typedef struct
{
    /* class name */
    java_token id;
} node_data_class_declaration;

/**
 * Interface Declaration
*/
typedef struct
{
    /* interface name */
    java_token id;
} node_data_interface_declaration;

/**
 * Class Body Declaration
*/
typedef struct
{
    /* modifier data */
    lbit_flag modifier;
} node_data_class_body_declaration;

/**
 * Constructor Declaration
*/
typedef struct
{
    /* constructor name */
    java_token id;
} node_data_constructor_declaration;

/**
 * Type
*/
typedef struct
{
    /* primitive type, JLT_MAX if not */
    java_lexeme_type primitive;
    /* array dimension */
    size_t dimension;
} node_data_type;

/**
 * Formal Parameter
*/
typedef struct
{
    /* parameter name */
    java_token id;
    /* array dimension */
    size_t dimension;
} node_data_formal_parameter;

/**
 * Method Declaration
*/
typedef struct
{
    /* method name */
    java_token id;
} node_data_method_declaration;

/**
 * Variable Declarator
*/
typedef struct
{
    /* parameter name */
    java_token id;
    /* array dimension */
    size_t dimension;
} node_data_variable_declarator;

/**
 * Primary: Simple
*/
typedef struct
{
    /* primary type */
    java_lexeme_type type;
} node_data_primary_simple;

/**
 * Primary: Field Access
*/
typedef struct
{
    /* token content */
    java_token token;
} node_data_primary_complex;

/**
 * Primary: Field Access
*/
typedef struct
{
    /* variadic length dimension count */
    size_t dims_var;
} node_data_primary_array_creation;

/**
 * Expression: Operator
*/
typedef struct
{
    operator_id op;
} node_data_operator;

/**
 * Statement
*/
typedef struct
{
    /* ID, optional */
    java_token id;
} node_data_statement;

/**
 * Constructor Invocation
*/
typedef struct
{
    /* true if calling from super class, this class otherwise */
    bool is_super;
} node_data_constructor_invoke;

/**
 * Switch Label
*/
typedef struct
{
    /* true if default label, case label otherwise */
    bool is_default;
} node_data_switch_label;

#endif
