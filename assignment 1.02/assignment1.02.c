#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//#include <endian.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>

#include "heap.h" 

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

typedef struct path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef uint8_t pair_t[num_dims];

#define MAP_X              80
#define MAP_Y              21
#define MIN_TREES          10
#define MIN_BOULDERS       10
#define TREE_PROB          95
#define BOULDER_PROB       95

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_debug,
  ter_boulder,
  ter_tree,
  ter_path,
  ter_mart,
  ter_center,
  ter_grass,
  ter_clearing,
  ter_mountain,
  ter_forest,
  ter_water
} terrain_type_t;

typedef struct map {
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  uint8_t n, s, e, w;
} map_t;

typedef struct worldMap {
  map_t* map;
  map_t* worldMap[401][401];
  int h;
  int w;  
}worldMap_t;
//only time u allocate memory os when u create a map for x and y location 
//free when the user hits q 
worldMap_t allMaps;

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;

static int32_t path_cmp(const void *key, const void *with) {
  return ((path_t *) key)->cost - ((path_t *) with)->cost;
}

static int32_t edge_penalty(uint8_t x, uint8_t y)
{
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to)
{
  static path_t path[MAP_Y][MAP_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      path[y][x].hn = heap_insert(&h, &path[y][x]);
    }
  }

  while ((p = heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        mapxy(x, y) = ter_path;
        heightxy(x, y) = 0;
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1)))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1));
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1)))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1));
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

static int build_paths(map_t *m)
{
  pair_t from, to;

  from[dim_x] = 1;
  to[dim_x] = MAP_X - 2;
  from[dim_y] = m->w;
  to[dim_y] = m->e;

  dijkstra_path(m, from, to);

  from[dim_y] = 1;
  to[dim_y] = MAP_Y - 2;
  from[dim_x] = m->n;
  to[dim_x] = m->s;

  dijkstra_path(m, from, to);

  return 0;
}

static int gaussian[5][5] = {
  {  1,  4,  7,  4,  1 },
  {  4, 16, 26, 16,  4 },
  {  7, 26, 41, 26,  7 },
  {  4, 16, 26, 16,  4 },
  {  1,  4,  7,  4,  1 }
};
static int smooth_height(map_t *m)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  /*  FILE *out;*/
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof (height));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (height[y][x]);
    height[y][x] = i;
    if (i == 1) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);
  */
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1]) {
      height[y - 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1]) {
      height[y][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1]) {
      height[y + 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x]) {
      height[y - 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x]) {
      height[y + 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1]) {
      height[y - 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1]) {
      height[y][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1]) {
      height[y + 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = head->next;
    free(tmp);
  }

  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->height, sizeof (m->height), 1, out);
  fclose(out);
  */

  return 0;
}

static void find_building_location(map_t *m, pair_t p)
{
  do {
    p[dim_x] = rand() % (MAP_X - 3) + 1;
    p[dim_y] = rand() % (MAP_Y - 3) + 1;

    if ((((mapxy(p[dim_x] - 1, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] - 1, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x] + 2, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] + 2, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] - 1) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] - 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] + 2) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 2) == ter_path)))   &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_center))) &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_path)))) {
          break;
    }
  } while (1);
}

static int place_pokemart(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m)
{  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_center;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_center;

  return 0;
}

/* Chooses tree or boulder for border cell.  Choice is biased by dominance *
 * of neighboring cells.                                                   */
static terrain_type_t border_type(map_t *m, int32_t x, int32_t y)
{
  int32_t p, q;
  int32_t r, t;
  int32_t miny, minx, maxy, maxx;
  
  r = t = 0;
  
  miny = y - 1 >= 0 ? y - 1 : 0;
  maxy = y + 1 <= MAP_Y ? y + 1: MAP_Y;
  minx = x - 1 >= 0 ? x - 1 : 0;
  maxx = x + 1 <= MAP_X ? x + 1: MAP_X;

  for (q = miny; q < maxy; q++) {
    for (p = minx; p < maxx; p++) {
      if (q != y || p != x) {
        if (m->map[q][p] == ter_mountain ||
            m->map[q][p] == ter_boulder) {
          r++;
        } else if (m->map[q][p] == ter_forest ||
                   m->map[q][p] == ter_tree) {
          t++;
        }
      }
    }
  }
  
  if (t == r) {
    return rand() & 1 ? ter_boulder : ter_tree;
  } else if (t > r) {
    if (rand() % 10) {
      return ter_tree;
    } else {
      return ter_boulder;
    }
  } else {
    if (rand() % 10) {
      return ter_boulder;
    } else {
      return ter_tree;
    }
  }
}

