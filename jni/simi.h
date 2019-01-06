/** similarity **/
#ifndef _SIMI_H
#define _SIMI_H

#include <stdint.h>

// this is not a set!
// this is a multi set.
struct Set;
struct Data;

struct Set * set_init();
void set_free(struct Set *s);

// this is not a set!
// this is a multi set.
void set_add(struct Set *s, struct Data *data);
int set_in(struct Set *s, struct Data * data);
void set_print(struct Set *s);

// return how many elements are in common
// set_common({ a, b, b, c},  {b ,b } ) = 2
int set_common(struct  Set *a, struct Set *b);
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