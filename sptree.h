#pragma once

#ifdef EDITOR
typedef int TreeElement_t;
#define SPTREE_IMPLEMENTATION
#endif

#include <stdio.h>
#include "lib/spdebug2/spdebug.h"

typedef struct TreeNode {
    struct TreeNode* parent;
    struct TreeNode* left;
    struct TreeNode* right;
    TreeElement_t el;
} TreeNode;

typedef enum {
    TREE_OK = 0,
    TREE_ERR_ALLOC,
    TREE_ERR_ATTACH,
    TREE_ERR_DETACH,
    TREE_ERR_INVALID_PARAM,
    TREE_ERR_PARSE,
    TREE_ERR_DUMP,
    TREE_ERR_WRITE,
} TreeError;

typedef enum {
    TREE_ATTACH_AUTO,
    TREE_ATTACH_LEFT,
    TREE_ATTACH_RIGHT,
} TreeAttachMode;

typedef int (*TreeSerializer_t)(TreeElement_t* el, FILE* file);
typedef int (*TreeDeserializer_t)(const char** strp, TreeElement_t* el);
typedef void (*NodeDestructor_t)(TreeElement_t* el);

extern TreeSerializer_t tree_element_serialize;
extern TreeDeserializer_t tree_element_deserialize;
extern NodeDestructor_t tree_element_destroy;

TreeError tree_new_node(TreeNode** node_ptr, TreeElement_t* elem);

TreeError tree_attach(TreeNode* parent, TreeNode* child, TreeAttachMode mode);

TreeError tree_detach(TreeNode* child);

TreeError tree_dump(TreeNode* root, FILE* file);

TreeError tree_dot_dump(TreeNode* node);

TreeError tree_parse(TreeNode** root, const char* str);

void tree_free(TreeNode** node_ptr);

int tree_serialize_int(int* num, FILE* file);
int tree_deserialize_int(const char** strp, int* num);

#ifdef SPTREE_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "sptree.h"

TreeError tree_new_node(TreeNode** node_ptr, TreeElement_t* el) {
    assert(node_ptr != NULL);

    TreeNode* node = (TreeNode*)calloc(1, sizeof(TreeNode));
    if (!node) {
        $errno;
        $err("[tree_new_node] Couldn't allocate new node");
        return TREE_ERR_ALLOC;
    }

    memcpy(&node->el, el, sizeof(TreeElement_t));
    *node_ptr = node;

    return TREE_OK;
}

TreeError tree_attach(TreeNode* parent, TreeNode* child, TreeAttachMode mode) {
    assert(parent != NULL);
    assert(child != NULL);

    TreeError err = TREE_OK;

    switch (mode) {
        case TREE_ATTACH_AUTO:

            err = tree_attach(parent, child, TREE_ATTACH_LEFT);
            if (!err) return TREE_OK;

            if (err == TREE_ERR_ATTACH) {
                err = tree_attach(parent, child, TREE_ATTACH_RIGHT);
                return err;
            } else {
                return err;
            }

        case TREE_ATTACH_LEFT:
            if (parent->left != NULL) return TREE_ERR_ATTACH;
            if (child->parent != NULL) return TREE_ERR_ATTACH;
            if (parent->right == child) return TREE_ERR_ATTACH;

            parent->left = child;
            child->parent = parent;

            return TREE_OK;

        case TREE_ATTACH_RIGHT:
            if (parent->right != NULL) return TREE_ERR_ATTACH;
            if (child->parent != NULL) return TREE_ERR_ATTACH;
            if (parent->left == child) return TREE_ERR_ATTACH;

            parent->right = child;
            child->parent = parent;

            return TREE_OK;

        default:
            return TREE_ERR_INVALID_PARAM;
    }
}

TreeError tree_detach(TreeNode* child) {
    assert(child != NULL);

    if (child->parent == NULL) {
        return TREE_ERR_DETACH;
        $err("[tree_detach] Node %p has no parent", child);
    }

    TreeNode* parent = child->parent;

    if (parent->left == child) {
        parent->left = NULL;
    } else if (parent->right == child) {
        parent->right = NULL;
    } else {
        $fatal("[tree_detach] Could not find child pointer in parent node. Tree structure may be corrupted.", child);
        abort();
    }

    child->parent = NULL;

    return TREE_OK;
}

TreeError tree_dump(TreeNode* root, FILE* file) {
    if (root == NULL) {
        fprintf(file, "nil");
        return TREE_OK;
    } else {
        TreeError err = TREE_OK;

        int ser_err = tree_element_serialize(&root->el, file);
        if (ser_err) return TREE_ERR_DUMP;
        fprintf(file, " (");

        err = tree_dump(root->left, file);
        if (err) return err;

        fprintf(file, ", ");

        err = tree_dump(root->right, file);
        if (err) return err;

        fprintf(file, ")");

        return TREE_OK;
    }
}

