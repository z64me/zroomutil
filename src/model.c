/*
 * model.c <z64.me>
 *
 * model types and functions
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "common.h"
#include "model.h"

// gbi stuff
#if 1
#define G_VTX           0x01
#define G_CULLDL        0x03
#define G_TRI           0x05
#define G_TRI2          0x06
#define G_DL            0xde
#define G_ENDDL         0xdf
#define VBUF_MAX        32
#endif

// private globals
static uint8_t *sgRoomSegment = 0;

// private types
#if 1
#define BBOX_INIT_V (struct bbox){INT16_MAX, INT16_MAX, INT16_MAX, INT16_MIN, INT16_MIN, INT16_MIN}
struct bbox
{
	int16_t xmin;
	int16_t ymin;
	int16_t zmin;
	int16_t xmax;
	int16_t ymax;
	int16_t zmax;
};

struct material
{
	struct material *next;
	void *data;
	int dataLen;
	uint32_t wroteAt;
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
	int8_t vbidx[3];
};

struct group
{
	struct group *next;
	struct group *child;
	struct material *mat;
	struct triangle *tri;
	struct bbox bbox;
	uint32_t wroteAt;
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

static struct material *appendMaterial(struct room *dst, const uint8_t *src, int srcLen)
{
	struct material *mat;
	
	/* check whether already exists */
	for (mat = dst->mat; mat; mat = mat->next)
		if (mat->dataLen == srcLen && !memcmp(mat->data, src, srcLen))
			return mat;
	
	/* temporary test: use Hylian Shield material for everything */
	if (true)
	{
		static const uint8_t hylianShieldMaterial[] = {
			0xE7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xE3, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00,
			0xD7, 0x00, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFD, 0x10, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00,
			0xF5, 0x10, 0x00, 0x00, 0x07, 0x05, 0x81, 0x50,
			0xE6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xF3, 0x00, 0x00, 0x00, 0x07, 0x7F, 0xF1, 0x00,
			0xE7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xF5, 0x10, 0x10, 0x00, 0x00, 0x05, 0x81, 0x50,
			0xF2, 0x00, 0x00, 0x00, 0x00, 0x07, 0xC0, 0xFC,
			0xFC, 0x12, 0x7E, 0x03, 0xFF, 0xFF, 0xFD, 0xF8,
			0xE2, 0x00, 0x00, 0x1C, 0xC8, 0x11, 0x20, 0x78,
			0xD9, 0xF3, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
			0xD9, 0xFF, 0xFF, 0xFF, 0x00, 0x03, 0x00, 0x00,
			0xFA, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF
		};
		src = hylianShieldMaterial;
		srcLen = sizeof(hylianShieldMaterial);
	}
	
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
	
	struct vertex vbuf[VBUF_MAX] = {0};
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

static void writeMaterials(struct room *room, FILE *dst)
{
	const int stride = 8;
	const uint8_t enddl[8] = { G_ENDDL };
	
	/* write every material */
	for (struct material *m = room->mat; m; m = m->next)
	{
		m->wroteAt = 0x03000000 | ftell(dst);
		for (uint8_t *d = m->data; d < ((uint8_t*)m->data) + m->dataLen; d += stride)
		{
			if (*d != G_CULLDL
				&& *d != G_DL
			)
				fwrite(d, 1, stride, dst);
		}
		fwrite(enddl, 1, sizeof(enddl), dst);
	}
}

/* find index of v in vbuf; adds it if it doesn't already exist; returns -1 on failure */
static int vbufGetVertexIndex(struct vertex *vbuf, int *vbufIndex, struct vertex v)
{
	if (!vbuf || !vbufIndex)
		return -1;
	
	/* return match if one already exists */
	for (int i = 0; i < *vbufIndex; ++i)
		if (!memcmp(vbuf + i, &v, sizeof(v)))
			return i;
	
	/* too little space */
	if (*vbufIndex >= VBUF_MAX)
		return -1;
	
	/* append */
	vbuf[*vbufIndex] = v;
	*vbufIndex += 1;
	
	return (*vbufIndex) - 1;
}

static void group_merge(struct group *dst, struct group *src)
{
	struct triangle *t;
	
	if (!dst || !src)
		return;
	
	if (src->child)
	{
		struct group *dst = src;
		struct group *next = 0;
		
		for (struct group *src = dst->child; src; src = next)
		{
			next = src->next;
			group_merge(dst, src);
		}
	}
	
	for (t = dst->tri; t && t->next; )
		t = t->next;
	
	t->next = src->tri;
	free(src);
}

