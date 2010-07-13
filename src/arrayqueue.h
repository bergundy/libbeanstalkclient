/**
 * =====================================================================================
 * @file   arrayqueue.h
 * @brief  header file for arrayqueue macros
 * @date   07/12/2010 03:34:11 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */
#ifndef ARRAYQUEUE_H
#define ARRAYQUEUE_H 

struct _arrayqueue {
    queue_node *nodes;
    size_t size;
    size_t used;
    off_t  front;
    off_t  rear;
};

#define AQUEUE_FULL(q)     ( (q)->used == (q)->size )
#define AQUEUE_EMPTY(q)    ( (q)->used == 0 )

#define AQUEUE_REAR_NV(q)  ( (q)->nodes + (q)->rear )
#define AQUEUE_REAR(q)     ( AQUEUE_EMPTY(q) ? NULL : AQUEUE_REAR_NV(q) )

#define AQUEUE_FRONT_NV(q) ( (q)->nodes + (q)->front )
#define AQUEUE_FRONT(q)    ( AQUEUE_FULL(q) ? NULL : AQUEUE_FRONT_NV(q) )

#define AQUEUE_FIN_GET(q) ( (q)->rear  = ( (q)->rear  + 1 ) % ( (q)->size - 1), (q)->used-- )
#define AQUEUE_FIN_PUT(q) ( (q)->front = ( (q)->front + 1 ) % ( (q)->size - 1), (q)->used++ )

#endif /* ARRAYQUEUE_H */
