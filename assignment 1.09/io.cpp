#include <unistd.h>
#include <ncurses.h>
#include <cctype>
#include <cstdlib>
#include <climits>
#include <string>
#include <vector>
#include "io.h"
#include "character.h"
#include "poke327.h"
#include "pokemon.h"
#include "db_parse.h"

typedef struct io_message {
  /* Will print " --more-- " at end of line when another message follows. *
   * Leave 10 extra spaces for that.                                      */
  char msg[71];
  struct io_message *next;
} io_message_t;

static io_message_t *io_head, *io_tail;

class weapons {
public:
    int pokeballs;
    int potions;
    int revives;
    weapons(int pokeballs, int potions, int revives) {
        this->pokeballs = pokeballs;
        this->potions = potions;
        this->revives = revives;
    }
};
weapons weaponsBag(5, 5, 5);

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

  if (!(tmp = (io_message_t *) malloc(sizeof (*tmp)))) {
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
  const character *const *c1 = (const character * const *) v1;
  const character *const *c2 = (const character * const *) v2;

  return (world.rival_dist[(*c1)->pos[dim_y]][(*c1)->pos[dim_x]] -
          world.rival_dist[(*c2)->pos[dim_y]][(*c2)->pos[dim_x]]);
}

static character *io_nearest_visible_trainer()
{
  character **c, *n;
  uint32_t x, y, count;

  c = (character **) malloc(world.cur_map->num_trainers * sizeof (*c));

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
  character *c;

  clear();
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.cur_map->cmap[y][x]) {
        mvaddch(y + 1, x, world.cur_map->cmap[y][x]->symbol);
      } else {
        switch (world.cur_map->map[y][x]) {
        case ter_boulder:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, BOULDER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_mountain:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, MOUNTAIN_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_tree:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, TREE_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_forest:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, FOREST_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_path:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, PATH_SYMBOL);
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_gate:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, GATE_SYMBOL);
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_bailey:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, BAILEY_SYMBOL);
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_mart:
          attron(COLOR_PAIR(COLOR_BLUE));
          mvaddch(y + 1, x, POKEMART_SYMBOL);
          attroff(COLOR_PAIR(COLOR_BLUE));
          break;
        case ter_center:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(y + 1, x, POKEMON_CENTER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_RED));
          break;
        case ter_grass:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, TALL_GRASS_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_clearing:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, SHORT_GRASS_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_water:
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, WATER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_CYAN));
          break;
        default:
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, ERROR_SYMBOL);
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
    mvprintw(22, 55, "%c at vector %d%cx%d%c.",
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
                                                [dest[dim_x]]] ==
             DIJKSTRA_PATH_MAX                                            ||
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

