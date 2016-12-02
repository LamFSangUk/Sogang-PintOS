#define FP_SIGN 1
#define FP_INTEGER 17
#define FP_DECIMAL 14

#define NUM_1 (1 << FP_DECIMAL)

int int_to_FP(int integer);
int FP_to_int(int decimal);
int FP_to_int_round_off(int decimal);
int add_FP_to_FP(int dec_a,int dec_b);
int mul_FP_to_FP(int dec_a,int dec_b);
int sub_FP_to_FP(int dec_a,int dec_b);
int div_FP_to_FP(int dec_a,int dec_b);
