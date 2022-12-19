/*
	A C program to test the matching function (for master-mind) as implemented in matches.s

$ as  -o mm-matches.o mm-matches.s
$ gcc -c -o testm.o testm.c
$ gcc -o testm testm.o matches.o
$ ./testm
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define LENGTH 3
#define COLORS 3

#define NAN1 8
#define NAN2 9

const int seqlen = LENGTH;
const int seqmax = COLORS;

/* ********************************** */
/* take these fcts from master-mind.c */
/* ********************************** */

/* Show given sequence */
// void showSeq(int *seq);

/* Parse an integer value as a list of digits of base MAX */
// void readSeq(int *seq, int val);

/* display the sequence on the terminal window, using the format from the sample run in the spec */
void showSeq(int *seq)
{
    //Printing the line
    fprintf(stderr,"The value of sequence is : ");
    //Running the for loop for prinitng the value
    for(int i=0;i<seqlen;i++)
    {
        fprintf(stderr,"%d",seq[i]);
    }
    fprintf(stderr,".\n");
}


#define NAN1 8
#define NAN2 9

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
/* or as a pointer to a pair of values */
int * countMatches(int *seq1, int *seq2) 
{
    //Variables to keep count for exact and approximate
    int exact = 0;
    int approx = 0;
    //Allocating the memory for array 
    int *visited = (int*)malloc(seqlen*sizeof(int));
    //Running for loop to check sequence
    for (int i = 0; i < seqlen; i++)
    {
        //Checking if the both values are same or not
        if (seq1[i] == seq2[i])
        {
            //If the values are same then we are incrementing the exact variable
            exact++;
            //If we have already visited the array then we decrementing the approx
            if (visited[i])
            {
                //Decrementing the approx
                approx--;
            }  
            //After incrementing we are saying that we have visited this position
            visited[i] = 1;
        }
        //If the values are not same
        else
        {
            //Then we are running loop qand checking if they are at other position or not
            for (int j = 0; j < seqlen; j++)
            {
                //If the value is same and the position is not visited
                if (seq2[i] == seq1[j] && !visited[j])
                {
                    //Then we are saying that the place is visited
                    visited[j] = 1;
                    //And then we are incrementing the approx
                    approx++;
                    //As we found the match then we breaking the loop
                    break;
                }
            }
        }   
    }
    //Using int pointer to store bith the values
    if (approx < 0){
        approx = 0;
    }
    int *val = malloc(2*sizeof(int));
    //Assigning the values
    val[0] = exact;
    val[1] = approx; 
    //Freeing the memory used for visisted array
    free(visited);
    //Returning the address of val
    return val;   
}

/* show the results from calling countMatches on seq1 and seq1 */
void showMatches(int* code, int *seq1, int *seq2,int lcd_format)
{
    //Printing all the values
	printf("Exact Matches are      : %d\n",code[0]);
	printf("Approx. Matches are    : %d\n",code[1]);
}

/* parse an integer value as a list of digits, and put them into @seq@ */
/* needed for processing command-line with options -s or -u            */
void readSeq(int *seq, int val) 
{
		//Running the loop to copy the values
		for(int i=seqlen;i>0;i--)
		{
			//Storing the value of modulus in a variable (Using Reverse Number Logic)
			int value = val%10;
			//Storing the value in sequence variable
			seq[i-1]=value;
			//Dividing it by 10
			val = val/10;
		}
}