static void group_bounds_recursive(const struct group *g, struct bbox *bbox)
{
	struct triangle *t;
	
	if (!g || !bbox)
		return;
	
	for (t = g->tri; t; t = t->next)
	{
#define DO_BOUNDS(V, X, FUNC) \
	bbox->V = FUNC( \
		bbox->V \
		, t->v[0].X \
		, t->v[1].X \
		, t->v[2].X \
	)
		DO_BOUNDS(xmin, x, min4_int);
		DO_BOUNDS(ymin, y, min4_int);
		DO_BOUNDS(zmin, z, min4_int);
		
		DO_BOUNDS(xmax, x, max4_int);
		DO_BOUNDS(ymax, y, max4_int);
		DO_BOUNDS(zmax, z, max4_int);
#undef DO_BOUNDS
	}
	
	if (g->child)
		group_bounds_recursive(g->child, bbox);
}

static struct bbox group_bounds(const struct group *g)
{
	struct bbox result = BBOX_INIT_V;
	
	if (!g)
		return result;
	
	group_bounds_recursive(g, &result);
	
	return result;
}

static bool triangle_inside_bbox(const struct triangle t, const struct bbox bb)
{
	/* use center point */
	struct vertex v;
	
	v.x = (t.v[0].x + t.v[1].x + t.v[2].x) / 3;
	v.y = (t.v[0].y + t.v[1].y + t.v[2].y) / 3;
	v.z = (t.v[0].z + t.v[1].z + t.v[2].z) / 3;
	
	return v.x >= bb.xmin
		&& v.x <= bb.xmax
		&& v.y >= bb.ymin
		&& v.y <= bb.ymax
		&& v.z >= bb.zmin
		&& v.z <= bb.zmax
	;
}

static void group_divide(struct group *g, struct bbox *bbox, const int divisions[], const int divisionsNum)
{
	if (!divisionsNum)
		return;
	
	int largest = max4_int(0, bbox->xmax - bbox->xmin, bbox->ymax - bbox->ymin, bbox->zmax - bbox->zmin);
	int div = divisions[0];
	int tmp = 0;
	int sec;
	
	/* ensure that the largest is evenly divisible by the number of divisions */
	while (largest % div)
		++largest;
	sec = largest / div;
	
	/* if bbox isn't a cube, make it a cube */
	//int delta;
	//if ((delta = (largest - (bbox->xmax - bbox->xmin))) != 0) // XXX old idea
	//	bbox->xmax += delta / 2, bbox->xmin -= delta / 2;
#define DO_ONE(VMIN, VMAX) \
	while ((bbox->VMAX - bbox->VMIN) != largest) \
		((++tmp)&1) ? (bbox->VMAX += 1) : (bbox->VMIN -= 1);
	DO_ONE(xmin, xmax)
	DO_ONE(ymin, ymax)
	DO_ONE(zmin, zmax)
#undef DO_ONE
	
	for (int x = 0; x < div; ++x)
	{
		for (int y = 0; y < div; ++y)
		{
			for (int z = 0; z < div; ++z)
			{
				//struct triangle *prev = g->tri;
				struct triangle *next = 0;
				struct group *child = calloc(1, sizeof(*child));
				struct bbox bb = {
					.xmin = bbox->xmin + sec * x,
					.ymin = bbox->ymin + sec * y,
					.zmin = bbox->zmin + sec * z
				};
				bb.xmax = bb.xmin + sec;
				bb.ymax = bb.ymin + sec;
				bb.zmax = bb.zmin + sec;
				
				/* setup */
				child->bbox = bb;
				for (struct triangle *t = g->tri; t; t = next)
				{
					next = t->next;
					if (next)
					{
						if (triangle_inside_bbox(*next, bb))
						{
							void *nnext = next->next;
							
							next->next = child->tri;
							child->tri = next;
							
							t->next = nnext;
							next = t;
						}
					}
					#if 0 /* XXX misc old experments */
					else if (triangle_inside_bbox(*t, bb))
					{
						Log("match");
					}
					continue;
					if (triangle_inside_bbox(*t, bb))
					{
						if (prev)
						{
							if (prev == g->tri)
								g->tri = next;
							else
								prev->next = next;
						}
						t->next = child->tri;
						child->tri = t;
					}
					else
						prev = t;
					#endif
				}
				
				/* debug helper: add 3D bbox to output */
				if (false)
				{
					struct triangle *t;
					#define QVTX(X, Y, Z) (struct vertex){bb.X, bb.Y, bb.Z, {0}}
					struct vertex v[] = {
						QVTX(xmax, ymin, zmin),
						QVTX(xmax, ymin, zmax),
						QVTX(xmin, ymin, zmax),
						QVTX(xmin, ymin, zmin),
						QVTX(xmax, ymax, zmin),
						QVTX(xmax, ymax, zmax),
						QVTX(xmin, ymax, zmax),
						QVTX(xmin, ymax, zmin)
					};
					#define PUSH_TRI(A, B, C) \
						t = calloc(1, sizeof(*t)); \
						t->next = child->tri; \
						t->v[0] = v[A - 1]; \
						t->v[1] = v[B - 1]; \
						t->v[2] = v[C - 1]; \
						child->tri = t;
					PUSH_TRI(2, 4, 1)
					PUSH_TRI(8, 6, 5)
					PUSH_TRI(5, 2, 1)
					PUSH_TRI(6, 3, 2)
					PUSH_TRI(3, 8, 4)
					PUSH_TRI(1, 8, 5)
					PUSH_TRI(2, 3, 4)
					PUSH_TRI(8, 7, 6)
					PUSH_TRI(5, 6, 2)
					PUSH_TRI(6, 7, 3)
					PUSH_TRI(3, 7, 8)
					PUSH_TRI(1, 4, 8)
					#undef PUSH_TRI
					#undef QVTX
				}
				
				/* link in */
				child->next = g->child;
				g->child = child;
				
				/* subdivide */
				if (divisionsNum > 1)
					group_divide(child, &bb, divisions + 1, divisionsNum - 1);
			}
		}
	}
}

