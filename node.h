#pragma once
#ifndef __COMPILER_TREE_NODE_H__
#define __COMPILER_TREE_NODE_H__

/**
 * ast node data query
 *
 * the type will help node determine which type
 * the data is binded to the tree node
 *
 * the value also serves as index to access serialized parser function name
*/
typedef enum
{
    JNT_UNIT = 0,
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
    JNT_INTERFACE_BODY_DECL,
    JNT_CLASS_BODY_DECL,
    JNT_STATIC_INIT,
    JNT_BLOCK,
    JNT_CTOR_DECL,
    JNT_TYPE,
    JNT_METHOD_HEADER,
    JNT_METHOD_DECL,
    JNT_VAR_DECLARATORS,
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
    JNT_EXPRESSION_LIST,
    JNT_FOR_INIT,
    JNT_FOR_UPDATE,
    JNT_LOCAL_VAR_DECL,
    JNT_AMBIGUOUS,

    JNT_MAX,
} java_node_query;

#endif
