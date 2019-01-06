#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"

struct data {
    int n;
    struct rb_node node;
};

// search by 'n'
static  struct data * rb_search_data(struct rb_root *root, int n)
{
    struct rb_node *node = root->rb_node;

    while (node) {
        struct data *cur_data = rb_entry(node, struct data, node);
        if (n < cur_data->n)
            node = node->rb_left;
        else if (n > cur_data->n)
            node = node->rb_right;
        else
            return cur_data;
    }
    return NULL;
}

static void rb_insert_data(struct rb_root *root, struct data *new)
{
    struct rb_node **link = &root->rb_node, *parent = NULL; // init parent! important!
    int value = new->n;

    while (*link) {
        parent = *link;
        struct data *cur_data = rb_entry(parent, struct data, node);
        if (cur_data->n > value)
            link = &parent->rb_left;
        else
            link = &parent->rb_right;
    }

    rb_link_node(&new->node, parent, link);
    rb_insert_color(&new->node, root);
}

struct rb_root the_root = RB_ROOT;

static void insert_int(int value) {
    struct data *d = (struct data*)malloc(sizeof(struct data));
    d->n = value;
    rb_insert_data(&the_root, d);
}

int main(int argc, char const *argv[])
{
    // int array[] = {13};
    int array[] = {13, 4, 5, 6, 778, 8, 89, 9, 0, 77, 54, 24, 3, 43, 5};
    int len = sizeof(array) / sizeof(int);
    printf("data : [");
    for (int i = 0; i < len ; i++) {
         printf("%d ",  array[i]);
    }
    printf("]\n");

    for (int i = 0; i < len; i++) {
        insert_int(array[i]);
    }

    printf("after :\n");
    for (struct rb_node* p = rb_first(&the_root); p; p = rb_next(p)) {
        struct data *data = rb_entry(p, struct data, node);
        printf("%d ", data->n);
    }

    return 0;
}
