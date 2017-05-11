#if 0
set -e; [ "$0" -nt "$0.bin" ] &&
gcc -Wall -Wcast-align -Wextra -pedantic -std=c99 \
	-D_XOPEN_SOURCE \
	"$0" -lm -lSDL -o "$0.bin" -O3
exec "$0.bin" "$@"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL/SDL.h>

#define LEN(x) (sizeof(x)/sizeof((x)[0]))

Uint32
get_pixel(SDL_Surface *surface, int x, int y)
{
	Uint32 *pixels = (Uint32*)surface->pixels;
	return pixels[ ( y * surface->w ) + x ];
}

void
put_pixel(SDL_Surface *surface, int x, int y, Uint32 color)
{
	Uint32 *pixels = (Uint32 *)surface->pixels;
	pixels[ ( y * surface->w ) + x ] = color;
}


static void
sdlerror(const char *msg)
{
	fprintf(stderr, "%s: %s\n", msg, SDL_GetError());
}

typedef struct {
	float x;
	float y;
} vec2;

struct edge {
	vec2 line[2];
	vec2 normal;
};

static vec2
plus(vec2 a, vec2 b)
{
	return (vec2){a.x + b.x, a.y + b.y};
}

static vec2
minus(vec2 a, vec2 b)
{
	return (vec2){a.x - b.x, a.y - b.y};
}

static float
dot(vec2 a, vec2 b)
{
	return a.x * b.x + a.y * b.y;
}

static float
cross(vec2 a, vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

static float
sqlen(vec2 v)
{
	return dot(v, v);
}

static Uint32
mix(Uint32 x, Uint32 y, float a)
{
	Uint32 v = 0;
	for(int i = 0; i < 24; i += 8) {
		Uint32 px = (x & (0xFF << i)) >> i;
		Uint32 py = (y & (0xFF << i)) >> i;
		Uint32 pv = px * (1 - a) + py * a;
		v += pv << i;
	}
	return v;
}

static bool
intersect(vec2 p, vec2 r, vec2 q, vec2 s)
{
	float rxs = cross(r, s);
	if(rxs == 0) {
		return false;
	}
	vec2 q_p = minus(q, p);
	float q_pxs = cross(q_p, s);
	float q_pxr = cross(q_p, r);
	float t = q_pxs / rxs;
	float u = q_pxr / rxs;
	return t >= 0 && t <= 1 && u >= 0 && u <= 1;
}

static float
ray_intersect(vec2 p, vec2 r, vec2 q, vec2 s, vec2 n)
{
	float dir = dot(r, n);
	if(dir > 0) {
		return -1;
	}
	float rxs = cross(r, s);
	vec2 q_p = minus(q, p);
	float q_pxr = cross(q_p, r);

	if(rxs == 0 && q_pxr == 0) {
		return 1;
	}

	float q_pxs = cross(q_p, s);
	float t = q_pxs / rxs;
	float u = q_pxr / rxs;
	if(t >= 0 && u >= 0 && u <= 1) {
		return t;
	} else {
		return -1;
	}
}

static float
frand()
{
	return (float)rand() / RAND_MAX;
}

int
main(int argc, char *argv[]) {
	(void)argc; (void)argv;
	int rc = -1;

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		sdlerror("SDL_Init");
		return rc;
	}

	#define width 800
	#define height 600

	SDL_Surface *dpy = SDL_SetVideoMode(
		width, height, 32,
		SDL_SWSURFACE// | SDL_ASYNCBLIT // | SDL_RESIZABLE
	);
	if(dpy == NULL) {
		sdlerror("SDL_SetVideoMode");
		goto out_quit;
	}

	vec2 light = {600, 400};
	float light_radius = 50;
	float light_range = 800;
	float light_sqrange = light_range * light_range;

	struct edge box[] = {
		{ {{200, 200}, {250, 200}}, {0, -1} },
		{ {{250, 200}, {250, 250}}, {1, 0} },
		{ {{250, 250}, {200, 250}}, {0, 1} },
		{ {{200, 250}, {200, 200}}, {-1, 0} }
	};

	if(SDL_MUSTLOCK(dpy) && SDL_LockSurface(dpy) < 0) {
		sdlerror("SDL_LockSurface");
	}
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			float il = 0;
			vec2 pos = {x, y};

			#define samples 1000
			for(size_t s = 0; s < samples; s++) {


			float angle = 2 * M_PI * frand();
			vec2 ray = {
				cosf(angle),
				sinf(angle)
			};
			float is = -1;
			size_t closest = 0;
			for(size_t i = 0; i < LEN(box); i++) {
				vec2 bd = minus(box[i].line[1], box[i].line[0]);
				float ris = ray_intersect(pos, ray, box[i].line[0], bd, box[i].normal);
				if(ris > is) {
					is = ris;
					closest = i;
				}
			}
			if(is >= 0) {
				il += 5;
			}


			}
			il /= samples;
			if(il > 1) {
				il = 1;
			}


			/*
			random_angle
			cast
			bounce n times until light found
			draw lines with color that are mixed with average
			*/

			/*
			vec2 diff = minus(pos, box[1].line[0]);
			float vv = dot(diff, box[1].normal);
			if(vv < 0) {
				il = 0; // back
			} else if(vv > 0) {
				il = 1; // front
			} else {
				il = 0.5f; //same
			}
			*/

			/*
			vec2 light_dir = minus(pos, light);
			float sl = sqlen(light_dir);
			if(sl < light_sqrange) {
				size_t i = 0;
				for(; i < LEN(box); i++) {
					vec2 b = box[i].line[0];
					vec2 b_dir = minus(box[i].line[1], b);
					if(intersect(light, light_dir, b, b_dir)) {
						break;
					}
				}
				if(i == LEN(box)) {
					float len = sqrtf(sl);
					il = (light_range - len) / light_range;
				} else {
					il = 0;
				}
			}
			*/
			put_pixel(dpy, x, y, mix(0, 0xFFFFFF, il));
		}
		SDL_UpdateRect(dpy, 0, y, width, 1);
	}
	if(SDL_MUSTLOCK(dpy)) {
		SDL_UnlockSurface(dpy);
	}

	SDL_Event ev;
	do {
		SDL_WaitEvent(&ev);
	} while(!(
		ev.type == SDL_QUIT ||
		(ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
	));

	rc = 0;
out_quit:
	SDL_Quit();
	return rc;
}
