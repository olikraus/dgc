
#include <stdlib.h>
#include <stdio.h>
#include "cmdline.h"
#include "config.h"

int multi_latch_bits = 3;

void multi_latch_transition(int src, int dest)
{
  int i;
  printf("%d %d \t", src, dest);
  for( i = 0; i < multi_latch_bits; i++ )
  {
    if ( (src & (1<<i)) != (dest & (1<<i)) )
    {
      if ( (src & (1<<i)) == 0 )
        printf("i%d+ ", i);
      else
        printf("i%d- ", i);
    }
  }
  printf("\t| ");
  if ( (src>>multi_latch_bits) != (dest>>multi_latch_bits) )
  {
    printf("o%d- ", src>>multi_latch_bits);
    printf("o%d+ ", dest>>multi_latch_bits);
  }
  printf("\n");
}

void multi_latch_zero_tree(int src, int l)
{
  int dest;
  int i;
  for( i = 0; i < multi_latch_bits; i++ )
  {
    if ( (src & (1<<i)) != 0 )
    {
      dest = (src&(~(1<<i)));
      multi_latch_transition(src, dest);
      if ( i >= l )
        multi_latch_zero_tree(dest, i);
    }
  }
}

void multi_latch_output_change(int tree)
{
  int i, j;
  int src, dest;
  for( i = 0; i < (1<<multi_latch_bits); i++ )
  {
    for( j = 0; j < multi_latch_bits; j++ )
    {
      if ( (i & (1<<j)) == 0 )
      {
        src = (tree<<multi_latch_bits)|i;
        dest = (j<<multi_latch_bits)|i|(1<<j);
        multi_latch_transition(src, dest);
        
      }
    }
  }
}

void multi_latch_build(int bits)
{
  int i;
  multi_latch_bits = bits;
  printf("; multi latch (bits: %d, states: %d)\n", bits, bits*(1<<bits));
  for( i = 0; i < bits; i++ )
  {
    printf("input i%d 0\n", i);
    printf("output o%d %d\n", i, i?0:1);
  }
  for( i = 0; i < bits; i++ )
  {
    printf("; output change, o%d = 1\n", i);
    multi_latch_output_change(i);
  }
  for( i = 0; i < bits; i++ )
  {
    printf("; go zero tree, o%d = 1\n", i);
    multi_latch_zero_tree((i<<multi_latch_bits)|((1<<multi_latch_bits)-1), 0);
  }
}

/*-------------------------------------------------------------------------*/

#define AD_PREV  0
#define AD_X     1
#define AD_0_CLK 2
#define AD_1_CLK 3

int ad_bits = 3;

int ad_state(int bit, int value, int sub)
{
  return 2+((((1<<bit)-1+value)<<2) | sub);
}


void ad_tree(int bit, int val, int last)
{
  int nval, i;

  printf("; bit %2d, last value %2d\n", bit, last);
   
  if ( last == 0 )
  {
    printf("%2d %2d c+ |\n",      ad_state(bit,val,AD_PREV), ad_state(bit,val,AD_0_CLK)      );
    printf("%2d %2d i+ |\n",      ad_state(bit,val,AD_PREV), ad_state(bit,val,AD_X)          );
    printf("%2d %2d c+ | o%d+\n", ad_state(bit,val,AD_X),    ad_state(bit,val,AD_1_CLK), bit );
  }
  else
  {
    printf("%2d %2d c+ | o%d+\n", ad_state(bit,val,AD_PREV), ad_state(bit,val,AD_1_CLK),bit );
    printf("%2d %2d i- |\n",      ad_state(bit,val,AD_PREV), ad_state(bit,val,AD_X)     );
    printf("%2d %2d c+ |\n",      ad_state(bit,val,AD_X),    ad_state(bit,val,AD_0_CLK) );
  }

  nval = val + (1<<bit);

  if ( bit >= ad_bits-1 )
  {
    printf("%2d %2d c- |", ad_state(bit,val,AD_0_CLK), 0);
    for( i = 0; i <= bit; i++ )
      if ( (val&(1<<i)) != 0 )
        printf(" o%d-", i);
    printf("\n");
    printf("%2d %2d c- |", ad_state(bit,val,AD_1_CLK), 2);
    for( i = 0; i <= bit; i++ )
      if ( (nval&(1<<i)) != 0 )
        printf(" o%d-", i);
    printf("\n");
    return;
  }

  printf("%2d %2d c- |\n", ad_state(bit,val,AD_0_CLK), ad_state(bit+1,val,AD_PREV));
  printf("%2d %2d c- |\n", ad_state(bit,val,AD_1_CLK), ad_state(bit+1,nval,AD_PREV));

    
  ad_tree(bit+1, val, 0);
  ad_tree(bit+1, nval, 1);
}

void ad_pre()
{
  int i;
  printf("input i 0\n");
  printf("input c 0\n");
  for( i = 0; i < ad_bits; i++ )
    printf("output o%d 0\n", i);
  printf("%2d %2d i+ |\n", 0, 1);
  printf("%2d %2d c+ | o0+\n", 1, ad_state(0,0,AD_1_CLK));
  printf("%2d %2d c+ |\n", 0, ad_state(0,0,AD_0_CLK));
  
}

/*-------------------------------------------------------------------------*/

long g_ml_cnt = 0;
long g_ad_bits = 0;

cl_entry_struct cl_list[] =
{
  { CL_TYP_LONG,    "ml-multi-latch", &g_ml_cnt, 0 },
  { CL_TYP_LONG,    "ad-successive-approximation analog digital converter", &g_ad_bits, 0 },
  CL_ENTRY_LAST
};


int main(int argc, char **argv)
{

  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }

  if ( g_ml_cnt > 0 )
  {
    multi_latch_build(g_ml_cnt);
    return 1;
  }

  if ( g_ad_bits > 0 )
  {
    ad_bits = g_ad_bits;
    ad_pre();
    ad_tree(0, 0, 1);
    return 1;
  }
   
  puts("BMS-Generator");
  puts(COPYRIGHT);
  puts(FREESOFT);
  puts(REDIST);
  printf("Usage: %s [options]\n", argv[0]);
  puts("options:");
  cl_OutHelp(cl_list, stdout, " ", 14);
    
  return 1;
}
