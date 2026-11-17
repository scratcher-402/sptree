#include <stdio.h>
#include <stdlib.h>
#include "sptree.h"

void handle_error(TreeError err) {
    if (err) {
        printf("Error in tree, code %d\n", err);
        exit(1);
    }
}

TreeNode* new_node(int n) {
    TreeNode* node;
    TreeError err = tree_new_node(&node, n);
    handle_error(err);
    return node;
}

int main() {
    TreeError err = TREE_OK;

    TreeNode* root = new_node(67);
    TreeNode* node1 = new_node(52);
    TreeNode* node2 = new_node(42);
    TreeNode* node3 = new_node(300);

    err = tree_attach(root, node1, TREE_ATTACH_LEFT);
    handle_error(err);

    err = tree_attach(node1, node2, TREE_ATTACH_LEFT);
    handle_error(err);

    err = tree_attach(node1, node3, TREE_ATTACH_RIGHT);
    handle_error(err);

    tree_dump(root, stdout);
    putchar('\n');

    err = tree_detach(node1, node3);
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

    tree_free(&root);

    return 0;
}