static inline void skip_spaces(const char** strp) {
    while (isspace(**strp)) *strp += 1;
}

static TreeError read_nil(const char** strp) {
    if (strncmp(*strp, "nil", 3) == 0) {
        *strp += 3;
        return TREE_OK;
    } else {
        return TREE_ERR_PARSE;
    }
}

static TreeError recursive_tree_parse(const char** strp, TreeNode** node_ptr, TreeNode* parent) {
    skip_spaces(strp);

    TreeError err = read_nil(strp);
    if (!err) {
        *node_ptr = NULL;
        return TREE_OK;
    }

    TreeElement_t el;
    int deser_err = tree_element_deserialize(strp, &el);
    if (deser_err) return TREE_ERR_PARSE;

    TreeNode* curr_node = NULL;
    TreeNode* left = NULL;
    TreeNode* right = NULL;
    err = tree_new_node(&curr_node, &el);
    if (err) goto cleanup;

    skip_spaces(strp);
    if (**strp != '(') goto error;
    *strp += 1;

    err = recursive_tree_parse(strp, &left, curr_node);
    if (err) goto error;

    skip_spaces(strp);
    if (**strp != ',') goto error;
    *strp += 1;
    skip_spaces(strp);

    err = recursive_tree_parse(strp, &right, curr_node);
    if (err) goto error;

    skip_spaces(strp);
    if (**strp != ')') goto error;
    *strp += 1;

    // Attach nodes
    if (left != NULL) tree_attach(curr_node, left, TREE_ATTACH_LEFT);
    if (right != NULL) tree_attach(curr_node, right, TREE_ATTACH_RIGHT);

    *node_ptr = curr_node;
    return TREE_OK;

error:
    err = TREE_ERR_PARSE;
    goto cleanup;

cleanup:
    $err("[libsptree] parsing stopped at %.20s..., error code %d", *strp, err);
    tree_free(&curr_node);
    tree_free(&left);
    tree_free(&right);
    return err;
}

TreeError tree_parse(TreeNode** root, const char* str) {
    const char** strp = &str;

    return recursive_tree_parse(&str, root, NULL);
}

void tree_free(TreeNode** node_ptr) {
    assert(node_ptr != NULL);

    TreeNode* node = *node_ptr;
    if (node == NULL) return;

    if (node->parent != NULL) {
        tree_detach(node);
    }

    tree_free(&node->left);
    tree_free(&node->right);

    if (tree_element_destroy) tree_element_destroy(&node->el);

    free(node);
    *node_ptr = NULL;
}

static TreeError recursive_tree_dot_dump(TreeNode* node, FILE* file) {
    // Define node
    fprintf(file, "Node%p [shape=box, style=rounded, label=\"", node);
    tree_element_serialize(&node->el, file);
    fprintf(file, "\"];\n");

    TreeError err = TREE_OK;

    if (node->left) {
        fprintf(file, "Node%p -> Node%p;\n", node, node->left);
        err = recursive_tree_dot_dump(node->left, file);
        if (err) return err;
    }

    if (node->right) {
        fprintf(file, "Node%p -> Node%p;\n", node, node->right);
        err = recursive_tree_dot_dump(node->right, file);
        if (err) return err;
    }

    return TREE_OK;
}

TreeError tree_dot_dump(TreeNode* node) {
    assert(node != NULL);

    TreeNode* root = node;
    while (root->parent != NULL) root = root->parent;

    FILE* file = fopen("tree_dump.dot", "w");
    if (!file) {
        $errno;
        $err("[tree_dot_dump] Error while opening file");
        return TREE_ERR_WRITE;
    }

    fprintf(file,   "digraph Tree {\n"
                    "nodesep=0.5;\n"
    );

    TreeError err = recursive_tree_dot_dump(root, file);
    if (err) return err;

    fprintf(file, "};\n");

    fclose(file);

    return TREE_OK;
}

int tree_serialize_int(int* num, FILE* file) {
    int result = fprintf(file, "%d", *num);
    if (result == EOF) {
        $errno;
        $err("[tree_serialize_int] Error while writing integer to file");
        return debug_saved_errno;
    }

    return 0;
}

int tree_deserialize_int(const char** strp, int* num) {
    char* num_end = NULL;

    long long_num = strtol(*strp, &num_end, 10);

    if (*strp == num_end) return TREE_ERR_PARSE;
    if (long_num > INT_MAX || long_num < INT_MIN) {
        $err("[tree_deserialize_int] Passed integer is too big");
        return TREE_ERR_PARSE;
    }

    *num = long_num;
    *strp = num_end;
    return TREE_OK;
}

#endif
