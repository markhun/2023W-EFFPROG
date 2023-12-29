#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define PLACEHOLDER_ENTRY_ID -1 // Placeholder entries are not actually part of the hexagon but used for padding.

/* constraint variable; if lo==hi, this is the variable's value */
struct hexagonEntry {
  long id; /* variable id; id == PLACEHOLDER_ENTRY_ID if the variable is not part of the hexagon */
  long lo; /* lower bound */
  long hi; /* upper bound */
};

typedef struct hexagonEntry HexagonEntry;

/* representation of a hexagon of order n: (2n-1)^2 square array
   for a hexagon of order 2:
     A B
    C D E
     F G
   the representation is:
    A B .
    C D E
    . F G
   The . slots have lo>hi.
   The . slots have id == PLACEHOLDER_ENTRY_ID

   The variable array is organized as a single-dimension array with accesses
   hexagon[y*r+x]
   This allows to access the diagonal with stride 2*order

   Variable names n, r, H, S according to German Wikipedia Article
   Instead of "i", the deviation variable is called "d" (d=0 means
   that the sum=0; to have the lowest value 1, d=2)
   
   n is the order (number of elements of a side of the hexagon).
   r = 2n-1 (length of the middle row/diagonals)
   H = 3n^2-3n+1 (number of variables)
   M = dH (sum of each row/diagonal)
   lowest  value = dr - (H-1)/2
   highest value = dr + (H-1)/2
*/

unsigned long solutions = 0; /* counter of solutions */
unsigned long leafs = 0; /* counter of leaf nodes visited in the search tree */

long min(long a, long b)
{
  return (a<b)?a:b;
}

long max(long a, long b)
{
  return (a>b)?a:b;
}


/* unless otherwise noted, the following functions return

   0 if there is no solution (i.e., the action eliminates all values
   from a variable),

   1 if there was a change 

   2 if there was no change 
*/
typedef enum RESULTTYPE { NOSOLUTION = 0, CHANGED = 1, NOCHANGE = 2 } CHANGE_IDENTIFIER;  


CHANGE_IDENTIFIER sethi(HexagonEntry *hexagonEntry, long x) {
  assert(hexagonEntry->id != PLACEHOLDER_ENTRY_ID);
  if (x < hexagonEntry->hi) {
    hexagonEntry->hi = x;
    if (hexagonEntry->lo <= hexagonEntry->hi)
      return CHANGED;
    else
      return NOCHANGE;
  }
  return NOCHANGE;
}

CHANGE_IDENTIFIER setlo(HexagonEntry *hexagonEntry, long x) {
  assert(hexagonEntry->id != PLACEHOLDER_ENTRY_ID);
  if (x > hexagonEntry->lo) {
    hexagonEntry->lo = x;
    if (hexagonEntry->lo <= hexagonEntry->hi)
      return CHANGED;
    else
      return NOSOLUTION;
  }
  return NOCHANGE;
}

/* returns NOSOLUTION if there is no solution, CHANGED if one of the variables has changed */
CHANGE_IDENTIFIER lessthan(HexagonEntry *v1, HexagonEntry *v2)
{
  assert(v1->id != PLACEHOLDER_ENTRY_ID);
  assert(v2->id != PLACEHOLDER_ENTRY_ID);
  CHANGE_IDENTIFIER f = sethi(v1, v2->hi-1);
  if (f == NOSOLUTION || f == CHANGED)
    return f;
  return (setlo(v2, v1->lo+1));
}

CHANGE_IDENTIFIER sum(HexagonEntry hexagon[], unsigned long nv, unsigned long stride, long sum,
        HexagonEntry *hexagonStart, HexagonEntry *hexagonEnd)
{
  unsigned long i;
  long hi = sum;
  long lo = sum;
  HexagonEntry *hexagonEntry_p;
#if 0
  printf("sum(hexagonStart+%ld, %lu, %lu, %ld, hexagonStart, hexagonStart+%ld)   ",hexagon-hexagonStart,nv,stride,sum,hexagonEnd-hexagonStart); fflush(stdout);
  for (i=0, hexagonEntry_p=hexagon; i<nv; i++, hexagonEntry_p+=stride) {
    assert(hexagonEntry_p>=hexagonStart);
    assert(hexagonEntry_p<hexagonEnd);
    assert(hexagonEntry_p->id != PLACEHOLDER_ENTRY_ID);
    printf("hexagonEntry%ld ",hexagonEntry_p->id);
  }
  printf("\n");
#endif
  for (i=0, hexagonEntry_p=hexagon; i<nv; i++, hexagonEntry_p+=stride) {
    assert(hexagonEntry_p>=hexagonStart);
    assert(hexagonEntry_p<hexagonEnd);
    assert(hexagonEntry_p->id != PLACEHOLDER_ENTRY_ID);
    hi -= hexagonEntry_p->lo;
    lo -= hexagonEntry_p->hi;
  }
  /* hi is the upper bound of sum-sum(hexagon), lo the lower bound */
  for (i=0, hexagonEntry_p=hexagon; i<nv; i++, hexagonEntry_p+=stride) {
    CHANGE_IDENTIFIER f = sethi(hexagonEntry_p,hi+hexagonEntry_p->lo); /* readd hexagonEntry_p->lo to get an upper bound of hexagonEntry_p */
    assert(hexagonEntry_p>=hexagonStart);
    assert(hexagonEntry_p<hexagonEnd);
    assert(hexagonEntry_p->id != PLACEHOLDER_ENTRY_ID);
    if (f == NOSOLUTION || f == CHANGED)
      return f;
    f = setlo(hexagonEntry_p,lo+hexagonEntry_p->hi); /* likewise, readd hexagonEntry_p->hi */
    if (f == NOSOLUTION || f == CHANGED)
      return f;
  }
  return NOCHANGE;
}
    
