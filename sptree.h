#pragma once

#include <stdio.h>

typedef struct TreeNode {
    struct TreeNode* parent;
    struct TreeNode* left;
    struct TreeNode* right;
    int n;
} TreeNode;

typedef enum {
    TREE_OK = 0,
    TREE_ERR_ALLOC,
    TREE_ERR_OCCUPIED,
    TREE_ERR_INVALID_PARAM,
} TreeError;

typedef enum {
    TREE_ATTACH_AUTO,
    TREE_ATTACH_LEFT,
    TREE_ATTACH_RIGHT,
} TreeAttachMode;

TreeError tree_new_node(TreeNode** node_ptr, int n);

TreeError tree_attach(TreeNode* parent, TreeNode* child, TreeAttachMode mode);

// TODO: detach

TreeError tree_dump(TreeNode* root, FILE* file);

TreeError tree_parse(TreeNode** root, const char* str);

void tree_free(TreeNode** node_ptr);