static void io_list_trainers_display(npc **c, uint32_t count)
{
  uint32_t i;
  char (*s)[40]; /* pointer to array of 40 char */

  s = (char (*)[40]) malloc(count * sizeof (*s));

  mvprintw(3, 19, " %-40s ", "");
  /* Borrow the first element of our array for this string: */
  snprintf(s[0], 40, "You know of %d trainers:", count);
  mvprintw(4, 19, " %-40s ", *s);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++) {
    snprintf(s[i], 40, "%16s %c: %2d %s by %2d %s",
             char_type_name[c[i]->ctype],
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
  npc **c;
  uint32_t x, y, count;

  c = (npc **) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = dynamic_cast<npc *> (world.cur_map->cmap[y][x]);
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
  mvprintw(0, 0, "Welcome to the Pokemart. Could I interest you in some Pokeballs?");
  if(weaponsBag.pokeballs < 5){
    weaponsBag.pokeballs = 5;
  }
  if(weaponsBag.potions < 5){
    weaponsBag.potions = 5;
  }
  if(weaponsBag.revives < 5){
    weaponsBag.revives = 5;
    }
    mvprintw(10,1, "Your supplies are now restored :)");
  refresh();
  getch();
}

void io_pokemon_center()
{
  mvprintw(0, 0, "Welcome to the Pokemon Center. Let's restore and heal your pokemons. ");
   for (int i = 0; i < 6 && (world.pc.buddy[i]); i++)
  {
    world.pc.buddy[i]->effective_stat[stat_hp] = 12;
  }
      mvprintw(10,1, "Your pokemons are now restored :)");

  refresh();
  getch();
}

int fixPoke(char pick) {
	int i;
  int poke;
  int numBuddies=0;

	clear();
  
    mvprintw(1, 1, "Which pokemon do you want to heal? Press any key to escape.");
  
    for (i = 0; i < 6 && world.pc.buddy[i]; i++) {
  numBuddies++;
    printw("%d) %s%s%s: HP:%d ATK:%d DEF:%d SPATK:%d SPDEF:%d SPEED:%d %s\n",i+1,
                   world.pc.buddy[i]->is_shiny() ? "*" : "",world.pc.buddy[i]->get_species(),
                   world.pc.buddy[i]->is_shiny() ? "*" : "", world.pc.buddy[i]->get_hp(),world.pc.buddy[i]->get_atk(),
                   world.pc.buddy[i]->get_def(), world.pc.buddy[i]->get_spatk(), world.pc.buddy[i]->get_spdef(),
                   world.pc.buddy[i]->get_speed(), world.pc.buddy[i]->get_gender_string());
     }

   
    
    while(true) {
        poke = getch();
        if(poke >= '1' && poke <= '6') {
            poke -= 48;
            if(poke > numBuddies) {
                mvprintw(10, 1, "You don't have that many pokemon.");
                getch();
            }
            else {
                poke--;
                break;
            }
        }
        else {
			    return 0;
        }
    }
    
    if(pick == '2') {
        if( world.pc.buddy[poke]-> get_hp() >= 0) {
            weaponsBag.potions--;
             world.pc.buddy[poke]->effective_stat[stat_hp] += 20;
            mvprintw(10, 1, "Used a Potion! Remaining: %d.", weaponsBag.potions);
            getch();
            return 0;
        }
        else {
            mvprintw(10, 1, "You can't heal a pokemon at 0 or Full HP.");
            getch();
            return 0;
        }
    }
    else {
        if(world.pc.buddy[poke]-> get_hp()<= 0) {
            weaponsBag.revives--;
             world.pc.buddy[poke]->effective_stat[stat_hp] =world.pc.buddy[poke]->effective_stat[stat_hp]/2;

            mvprintw(10, 1, "Used a Revive! Remaining: %d.", weaponsBag.revives);
            getch();
            return 0;
        }
        else {
            mvprintw(10, 1, "You can't revive a non fainted pokemon.");
            getch();
            return 0;
        }
    }
}

int bagOp(char choice, int cur_pokemon) {
 int numBuddies=0;
 int i;

  for (i = 0; i < 6 && world.pc.buddy[i]; i++) {
  numBuddies++;
   
     }
    if(choice == '1') {
		if(weaponsBag.pokeballs > 0) {
			if(numBuddies< 6) {
				weaponsBag.pokeballs--;
				mvprintw(10, 1, "Used a Pokeball! You now have %d.", weaponsBag.pokeballs);
				getch();
				return 2;
			}else {
				mvprintw(10, 1, "Party is full.");
				getch();
				return 3;
			}			
		}
		else {
      mvprintw(10, 1, "You're out of pokeballs.");
      getch();
      return 4;			
		  }
    }
    else if(choice == '2') {
		if(weaponsBag.potions > 0) {
			return fixPoke(choice);
		}
        else {
            mvprintw(10, 1, "You're out of potions.");
            getch();
            return 4;			
		}
    }
    else if(choice == '3') {
		if(weaponsBag.revives > 0) {
			return fixPoke(choice);
		}
        else {
            mvprintw(10, 1, "You're out of revives.");
            getch();
            return 4;			
		}
    }
    else {
        return 4;
    }
}
void changePokemons(int cur, int type) {
    int i;
    int choice;
    class pokemon* temp = world.pc.buddy[cur];
    int numBuddies=0;
	 
    for (i = 0; i < 6 && world.pc.buddy[i]; i++) {
    numBuddies++;
     }
    
    mvprintw(10, 1, "Which Pokemon would you like to to use?");
    if(type == 0) {
        mvprintw(14, 1, "Press any other key to exit.");
    }
    while(true) {
        choice = getch();
        if(choice >= '1' && choice <= '6') {
            choice -= 48;
            if(choice > numBuddies) {
                mvprintw(12, 1, "Not valid. You don't have that many pokemon.");
                getch();
            }
            else {
                choice--;
                if( world.pc.buddy[choice]->get_hp() <= 0) {
                    mvprintw(12, 1, "You cannot use a pokemon with 0 HP.");
                    getch();
                }
                else if(choice == cur) {
                    mvprintw(12, 1, "You are already using that pokemon.");
                    getch();
                }
                else {
                    world.pc.buddy[cur] = world.pc.buddy[choice];
                    world.pc.buddy[choice] = temp;
                        mvprintw(12, 1, "Your current Pokemon is now: Species: %s, Speed: %d, HP: %d", world.pc.buddy[cur]->get_species(),  world.pc.buddy[cur]->get_speed(), world.pc.buddy[cur]->get_hp());
                    getch();
                    break;
                }
            }
        }
        else {
            if(type == 1) {
                mvprintw(12, 1, "Please select a Pokemon.");
                getch();
            }
            else {
                break;
            }
        }
    }
}

void io_battle(character *aggressor, character *defender)
{
  npc *n = (npc *)((aggressor != &world.pc) ? defender : aggressor);
  npc *x = (npc *)((aggressor == &world.pc) ? defender : aggressor);
  int user=1;
  int i, choice, weaponPick, num;
  int printLine = 0;
  int turn = 0;
 
  clear();
  mvprintw(1, 1, "A trainer has challenged you!");

    mvprintw(2, 1, "Your Pokemon: Species: %s, Speed: %d, HP: %d", world.pc.buddy[0]->get_species(),  world.pc.buddy[0]->get_speed(), world.pc.buddy[0]->get_hp());
  printw("\n What would you like to do?");
    mvprintw(5, 1, "1. Fight");
    mvprintw(6, 1, "2. Bag");
    mvprintw(7, 1, "3. Run");
    mvprintw(8, 1, "4. Pokemon");

    choice = getch();
    clear();
if (choice == '1'){
   for (num = 0; num < 6 && x->buddy[num]; num++)
  {
    if (printLine > 24)
    {
      clear();
      printLine = 0;
    }
    mvprintw(printLine, 0, "NPCs pokemon: ");
    printLine++;

    mvprintw(printLine, 0, "%s", x->buddy[num]->get_species());
    printLine+=2;
    mvprintw(printLine, 0, "Your pokemon: ");
    printLine++;
    mvprintw(printLine, 0, "1. %s", n->buddy[0]->get_species());
    printLine++;
    for (i = 1; i < 2 && n->buddy[i]; i++)
    {
      mvprintw(printLine, 0, "%d. %s", i + 1, n->buddy[i]->get_species());
      refresh();
    }
printLine+=2;
    mvprintw(printLine, 0, "Enter a number to pick one. if the input isnt working, its because your Pokemon need to recharge. Press q to continue ");
    printLine+=2;
    refresh();

    bool repeat = true;
    do
    {
      refresh();
      user = getch() - '0';

      if ((user >= 1 && user <= 6) && n->buddy[user - 1] != NULL && n->buddy[0]->effective_stat[stat_hp] > 0)
      {
        mvprintw(printLine, 0, "You chose %s!", n->buddy[user - 1]->get_species());
        refresh();
        repeat = false;
      }
      else if (user == 65)
      {
        num = 6;
        break;
      }
      else
      {
        mvprintw(printLine, 0, "Invalid. Try again.");
        refresh();
      }

      if (printLine > 24)
      {
        clear();
        printLine = 0;
      }

    } while (repeat);
    printLine++;

    if (n->buddy[0]->effective_stat[stat_hp] < 0)
    {
      num = 6;
      break;
    }
    int tempNum = num;

    while (user < 6 && n->buddy[user - 1]->effective_stat[stat_hp] > 1 && x->buddy[tempNum]->effective_stat[stat_hp] > 1)
    {
      if (turn == 0)
      { 
        mvprintw(printLine, 0, "Your moves: ");
        refresh();
        printLine++;

        for (i = 0; i < 4; i++)
        {
          if (n->buddy[user - 1]->get_move(i)[0] != '\0')
            printw(" %d: %s ", i + 1, n->buddy[user - 1]->get_move(i));
        }
        refresh();

        mvprintw(printLine, 0, "Choose your move. Enter the number.");
        refresh();
        printLine++;
        int userMove = 0;
        bool repeat = true;
        do
        {
          refresh();
          userMove = getch() - '0';

          if ((userMove >= 1 && userMove <= 4) && n->buddy[user - 1]->get_move(userMove - 1)[0] != '\0')
          {
            mvprintw(printLine, 0, "You picked %s!", n->buddy[user - 1]->get_move(userMove - 1));
            refresh();
            repeat = false;
          }
          else if (user == 65)
          {
            num = 6;
            break;
          }
          else
          {
            mvprintw(printLine, 0, "Invalid %d. Try again.", userMove);
            refresh();
          }
          if (printLine > 24)
          {
            clear();
            printLine = 0;
          }
        } while (repeat);
        printLine++;
        if (rand() % 100 < moves[n->buddy[user - 1]->move_index[userMove]].accuracy + 75)
        {
          if (printLine > 24)
          {
            clear();
            printLine = 0;
          }
          mvprintw(printLine, 0,  "Nice! Pokemon is defeated :)");
          refresh();
          printLine += 2;
          int level = n->buddy[user - 1]->level;
          int power = moves[n->buddy[user - 1]->move_index[userMove]].power;
          int random = rand() % 16 + 85;
          double equation = (((((2 * level) / 5) * power) / 50) + 2);
          x->buddy[num]->effective_stat[stat_hp] = (x->buddy[num]->effective_stat[stat_hp] - equation * random) - 12;
          turn = 1;
          if (x->buddy[num]->effective_stat[stat_hp] < 1)
          {
            num++;
            break;
          }
        }
        else
        {
          mvprintw(printLine, 0,  "Oops. Attack missed!");
          refresh();
          printLine += 2;
          turn = 1;
        }
      }
      else
      { 
        if (printLine > 24)
        {
          clear();
          printLine = 0;
        }
        mvprintw(printLine, 0, "NPCs Move: ");
        refresh();
        printLine++;
        mvprintw(printLine, 0, "%s ", x->buddy[num]->get_move(0));
        refresh();
        printLine++;

        if (rand() % 100 < moves[x->buddy[num]->move_index[0]].accuracy + 50)
        {
          mvprintw(printLine, 15, "Nice! Pokemon is defeated :)");
          refresh();
          printLine += 2;
          int level = x->buddy[num]->level;
          int power = moves[x->buddy[num]->move_index[0]].power;
          int random = rand() % 16 + 85;
          double equation = (((((2 * level) / 5) * power) / 50) + 2);
          n->buddy[user - 1]->effective_stat[stat_hp] = (n->buddy[user - 1]->effective_stat[stat_hp] - equation * random) - 12;

          turn = 0;
          num = 6;
          break;
        }
        else
        {
          mvprintw(printLine, 15, "Oops. Attack missed!");
          refresh();
          printLine += 2;
          turn = 0;
        }
      }
    }
  }

  if (user > 6 && n->buddy[0]->effective_stat[stat_hp] < 0)
  {
    mvprintw(printLine, 0, "Your pokemon needs to recharge. Press B or go to the Bag. Press any key to continue!");
    refresh();
    printLine += 2;
  }
  else if(user == 65 || turn == 0){
    mvprintw(printLine, 0, "Try again later. Press any key to continue!");
    refresh();
    printLine += 2;
  }
  else
  {
    mvprintw(printLine, 0, "Good job! You defeated the NPC! Press any key to continue.");
    refresh();
    printLine += 2;
    x->defeated = 1;
    if (x->ctype == 1 || x->ctype == 2)
    {
      x->mtype = move_wander;
    }
  }
}else if(choice == '2'){
  clear();
    
    mvprintw(1, 1, "Supplies: ");
    mvprintw(3, 1, "1. Pokeballs: %d", weaponsBag.pokeballs);
    mvprintw(4, 1, "2. Potions: %d", weaponsBag.potions);
    mvprintw(5, 1, "3. Revives: %d", weaponsBag.revives);
    mvprintw(7, 1, "Press any other key to exit.");

  mvprintw(9, 1, "Pick 2 to heal a pokemon using potions. Pick 3 to revive a pokemon.");

   weaponPick = getch();
    if(weaponPick == '1') {
        mvprintw(9, 1, "You can't catch other trainers pokemon!");
        getch();
                    
  }
  else {
      bagOp(weaponPick, user - 1);
      clear();
        mvprintw(10, 1, "Your Pokemon: Species: %s, Speed: %d, HP: %d", world.pc.buddy[user - 1]->get_species(),  world.pc.buddy[user - 1]->get_speed(), world.pc.buddy[user - 1]->get_hp());
        printw("\n Pick one: ");
          mvprintw(13, 1, "1. Fight");
          mvprintw(14, 1, "2. Bag");
          mvprintw(15, 1, "3. Run");
          mvprintw(16, 1, "4. Pokemon");
  }
}else if(choice == '3'){
  mvprintw(15, 1, "You can't escape from trainer battles!");
  getch();
}else if(choice == '4'){
   clear();
    
    mvprintw(1, 1, "Your Pokemon: ");
    
    for (i = 0; i < 6 && world.pc.buddy[i]; i++) {
    printw("%d) %s%s%s: HP:%d ATK:%d DEF:%d SPATK:%d SPDEF:%d SPEED:%d %s\n",i+1,
                   world.pc.buddy[i]->is_shiny() ? "*" : "",world.pc.buddy[i]->get_species(),
                   world.pc.buddy[i]->is_shiny() ? "*" : "", world.pc.buddy[i]->get_hp(),world.pc.buddy[i]->get_atk(),
                   world.pc.buddy[i]->get_def(), world.pc.buddy[i]->get_spatk(), world.pc.buddy[i]->get_spdef(),
                   world.pc.buddy[i]->get_speed(), world.pc.buddy[i]->get_gender_string());
     }
   changePokemons(user - 1, 0);

}else{
  mvprintw(0, 0, "Invalid input.");
}
  refresh();
  getch();
}

uint32_t move_pc_dir(uint32_t input, pair_t dest)
{
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

  if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) {
    if (dynamic_cast<npc *> (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) &&
        ((npc *) world.cur_map->cmap[dest[dim_y]][dest[dim_x]])->defeated) {
      // Some kind of greeting here would be nice
      return 1;
    } else if ((dynamic_cast<npc *>
                (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]))) {
      io_battle(&world.pc, world.cur_map->cmap[dest[dim_y]][dest[dim_x]]);
      // Not actually moving, so set dest back to PC position
      dest[dim_x] = world.pc.pos[dim_x];
      dest[dim_y] = world.pc.pos[dim_y];
    }
  }
  
  if (move_cost[char_pc][world.cur_map->map[dest[dim_y]][dest[dim_x]]] ==
      DIJKSTRA_PATH_MAX) {
    return 1;
  }

  if (world.cur_map->map[dest[dim_y]][dest[dim_x]] == ter_gate &&
      dest[dim_y] != world.pc.pos[dim_y]                       &&
      dest[dim_x] != world.pc.pos[dim_x]) {
    return 1;
  }

  return 0;
}

