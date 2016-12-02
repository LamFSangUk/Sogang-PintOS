#include "arith_fp.h"

int
int_to_FP(int integer){
	return integer * NUM_1;
}

int
FP_to_int(int decimal){
	return decimal / NUM_1;
}

int
FP_to_int_round_off(int decimal){
	if(decimal>=0) return (decimal+NUM_1/2)/NUM_1;
	else return (decimal-NUM_1/2)/NUM_1;
}

int 
add_FP_to_FP(int dec_a,int dec_b){
	return dec_a+dec_b;
}

int
mul_FP_to_FP(int dec_a,int dec_b){
	return (int)((long long)dec_a * dec_b / NUM_1);
}

int
sub_FP_to_FP(int dec_a,int dec_b){
	return dec_a-dec_b;
}

int
div_FP_to_FP(int dec_a,int dec_b){
	return (int)((long long)dec_a * NUM_1 / dec_b);
}
