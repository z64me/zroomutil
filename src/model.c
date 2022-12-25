/*
 * model.c <z64.me>
 *
 * model types and functions
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "model.h"

// gbi stuff
#if 1
#define G_VTX           0x01
#define G_TRI           0x05
#define G_TRI2          0x06
#define G_ENDDL         0xdf
#endif

// private globals
static uint8_t *sgRoomSegment = 0;

// private types
#if 1
struct material
{
	struct material *next;
	void *data;
	int dataLen;
};

struct vertex
{
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t other[10];
};

struct triangle
{
	struct triangle *next;
	struct material *mat;
	struct vertex v[3];
};

struct group
{
	struct group *next;
	struct material *mat;
	struct triangle *tri;
};

struct room
{
	struct group *group;
	struct material *mat;
};
#endif // private types

// private helpers
#if 1
static void *segmentReadV(const uint32_t v)
{
	if ((v >> 24) != 0x03)
		return 0;
	
	return &sgRoomSegment[v & 0xffffff];
}

static void *segmentReadP(const void *p)
{
	return segmentReadV(BEr32(p));
}

static void appendTri(struct group *dst, struct material *mat, struct vertex *vbuf, int a, int b, int c)
{
	/* create new triangle and link into list */
	struct triangle *tri = calloc(1, sizeof(*tri));
	tri->v[0] = vbuf[a / 2];
	tri->v[1] = vbuf[b / 2];
	tri->v[2] = vbuf[c / 2];
	tri->mat = mat;
	tri->next = dst->tri;
	dst->tri = tri;
}

static struct material *appendMaterial(struct room *dst, const uint8_t *src, const int srcLen)
{
	struct material *mat;
	
	/* check whether already exists */
	for (mat = dst->mat; mat; mat = mat->next)
		if (mat->dataLen == srcLen && !memcmp(mat->data, src, srcLen))
			return mat;
	
	/* create new material and link into list */
	mat = calloc(1, sizeof(*mat));
	mat->data = Memdup(src, srcLen);
	mat->dataLen = srcLen;
	mat->next = dst->mat;
	dst->mat = mat;
	
	return mat;
}

static void appendDL(struct room *dst, const uint8_t *src)
{
	if (!src)
		return;
	
	struct vertex vbuf[32] = {0};
	struct material *mat = 0;
	struct group *group = calloc(1, sizeof(*group));
	const int stride = 8;
	
	while (*src != G_ENDDL)
	{
		/* material */
		{
			const uint8_t *start = src;
			
			while (*src != G_VTX
				&& *src != G_TRI
				&& *src != G_TRI2
				&& *src != G_ENDDL
			)
				src += stride;
			
			if (src > start)
				mat = appendMaterial(dst, start, src - start);
		}
		
		/* mesh */
		while (*src == G_VTX
			|| *src == G_TRI
			|| *src == G_TRI2
		)
		{
			switch (*src)
			{
				case G_VTX:
				{
					int numv = (src[1] << 4) | (src[2] >> 4);
					int vbidx = (src[3] >> 1) - numv;
					uint8_t *vaddr = segmentReadP(src + 4);
					
					while (numv--)
					{
						struct vertex *v = vbuf + vbidx;
						
						memcpy(v, vaddr, 16);
						v->x = BEr16(vaddr + 0);
						v->y = BEr16(vaddr + 2);
						v->z = BEr16(vaddr + 4);
						
						vaddr += 16;
						vbidx += 1;
					}
					break;
				}
				
				case G_TRI:
					appendTri(group, mat, vbuf, src[1], src[2], src[3]);
					break;
				
				case G_TRI2:
					appendTri(group, mat, vbuf, src[1], src[2], src[3]);
					appendTri(group, mat, vbuf, src[5], src[6], src[7]);
					break;
			}
			
			src += stride;
		}
	}
	
	group->next = dst->group;
	dst->group = group;
}
#endif // private helpers

// public functions
#if 1
/* merges all groups into one */
void room_flatten(struct room *room)
{
	Log("TODO: room_flatten");
}

/* merges src into dst (src will be destroyed) */
void room_merge(struct room *dst, struct room *src)
{
	if (!dst || !src)
		return;
	
	for (struct material *m = dst->mat; m; m = m->next)
	{
		if (!m->next)
		{
			m->next = src->mat;
			break;
		}
	}
	
	for (struct group *g = dst->group; g; g = g->next)
	{
		if (!g->next)
		{
			g->next = src->group;
			break;
		}
	}
	
	free(src);
}

/* loads a room */
struct room *room_load(const char *fn)
{
	size_t len = 0;
	struct room *room = calloc(1, sizeof(*room));
	uint8_t *data = loadfile(fn, &len);
	uint8_t *meshHeader = 0;
	
	if (!data)
		die("failed to load room file '%s'", fn);
	sgRoomSegment = data;
	
	/* find mesh header */
	for (size_t i = 0; i < len - 8 && data[i] != 0x14; i += 8)
		if (data[i] == 0x0A)
			meshHeader = segmentReadP(data + i + 4);
	if (!meshHeader)
		die("failed to locate mesh header in room '%s'", fn);
	
	/* parse mesh header */
	{
		uint8_t *s = segmentReadP(meshHeader + 4);
		uint8_t *e = segmentReadP(meshHeader + 8);
		const int stride = 16;
		uint8_t num = meshHeader[1];
		
		if (*meshHeader != 0x02)
			die("only mesh header type 0x02 supported; '%s' type is %02x'"
				, fn, *meshHeader
			);
		
		/* unnecessary sanity check */
		if ((e - s) / stride != num)
			die("mesh header sanity check failed");
		
		while (s < e)
		{
			appendDL(room, segmentReadP(s + 8));
			appendDL(room, segmentReadP(s + 12));
			
			s += stride;
		}
	}
	
	free(data);
	
	return room;
}

/* cleanup */
void room_free(struct room *room)
{
	struct material *mNext = 0;
	struct triangle *tNext = 0;
	struct group *gNext = 0;
	
	if (!room)
		return;
	
	for (struct group *g = room->group; g; g = gNext)
	{
		gNext = g->next;
		
		for (struct triangle *t = g->tri; t; t = tNext)
		{
			tNext = t->next;
			
			free(t);
		}
		
		free(g);
	}
		
	for (struct material *m = room->mat; m; m = mNext)
	{
		mNext = m->next;
		
		if (m->data)
			free(m->data);
		
		free(m);
	}
	
	free(room);
}

/* write a room to wavefront */
void room_writeWavefront(struct room *room, const char *outfn)
{
	FILE *fp;
	int v = 1; /* wavefront vertex indexing starts at 1 */
	
	if (!room
		|| !outfn
		|| !(fp = fopen(outfn, "wb"))
	)
		return;
	
	/* write every group */
	for (struct group *g = room->group; g; g = g->next)
	{
		fprintf(fp, "g %p\n", (void*)g);
		
		for (struct triangle *t = g->tri; t; t = t->next)
		{
			for (int i = 0; i < 3; ++i)
				fprintf(fp, "v %d %d %d\n"
					, t->v[i].x, t->v[i].y, t->v[i].z
				);
			fprintf(fp, "f %d %d %d\n", v, v + 1, v + 2);
			v += 3;
		}
	}
	
	fclose(fp);
}
#endif // public functions
