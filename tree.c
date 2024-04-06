#include "tree.h"

/**
 * AST node data token allocator
*/
static inline java_token* __new_node_data_token()
{
    ALLOCATE(java_token, t);
    init_token(t);
    return t;
}

static inline node_data_id* __new_node_data_id_simple()
{
    ALLOCATE(node_data_id, d);
    d->simple = JLT_MAX;
    return d;
}

static inline node_data_id* __new_node_data_id_complex()
{
    ALLOCATE(node_data_id, d);
    d->complex = __new_node_data_token();
    return d;
}

static inline node_data_import* __new_node_data_import()
{
    ALLOCATE(node_data_import, d);
    d->on_demand = false;
    return d;
}

static inline node_data_top_level* __new_node_data_top_level()
{
    ALLOCATE(node_data_top_level, d);
    d->modifier = JLT_UNDEFINED;
    return d;
}

static inline node_data_declarator* __new_node_data_declarator_simple()
{
    ALLOCATE(node_data_declarator, d);
    d->id.simple = JLT_MAX;
    d->dimension = 0;
    return d;
}

static inline node_data_declarator* __new_node_data_declarator_complex()
{
    ALLOCATE(node_data_declarator, d);
    d->id.complex = __new_node_data_token();
    d->dimension = 0;
    return d;
}

static inline node_data_expression* __new_node_data_expression()
{
    ALLOCATE(node_data_expression, d);
    d->op = OPID_UNDEFINED;
    return d;
}

static inline node_data_contructor_invoke* __new_node_data_contructor_invoke()
{
    ALLOCATE(node_data_contructor_invoke, d);
    d->is_super = false;
    return d;
}

static inline node_data_switch_label* __new_node_data_switch_label()
{
    ALLOCATE(node_data_switch_label, d);
    d->is_default = false;
    return d;
}

static inline node_data_ambiguity* __new_node_data_ambiguity()
{
    ALLOCATE(node_data_ambiguity, d);
    d->error = NULL;
    return d;
}

/**
 * AST node data generator
 *
*/
static void init_node_data(tree_node* node)
{
    switch (node->type)
    {
        case JNT_NAME_UNIT:
        case JNT_CLASS_TYPE_UNIT:
        case JNT_INTERFACE_TYPE_UNIT:
        case JNT_CLASS_DECL:
        case JNT_INTERFACE_DECL:
        case JNT_PRIMARY_COMPLEX:
        case JNT_STATEMENT_BREAK:
        case JNT_STATEMENT_CONTINUE:
        case JNT_STATEMENT_LABEL:
            node->data.id = __new_node_data_id_complex();
            break;
        case JNT_IMPORT_DECL:
            node->data.import = __new_node_data_import();
            break;
        case JNT_TOP_LEVEL:
        case JNT_INTERFACE_BODY_DECL:
        case JNT_CLASS_BODY_DECL:
            node->data.top_level = __new_node_data_top_level();
            break;
        case JNT_CTOR_DECL:
        case JNT_METHOD_HEADER:
        case JNT_FORMAL_PARAM:
        case JNT_VAR_DECL:
            node->data.declarator = __new_node_data_declarator_complex();
            break;
        case JNT_TYPE:
        case JNT_PRIMARY_ARR_CREATION:
            node->data.declarator = __new_node_data_declarator_simple();
            break;
        case JNT_PRIMARY_SIMPLE:
            node->data.id = __new_node_data_id_simple();
            break;
        case JNT_EXPRESSION:
            node->data.expression = __new_node_data_expression();
            break;
        case JNT_CTOR_INVOCATION:
            node->data.constructor_invoke = __new_node_data_contructor_invoke();
            break;
        case JNT_SWITCH_LABEL:
            node->data.switch_label = __new_node_data_switch_label();
            break;
        case JNT_AMBIGUOUS:
            node->data.ambiguity = __new_node_data_ambiguity();
            break;
        default:
            // no-op, hence no data
            break;
    }
}

