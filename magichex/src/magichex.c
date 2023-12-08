#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct var Var;

/* constraint variable; if lo==hi, this is the variable's value */
typedef struct var {
  long id; /* variable id; id<0 if the variable is not part of the hexagon */
  long lo; /* lower bound */
  long hi; /* upper bound */
} Var;

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

   The variable array is organized as a single-dimension array with accesses
   vs[y*r+x]
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


int sethi(Var *v, long x) {
  assert(v->id >= 0);
  if (x < v->hi) {
    v->hi = x;
    if (v->lo <= v->hi)
      return 1;
    else
      return 0;
  }
  return 2;
}

int setlo(Var *v, long x) {
  assert(v->id >= 0);
  if (x > v->lo) {
    v->lo = x;
    if (v->lo <= v->hi)
      return 1;
    else
      return 0;
  }
  return 2;
}

/* returns 0 if there is no solution, 1 if one of the variables has changed */
int lessthan(Var *v1, Var *v2)
{
  assert(v1->id >= 0);
  assert(v2->id >= 0);
  int f = sethi(v1, v2->hi-1);
  if (f < 2)
    return f;
  return (setlo(v2, v1->lo+1));
}

int sum(Var vs[], unsigned long nv, unsigned long stride, long sum,
        Var *vsstart, Var *vsend)
{
  unsigned long i;
  long hi = sum;
  long lo = sum;
  Var *vp;
#if 0
  printf("sum(vsstart+%ld, %lu, %lu, %ld, vsstart, vsstart+%ld)   ",vs-vsstart,nv,stride,sum,vsend-vsstart); fflush(stdout);
  for (i=0, vp=vs; i<nv; i++, vp+=stride) {
    assert(vp>=vsstart);
    assert(vp<vsend);
    assert(vp->id >= 0);
    printf("v%ld ",vp->id);
  }
  printf("\n");
#endif
  for (i=0, vp=vs; i<nv; i++, vp+=stride) {
    assert(vp>=vsstart);
    assert(vp<vsend);
    assert(vp->id >= 0);
    hi -= vp->lo;
    lo -= vp->hi;
  }
  /* hi is the upper bound of sum-sum(vs), lo the lower bound */
  for (i=0, vp=vs; i<nv; i++, vp+=stride) {
    int f = sethi(vp,hi+vp->lo); /* readd vp->lo to get an upper bound of vp */
    assert(vp>=vsstart);
    assert(vp<vsend);
    assert(vp->id >= 0);
    if (f < 2)
      return f;
    f = setlo(vp,lo+vp->hi); /* likewise, readd vp->hi */
    if (f < 2)
      return f;
  }
  return 2;
}
    
/* reduce the ranges of the variables as much as possible (with the
   constraints we use);  returns 1 if all variables still have a
   non-empty range left, 0 if one has an empty range */