static void group_free(struct group *g)
{
	struct triangle *tNext = 0;
	struct group *gNext = 0;
	
	for ( ; g; g = gNext)
	{
		gNext = g->next;
		
		for (struct triangle *t = g->tri; t; t = tNext)
		{
			tNext = t->next;
			
			free(t);
		}
		
		if (g->child)
			group_free(g->child);
		
		free(g);
	}
}
#endif // private helpers

// public functions
#if 1
/* merges all groups into one */
void room_flatten(struct room *room)
{
	struct group *dst;
	struct group *next = 0;
	
	if (!room || !(dst = room->group))
		return;
	
	for (struct group *src = dst->next; src; src = next)
	{
		next = src->next;
		group_merge(dst, src);
	}
	
	dst->next = 0;
}

/* divide a flattened room into nested group structure */
void room_divide(struct room *room, const int divisions[], const int divisionsNum)
{
	struct bbox bbox;
	
	if (!room || !divisions || divisionsNum <= 0 || !room->group)
		return;
	
	// TODO: do I want a fatal error here, or just a warning + invoke room_flatten?
	if (room->group->next)
		die("room_divide error: trying to divide a non-flattened room");
	
	bbox = group_bounds(room->group);
	
	group_divide(room->group, &bbox, divisions, divisionsNum);
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
	
	if (!room)
		return;
	
	group_free(room->group);
		
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
void room_writeWavefront(struct room *room, struct group *group, const char *outfn)
{
	static FILE *fp = 0; /* XXX statics to support recursion */
	static int v = 1;
	bool ownsFp = false;
	
	/* initial load */
	if (fp == 0)
	{
		if (!room
			|| !outfn
			|| !(fp = fopen(outfn, "wb"))
		)
			return;
		
		v = 1; /* wavefront vertex indexing starts at 1 */
		ownsFp = true;
	}
	
	/* write every group */
	for (struct group *g = group ? group : room->group; g; g = g->next)
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
		
		/* recursion */
		if (g->child)
			room_writeWavefront(0, g->child, 0);
	}
	
	/* initial load */
	if (ownsFp)
	{
		fclose(fp);
		fp = 0;
	}
}