static int map_terrain(map_t *m, uint8_t n, uint8_t s, uint8_t e, uint8_t w)
{
  int32_t i, x, y;
  queue_node_t *head, *tail, *tmp;
  //  FILE *out;
  int num_grass, num_clearing, num_mountain, num_forest, num_water, num_total;
  terrain_type_t type;
  int added_current = 0;
  
  num_grass = rand() % 4 + 2;
  num_clearing = rand() % 4 + 2;
  num_mountain = rand() % 2 + 1;
  num_forest = rand() % 2 + 1;
  num_water = rand() % 2 + 1;
  num_total = num_grass + num_clearing + num_mountain + num_forest + num_water;

  memset(&m->map, 0, sizeof (m->map));

  /* Seed with some values */
  for (i = 0; i < num_total; i++) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (m->map[y][x]);
    if (i == 0) {
      type = ter_grass;
    } else if (i == num_grass) {
      type = ter_clearing;
    } else if (i == num_grass + num_clearing) {
      type = ter_mountain;
    } else if (i == num_grass + num_clearing + num_mountain) {
      type = ter_forest;
    } else if (i == num_grass + num_clearing + num_mountain + num_forest) {
      type = ter_water;
    }
    m->map[y][x] = type;
    if (i == 0) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = m->map[y][x];
    
    if (x - 1 >= 0 && !m->map[y][x - 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x - 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m->map[y - 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y - 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m->map[y + 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y + 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m->map[y][x + 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x + 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    added_current = 0;
    tmp = head;
    head = head->next;
    free(tmp);
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (y == 0 || y == MAP_Y - 1 ||
          x == 0 || x == MAP_X - 1) {
        mapxy(x, y) = type = border_type(m, x, y);
      }
    }
  }

  m->n = n;
  m->s = s;
  m->e = e;
  m->w = w;

  mapxy(n,         0        ) = ter_path;
  mapxy(n,         1        ) = ter_path;
  mapxy(s,         MAP_Y - 1) = ter_path;
  mapxy(s,         MAP_Y - 2) = ter_path;
  mapxy(0,         w        ) = ter_path;
  mapxy(1,         w        ) = ter_path;
  mapxy(MAP_X - 1, e        ) = ter_path;
  mapxy(MAP_X - 2, e        ) = ter_path;

  return 0;
}

static int place_boulders(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_BOULDERS || rand() % 100 < BOULDER_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_forest && m->map[y][x] != ter_path) {
      m->map[y][x] = ter_boulder;
    }
  }

  return 0;
}

static int place_trees(map_t *m)
{
  int i;
  int x, y;
  
  for (i = 0; i < MIN_TREES || rand() % 100 < TREE_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_mountain &&
        m->map[y][x] != ter_path     &&
        m->map[y][x] != ter_water) {
      m->map[y][x] = ter_tree;
    }
  }

  return 0;
}

static int new_map(map_t *m, int n, int s, int e, int w)
{
  smooth_height(m);
  map_terrain(m,n,s,e,w);
  place_boulders(m);
  place_trees(m);
  build_paths(m);
  place_pokemart(m);
  place_center(m);

  if (allMaps.worldMap[200][200] == NULL) {
    allMaps.h=200;
    allMaps.w=200;
    allMaps.worldMap[allMaps.h][allMaps.w]=malloc(sizeof (*allMaps.worldMap));//placing first map at the origin 
    allMaps.map=malloc(sizeof(*allMaps.map));
    allMaps.worldMap[allMaps.h][allMaps.w]=m;
    allMaps.map=m;
    
  }
  //edgse cases
  if(allMaps.h==0){
    allMaps.map->height[0][n]='%';
  }
  if(allMaps.h==400){
    allMaps.map->height[MAP_Y-1][s]='%';

  }
  if(allMaps.w==0){
    allMaps.map->height[w][0]='%';
  
  }
  if(allMaps.w==400){
    allMaps.map->height[e][MAP_X-1]='%';
    
  }
  return 0;
}


static void print_map(map_t *m)
{
  int x, y;
  int default_reached = 0;
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      switch (m->map[y][x]) {
      case ter_boulder:
      case ter_mountain:
        putchar('%');
        break;
      case ter_tree:
      case ter_forest:
        putchar('^');
        break;
      case ter_path:
        putchar('#');
        break;
      case ter_mart:
        putchar('M');
        break;
      case ter_center:
        putchar('C');
        break;
      case ter_grass:
        putchar(':');
        break;
      case ter_clearing:
        putchar('.');
        break;
      case ter_water:
        putchar('~');
        break;
      default:
        default_reached = 1;
        break;
      }
      
    }
    putchar('\n');
  }

  if (default_reached) {
    printf("%c",(m->map[y][x]));
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
}