/* reduce the ranges of the variables as much as possible (with the
   constraints we use);  returns 1 if all variables still have a
   non-empty range left, 0 if one has an empty range */
bool solve(unsigned long n, long d, HexagonEntry hexagon[])
{
  unsigned long r = 2*n-1;
  unsigned long H = 3*n*n-3*n+1;
  long M = d*H;
  long o = d*r - (H-1)/2; /* offset in occupation array */
  unsigned long occupation[H]; /* if hexagon[i] has value x, occupation[x-o]==i, 
                                  if no hexagon[*] has value x, occupation[x-o]==H */
  unsigned long corners[] = {0, n-1, (n-1)*r+0, (n-1)*r+r-1, (r-1)*r+n-1, (r-1)*r+r-1};
  unsigned long i;
  //printf("(re)start\n");
  /* deal with the alldifferent constraint */
  for (i=0; i<H; i++)
    occupation[i] = r*r;
 restart:
  for (i=0; i<r*r; i++) {
    HexagonEntry *hexagonEntry = &hexagon[i];
    if (hexagonEntry->lo == hexagonEntry->hi && occupation[hexagonEntry->lo-o] != i) {
      if (occupation[hexagonEntry->lo-o] < r*r)
        return false; /* another variable has the same value */
      occupation[hexagonEntry->lo-o] = i; /* occupy hexagonEntry->lo */
      //goto restart;
    }
  }
  bool k = false;
  /* now propagate the alldifferent results to the bounds */
  for (i=0; i<r*r; i++) {
    HexagonEntry *hexagonEntry = &hexagon[i];
    if (hexagonEntry->lo < hexagonEntry->hi) {
      if (occupation[hexagonEntry->lo-o] < r*r) {
        hexagonEntry->lo++;
        k = true;
      }
      if (occupation[hexagonEntry->hi-o] < r*r) {
        hexagonEntry->hi--;
        k = true;
      }
    }
  }
  if(k){
    goto restart;
  }
  /* the < constraints; all other corners are smaller than the first
     one (eliminate rotational symmetry) */
  for (i=1; i<sizeof(corners)/sizeof(corners[0]); i++) {
    CHANGE_IDENTIFIER f = lessthan(&hexagon[corners[0]],&hexagon[corners[i]]);
    if (f==NOSOLUTION) return false;
    if (f==CHANGED) goto restart;
  }
  /* eliminate the mirror symmetry between the corners to the right
     and left of the first corner */
  {
    CHANGE_IDENTIFIER f = lessthan(&hexagon[corners[2]],&hexagon[corners[1]]); 
    if (f==NOSOLUTION) return false;
    if (f==CHANGED) goto restart;
  }
  /* sum constraints: each line and diagonal sums up to M */
  /* line sum constraints */
  for (i=0; i<r; i++) {
    CHANGE_IDENTIFIER f;
    /* line */
    f = sum(hexagon+r*i+max(0,i+1-n), min(i+n,r+n-i-1), 1, M, hexagon, hexagon+r*r);
    if (f==NOSOLUTION) return false;
    if (f==CHANGED) goto restart;
    /* column (diagonal down-left in the hexagon) */
    f = sum(hexagon+i+max(0,i+1-n)*r, min(i+n,r+n-i-1), r, M, hexagon, hexagon+r*r);
    if (f==NOSOLUTION) return false;
    if (f==CHANGED) goto restart;
    /* diagonal (down-right) */
    f = sum(hexagon-n+1+i+max(0,n-i-1)*(r+1), min(i+n,r+n-i-1), r+1, M, hexagon, hexagon+r*r);
    if (f==NOSOLUTION) return false;
    if (f==CHANGED) goto restart;
  }
  return true;  // all done
}