/* write a room to zroom format */
void room_writeZroom(struct room *room, const char *outfn, bool withMaterials)
{
	const uint8_t enddl[8] = { G_ENDDL };
	struct vertex vbuf[VBUF_MAX];
	struct material *mat = 0;
	FILE *fp;
	int vbufIndex = 0;
	int opaNum = 0;
	unsigned char roomHeader[] = {
		0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x08, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
		0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x10, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00,
		0x0A, 0x00, 0x00, 0x00,	0x03, 0x00, 0x00, 0x00,
		0x05, 0x00, 0x00, 0x00, 0x0F, 0x28, 0x6D, 0xBE,
		0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00
	};
	
	if (!room
		|| !outfn
		|| !(fp = fopen(outfn, "wb+"))
	)
		return;
	
	/* write placeholder room header */
	fwrite(roomHeader, 1, sizeof(roomHeader), fp);
	
	/* write every material */
	if (withMaterials)
		writeMaterials(room, fp);
	
	/* write every group */
	for (struct group *g = room->group; g; g = g->next, ++opaNum)
	{
		struct triangle *tBegin = g->tri;
		FILE *dl;
		size_t dlLen;
		
		if (!(dl = tmpfile()))
			die("failed to create tmpfile");
		
		Log("processing group %p...", (void*)g);
		
		/* triangle data first */
		for (struct triangle *t = tBegin; t; t = t->next)
		{
			int vbufIndexOld = vbufIndex;
			bool didJump = false;
			
			if (withMaterials && t->mat != mat)
			{
				mat = t->mat;
				
				if (t != tBegin)
				{
					didJump = true;
					goto L_flush;
				}
			L_postFlushJump:
				didJump = false;
				uint32_t a = mat->wroteAt;
				uint8_t branch[8] = { G_DL, 0, 0, 0, a >> 24, a >> 16, a >> 8, a };
				
				fwrite(branch, 1, sizeof(branch), dl);
			}
			
			/* retry in perpetuity until it fits */
			for (int i = 0; i < 3; )
			{
				while (i < 3)
				{
					/* doesn't fit */
					if ((t->vbidx[i] = vbufGetVertexIndex(vbuf, &vbufIndex, t->v[i])) < 0)
					{
					L_flush:do{}while(0);
						uint32_t addr = 0x03000000 | ftell(fp);
						
						/* flush compiled vertex buffer to file */
						for (i = 0; i < vbufIndexOld; ++i)
						{
							struct vertex *v = vbuf + i;
							uint8_t result[16];
							
							memcpy(result, v, sizeof(*v));
							BEw16(result + 0, v->x);
							BEw16(result + 2, v->y);
							BEw16(result + 4, v->z);
							
							fwrite(result, 1, sizeof(result), fp);
						}
						
						/* flush vertex load command to display list */
						{
							uint8_t cmd[8] = {
								G_VTX
								, vbufIndexOld >> 4
								, vbufIndexOld << 4
								, ((0 + vbufIndexOld) & 0x7f) << 1
								, addr >> 24, addr >> 16, addr >> 8, addr
							};
							
							fwrite(cmd, 1, sizeof(cmd), dl);
							vbufIndex = 0;
						}
						
						/* flush triangles to display list */
						{
							for (struct triangle *w = tBegin; w != t && w; w = w->next)
							{
								struct triangle *n = w->next != t ? w->next : 0;
								uint8_t cmd[8] = {
									G_TRI
									, w->vbidx[0] << 1
									, w->vbidx[1] << 1
									, w->vbidx[2] << 1
								};
								
								if (n)
									cmd[0] = G_TRI2
									, cmd[5] = n->vbidx[0] << 1
									, cmd[6] = n->vbidx[1] << 1
									, cmd[7] = n->vbidx[2] << 1
									, w = n
								;
								
								fwrite(cmd, 1, sizeof(cmd), dl);
							}
							
							tBegin = t;
						}
						
						if (didJump)
							goto L_postFlushJump;
						
						if (!t->next)
							goto L_triangleDataDone;
						
						i = 0;
					}
					else
						++i;
				}
			}
			
			/* flush any remaining triangles */
			if (!t->next)
				goto L_flush;
			L_triangleDataDone:do{}while(0);
		}
		
		g->wroteAt = 0x03000000 | ftell(fp);
		Log(" > writing it at %08x", g->wroteAt);
		
		/* append dl's contents onto fp */
		fwrite(enddl, 1, sizeof(enddl), dl);
		dlLen = ftell(dl);
		fseek(dl, 0, SEEK_SET);
		while(dlLen--)
			fputc(fgetc(dl), fp);
		fclose(dl);
	}
	
	/* write mesh header */
	{
		const int type = 0x00;
		const int stride = (type == 0x00) ? 8 : 16;
		uint32_t wroteAt = 0x03000000 | ftell(fp);
		uint32_t start = wroteAt + 12;
		uint32_t end = start + opaNum * stride;
		uint8_t meshHeader[] = {
			type // type
			, opaNum // number of entries
			, 0 // padding
			, 0 // padding
			, U32_BYTES(start)
			, U32_BYTES(end)
		};
		
		/* main header structure */
		fwrite(meshHeader, 1, sizeof(meshHeader), fp);
		
		/* the mesh pointer array referenced by the header */
		for (struct group *g = room->group; g; g = g->next)
		{
			if (type == 0x00)
			{
				uint8_t tmp[4] = { U32_BYTES(g->wroteAt) };
				uint8_t zero[4] = { 0 };
				
				fwrite(tmp, 1, sizeof(tmp), fp); // opa
				fwrite(zero, 1, sizeof(zero), fp); // xlu
			}
			else if (type == 0x02)
			{
				// TODO
			}
		}
		
		/* 16-byte alignment */
		while (ftell(fp) & 0xf)
			fputc(0, fp);
		
		/* update room header to point to mesh header */
		{
			uint8_t tmp[4] = { U32_BYTES(wroteAt) };
			
			fseek(fp, 0, SEEK_SET);
			while (fgetc(fp) != 0x0A)
				fseek(fp, 7, SEEK_CUR);
			fseek(fp, 3, SEEK_CUR);
			fwrite(tmp, 1, sizeof(tmp), fp);
		}
	}
	
	fclose(fp);
}
#endif // public functions