/**
 * Node Data Deleter
*/
static void release_node_data(tree_node* node)
{
    switch (node->type)
    {
        case JNT_NAME_UNIT:
        case JNT_CLASS_TYPE_UNIT:
        case JNT_INTERFACE_TYPE_UNIT:
        case JNT_CLASS_DECL:
        case JNT_INTERFACE_DECL:
        case JNT_PRIMARY_COMPLEX:
        case JNT_STATEMENT_BREAK:
        case JNT_STATEMENT_CONTINUE:
        case JNT_STATEMENT_LABEL:
            delete_token(node->data.id->complex);
            free(node->data.id);
            break;
        case JNT_IMPORT_DECL:
            free(node->data.import);
            break;
        case JNT_TOP_LEVEL:
        case JNT_INTERFACE_BODY_DECL:
        case JNT_CLASS_BODY_DECL:
            free(node->data.top_level);
            break;
        case JNT_CTOR_DECL:
        case JNT_METHOD_HEADER:
        case JNT_FORMAL_PARAM:
        case JNT_VAR_DECL:
            delete_token(node->data.declarator->id.complex);
            free(node->data.declarator);
            break;
        case JNT_TYPE:
        case JNT_PRIMARY_ARR_CREATION:
            free(node->data.declarator);
            break;
        case JNT_PRIMARY_SIMPLE:
            free(node->data.id);
            break;
        case JNT_EXPRESSION:
            free(node->data.expression);
            break;
        case JNT_CTOR_INVOCATION:
            free(node->data.constructor_invoke);
            break;
        case JNT_SWITCH_LABEL:
            free(node->data.switch_label);
            break;
        case JNT_AMBIGUOUS:
            free(node->data.ambiguity);
            break;
        default:
            // no-op, hence no data
            break;
    }
}

/**
 * init tree node fields
*/
void init_tree_node(tree_node* node)
{
    memset(node, 0, sizeof(tree_node));
    node->type = JNT_MAX;
    node->ambiguous = true;
}

/**
 * add a child node to the target node
 *
 * NOTE: if node=NULL, child will be deleted completely
*/
void tree_node_add_child(tree_node* node, tree_node* child)
{
    // if child is NULL, tree will be broken, so it is disallowed
    if (!child) { return; }

    // if node is NULL, free child
    if (!node)
    {
        tree_node_delete(child);
        return;
    }

    child->prev_sibling = node->last_child;
    child->next_sibling = NULL;

    if (node->last_child)
    {
        node->last_child->next_sibling = child;
    }
    else
    {
        node->first_child = child;
    }

    node->last_child = child;
    node->ambiguous = node->ambiguous || child->ambiguous;
}

/**
 * add a child node as first child to the target node
 *
 * NOTE: if node=NULL, child will be deleted completely
*/
void tree_node_add_first_child(tree_node* node, tree_node* child)
{
    // if child is NULL, tree will be broken, so it is disallowed
    if (!child) { return; }

    // if node is NULL, free child
    if (!node)
    {
        tree_node_delete(child);
        return;
    }

    child->prev_sibling = NULL;
    child->next_sibling = node->first_child;

    if (node->first_child)
    {
        node->first_child->prev_sibling = child;
    }
    else
    {
        node->last_child = child;
    }

    node->first_child = child;
    node->ambiguous = node->ambiguous || child->ambiguous;
}

/**
 * recursively delete ast and data attached to each node
 *
 * NOTE: always try deletion subtree before attaching, otherwise it will cause avalanche
 * effect
*/
void tree_node_delete(tree_node* node)
{
    if (!node)
    {
        return;
    }

    // go deeper first
    tree_node_delete(node->first_child);

    // then go to next sibling
    tree_node_delete(node->next_sibling);

    // delete data
    release_node_data(node);

    // finally, delete self
    free(node);
}

/**
 * AST node generator
*/
tree_node* ast_node_new(java_node_query type)
{
    tree_node* node = (tree_node*)malloc_assert(sizeof(tree_node));

    // init node
    init_tree_node(node);
    node->type = type;

    // init node data
    init_node_data(node);

    return node;
}