/* read a guess sequence fron stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
//int readNum(int max); 

// The ARM assembler version of the matching fct
//extern int /* or int* */ matches(int *val1, int *val2);

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main (int argc, char **argv) {
	int res, t, t_c, m, n;
	int *seq1, *seq2, *cpy1, *cpy2;
	struct timeval t1, t2 ;
	char str_in[20], str[20] = "some text";
	int verbose = 0, debug = 0, help = 0, opt_s = 0, opt_n = 0;
	int *res_c;
	// see: man 3 getopt for docu and an example of command line parsing
	{ // see the CW spec for the intended meaning of these options
		int opt;
		while ((opt = getopt(argc, argv, "hvs:n:")) != -1) {
			switch (opt) {
			case 'v':
	verbose = 1;
	break;
			case 'h':
	help = 1;
	break;
			case 'd':
	debug = 1;
	break;
			case 's':
	opt_s = atoi(optarg); 
	break;
			case 'n':
	opt_n = atoi(optarg); 
	break;
			default: /* '?' */
	fprintf(stderr, "Usage: %s [-h] [-v] [-s <seed>] [-n <no. of iterations>]  \n", argv[0]);
	exit(EXIT_FAILURE);
			}
		}
	}

	seq1 = (int*)malloc(seqlen*sizeof(int));
	seq2 = (int*)malloc(seqlen*sizeof(int));
	cpy1 = (int*)malloc(seqlen*sizeof(int));
	cpy2 = (int*)malloc(seqlen*sizeof(int));
	
	if (argc > optind+1) {
		strcpy(str_in, argv[optind]);
		m = atoi(str_in);
		strcpy(str_in, argv[optind+1]);
		n = atoi(str_in);
		fprintf(stderr, "Testing matches function with sequences %d and %d\n", m, n);
	} else {
		int i, j, n = 10, res, res_c, oks = 0, tot = 0; // number of test cases
		fprintf(stderr, "Running tests of matches function with %d pairs of random input sequences ...\n", n);
		if (opt_n != 0)
			n = opt_n;
		if (opt_s != 0)
			srand(opt_s);
		else
			srand(1701);
		for (i=0; i<n; i++) {
			for (j=0; j<seqlen; j++) {
	seq1[j] = (rand() % seqlen + 1);
	seq2[j] = (rand() % seqlen + 1);
			}
			memcpy(cpy1, seq1, seqlen*sizeof(int));
			memcpy(cpy2, seq2, seqlen*sizeof(int));
			if (verbose) {
	fprintf(stderr, "Random sequences are:\n");
	showSeq(seq1);
	showSeq(seq2);
			}
			res = matches(seq1, seq2);         // extern; code in matches.s
			memcpy(seq1, cpy1, seqlen*sizeof(int));
			memcpy(seq2, cpy2, seqlen*sizeof(int));
			res_c = countMatches(seq1, seq2);  // local C function
			if (debug) {
	fprintf(stdout, "DBG: sequences after matching:\n");	
	showSeq(seq1);
	showSeq(seq2);
			}
			fprintf(stdout, "Matches (encoded) (in C):   %d\n", res_c);
			fprintf(stdout, "Matches (encoded) (in Asm): %d\n", res);
			memcpy(seq1, cpy1, seqlen*sizeof(int));
			memcpy(seq2, cpy2, seqlen*sizeof(int));
			showMatches(res_c, seq1, seq2, 0);
			showMatches(res, seq1, seq2, 0);
			tot++;
			if (res == res_c) {
	fprintf(stdout, "__ result OK\n");
	oks++;
			} else {
	fprintf(stdout, "** result WRONG\n");
			}
		}
		fprintf(stderr, "%d out of %d tests OK\n", oks, tot);
		exit(oks==tot ? 0 : 1);
	}    

	readSeq(seq1, m);
	readSeq(seq2, n);

	memcpy(cpy1, seq1, seqlen*sizeof(int));
	memcpy(cpy2, seq2, seqlen*sizeof(int));
	memcpy(seq1, cpy1, seqlen*sizeof(int));
	memcpy(seq2, cpy2, seqlen*sizeof(int));
		
	gettimeofday (&t1, NULL) ;
	res_c = countMatches(seq1, seq2);         // local C function
	gettimeofday (&t2, NULL) ;
	// d = difftime(t1,t2);
	if (t2.tv_usec < t1.tv_usec)	// Counter wrapped
		t_c = (1000000 + t2.tv_usec) - t1.tv_usec;
	else
		t_c = t2.tv_usec - t1.tv_usec ;

	if (debug) {
		fprintf(stdout, "DBG: sequences after matching:\n");	
		showSeq(seq1);
		showSeq(seq2);
	}
	memcpy(seq1, cpy1, seqlen*sizeof(int));
	memcpy(seq2, cpy2, seqlen*sizeof(int));
	
	gettimeofday (&t1, NULL) ;
	res = matches(seq1, seq2);         // extern; code in hamming4.s
	gettimeofday (&t2, NULL) ;
	// d = difftime(t1,t2);
	if (t2.tv_usec < t1.tv_usec)	// Counter wrapped
		t = (1000000 + t2.tv_usec) - t1.tv_usec;
	else
		t = t2.tv_usec - t1.tv_usec ;

	if (debug) {
		fprintf(stdout, "DBG: sequences after matching:\n");	
		showSeq(seq1);
		showSeq(seq2);
	}

	memcpy(seq1, cpy1, seqlen*sizeof(int));
	memcpy(seq2, cpy2, seqlen*sizeof(int));
	showMatches(res_c, seq1, seq2, 0);
	showMatches(res, seq1, seq2, 0);

	if (res == res_c) {
		fprintf(stdout, "__ result OK\n");
	} else {
		fprintf(stdout, "** result WRONG\n");
	}
	fprintf(stderr, "C   version:\t\tresult=%d (elapsed time: %dms)\n", res_c, t_c);
	fprintf(stderr, "Asm version:\t\tresult=%d (elapsed time: %dms)\n", res, t);


	return 0;
}
