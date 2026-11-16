#include <assert.h>
#include <stdlib.h>
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

// TODO: add more checks
TreeError tree_attach(TreeNode* parent, TreeNode* child, TreeAttachMode mode) {
    assert(parent != NULL);
    assert(child != NULL);

    switch (mode) {
        case TREE_ATTACH_AUTO:
            TreeError err = TREE_OK;

            err = tree_attach(parent, child, TREE_ATTACH_LEFT);
            if (!err) return TREE_OK;

            if (err == TREE_ERR_OCCUPIED) {
                err = tree_attach(parent, child, TREE_ATTACH_RIGHT);
                return err;
            } else {
                return err;
            }

        case TREE_ATTACH_LEFT:
            if (parent->left != NULL) return TREE_ERR_OCCUPIED;
            if (child->parent != NULL) return TREE_ERR_OCCUPIED;

            parent->left = child;
            child->parent = parent;

            return TREE_OK;

        case TREE_ATTACH_RIGHT:
            if (parent->right != NULL) return TREE_ERR_OCCUPIED;
            if (child->parent != NULL) return TREE_ERR_OCCUPIED;

            parent->right = child;
            child->parent = parent;

            return TREE_OK;

        default:
            return TREE_ERR_INVALID_PARAM;
    }
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

// TODO: implement tree_parse

void tree_free(TreeNode** node_ptr) {
    assert(node_ptr != NULL);

    TreeNode* node = *node_ptr;
    if (node == NULL) return;

    if (node->parent != NULL) {
        // TODO: detach and warning
    }

    tree_free(&node->left);
    tree_free(&node->right);

    free(node);
    *node_ptr = NULL;
}
