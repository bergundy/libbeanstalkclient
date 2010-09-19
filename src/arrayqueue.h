/**
 * =====================================================================================
 * @file   arrayqueue.h
 * @brief  header file for arrayqueue macros
 * @date   07/12/2010 03:34:11 PM
 * @author Roey Berman, (roey.berman@gmail.com)
 * =====================================================================================
 */
#ifndef ARRAYQUEUE_H
#define ARRAYQUEUE_H 

#define AQ_DEFINE_STRUCT(struct_name, node_type) \
struct struct_name {                             \
    node_type *nodes;                            \
    size_t     size;                             \
    size_t     used;                             \
    off_t      front;                            \
    off_t      rear;                             \
}

#define AQ_NODES_FREE(q) ( (q)->size - (q)->used )
#define AQ_FULL(q)       ( (q)->used == (q)->size )
#define AQ_EMPTY(q)      ( (q)->used == 0 )

#define AQ_REAR_(q)      ( (q)->nodes + (q)->rear )
#define AQ_REAR(q)       ( AQ_EMPTY(q) ? NULL : AQ_REAR_(q) )

#define AQ_FRONT_(q)     ( (q)->nodes + (q)->front )
#define AQ_FRONT(q)      ( AQ_FULL(q) ? NULL : AQ_FRONT_(q) )

#define AQ_DEQ_FIN(q)    ( (q)->rear  = ( (q)->rear  + 1 ) % (q)->size, (q)->used-- )
#define AQ_ENQ_FIN(q)    ( (q)->front = ( (q)->front + 1 ) % (q)->size, (q)->used++ )

#endif /* ARRAYQUEUE_H */
