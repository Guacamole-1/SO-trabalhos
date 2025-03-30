#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <math.h>

double calcPi(long long int terms){

    long long int x = (long long int)pow(10, 10);
    if (terms < x)
    {
        printf("Insert a number larger than 1^10!\n%lld",x);
        exit(EXIT_FAILURE);
    }
    

    double result;
    for (int i = 1; i <= terms; i++)
    {
        result += pow(-1,i+1)/(2*i - 1);
    }
    return 4*result;
    
}

int* DivideWork(int workers){

    double sum = round(workers/2);
    printf("%f",sum);

}

int main(int argc, char const *argv[])
{
    long long int terms;
    if (argc != 2)
    {
        printf("ERROR!\nUsage: %s [number of terms]\n", argv[0]);
        exit(EXIT_FAILURE);
    } else if (!(terms = atoll(argv[1])))
    {
        printf("Argument is not a valid number!\nUsage: %s [number of terms]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    double a = calcPi(terms);
    printf("%0.9f",a);
    



    return 0;
}