void io_teleport_world(pair_t dest)
{
  /* mvscanw documentation is unclear about return values.  I believe *
   * that the return value works the same way as scanf, but instead   *
   * of counting on that, we'll initialize x and y to out of bounds   *
   * values and accept their updates only if in range.                */
  int x = INT_MAX, y = INT_MAX;
  
  world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = NULL;

  echo();
  curs_set(1);
  do {
    mvprintw(0, 0, "Enter x [-200, 200]:           ");
    refresh();
    mvscanw(0, 21, (char *) "%d", &x);
  } while (x < -200 || x > 200);
  do {
    mvprintw(0, 0, "Enter y [-200, 200]:          ");
    refresh();
    mvscanw(0, 21, (char *) "%d", &y);
  } while (y < -200 || y > 200);

  refresh();
  noecho();
  curs_set(0);

  x += 200;
  y += 200;

  world.cur_idx[dim_x] = x;
  world.cur_idx[dim_y] = y;

  new_map(1);
  io_teleport_pc(dest);
}

void io_handle_input(pair_t dest)
{
  uint32_t turn_not_consumed;
  int key;
 char pick;

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
    case 'B':
      clear();
    
      mvprintw(1, 1, "Supplies: ");
      mvprintw(3, 1, "1. Pokeballs: %d", weaponsBag.pokeballs);
      mvprintw(4, 1, "2. Potions: %d", weaponsBag.potions);
      mvprintw(5, 1, "3. Revives: %d", weaponsBag.revives);
      mvprintw(7, 1, "Press any other key to exit.");

      mvprintw(9, 1, "Pick 2 to heal a pokemon using potions. Pick 3 to revive a pokemon.");
      pick = getch();
      if(pick == '1') {
        mvprintw(9, 1, "Can not use a pokeball outside of battle.");
        getch();
      }else if(pick == '2') {
					if(weaponsBag.potions > 0) {
						fixPoke(pick);
					}
					else {
						mvprintw(9, 1, "No potions left :(");
						getch();			
					}
				}else if(pick == '3') {
					if(weaponsBag.revives > 0) {
						fixPoke(pick);
					}
					else {
						mvprintw(9, 1, "No revives left :(");
						getch();			
					}
				}
          io_display();
          turn_not_consumed = 1;
          break;
    case 'p':
      /* Teleport the PC to a random place in the map.              */
      io_teleport_pc(dest);
      turn_not_consumed = 0;
      break;
    case 'f':
      /* Fly to any map in the world.                                */
      io_teleport_world(dest);
      turn_not_consumed = 0;
      break;    
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

void io_encounter_pokemon()
{
  class pokemon *p;

  p = new class pokemon();

  io_queue_message("%s%s%s: HP:%d ATK:%d DEF:%d SPATK:%d SPDEF:%d SPEED:%d %s",
                   p->is_shiny() ? "*" : "", p->get_species(),
                   p->is_shiny() ? "*" : "", p->get_hp(), p->get_atk(),
                   p->get_def(), p->get_spatk(), p->get_spdef(),
                   p->get_speed(), p->get_gender_string());
  io_queue_message("%s's moves: %s %s", p->get_species(),
                   p->get_move(0), p->get_move(1));

  // Later on, don't delete if captured
  delete p;
}

void io_choose_starter()
{
  class pokemon *choice[3];
  int i;
  bool again = true;
  
  choice[0] = new class pokemon();
  choice[1] = new class pokemon();
  choice[2] = new class pokemon();

  echo();
  curs_set(1);
  do {
    mvprintw( 4, 20, "Before you are three Pokemon, each of");
    mvprintw( 5, 20, "which wants absolutely nothing more");
    mvprintw( 6, 20, "than to be your best buddy forever.");
    mvprintw( 8, 20, "Unfortunately for them, you may only");
    mvprintw( 9, 20, "pick one.  Choose wisely.");
    mvprintw(11, 20, "   1) %s", choice[0]->get_species());
    mvprintw(12, 20, "   2) %s", choice[1]->get_species());
    mvprintw(13, 20, "   3) %s", choice[2]->get_species());
    mvprintw(15, 20, "Enter 1, 2, or 3: ");

    refresh();
    i = getch();

    if (i == '1' || i == '2' || i == '3') {
      world.pc.buddy[0] = choice[(i - '0') - 1];
      delete choice[(i - '0') % 3];
      delete choice[((i - '0') + 1) % 3];
      again = false;
    }
  } while (again);
  noecho();
  curs_set(0);
}
