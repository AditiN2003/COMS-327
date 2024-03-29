#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <vector>
#include "io.h"
#include "character.h"
#include "poke327.h"
//aditi
typedef struct io_message {
  /* Will print " --more-- " at end of line when another message follows. *
   * Leave 10 extra spaces for that.                                      */
  char msg[71];
  struct io_message *next;
} io_message_t;

class trainer_pokemon {
public:
    string name, gender, shiny, move_one, move_two;
    int level, hp, attack, defense, sp_attack, sp_defense, speed, cur_hp, move_one_id, move_two_id, base_speed, poke_id;
    trainer_pokemon(string name, string gender, string shiny, string move_one, string move_two, int level, vector<int> stats, int move_one_id, int move_two_id, int base_speed, int poke_id) {
        this->name = name;
        this->gender = gender;
        this->shiny = shiny;
        this->move_one = move_one;
        this->move_two = move_two;
        this->level = level;
        hp = stats[0];
        attack = stats[1];
        defense = stats[2];
        sp_attack = stats[3];
        sp_defense = stats[4];
        speed = stats[5];
        this->base_speed = base_speed;
        cur_hp = stats[0];
        this->move_one_id = move_one_id;
        this->move_two_id = move_two_id;
        this->poke_id = poke_id;
    }
};

vector<trainer_pokemon> pc_pokes;

static io_message_t *io_head, *io_tail;