void printhexagon(unsigned long n, HexagonEntry hexagon[])
{
  unsigned long i,j;
  unsigned r=2*n-1;
  for (i=0; i<r; i++) {
    unsigned long l=0;
    unsigned long h=r;
    if (i+1>n)
      l = i+1-n;
    if (i+1<n)
      h = n+i;
    for (j=h-l; j<r; j++)
      printf("    ");
    for (j=l; j<h; j++) {
      assert(i<r);
      assert(j<r);
      HexagonEntry *hexagonEntry=&hexagon[i*r+j];
      assert(hexagonEntry->lo <= hexagonEntry->hi);
#if 0
      printf("%6ld  ",hexagonEntry->id);
#else
      if (hexagonEntry->lo < hexagonEntry->hi)
        printf("%4ld-%-3ld",hexagonEntry->lo,hexagonEntry->hi);
      else
        printf("%6ld  ",hexagonEntry->lo);
#endif
    }
    printf("\n");
  }
}

unsigned long spiralPermutation(unsigned long n, unsigned long r, unsigned long i)
{
  unsigned long index;

  if (n == 1) {
    index = 0;
  } else if (i >= 6*(n-1)) {
    index = spiralPermutation(n-1,r,i-6*(n-1))+r+1;
  } else if (i >= 3*(n-1)) {
    index = (r+1)*(2*n-2) - spiralPermutation(n,r,i-3*(n-1));
  } else {
    int x = i < n ? i : n-1;
    i -= x;
    int y = i < n ? i : n-1;
    i -= y;
    index = x + y*(r+1) + i*r;
  }
  return index;
}

/* assign values to hexagon[index] and all later variables in hexagon such that
   the constraints hold */
void labeling(unsigned long n, long d, HexagonEntry hexagon[], unsigned long index)
{
  long i;
  unsigned long r = 2*n-1;
  unsigned long entryIndex = spiralPermutation(n,r,index);
  HexagonEntry *hexagonEntry = &hexagon[entryIndex];
  if (index >= r*r) {
    printhexagon(n,hexagon);
    solutions++;
    leafs++;
    printf("leafs visited: %lu\n\n",leafs);
    return;
  }
  if (hexagonEntry->id == PLACEHOLDER_ENTRY_ID)
    // call labeling again as we do not need to process placeholders
    return labeling(n,d,hexagon,index+1); 
  for (i = hexagonEntry->lo; i <= hexagonEntry->hi; i++) {
    HexagonEntry newHexagon[r*r];
    HexagonEntry* newHexagonEntry = &newHexagon[entryIndex];
    memmove(newHexagon,hexagon,r*r*sizeof(HexagonEntry));
    newHexagonEntry->lo = i;
    newHexagonEntry->hi = i;
#if 0
    for (HexagonEntry *hexagonEntry = newHexagon; hexagonEntry<=newHexagonEntry; hexagonEntry++) {
      if (hexagonEntry->id != PLACEHOLDER_ENTRY_ID) {
        assert(hexagonEntry->lo == hexagonEntry->hi);
        printf(" %ld",hexagonEntry->lo); fflush(stdout);
      }
    }
    printf("\n");
#endif
    if (solve(n,d,newHexagon))
      labeling(n,d,newHexagon,index+1);
    else
      leafs++;
  }
}

HexagonEntry *makehexagon(unsigned long n, long d)
{
  unsigned long i,j;
  unsigned long r = 2*n-1;
  unsigned long H = 3*n*n-3*n+1;
  
  HexagonEntry *hexagon = calloc(r*r,sizeof(HexagonEntry));
  unsigned long id = 0;
  for (i=0; i<r*r; i++) {
    hexagon[i].id = PLACEHOLDER_ENTRY_ID;
    hexagon[i].lo = 1;
    hexagon[i].hi = 0;
  }
  for (i=0; i<r; i++) {
    unsigned long l=0;
    unsigned long h=r;
    if (i+1>n)
      l = i+1-n;
    if (i+1<n)
      h = n+i;
    for (j=l; j<h; j++) {
      assert(i<r);
      assert(j<r);
      assert(hexagon[i*r+j].lo>hexagon[i*r+j].hi);
      hexagon[i*r+j].id = id++;
      hexagon[i*r+j].lo = d*r - (H-1)/2;
      hexagon[i*r+j].hi = d*r + (H-1)/2;
    }
  }
  return hexagon;
}

int main(int argc, char *argv[])
{
  unsigned long i;
  unsigned long j=0;
  unsigned long n;
  long d;
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <order> <deviation> <value> ... <value>\n", argv[0]);
    exit(1);
  }
  n = strtoul(argv[1],NULL,10);
  if (n<1) {
    fprintf(stderr, "order must be >=1\n");
    exit(1);
  }
  d = strtol(argv[2],NULL,10);
  HexagonEntry *hexagon = makehexagon(n,d);
  for (i=3; i<argc; i++) {
    while (hexagon[j].id == PLACEHOLDER_ENTRY_ID)
      j++; // skip this entry as it is a placeholder
    hexagon[j].lo = hexagon[j].hi = strtol(argv[i],NULL,10);
    j++;
  }
  labeling(n,d,hexagon,0);
  printf("%lu solution(s), %lu leafs visited\n",solutions, leafs);
  //(void)solve(n, d, hexagon);
  //printhexagon(n, hexagon);
  return 0;
}
