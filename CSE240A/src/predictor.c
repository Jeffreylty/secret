//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Tianyi Liu & Sihao Liu";
const char *studentID   = "";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//
int gp_states;
int g_history;
int * g_table;    
int g_mask;
int pc_mask;

int * l_his_table; 
int * l_predic_table;
int * choice_table;  
int l_mask;
//
//TODO: Add your own Branch Predictor data structures here
//


int get_mask(int n)
{
	int mask = 0;
	for(int i = 0; i< n; i++)
	{
		mask <<= 1;  
		mask |= 1;
	} 
	return (mask);
}


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  switch (bpType)
  {
  case GSHARE:
    g_mask = get_mask(ghistoryBits);
    gp_states = 1 << ghistoryBits;  
    pc_mask = get_mask(pcIndexBits);
    g_table =  (int*) malloc(sizeof(int) * gp_states);
    for(int i = 0; i<gp_states; i++) {g_table[i] = WT;}	
    break;
  case TOURNAMENT:
    l_mask = get_mask(lhistoryBits);
    pc_mask = get_mask(pcIndexBits);
    g_mask = get_mask(ghistoryBits);
    int lht_size, lpt_size, ct_size;

    //local history table
    lht_size = 1 << pcIndexBits;
    l_his_table = (int*) malloc(sizeof(int) * lht_size);
    for(int i = 0; i < lht_size; i++){
      l_his_table[i] = 0;
    }

    //local prediction table
    lpt_size = 1 << lhistoryBits;
    l_predic_table = (int*) malloc(sizeof(int) * lpt_size);
    for(int i =0; i< lpt_size; i++){
      l_predic_table[i] = WT; 
    }

    //global prediction table
    gp_states = 1 << ghistoryBits;  
    g_table =  (int*) malloc(sizeof(int) * gp_states);

    //choise table
    ct_size = 1 << ghistoryBits;
    choice_table = (int*) malloc(sizeof(int) * ct_size);

       for(int i = 0; i<gp_states; i++) {
         g_table[i] = WT;
         choice_table[i] = WT; // weakly select gshare
         }	
    break;
  case CUSTOM:
    break;
  default:
    break;
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      g_history = g_mask & g_history;
      int index = pc & pc_mask;
      int predic = index ^ g_history ;
      if(g_table[predic] >= 2){
        return TAKEN;
      }
      return NOTTAKEN;
    case TOURNAMENT:
      g_history = g_mask & g_history;
      int choice = choice_table[g_history];
      if(choice >=2)
      {
        int index = pc & pc_mask;
        int predic = index ^ g_history ;
        if(g_table[predic] >= 2){
          return TAKEN;
        }
      }else{
          int index = pc_mask & pc;
          int predic = l_mask & l_his_table[index];
          if(l_predic_table[predic] >= 2){
            return TAKEN;
          }
        }
      return NOTTAKEN;
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  int index;
  switch (bpType)
  {
    case GSHARE:
      g_history = g_mask & g_history;
      index = pc & pc_mask;
      int predic = index ^ g_history ;
      if(outcome == TAKEN){
        if(g_table[predic] <= 2){
          g_table[predic]++;
        }
      }else{
        if(g_table[predic] >= 1){
          g_table[predic]--;
        }
      }
      g_history = ((g_history << 1) | outcome) & g_mask; 
      return;
    case TOURNAMENT:
      g_history = g_mask & g_history;
      index = pc & pc_mask;
      int g_predic = g_table[(index ^ g_history)];
      int g_outcome;
      if(g_predic >= 2){
        g_outcome = TAKEN;
      }else{
        g_outcome = NOTTAKEN;
      }

      int l_predic = l_predic_table[ (l_his_table[index]) ];
      int l_outcome;

      if(l_predic >=2 ){
        l_outcome = TAKEN;
      }else{
        l_outcome = NOTTAKEN;
      }


      if(outcome == TAKEN){
        if(g_predic <= 2){
          g_table[(index ^ g_history)]++;
        }

        if(l_predic <=2){
          l_predic_table[ (l_his_table[index]) ]++;
        }
      }else{
        if(g_predic >= 1){
          g_table[(index ^ g_history)]--;
        }
        
        if(l_predic >= 1){
          l_predic_table[ (l_his_table[index]) ]--;
        }
      }

      int g = g_outcome == outcome ? 0:1;
      int l = l_outcome == outcome ? 0:1;
      if(g >l){
        if(choice_table[g_history] <=2){choice_table[g_history]++;}
      }else if( l < g){
        if(choice_table[g_history] >=1 ){choice_table[g_history]--;}
      }
      g_history = ((g_history << 1) | outcome) & g_mask; 
      return;
    case CUSTOM:
    default:
      break;
  }
}
