/*
 * model.h <z64.me>
 *
 * model types and functions
 *
 */

#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED 1

struct material;
struct vertex;
struct triangle;
struct group;
struct room;

void room_flatten(struct room *room);
void room_merge(struct room *dst, struct room *src);
struct room *room_load(const char *fn);
void room_free(struct room *room);
void room_writeWavefront(struct room *room, const char *outfn);

#endif /* MODEL_H_INCLUDED */