int solve(unsigned long n, long d, Var vs[])
{
  unsigned long r = 2*n-1;
  unsigned long H = 3*n*n-3*n+1;
  long M = d*H;
  long o = d*r - (H-1)/2; /* offset in occupation array */
  unsigned long occupation[H]; /* if vs[i] has value x, occupation[x-o]==i, 
                                  if no vs[*] has value x, occupation[x-o]==H*/
  unsigned long corners[] = {0, n-1, (n-1)*r+0, (n-1)*r+r-1, (r-1)*r+n-1, (r-1)*r+r-1};
  unsigned long i;
  //printf("(re)start\n");
  /* deal with the alldifferent constraint */
  for (i=0; i<H; i++)
    occupation[i] = r*r;
 restart:
  for (i=0; i<r*r; i++) {
    Var *v = &vs[i];
    if (v->lo == v->hi && occupation[v->lo-o] != i) {
      if (occupation[v->lo-o] < r*r)
        return 0; /* another variable has the same value */
      occupation[v->lo-o] = i; /* occupy v->lo */
      goto restart;
    }
  }
  /* now propagate the alldifferent results to the bounds */
  for (i=0; i<r*r; i++) {
    Var *v = &vs[i];
    if (v->lo < v->hi) {
      if (occupation[v->lo-o] < r*r) {
        v->lo++;
        goto restart;
      }
      if (occupation[v->hi-o] < r*r) {
        v->hi--;
        goto restart;
      }
    }
  }
  /* the < constraints; all other corners are smaller than the first
     one (eliminate rotational symmetry) */
  for (i=1; i<sizeof(corners)/sizeof(corners[0]); i++) {
    int f = lessthan(&vs[corners[0]],&vs[corners[i]]);
    if (f==0) return 0;
    if (f==1) goto restart;
  }
  /* eliminate the mirror symmetry between the corners to the right
     and left of the first corner */
  {
    int f = lessthan(&vs[corners[2]],&vs[corners[1]]); 
    if (f==0) return 0;
    if (f==1) goto restart;
  }
  /* sum constraints: each line and diagonal sums up to M */
  /* line sum constraints */
  for (i=0; i<r; i++) {
    int f;
    /* line */
    f = sum(vs+r*i+max(0,i+1-n), min(i+n,r+n-i-1), 1, M, vs, vs+r*r);
    if (f==0) return 0;
    if (f==1) goto restart;
    /* column (diagonal down-left in the hexagon) */
    f = sum(vs+i+max(0,i+1-n)*r, min(i+n,r+n-i-1), r, M, vs, vs+r*r);
    if (f==0) return 0;
    if (f==1) goto restart;
    /* diagonal (down-right) */
    f = sum(vs-n+1+i+max(0,n-i-1)*(r+1), min(i+n,r+n-i-1), r+1, M, vs, vs+r*r);
    if (f==0) return 0;
    if (f==1) goto restart;
  }
  return 1;
}

void printhexagon(unsigned long n, Var vs[])
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
      Var *v=&vs[i*r+j];
      assert(v->lo <= v->hi);
#if 0
      printf("%6ld  ",v->id);
#else
      if (v->lo < v->hi)
        printf("%4ld-%-3ld",v->lo,v->hi);
      else
        printf("%6ld  ",v->lo);
#endif
    }
    printf("\n");
  }
}

/* assign values to vs[index] and all later variables in vs such that
   the constraints hold */
void labeling(unsigned long n, long d, Var vs[], unsigned long index)
{
  long i;
  unsigned long r = 2*n-1;
  Var *vp = vs+index;
  if (index >= r*r) {
    printhexagon(n,vs);
    solutions++;
    leafs++;
    printf("leafs visited: %lu\n\n",leafs);
    return;
  }
  if (vp->id < 0)
    return labeling(n,d,vs,index+1);
  for (i = vp->lo; i <= vp->hi; i++) {
    Var newvs[r*r];
    Var* newvp=newvs+index;
    memmove(newvs,vs,r*r*sizeof(Var));
    newvp->lo = i;
    newvp->hi = i;
#if 0
    for (Var *v = newvs; v<=newvp; v++) {
      if (v->id >= 0) {
        assert(v->lo == v->hi);
        printf(" %ld",v->lo); fflush(stdout);
      }
    }
    printf("\n");
#endif
    if (solve(n,d,newvs))
      labeling(n,d,newvs,index+1);
    else
      leafs++;
  }
}

Var *makehexagon(unsigned long n, long d)
{
  unsigned long i,j;
  unsigned long r = 2*n-1;
  unsigned long H = 3*n*n-3*n+1;
  
  Var *vs = calloc(r*r,sizeof(Var));
  unsigned long id = 0;
  for (i=0; i<r*r; i++) {
    Var *v = &vs[i];
    v->id = -1;
    v->lo = 1;
    v->hi = 0;
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
      Var *v=&vs[i*r+j];
      assert(v->lo>v->hi);
      v->id = id++;
      v->lo = d*r - (H-1)/2;
      v->hi = d*r + (H-1)/2;
    }
  }
  return vs;
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
  Var *vs = makehexagon(n,d);
  for (i=3; i<argc; i++) {
    while (vs[j].id < 0)
      j++;
    vs[j].lo = vs[j].hi = strtol(argv[i],NULL,10);
    j++;
  }
  labeling(n,d,vs,0);
  printf("%lu solution(s), %lu leafs visited\n",solutions, leafs);
  //(void)solve(n, d, vs);
  //printhexagon(n, vs);
  return 0;
}