void moveN(int nGate, int sGate, int eGate, int wGate){

  sGate = allMaps.map->n;
  if(allMaps.worldMap[allMaps.h-1][allMaps.w+1]!=NULL){
    eGate=allMaps.worldMap[allMaps.h-1][allMaps.w+1]->w;
  }
    if(allMaps.worldMap[allMaps.h-1][allMaps.w-1]!=NULL){
      wGate=allMaps.worldMap[allMaps.h-1][allMaps.w-1]->e;
  }
    if(allMaps.worldMap[allMaps.h-2][allMaps.w]!=NULL){
    nGate=allMaps.worldMap[allMaps.h-2][allMaps.w]->s;
  }

  allMaps.h--;
  allMaps.worldMap[allMaps.h][allMaps.w]=malloc(sizeof(*allMaps.map));
  allMaps.map=malloc(sizeof(*allMaps.map));
   
  new_map(allMaps.map, nGate,sGate,eGate,wGate);
  allMaps.worldMap[allMaps.h][allMaps.w]=allMaps.map;

  // print_map(allMaps.worldMap[allMaps.x][allMaps.y]);
}

void moveS(int nGate, int sGate, int eGate, int wGate){
    nGate = allMaps.map->s;

    if(allMaps.worldMap[allMaps.h+1][allMaps.w-1]!=NULL){
      wGate=allMaps.worldMap[allMaps.h+1][allMaps.w-1]->e;
    }
      if(allMaps.worldMap[allMaps.h+1][allMaps.w+1]!=NULL){
        eGate=allMaps.worldMap[allMaps.h+1][allMaps.w+1]->w;
    }
      if(allMaps.worldMap[allMaps.h+2][allMaps.w]!=NULL){
      sGate=allMaps.worldMap[allMaps.h+2][allMaps.w]->n;
    }
    
    allMaps.h++;//new y
    allMaps.worldMap[allMaps.h][allMaps.w]=malloc(sizeof (*allMaps.map));
    allMaps.map=malloc(sizeof(*allMaps.map));
    new_map(allMaps.map, nGate, sGate, eGate, wGate);
      allMaps.worldMap[allMaps.h][allMaps.w]=allMaps.map;

  //       print_map(allMaps.worldMap[allMaps.x][allMaps.y]);

  
}

void moveE(int nGate, int sGate, int eGate, int wGate){
    wGate = allMaps.map->e;

    if(allMaps.worldMap[allMaps.h-1][allMaps.w+1]!=NULL){
      nGate=allMaps.worldMap[allMaps.h-1][allMaps.w+1]->s;
    }
      if(allMaps.worldMap[allMaps.h+1][allMaps.w+1]!=NULL){
        sGate=allMaps.worldMap[allMaps.h+1][allMaps.w+1]->n;
    }
      if(allMaps.worldMap[allMaps.h][allMaps.w+2]!=NULL){
      eGate=allMaps.worldMap[allMaps.h][allMaps.w+2]->w;
    }
    allMaps.w++;//new w
    allMaps.worldMap[allMaps.h][allMaps.w]=malloc(sizeof (*allMaps.map));
    allMaps.map=malloc(sizeof(*allMaps.map));
    new_map(allMaps.map, nGate, sGate, eGate, wGate);
      allMaps.worldMap[allMaps.h][allMaps.w]=allMaps.map;

  //       print_map(allMaps.worldMap[allMaps.x][allMaps.y]);

}

void moveW(int nGate, int sGate, int eGate, int wGate){
    eGate = allMaps.map->w;
    if(allMaps.worldMap[allMaps.h-1][allMaps.w-1]!=NULL){
      nGate=allMaps.worldMap[allMaps.h-1][allMaps.w-1]->s;
    }
      if(allMaps.worldMap[allMaps.h+1][allMaps.w-1]!=NULL){
        sGate=allMaps.worldMap[allMaps.h+1][allMaps.w-1]->n;
    }
      if(allMaps.worldMap[allMaps.h][allMaps.w-2]!=NULL){
      wGate=allMaps.worldMap[allMaps.h][allMaps.w-2]->e;
    }
    allMaps.w--;//new y
    allMaps.worldMap[allMaps.h][allMaps.w]=malloc(sizeof (*allMaps.map));
    allMaps.map=malloc(sizeof(*allMaps.map));
    new_map(allMaps.map, nGate, sGate, eGate, wGate);
    
      allMaps.worldMap[allMaps.h][allMaps.w]=allMaps.map;

  //       print_map(allMaps.worldMap[allMaps.x][allMaps.y]);  
}

