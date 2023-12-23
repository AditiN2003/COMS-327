#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define HEIGHT 21
#define WIDTH 80
#define MOUNTAIN '%'
#define TREES '^'
#define ROADS '#'
#define BUILDINGC 'C'
#define BUILDINGM 'M'
#define TALLGRASS ':'
#define CLEARINGS '.'
#define WATER '~'

void printMap(char Map[HEIGHT][WIDTH]){
  
    int r, c;
  for (r=0; r<HEIGHT;r++){
      for (c=0;c<WIDTH;c++){
        if(c==0){
         printf("\n");
        }
         printf("%c", Map[r][c]);
      }
    }
}

void createMap(char Map[HEIGHT][WIDTH]){
  int r, c;
  for (r=0; r<HEIGHT;r++){
      for (c=0;c<WIDTH;c++){
        if (c==0||r==0||c==WIDTH-1||r==HEIGHT-1){
            Map[r][c]=MOUNTAIN;
        }else{
            Map[r][c]=CLEARINGS;
        }
      }
    }
}

int randomGenerator(int upper, int lower){
  return (rand() % (upper - lower + 1)) + lower;
}


int addBuildings(char Map[HEIGHT][WIDTH], char letter ){

  int hBuild = randomGenerator(HEIGHT-2,1);
  int wBuild = randomGenerator(WIDTH-2,1);
  if( Map[hBuild][wBuild]==ROADS){
    if (Map[hBuild][wBuild-2]==CLEARINGS && Map[hBuild][wBuild-1]==CLEARINGS && Map[hBuild+1][wBuild-2]==CLEARINGS && Map[hBuild+1][wBuild-1]==CLEARINGS)
    {
      Map[hBuild][wBuild-2]=letter;
      Map[hBuild][wBuild-1]=letter;
      Map[hBuild+1][wBuild-2]=letter;
      Map[hBuild+1][wBuild-1]=letter;
    }else{
      return -1;
    }
    
  }else{
    return -1; 
  }


return 0;

}

int AddTrees(char Map[HEIGHT][WIDTH], char letter ){

  int hTrees = randomGenerator(HEIGHT-2,1);
  int wTrees = randomGenerator(WIDTH-2,1);
  if( Map[hTrees][wTrees]!=ROADS&&Map[hTrees][wTrees]!=MOUNTAIN && Map[hTrees][wTrees]!=BUILDINGC && Map[hTrees][wTrees]!=BUILDINGM){
    Map[hTrees][wTrees]=TREES;
  }

return 0;
}

void setExitsPaths(char Map[HEIGHT][WIDTH]){
  int top = randomGenerator(WIDTH-2,1);
  int bottom = randomGenerator(WIDTH-2,1);
  int left = randomGenerator(HEIGHT-2,1);
  int right = randomGenerator(HEIGHT-2,1);

  Map[0][top]=ROADS;
  Map[HEIGHT-1][bottom]=ROADS;
  Map[left][0]=ROADS;
  Map[right][WIDTH-1]=ROADS;
  
  int startPath = (rand() % (6)) + 5;
  int diagnol = (rand() % (6)) + 5;

  
  //adding  starting path
  for(int k=0;k<startPath;k++) {
    Map[k][top]=ROADS;
    Map[left][k]=ROADS;
  }

  int NSPath  = bottom - top;
  int EWPath  = right - left;

  int heightNS = startPath - 1;
  int widthNS=top;

  int heightEW = left;
  int widthEW= startPath-1;

//adding diagnols
for (int k1 = 0; k1<diagnol; k1++){
  if (NSPath >0){ //diganol for NS
    widthNS=widthNS+1;
  }else if (NSPath <0){
    widthNS=widthNS-1;
  }
  
  heightNS=heightNS+1;
  Map[heightNS][widthNS]=ROADS;
  } 

  for (int k1 = 0; k1<diagnol; k1++){
  
  if (EWPath >0){ //dagnol for EW
    heightEW=heightEW+1;
  }else if (EWPath <0){
    heightEW=heightEW-1;
  }

  widthEW=widthEW+1;
  if(Map[heightEW][widthEW]==MOUNTAIN){

    k1=diagnol;
    break;
  }
      Map[heightEW][widthEW]=ROADS;

  } 

  if(heightEW!=right-1){
  int EWDist=right-heightEW;
for (int d = 0; d<abs(EWDist);d++){
  if(EWDist<0){
    heightEW=heightEW-1;
  }else{
    heightEW=heightEW+1;
  }
      Map[heightEW][widthEW]=ROADS;
  } 

} 

if(widthEW!=WIDTH-1){
  int unitsEW = WIDTH-widthEW;
 for (int k2 = 0; k2< unitsEW-2;k2++){
    widthEW=widthEW+1;
    Map[heightEW][widthEW]=ROADS;
 }
}

if(widthNS!=bottom-1){
  int unitsNS=bottom-widthNS;
for (int d = 0; d<abs(unitsNS);d++){
  if(unitsNS<0){
widthNS=widthNS-1;
  }else{  
    widthNS=widthNS+1;
  }
      Map[heightNS][widthNS]=ROADS;

  } 

}

if(heightNS!=HEIGHT-1){
  int unitsNS1 = HEIGHT-heightNS;
  //IF DIAGNOLS DONE AND PATH IS STILL NOT FILLED FOR NS
 for (int k2 = 0; k2< unitsNS1-2;k2++){
  heightNS=heightNS+1;
    Map[heightNS][widthNS]=ROADS;
 }
}

int returnVal;
int returnVal1;

 do {
   returnVal = addBuildings(Map, BUILDINGC);
} while (returnVal != 0);

 do {
   returnVal1 = addBuildings(Map, BUILDINGM);
} while (returnVal1 != 0);
}

void region(char Map[HEIGHT][WIDTH], char letter){
  int h1 = randomGenerator(HEIGHT-2,1);
  int h2 = randomGenerator(HEIGHT-2,1);
  int w1 = randomGenerator(WIDTH-2,1);
  int w2 = randomGenerator(WIDTH-2,1);

  int r, c;
  int startR, startC, maxR, maxC; 

  if(h2>h1){
    startR=h1;
    maxR=h2;
  }else{
    startR=h2;
    maxR=h1;
  }

  if(w2>w1){
    startC=w1;
    maxC=w2;
  }else{
    startC=w2;
    maxC=w1;
  }

  for (r = startR; r<maxR; r++){
    for (c = startC; c<maxC; c++){
      if(Map[r][c]!=ROADS && Map[r][c]!=BUILDINGC&&Map[r][c]!=BUILDINGM){
        Map[r][c]=letter;
      }
    }
  }

}

int main(int argc, char *argv[])
{
    int i;
    char Map[HEIGHT][WIDTH];
    int tressCount = rand()% 10; 
    srand(time(NULL));
    createMap(Map);
    setExitsPaths(Map);
    region(Map, TALLGRASS); //tall grass
    region(Map, TALLGRASS); //tall grass
    region(Map, TALLGRASS); //tall grass
    region(Map, WATER); //water
    region(Map, WATER); //water
    for (i = 0; i<tressCount;i++){
      AddTrees(Map, TREES);
    }
    printMap(Map);

    
  return 0;
}