void io_init_terminal(void)
{
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

void io_reset_terminal(void)
{
  endwin();

  while (io_head) {
    io_tail = io_head;
    io_head = io_head->next;
    free(io_tail);
  }
  io_tail = NULL;
}

void io_queue_message(const char *format, ...)
{
  io_message_t *tmp;
  va_list ap;

  if (!(tmp = (io_message_t*)malloc(sizeof (*tmp)))) {
    perror("malloc");
    exit(1);
  }

  tmp->next = NULL;

  va_start(ap, format);

  vsnprintf(tmp->msg, sizeof (tmp->msg), format, ap);

  va_end(ap);

  if (!io_head) {
    io_head = io_tail = tmp;
  } else {
    io_tail->next = tmp;
    io_tail = tmp;
  }
}

static void io_print_message_queue(uint32_t y, uint32_t x)
{
  while (io_head) {
    io_tail = io_head;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(y, x, "%-80s", io_head->msg);
    attroff(COLOR_PAIR(COLOR_CYAN));
    io_head = io_head->next;
    if (io_head) {
      attron(COLOR_PAIR(COLOR_CYAN));
      mvprintw(y, x + 70, "%10s", " --more-- ");
      attroff(COLOR_PAIR(COLOR_CYAN));
      refresh();
      getch();
    }
    free(io_tail);
  }
  io_tail = NULL;
}

/**************************************************************************
 * Compares trainer distances from the PC according to the rival distance *
 * map.  This gives the approximate distance that the PC must travel to   *
 * get to the trainer (doesn't account for crossing buildings).  This is  *
 * not the distance from the NPC to the PC unless the NPC is a rival.     *
 *                                                                        *
 * Not a bug.                                                             *
 **************************************************************************/
static int compare_trainer_distance(const void *v1, const void *v2)
{
  const character_t *const *c1 = (const character_t *const *)v1;
  const character_t *const *c2 = (const character_t *const *)v2;

  return (world.rival_dist[(*c1)->pos[dim_y]][(*c1)->pos[dim_x]] -
          world.rival_dist[(*c2)->pos[dim_y]][(*c2)->pos[dim_x]]);
}

static character_t *io_nearest_visible_trainer()
{
  character_t **c, *n;
  uint32_t x, y, count;

  c = (character_t**) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = world.cur_map->cmap[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  n = c[0];

  free(c);

  return n;
}

void io_display()
{
  uint32_t y, x;
  character_t *c;

  clear();
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.cur_map->cmap[y][x]) {
        mvaddch(y + 1, x, world.cur_map->cmap[y][x]->symbol);
      } else {
        switch (world.cur_map->map[y][x]) {
        case ter_boulder:
        case ter_mountain:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, '%');
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_tree:
        case ter_forest:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, '^');
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_path:
        case ter_exit:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, '#');
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_mart:
          attron(COLOR_PAIR(COLOR_BLUE));
          mvaddch(y + 1, x, 'M');
          attroff(COLOR_PAIR(COLOR_BLUE));
          break;
        case ter_center:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(y + 1, x, 'C');
          attroff(COLOR_PAIR(COLOR_RED));
          break;
        case ter_grass:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, ':');
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_clearing:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, '.');
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        default:
 /* Use zero as an error symbol, since it stands out somewhat, and it's *
  * not otherwise used.                                                 */
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, '0');
          attroff(COLOR_PAIR(COLOR_CYAN)); 
       }
      }
    }
  }

  mvprintw(23, 1, "PC position is (%2d,%2d) on map %d%cx%d%c.",
           world.pc.pos[dim_x],
           world.pc.pos[dim_y],
           abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_x] - (WORLD_SIZE / 2) >= 0 ? 'E' : 'W',
           abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2) <= 0 ? 'N' : 'S');
  mvprintw(22, 1, "%d known %s.", world.cur_map->num_trainers,
           world.cur_map->num_trainers > 1 ? "trainers" : "trainer");
  mvprintw(22, 30, "Nearest visible trainer: ");
  if ((c = io_nearest_visible_trainer())) {
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(22, 55, "%c at %d %c by %d %c.",
             c->symbol,
             abs(c->pos[dim_y] - world.pc.pos[dim_y]),
             ((c->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              'N' : 'S'),
             abs(c->pos[dim_x] - world.pc.pos[dim_x]),
             ((c->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              'W' : 'E'));
    attroff(COLOR_PAIR(COLOR_RED));
  } else {
    attron(COLOR_PAIR(COLOR_BLUE));
    mvprintw(22, 55, "NONE.");
    attroff(COLOR_PAIR(COLOR_BLUE));
  }

  io_print_message_queue(0, 0);

  refresh();
}

uint32_t io_teleport_pc(pair_t dest)
{
  /* Just for fun. And debugging.  Mostly debugging. */

  do {
    dest[dim_x] = rand_range(1, MAP_X - 2);
    dest[dim_y] = rand_range(1, MAP_Y - 2);
  } while (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]                  ||
           move_cost[char_pc][world.cur_map->map[dest[dim_y]]
                                                [dest[dim_x]]] == INT_MAX ||
           world.rival_dist[dest[dim_y]][dest[dim_x]] < 0);

  return 0;
}

static void io_scroll_trainer_list(char (*s)[40], uint32_t count)
{
  uint32_t offset;
  uint32_t i;

  offset = 0;

  while (1) {
    for (i = 0; i < 13; i++) {
      mvprintw(i + 6, 19, " %-40s ", s[i + offset]);
    }
    switch (getch()) {
    case KEY_UP:
      if (offset) {
        offset--;
      }
      break;
    case KEY_DOWN:
      if (offset < (count - 13)) {
        offset++;
      }
      break;
    case 27:
      return;
    }

  }
}

