#include <stdio.h>
#include <stdlib.h>

typedef int TreeElement_t;
#define SPTREE_IMPLEMENTATION
#define SPDEBUG_IMPLEMENTATION
#include "sptree.h"

const NodeSerializer_t tree_element_serialize = tree_serialize_int;
const NodeDeserializer_t tree_element_deserialize = tree_deserialize_int;
const NodeDestructor_t tree_element_destroy = NULL;
const NodeDotSerializer_t tree_element_dot_serialize = tree_serialize_int;

void handle_error(TreeError err) {
    if (err) {
        printf("Error in tree, code %d\n", err);
        exit(1);
    }
}

int walk_handler(int* n, TreeWalkStatus status, size_t depth) {
    if (status == TWS_PRE) printf("value: %d, depth: %zu\n", *n, depth);

    return 0;
}

TreeNode* new_node(int n) {
    TreeNode* node;
    TreeError err = tree_new_node(&node, &n);
    handle_error(err);
    return node;
}

int main() {
    debug_config.log_file_path = "tree_demo.log";
    $initp("Tree demo");

    TreeError err = TREE_OK;

    TreeNode* root = new_node(67);
    TreeNode* node1 = new_node(52);
    TreeNode* node2 = new_node(42);
    TreeNode* node3 = new_node(300);
    TreeNode* node4 = new_node(228);
    TreeNode* node5 = new_node(1488);

    err = tree_attach(root, node1, TREE_ATTACH_LEFT);
    handle_error(err);

    err = tree_attach(node1, node2, TREE_ATTACH_LEFT);
    handle_error(err);

    err = tree_attach(node1, node3, TREE_ATTACH_RIGHT);
    handle_error(err);

    err = tree_attach(node2, node4, TREE_ATTACH_LEFT);
    handle_error(err);

    err = tree_attach(node2, node5, TREE_ATTACH_RIGHT);
    handle_error(err);

    tree_dump(root, stdout);
    putchar('\n');

    tree_walk(root, walk_handler);

    err = tree_detach(node3);
    handle_error(err);
    tree_free(&node3);

    tree_dump(root, stdout);
    putchar('\n');

    tree_free(&root);

    char buf[1024] = {0};
    fgets((char*)&buf, 1023, stdin);
    buf[1023] = '\0';

    err = tree_parse(&root, (char*)buf);
    if (err) {
        puts("Parsing error");
        return 1;
    }

    tree_dump(root, stdout);
    putchar('\n');

    tree_dot_dump(root);

    tree_free(&root);

    return 0;
}
