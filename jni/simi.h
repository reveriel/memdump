/** similarity **/
#ifndef _SIMI_H
#define _SIMI_H

#include <stdint.h>

// this is not a set!
// this is a multi set.
struct Set;
struct Data;

struct Set *set_init();
void set_free(struct Set *s);

// this is not a set!
// this is a multi set.
void set_add(struct Set *s, struct Data *data);
int set_in(struct Set *s, struct Data *data);
void set_print(struct Set *s);
int set_size(struct Set *s);

struct Data *set_first(struct Set *s);
struct Data *set_next(struct Data *d);

// return
// how many elem in a can be found in b.
// set_found_in({ a, b, b, c},  {b ,b } ) = 2
// set_found_in({a,a,a,a}, {a}) = 4
int set_found_in(struct Set *a, struct Set *b);
/*
how many element in a  are also in b ?
*/
// int set_a_in_b(struct Set *a, struct Set *b);
// int set_size(struct Set *s);

/*
 jaccard index
            A \cap B
 =   ---------------------
            A \cup B
*/
// set_jaccard({a,b, b}, {b,b, c}) = b,b/ a, b,b, c = 0.5
double set_jaccard(struct Set *a, struct Set *b);

struct Data *data_init(uint32_t v);
void data_free(struct Data *);
void data_print(struct Data *);

#endif