static void io_list_trainers_display(character_t **c,
                                     uint32_t count)
{
  uint32_t i;
  char (*s)[40]; /* pointer to array of 40 char */

  s = (char(*)[40])malloc(count * sizeof (*s));

  mvprintw(3, 19, " %-40s ", "");
  /* Borrow the first element of our array for this string: */
  snprintf(s[0], 40, "You know of %d trainers:", count);
  mvprintw(4, 19, " %-40s ", *s);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++) {
    snprintf(s[i], 40, "%16s %c: %2d %s by %2d %s",
             char_type_name[c[i]->npc->ctype],
             c[i]->symbol,
             abs(c[i]->pos[dim_y] - world.pc.pos[dim_y]),
             ((c[i]->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              "North" : "South"),
             abs(c[i]->pos[dim_x] - world.pc.pos[dim_x]),
             ((c[i]->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              "West" : "East"));
    if (count <= 13) {
      /* Handle the non-scrolling case right here. *
       * Scrolling in another function.            */
      mvprintw(i + 6, 19, " %-40s ", s[i]);
    }
  }

  if (count <= 13) {
    mvprintw(count + 6, 19, " %-40s ", "");
    mvprintw(count + 7, 19, " %-40s ", "Hit escape to continue.");
    while (getch() != 27 /* escape */)
      ;
  } else {
    mvprintw(19, 19, " %-40s ", "");
    mvprintw(20, 19, " %-40s ",
             "Arrows to scroll, escape to continue.");
    io_scroll_trainer_list(s, count);
  }

  free(s);
}

static void io_list_trainers()
{
  character_t **c;
  uint32_t x, y, count;

  c = (character_t**)malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = world.cur_map->cmap[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  /* Display it */
  io_list_trainers_display(c, count);
  free(c);

  /* And redraw the map */
  io_display();
}

void io_pokemart()
{
  mvprintw(0, 0, "Welcome to the Pokemart.  Could I interest you in some Pokeballs?");
  refresh();
  getch();
}

void io_pokemon_center()
{
  mvprintw(0, 0, "Welcome to the Pokemon Center.  How can Nurse Joy assist you?");
  refresh();
  getch();
}

vector<int> moves_finder(int rand_poke, int level, int *moves_start, int *moves_end, vector<pokemon_moves> pokemon_moves_vec) {
    int i;
    vector<int> possible_moves;
    
    for(i = 0; i < (int)pokemon_moves_vec.size(); i++) {
        if(pokemon_moves_vec[i].pokemon_id == pokes[rand_poke].species_id) {
            *moves_start = i;
            break;
        }
    }
    for(i = *moves_start; i < (int)pokemon_moves_vec.size(); i++) {
        if(pokemon_moves_vec[i].pokemon_id != pokes[rand_poke].species_id) {
            *moves_end = i;
            break;
        }
        else if(i == (int)pokemon_moves_vec.size() - 1) {
            *moves_end = (int)pokemon_moves_vec.size();
            break;
        }
    }
    
    for(i = *moves_start; i < *moves_end; i++) {
        if(pokemon_moves_vec[i].pokemon_move_method_id == 1 && pokemon_moves_vec[i].level <= level && pokemon_moves_vec[i].level != 0) {
            possible_moves.push_back(pokemon_moves_vec[i].move_id);
        }
    }
    
    return possible_moves;
}
trainer_pokemon generate_pokemon() {
    int rand_poke, rand_move, i, man_distance, level, moves_start, moves_end, move_one_id, move_two_id, stat_count;
    string move_one, move_two, gender, shiny;
    vector<int> possible_moves;
    vector<int> base_stats;
    vector<int> ivs;
    vector<int> stats;
    
    rand_poke = (rand() % 1092) + 1;
    
    man_distance = abs(world.cur_idx[dim_x] - 200) + abs(world.cur_idx[dim_y] - 200);
    
    if(man_distance == 1 || man_distance == 0) {
        level = 1;
    }
    else if(man_distance > 200) {
        level = (rand() % (100 - ((man_distance - 200) / 2) + 1)) + ((man_distance - 200) / 2);
    }
    else {
        level = (rand() % (man_distance / 2)) + 1;
    }

    moves_start = moves_end = -1;
    if(pokes[rand_poke].species_id < 808) {
        possible_moves = moves_finder(rand_poke, level, &moves_start, &moves_end, pokemon_moves_vec);
    }
    else {
        possible_moves = moves_finder(rand_poke, level, &moves_start, &moves_end, pokemon_moves_20_vec);
    }
    
    move_one_id = move_two_id = -1;
    rand_move = rand() % (int)possible_moves.size();
    move_one_id = possible_moves[rand_move];
    
    if((int)possible_moves.size() > 1) {
        while(true) {
            rand_move = rand() % (int)possible_moves.size();
            if(possible_moves[rand_move] != move_one_id) {
                move_two_id = possible_moves[rand_move];
                break;
            }
        }
    }
    
    for(i = 0; i < (int)moves_vec.size(); i++) {
        if(move_one_id == moves_vec[i].id) {
            move_one = moves_vec[i].name;
            move_one[0] = toupper(move_one[0]);
            break;
        }
    }
    if(move_two_id != -1) {
        for(i = 0; i < (int)moves_vec.size(); i++) {
            if(move_two_id == moves_vec[i].id) {
                move_two = moves_vec[i].name;
                move_two[0] = toupper(move_two[0]);
                break;
            }
        }
    }
    
    for(i = 0; i < (int)poke_stats_vec.size(); i++) {
        if(poke_stats_vec[i].pokemon_id == pokes[rand_poke].id) {
            break;
        }
    }
    for(stat_count = 0; stat_count < 6; stat_count++) {
        base_stats.push_back(poke_stats_vec[i + stat_count].base_stat);
    }
    for(i = 0; i < 6; i++) {
        ivs.push_back(rand() % 16);
    }
    stats.push_back(((((base_stats[0] + ivs[0]) * 2) * level) / 100) + level + 10);
    for(i = 1; i < 6; i++) {
        stats.push_back(((((base_stats[i] + ivs[i]) * 2) * level) / 100) + 5);
    }
    
    if(rand() % 2 == 0 ) {
        gender = "Male";
    } else {
        gender = "Female";
    }
    
    if(rand() % 8192 == 0) {
        shiny = "Yes!";
    } else {
        shiny = "No.";
    }
    
    pokes[rand_poke].name[0] = toupper(pokes[rand_poke].name[0]);
    
    trainer_pokemon generated_poke(pokes[rand_poke].name, gender, shiny, move_one, move_two, level, stats, move_one_id, move_two_id, base_stats[5], pokes[rand_poke].id);
    
    return generated_poke;
}
void battle_display(trainer_pokemon opponent, int cur_pokemon, int type) {
    clear();

    
    mvprintw(3, 1, "Enemy Pokemon: Name: %s, Level: %d", opponent.name.c_str(), opponent.level);
    mvprintw(4, 1, "Gender: %s", opponent.gender.c_str());
    mvprintw(5, 1, "Shiny?: %s", opponent.shiny.c_str());
    mvprintw(6, 1, "Move One: %s", opponent.move_one.c_str());
    mvprintw(7, 1, "Move Two: %s", opponent.move_two.c_str());
    mvprintw(8, 1, "HP: %d/%d", opponent.cur_hp, opponent.hp);
     mvprintw(14, 28, "Press any key to continue");

    refresh();
    getch();
}

void io_battle(character_t *aggressor, character_t *defender)
{
    character_t *npc;
    vector<trainer_pokemon> npc_pokes;
    int i;
    int num_pokemon = 1;
    int pc_poke_check = 0;
    
    for(i = 0; i < (int)pc_pokes.size(); i++) {
        if(pc_pokes[i].cur_hp == 0) {
            pc_poke_check++;
        }
    }
    if(pc_poke_check != (int)pc_pokes.size()) {
        for(i = 0; i < 5; i++) {
            if(rand() % 5 == 0 || rand() % 5 == 1 || rand() % 5 == 2) {
                num_pokemon++;
            }
            else {
                break;
            }
        }
        
        for(i = 0; i < num_pokemon; i++) {
            npc_pokes.push_back(generate_pokemon());
        }
        
         int cur_pokemon, npc_cur_pokemon = 0;
       
        
        battle_display(npc_pokes[npc_cur_pokemon], cur_pokemon, 1);

        if (aggressor->pc) {
            npc = defender;
        } else {
            npc = aggressor;
        }
        
        npc->npc->defeated = 1;
        if (npc->npc->ctype == char_hiker || npc->npc->ctype == char_rival) {
            npc->npc->mtype = move_wander;
        }
    }
    else {
        mvprintw(0, 0, "All your pokemon are fainted, go heal them!");
        getch();
        mvprintw(0, 0, "                                           ");
    }
}
void io_encounter() {
    int odds_escape, turn_used, cur_pokemon, i, encounters_move;
    int attempts = 0;
    char choice, move_choice;
    trainer_pokemon encounter = generate_pokemon();

    cur_pokemon = 0;
    for(i = 0; i < (int)pc_pokes.size(); i++) {
        if(pc_pokes[i].cur_hp != 0) {
            cur_pokemon = i;
            break;
        }
    }
    battle_display(encounter, cur_pokemon, 0);
    
   
}
void io_starter() {
    int choice;
    vector<trainer_pokemon> starters;
    
    starters.push_back(generate_pokemon());
    starters.push_back(generate_pokemon());
    starters.push_back(generate_pokemon());
    
    mvprintw(1, 1, "Chose your starter: ");
    mvprintw(3, 1, "%s, Moves: %s, %s", starters[0].name.c_str(), starters[0].move_one.c_str(), starters[0].move_two.c_str());
    mvprintw(4, 1, "%s, Moves: %s, %s", starters[1].name.c_str(), starters[1].move_one.c_str(), starters[1].move_two.c_str());
    mvprintw(5, 1, "%s, Moves: %s, %s", starters[2].name.c_str(), starters[2].move_one.c_str(), starters[2].move_two.c_str());
    mvprintw(7, 1, "Press 1 for %s, press 2 for %s, or press 3 for %s.", starters[0].name.c_str(), starters[1].name.c_str(), starters[2].name.c_str());
    
    while(true) {
        choice = getch();
        if(choice == '1' || choice == '2' || choice == '3') {
            choice -= 49;
            pc_pokes.push_back(starters[choice]);
            break;
        }
        else {
            mvprintw(9, 1, "Not a valid input.");
            refresh();
        }
    }
    
    mvprintw(9, 1, "You picked %s!", pc_pokes[0].name.c_str());
    mvprintw(11, 1, "Press any key to continue.");
    
    getch();
}
uint32_t move_pc_dir(uint32_t input, pair_t dest)
{
  int num_fainted, i, rand_num;
  num_fainted = 0;
  dest[dim_y] = world.pc.pos[dim_y];
  dest[dim_x] = world.pc.pos[dim_x];

  switch (input) {
  case 1:
  case 2:
  case 3:
    dest[dim_y]++;
    break;
  case 4:
  case 5:
  case 6:
    break;
  case 7:
  case 8:
  case 9:
    dest[dim_y]--;
    break;
  }
  switch (input) {
  case 1:
  case 4:
  case 7:
    dest[dim_x]--;
    break;
  case 2:
  case 5:
  case 8:
    break;
  case 3:
  case 6:
  case 9:
    dest[dim_x]++;
    break;
  case '>':
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_mart) {
      io_pokemart();
    }
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_center) {
      io_pokemon_center();
    }
    break;
  }

  if (world.cur_map->map[dest[dim_y]][dest[dim_x]] == ter_exit) {
  	
  }
  if(world.cur_map->map[dest[dim_y]][dest[dim_x]] == ter_grass){
      for(i = 0; i < (int)pc_pokes.size(); i++) {
          if(pc_pokes[i].cur_hp == 0) {
              num_fainted++;
          }
      }
      if(num_fainted != (int)pc_pokes.size()) {
          rand_num = rand() % 9;
          if(rand_num == 0) {
              io_encounter();
          }
      }
      else {
          mvprintw(0, 0, "You have no healthy pokemon! Get out of the grass!");
          getch();
          mvprintw(0, 0, "                                                  ");
      }
  }
  if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) {
    if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]->npc &&
        world.cur_map->cmap[dest[dim_y]][dest[dim_x]]->npc->defeated) {
      // Some kind of greeting here would be nice
      return 1;
    } else if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]->npc) {
      io_battle(&world.pc, world.cur_map->cmap[dest[dim_y]][dest[dim_x]]);
      // Not actually moving, so set dest back to PC position
      dest[dim_x] = world.pc.pos[dim_x];
      dest[dim_y] = world.pc.pos[dim_y];
    }
  }
  
  if (move_cost[char_pc][world.cur_map->map[dest[dim_y]][dest[dim_x]]] ==
      INT_MAX) {
    return 1;
  }

  return 0;
}

