#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "sptree.h"

TreeError tree_new_node(TreeNode** node_ptr, int n) {
    assert(node_ptr != NULL);

    TreeNode* node = calloc(1, sizeof(TreeNode));
    if (!node) {
        return TREE_ERR_ALLOC;
    }

    node->n = n;
    *node_ptr = node;

    return TREE_OK;
}

TreeError tree_attach(TreeNode* parent, TreeNode* child, TreeAttachMode mode) {
    assert(parent != NULL);
    assert(child != NULL);

    switch (mode) {
        case TREE_ATTACH_AUTO:
            TreeError err = TREE_OK;

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

TreeError tree_detach(TreeNode* parent, TreeNode* child) {
    assert(parent != NULL);
    assert(child != NULL);

    if (child->parent != parent) {
        return TREE_ERR_DETACH;
    }
    child->parent = NULL;

    if (parent->left == child) {
        parent->left = NULL;
    } else if (parent->right == child) {
        parent->right = NULL;
    } else {
        return TREE_ERR_DETACH;
    }

    return TREE_OK;
}

TreeError tree_dump(TreeNode* root, FILE* file) {
    if (root == NULL) {
        fprintf(file, "nil");
        return TREE_OK;
    } else {
        TreeError err = TREE_OK;
        fprintf(file, "%d (", root->n);

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

static TreeError tree_parse_int(const char** strp, int* num) {
    char* num_end = NULL;

    long long_num = strtol(*strp, &num_end, 10);

    if (*strp == num_end) return TREE_ERR_PARSE;
    if (long_num > INT_MAX || long_num < INT_MIN) return TREE_ERR_PARSE;

    *num = long_num;
    *strp = num_end;
    return TREE_OK;
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

    int n;
    err = tree_parse_int(strp, &n);
    if (err) return err;

    TreeNode* curr_node = NULL;
    TreeNode* left = NULL;
    TreeNode* right = NULL;
    err = tree_new_node(&curr_node, n);
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
    fprintf(stderr, "[libsptree] parsing stopped at %s, error code %d\n", *strp, err);
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
        tree_detach(node->parent, node);
    }

    tree_free(&node->left);
    tree_free(&node->right);

    free(node);
    *node_ptr = NULL;
}
