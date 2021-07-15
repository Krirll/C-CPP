#include<stdio.h>
double f(double eps);
int main(){
	double eps;
    printf("enter a number -> ");
	int res1 = scanf("%lf", &eps);
    if (eps >= 1) printf("must be < 1\n");
    else {
        if (res1 != EOF){
	        double res = f(eps);
	        printf("%lf\n", res);
        }
    }
	return 0;
}
//objdump -d -M intel a.out|less