void io_handle_input(pair_t dest)
{
  uint32_t turn_not_consumed;
  int key;

  do {
    switch (key = getch()) {
    case '7':
    case 'y':
    case KEY_HOME:
      turn_not_consumed = move_pc_dir(7, dest);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      turn_not_consumed = move_pc_dir(8, dest);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      turn_not_consumed = move_pc_dir(9, dest);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      turn_not_consumed = move_pc_dir(6, dest);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      turn_not_consumed = move_pc_dir(3, dest);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      turn_not_consumed = move_pc_dir(2, dest);
      break;
    case '1':
    case 'b':
    case KEY_END:
      turn_not_consumed = move_pc_dir(1, dest);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      turn_not_consumed = move_pc_dir(4, dest);
      break;
    case '5':
    case ' ':
    case '.':
    case KEY_B2:
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    case '>':
      turn_not_consumed = move_pc_dir('>', dest);
      break;
    case 'Q':
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      world.quit = 1;
      turn_not_consumed = 0;
      break;
      break;
    case 't':
      io_list_trainers();
      turn_not_consumed = 1;
      break;

    case 'p':
      /* Teleport the PC to a random place in the map.              */
      io_teleport_pc(dest);
      turn_not_consumed = 0;
      break;
    case 'm':
      
    case 'q':
      /* Demonstrate use of the message queue.  You can use this for *
       * printf()-style debugging (though gdb is probably a better   *
       * option.  Not that it matters, but using this command will   *
       * waste a turn.  Set turn_not_consumed to 1 and you should be *
       * able to figure out why I did it that way.                   */
      io_queue_message("This is the first message.");
      io_queue_message("Since there are multiple messages, "
                       "you will see \"more\" prompts.");
      io_queue_message("You can use any key to advance through messages.");
      io_queue_message("Normal gameplay will not resume until the queue "
                       "is empty.");
      io_queue_message("Long lines will be truncated, not wrapped.");
      io_queue_message("io_queue_message() is variadic and handles "
                       "all printf() conversion specifiers.");
      io_queue_message("Did you see %s?", "what I did there");
      io_queue_message("When the last message is displayed, there will "
                       "be no \"more\" prompt.");
      io_queue_message("Have fun!  And happy printing!");
      io_queue_message("Oh!  And use 'Q' to quit!");

      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    default:
      /* Also not in the spec.  It's not always easy to figure out what *
       * key code corresponds with a given keystroke.  Print out any    *
       * unhandled key here.  Not only does it give a visual error      *
       * indicator, but it also gives an integer value that can be used *
       * for that key in this (or other) switch statements.  Printed in *
       * octal, with the leading zero, because ncurses.h lists codes in *
       * octal, thus allowing us to do reverse lookups.  If a key has a *
       * name defined in the header, you can use the name here, else    *
       * you can directly use the octal value.                          */
      mvprintw(0, 0, "Unbound key: %#o ", key);
      turn_not_consumed = 1;
    }
    refresh();
  } while (turn_not_consumed);
}