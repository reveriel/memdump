#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "simi.h"
#include "rbtree.h"

struct Set
{
    struct rb_root root;
    int size;
};

struct Data
{
    uint32_t v; // search by v;
    struct rb_node node;
};

// search by 'n'
static struct Data *rb_search_data(struct rb_root *root, uint32_t v)
{
    struct rb_node *node = root->rb_node;

    while (node)
    {
        struct Data *cur_data = rb_entry(node, struct Data, node);
        if (v < cur_data->v)
            node = node->rb_left;
        else if (v > cur_data->v)
            node = node->rb_right;
        else
            return cur_data;
    }
    return NULL;
}

static void rb_insert_data(struct rb_root *root, struct Data *new)
{
    struct rb_node **link = &root->rb_node, *parent = NULL; // init parent! important!
    uint32_t value = new->v;

    while (*link)
    {
        parent = *link;
        struct Data *cur_data = rb_entry(parent, struct Data, node);
        if (cur_data->v > value)
            link = &parent->rb_left;
        else
            link = &parent->rb_right;
    }

    rb_link_node(&new->node, parent, link);
    rb_insert_color(&new->node, root);
}

void set_add(struct Set *s, struct Data *data)
{
    s->size++;
    rb_insert_data(&s->root, data);
}

int set_in(struct Set *s, struct Data *data)
{
    struct Data *d = rb_search_data(&s->root, data->v);
    return d != NULL;
}

struct Set *set_init()
{
    struct Set *s = (struct Set *)malloc(sizeof(struct Set));
    s->root = RB_ROOT;
    s->size = 0;
    return s;
}

int set_size(struct Set *s)
{
    return s->size;
}

void set_free(struct Set *s)
{
    while (s->root.rb_node != NULL)
    {
        struct Data *data = rb_entry(s->root.rb_node, struct Data, node);
        rb_erase(s->root.rb_node, &s->root);
        free(data);
    }
    free(s);
}

void set_print(struct Set *s)
{
    struct rb_root *root = &s->root;
    printf("{ ");
    for (struct rb_node *p = rb_first(root); p; p = rb_next(p))
    {
        struct Data *data = rb_entry(p, struct Data, node);
        data_print(data);
        printf(" ");
    }
    printf(" }\n");
}

/*
 jaccard index
          |  A \cap B |
 =   ---------------------
          |  A \cup B |

    x = in a not in b
    y = in a in b
    z = in b not in a
    return ( y ) / (x + y + z);
*/
double set_jaccard(struct Set *a, struct Set *b)
{
    int x = 0, y = 0;
    // for data in set 'a', search in set 'b';
    for (struct rb_node *p = rb_first(&a->root); p; p = rb_next(p))
    {
        struct Data *data = rb_entry(p, struct Data, node);
        if (set_in(b, data))
            ++y;
        else
            ++x;
    }
    int z = 0;
    for (struct rb_node *p = rb_first(&b->root); p; p = rb_next(p))
    {
        struct Data *data = rb_entry(p, struct Data, node);
        if (!set_in(a, data))
            ++z;
    }

    return (double)y / (double)(x + y + z);
}

// return size of a \cap b
int set_common(struct Set *a, struct Set *b)
{
    int cnt = 0;
    for (struct rb_node *p = rb_first(&a->root); p; p = rb_next(p))
    {
        struct Data *data = rb_entry(p, struct Data, node);
        if (set_in(b, data))
            ++cnt;
    }
    return cnt;
}

struct Data *set_first(struct Set *s)
{
    return rb_entry(rb_first(&s->root), struct Data, node);
}

struct Data *set_next(struct Data *d)
{
    struct rb_node *n = rb_next(&d->node);
    if (!n)
        return NULL;
    return rb_entry(rb_next(&d->node), struct Data, node);
}

struct Data *data_init(uint32_t v)
{
    struct Data *d = (struct Data *)malloc(sizeof(struct Data));
    d->v = v;
    return d;
}

void data_free(struct Data *d)
{
    free(d);
}

void data_print(struct Data *d)
{
    printf("%u", d->v);
}

