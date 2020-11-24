//David Lee
//08-03-2020

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

//function to check user input is an integer
//reference from https://www.tutorialspoint.com/how-to-check-if-an-input-is-an-integer-using-c-cplusplus
int is_num(char * str) {
    for (int i = 0; i < strlen(str); i++)
        if (isdigit(str[i]) < 1)
            return 0; //when one non numeric value is found, return false
    return 1;
}

//Function to produce encryption to stdout
    int main(int argc, char* argv[]) {

        // Check usage & args
        if (argc < 2) {
            fprintf(stderr,"USAGE: %s length\n", argv[0]);
            exit(0);
        }

        if (!is_num(argv[1])) { //checks if argv[1] is a number or integer
            fprintf(stderr,"%s is an incorrect input. Please enter a + integer for length.\n", argv[1]);
            exit(1);
        }

        char key_char[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; //27 characters from A - Z and Space
        char key[atoi(argv[1])+1];
        srand(time(NULL)); //Random seed used to randomize output
        int i;
        for (i = 0; i < atoi(argv[1]); i++){ //loop through number of count given in argument 1
            key[i] = key_char[rand() % 27]; //store randomized char
        }

    key[atoi(argv[1])] = '\0';

    printf("%s\n", key); //output key

    return 0;
}