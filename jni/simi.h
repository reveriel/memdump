/** similarity **/
#ifndef _SIMI_H
#define _SIMI_H

#include <stdint.h>

struct Set;
struct Data;


/* super simple interface !  (0 w 0  */

struct Set * set_init();
void set_free(struct Set *s);
void set_add(struct Set *s, struct Data *data);
int set_in(struct Set *s, struct Data * data);
void set_print(struct Set *s);
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
double set_jaccard(struct Set *a, struct Set *b);


struct Data *data_init(uint32_t v);
void data_free(struct Data *);
void data_print(struct Data *);


#endif