void changeMap(char direction){
      
  int N = 1 + rand() % (MAP_X - 2);
  int S = 1 + rand() % (MAP_X - 2);
  int E = 1 + rand() % (MAP_Y - 2);
  int W = 1 + rand() % (MAP_Y - 2);

  switch (direction)
  {
  case 'n':
    if (allMaps.worldMap[allMaps.h-1][allMaps.w] == NULL) {
      moveN(N, S, E, W);

    }else{
      allMaps.h--;
    }
    break;
  case 's':
    if (allMaps.worldMap[allMaps.h+1][allMaps.w] == NULL) {
      moveS(N, S, E, W);
    }else{
      allMaps.h++;
    }
    break;
  case 'e':
    if (allMaps.worldMap[allMaps.h][allMaps.w+1] == NULL) {
      moveE(N, S, E, W);
    }else{
      allMaps.w++;
    }
    break;
  case 'w':
    if (allMaps.worldMap[allMaps.h][allMaps.w-1] == NULL) {
      moveW(N, S, E, W);
    }else{
      allMaps.w--;
    }
    break;
  default:
    printf("Error\n");
    break;
  }
  print_map(allMaps.worldMap[allMaps.h][allMaps.w]);

}

void flyTo(int h, int w){
  allMaps.h=h;
  allMaps.w=w;
  int N = 1 + rand() % (MAP_X - 2);
  int S = 1 + rand() % (MAP_X - 2);
  int E = 1 + rand() % (MAP_Y - 2);
  int W = 1 + rand() % (MAP_Y - 2);

    if (allMaps.worldMap[h][w] != NULL) {
      allMaps.map=allMaps.worldMap[h][w];
  }else{
    printf("k");
    if(allMaps.worldMap[h-1][w] != NULL){
      N = allMaps.worldMap[h-1][w]->s;
    }
    if(allMaps.worldMap[h+1][w] != NULL){
      S = allMaps.worldMap[h+1][w]->n;
    }
    if(allMaps.worldMap[h][w+1] != NULL){
      E = allMaps.worldMap[h][w+1]->w;
    }
    if(allMaps.worldMap[h][w-1] != NULL){
      W = allMaps.worldMap[h][w-1]->e;
    }

     allMaps.worldMap[allMaps.h][allMaps.w]=malloc(sizeof (*allMaps.map));
    allMaps.map=malloc(sizeof(*allMaps.map));
    new_map(allMaps.map, N, S, E, W);
    allMaps.worldMap[allMaps.h][allMaps.w]=allMaps.map;
  } 
    print_map(allMaps.worldMap[h][w]);

}

void freeMapMem() {
  int i,j; 

  for (i = 0; i <401; i++) {
      for ( j = 0; j < 401; j++) {
        if (allMaps.worldMap[i][j] != NULL) {
          free(allMaps.worldMap[i][j]);
          allMaps.worldMap[i][j] = NULL;  
        }
      }
    }
}

int main(int argc, char *argv[])
{
  map_t d;
  struct timeval tv;
  uint32_t seed;
 char direction; 
 int fx, fy; 

  if (argc == 2) {
    seed = atoi(argv[1]);
  } else {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  printf("Using seed: %u\n", seed);
  srand(seed);

  // map_terrain(&d,
  //             1 + rand() % (MAP_X - 2), 
  //             1 + rand() % (MAP_X - 2),
  //             1 + rand() % (MAP_Y - 2), 
  //             1 + rand() % (MAP_Y - 2));
  new_map(&d, 1 + rand() % (MAP_X - 2), 
              1 + rand() % (MAP_X - 2),
              1 + rand() % (MAP_Y - 2), 
              1 + rand() % (MAP_Y - 2));
  print_map(&d);


  do{
    printf("Enter the command: n for north, s for south, e for east, w for west, f x y for fly and q to quit\n");
    scanf(" %c", &direction);

    if(direction=='q'){
      printf("Thank you for playing!\n");
      freeMapMem();
    }else if(direction=='f'){
      scanf(" %d %d", &fx, &fy);
      if(fx<1 || fx>399||fy<1||fy>399){
        printf("Out of bounds! Can not go to that location\n");
      }else{
        flyTo(fx,fy);
      }
      
    }
    
    else if (direction=='n'||direction=='s'|| direction=='e'|| direction=='w'){
     if((direction=='n'&& allMaps.h==0 )|| (direction=='s' && allMaps.h==400 )|| (direction=='e'&& allMaps.w==400)|| (direction=='w' && allMaps.w==0)){
      printf("Out of bounds! Can not go in that direction\n");
     }else{
      changeMap(direction);
     } 

      
    }else{
      printf("Wrong character! Please try again!\n");

    }

  }while(direction!='q');

  return 0;
}


//after creating a map, if nothing has been created at the x and y, allocate memoery and out it in the world map