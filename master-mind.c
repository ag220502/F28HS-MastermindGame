
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>



//========================================= Constants Declaration ==========================================

//Constants for printing errors
#define DEBUG
#undef ASM_CODE

//Constants for LED's
//Constant for Green LED
#define LED 13
//Constant for Red LED
#define LED2 5
//Constant for Buttons
#define BUTTON 19

//Constant for delays in code
// in mili-seconds: 0.2s
#define DELAY   200
// in micro-seconds: 3s
#define TIMEOUT 3000000

//Contant for number of colors
#define COLS 3
//Constant for length of sequence
#define SEQL 3

//Other Constants for program
#ifndef	TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT		0
#define	OUTPUT		1

#define	LOW			0
#define	HIGH		1

#define NAN1 8
#define NAN2 9

//Static variables
static const int colors = COLS;
static const int seqlen = SEQL;

static char* color_names[] = { "red", "green", "blue" };

static int* theSeq = NULL;

static int *seq1, *seq2, *cpy1, *cpy2;

//Constant for masking with bottom pins
#define	PI_GPIO_MASK	(0xFFFFFFC0)

//Static variable for gpio
static unsigned int gpiobase ;
static uint32_t *gpio ;
static int timed_out = 0;

//======================================= Function Prototyping           ========================================
//Function to print the error message
int failure (int fatal, const char *message, ...);

//Function to delay for specific time
void delay (unsigned int howLong);
//Function to delay for less than 100 micro seconds 
void delayMicroseconds (unsigned int howLong);

//Function to set the mode for the pin
void pinMode(uint32_t *gpio, int pin, int mode);
//Function to write value on specific LED
void writeLED(uint32_t *gpio, int led, int value);
//Function to read whether button is clicked or not
int readButton(uint32_t *gpio, int button);
//Function to Blink LED for specific number of times(c)
void blinkN(uint32_t *gpio, int led, int c);

//Function to initialize the random secret sequence 
void initSeq();
//Function to display the sequence on console
void showSeq(int *seq);
//Function to count number of exact and approximate matches in sequence
int* countMatches(int *seq1, int *seq2);
//Function to print the result of countMatches function
void showMatches( int* code,int *seq1, int *seq2,int lcd_format);
//Function to parse the value into seq variable
void readSeq(int *seq, int val);

//======================================= Delay Function Declarations    ========================================

//Function to delay for specific time
void delay (unsigned int howLong)
{
	struct timespec sleeper, dummy ;
	sleeper.tv_sec  = (time_t)(howLong / 1000) ;
	sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;
	nanosleep (&sleeper, &dummy) ;
}

//Function to delay for less than 100 micro seconds 
void delayMicroseconds (unsigned int howLong)
{
	struct timespec sleeper ;
	unsigned int uSecs = howLong % 1000000 ;
	unsigned int wSecs = howLong / 1000000 ;

		if (howLong ==   0)
			return ;
	#if 0
		else if (howLong  < 100)
			delayMicrosecondsHard (howLong) ;
	#endif
		else
		{
			sleeper.tv_sec  = wSecs ;
			sleeper.tv_nsec = (long)(uSecs * 1000L) ;
			nanosleep (&sleeper, NULL) ;
		}
}

//======================================= Pin Function Declarations      ========================================

//Function to set the mode for the pin
void pinMode(uint32_t *gpio, int pin, int mode)
{
    int rslt;
    //setting the register for the pin
    int fSel = pin/10;
    // setting the value to shift 
    int shift =  (pin%10)*3;
	asm
	(
		/* inline assembler version of setting LED to ouput" */
		// Putting the gpio value into R1 register
        "\tLDR R1, %[gpio]\n"
		// add the fsel value to R1 and store the result into R0
        "\tADD R0, R1, %[fSel]\n"
		// load the value from R0 to R1
        "\tLDR R1, [R0, #0]\n"
		//put the binary value of 7 into R2
        "\tMOV R2, #0b111\n"
		// Shift 111, shift number of times to the left
        "\tLSL R2, %[shift]\n"
		// bitwise clear those bits (not 111)
        "\tBIC R1, R1, R2\n"
		// put the mode into R2
        "\tMOV R2, %[mode]\n"
		// shift 1, shift number of times
        "\tLSL R2, %[shift]\n"
		// or the 2 values found
        "\tORR R1, R2\n"
		//store the value from R1 to R0
        "\tSTR R1, [R0, #0]\n"
		// Store R2 into the output variable result
        "\tMOV %[result], R1\n"
		// output operands
        : [result] "=r" (rslt)
        // : [act] "r" (PIN)
		// Input operands
        : [gpio] "m" (gpio)
        , [fSel] "r" (fSel * 4)
        , [shift] "r" (shift)
        , [mode] "r" (mode)
		//clobbers - the registers being used
        :"r0", "r1","r2", "cc"
	);
}

//Function to write value on specific LED
void writeLED(uint32_t *gpio, int led, int value)
{
	int onOff =0;
	int rslt;
	//If the value is high, then turning On the LED
	if(value==HIGH){
		onOff = 7;
	}
	//Else turning Off the LED
	else{
		onOff = 10;
	}
	asm volatile(
	// Putting the gpio value into R1 register
	"\tLDR R1, %[gpio]\n"
	// add the (register number * 4)onff to R1 and store the result into R0
	"\tADD R0, R1, %[onOff]\n"
	// store 1 into R2
	"\tMOV R2, #1\n"
	//move the pin number into R1
	"\tMOV R1, %[led]\n" 
	//AND the pin value with 31
	"\tAND R1, #31\n"
	// Shift R2, R1 number of times to the left
	"\tLSL R2, R1\n"
	// store the value in R2 inside R0
	"\tSTR R2, [R0, #0]\n"
	// Store R2 into the output variable result
	"\tMOV %[result], R2\n"
	// output operands
	: [result] "=r" (rslt)
	// Input operands
	: [led] "r" (led)
	, [gpio] "m" (gpio)
	, [onOff] "r" (onOff*4)
	//clobbers - the registers being used
	: "r0", "r1", "r2", "cc");
}

//Function to read whether button is clicked or not
int readButton(uint32_t *gpio, int button)
{
	int rslt = 0;
    int gplev = 13 * 4;
	int btnvalue=LOW;
	asm volatile(

		// load the value if gpio + gplev into R0
        "\tLDR R0, [%[gpio], %[gplev]]\n"
		// move the value of button into R1
        "\tMOV R1, %[button]\n"
		// AND the value from R1 with 31
        "\tAND R1, #31\n"
		// Put 1 into R2
        "\tMOV R2, #1\n"
		// Left shift the value in R2, R1 times 
        "\tLSL R2, R1\n"
		// AND the value of R0 with the value in R2
        "\tAND R0, R2\n"
		// move the value of R0 to the result
        "\tMOV %[result], R0\n"
		// output operands
        :[result] "=r" (rslt)
		// input operands
        :[button] "r" (button)
        ,[gpio] "r" (gpio)
        ,[gplev] "r" (gplev)
		//clobbers - the registers being used
        : "r0", "r1", "r2","cc"
	);
    if(rslt != 0){
		btnvalue = HIGH;
	}
	else{
		btnvalue = LOW;
	}
	return btnvalue;
}


//Function to Blink LED for specific number of times(c)
void blinkN(uint32_t *gpio, int led, int c) 
{
	for(int i=0;i<c;i++)
	{
		if ((led & 0xFFFFFFC0) == 0)
    	{    
			writeLED(gpio,led,HIGH);
			delay(500);
			writeLED(gpio,led,LOW);
			delay(200);
		}	
	}
}

//======================================= Match Function Declarations    ========================================

//Function to count number of exact and approximate matches in sequence
int* countMatches(int *seq1, int *seq2) 
{
    //Variables to keep count for exact and approximate
    int exact = 0;
    int approx = 0;
    //Allocating the memory for array 
    int *visited = (int*)malloc(seqlen*sizeof(int));

	for(int i = 0; i<seqlen;i++){
		visited[i] = 0;
	}

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
    int *val = malloc(2*sizeof(int));
    //Assigning the values
    val[0] = exact;
    val[1] = approx; 
    //Freeing the memory used for visisted array
    free(visited);
    //Returning the address of val
    return val;   
}

//Function to print the result of countMatches function
void showMatches( int* code,int *seq1, int *seq2,int lcd_format) 
{
	//Printing all the values
	printf("Exact Matches are      : %d\n",code[0]);
	printf("Approx. Matches are    : %d\n",code[1]);
}

//======================================= Sequence Function Declarations ========================================

//Function to initialize the random secret sequence 
void initSeq() 
{
    //Dynamically allocated the memory for theSeq variable
    theSeq = (int*)malloc(seqlen * sizeof(int));
	//Writing this so that new random sequence is generated everytime
	srand(time(NULL));
	//Running for loop to generate numbers according to seqlen
	for(int i=0;i<seqlen;i++)
	{
		//Generating random number between 1 and seqlen
		int random = rand()%seqlen + 1;
		//Creating the sequence
		theSeq[i] = random;
	}
}

//Function to display the sequence on console
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

//Function to parse the value into seq variable
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

//======================================= Faliure Function Declarations ========================================

//Function to print error message
int failure (int fatal, const char *message, ...)
{
	va_list argp ;
	char buffer [1024] ;

	if (!fatal) //  && wiringPiReturnCodes)
		return -1 ;

	va_start (argp, message) ;
	vsnprintf (buffer, 1023, message, argp) ;
	va_end (argp) ;

	fprintf (stderr, "%s", buffer) ;
	exit (EXIT_FAILURE) ;

	return 0 ;
}

//======================================= Game Function Declarations    ========================================

//Function to print the starting round
void printRound(int num)
{
	// fprintf(stderr,"=======================================\n");
	// fprintf(stderr,"==       Starting The Round %d       ==\n",num);
	fprintf(stderr,"Starting The Round %d\n",num);
	// fprintf(stderr,"=======================================\n\n");
}

//======================================= Main Function Declarations    ========================================

int main(int argc, char *argv[])
{
	int bits, rows, cols ;
	unsigned char func ;

	int found = 0, attempts = 0, i, j, code;
	int c, d, buttonPressed, rel, foo;
	int *attSeq;

	int pinLED = LED, pin2LED2 = LED2, pinButton = BUTTON;
	int fSel, shift, pin,  clrOff, setOff, off, res;
	int fd ;

	int  exact, contained;
	char str1[32];
	char str2[32];

	struct timeval t1, t2 ;

	int t ;
	char buf [32] ;
	//Variables for command-line processing
	char str_in[20], str[20] = "some text";
	int verbose = 0, debug = 0, help = 0, opt_m = 0, opt_n = 0, opt_s = 0, unit_test = 0;
	int *res_matches;

    //For checking the mode of the program
    {
		int opt;
		while ((opt = getopt(argc, argv, "hvdus:")) != -1) 
        {
			switch (opt) 
            {
                case 'v':
                    verbose = 1;
                    break;
                case 'h':
                    help = 1;
                    break;
                case 'd':
                    debug = 1;
                    break;
                case 'u':
                    unit_test = 1;
                    break;
                case 's':
                    opt_s = atoi(optarg); 
                    break;
                default:
                    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
                    exit(EXIT_FAILURE);
			}
		}
	} 
   
    //If the option is selected to help
    if(help) 
	{
		//Printing the lines for user
		fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n");
		fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n"); 
		fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n"); 
		fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
		exit(EXIT_SUCCESS);
	}
    
    //If the arguments are not correct after u is selected
    if (unit_test && optind >= argc-1) 
    {
		fprintf(stderr, "Expected 2 arguments after option -u\n");
		exit(EXIT_FAILURE);
	}
    
    //If verbose anf unit test is selected together
	if (verbose && unit_test) 
    {
		printf("1st argument = %s\n", argv[optind]);
		printf("2nd argument = %s\n", argv[optind+1]);
	}

    //If Verbose is on then printing which one is selected
	if (verbose) 
    {
		fprintf(stdout, "Settings for running the program\n");
		fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
		fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
		fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
		if (opt_s)  
        {
            fprintf(stdout, "Secret sequence set to %d\n", opt_s);
        }
	}
 
    //Allocating dynamic memory to variables
    seq1 = (int*)malloc(seqlen*sizeof(int));
	seq2 = (int*)malloc(seqlen*sizeof(int));
	cpy1 = (int*)malloc(seqlen*sizeof(int));
	cpy2 = (int*)malloc(seqlen*sizeof(int));
    
	//If the -u option is selected then run the unit test for the same
	if (unit_test && argc > optind+1) 
    { 
        //Copying the value of secret code in str_in variable 
		strcpy(str_in, argv[optind]);
        //Converting it into integer and storing it
		opt_m = atoi(str_in);
        //Copying the result of secret code in str_in variable 
		strcpy(str_in, argv[optind+1]);
        //Converting it into integer and storing it
		opt_n = atoi(str_in);
        //Calling functions to read and store values in global variables
		readSeq(seq1, opt_m);
		readSeq(seq2, opt_n);
        //If verbose is selected then printing the statement
		if (verbose)
        {
            fprintf(stdout, "\nTesting matches function with sequences %d and %d\n", opt_m, opt_n);
        }
        //Matching the numbers
		res_matches = countMatches(seq1, seq2);
        //Printing the numbers
		showMatches(res_matches, seq1, seq2, 1);
        //Exiting the case
		exit(EXIT_SUCCESS);
	} 
    else 
    {

	}
    
	//If the -s mode is selected
    if (opt_s) 
    { 
        //Checking if the secret code is NULL or not
        if (theSeq==NULL)
        {
			//If the Secret Code is NULL then allocating dynamic memory
            theSeq = (int*)malloc(seqlen*sizeof(int));
        }
		//Calling the function to copy the value in theSeq variable
        readSeq(theSeq, opt_s);
		//If verbose is selected
        if (verbose) 
        {
			//Printing the value of Secret code/sequence
            fprintf(stderr, "Running program with secret sequence:\n");
            showSeq(theSeq);
        }
	}
	//If the program is not executed with sudo in it, then printing error and exiting
    if (geteuid () != 0)
    {
        fprintf (stderr, "setup: Must be root. (Did you forget sudo?)\n") ;
    }
		

	// Variable to store the guessed code
	attSeq = (int*) malloc(seqlen*sizeof(int));

	//Assigning value to gpiobase
	gpiobase = 0x3F200000 ;

    if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    {
        return failure (FALSE, "setup: Unable to open /dev/mem: %s\n", strerror(errno)) ;
    }

	// GPIO
	gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
	if ((int32_t)gpio == -1)
    {
        return failure (FALSE, "setup: mmap (GPIO) failed: %s\n", strerror (errno)) ;
    }

	// Configuration of LED and BUTTON
    pinMode(gpio,pinLED,OUTPUT);
	pinMode(gpio,pin2LED2,OUTPUT);
	pinMode(gpio,pinButton,INPUT);

    // Start of game
	fprintf(stderr,"Starting Game\n\n");

	//If the -s mode is not selected then we are inititalising the secret code
	if (!opt_s)
    {
        initSeq();   
    }

	//If the debug mode is selected
	if (debug)
	{
		showSeq(theSeq);
	}
	printf("The length of the secret code is : %d.\n",seqlen);

	//Starting the main loop
	while (attempts!=3) 
	{
		//Incrementing the attempts
		attempts++;
		//Printing the round number
		printRound(attempts);
		//Declaring variable to store whether the button is clicked or not 
		int res = 0;
		//Taking the temperory variable
		int current=HIGH;
		//Running the for loop till the length of the sequence
		for(int i=0;i<seqlen;i++)
		{
			//Declaring the variable to count how many times button is pressed 
			int count=0;
			//Printing the message
			fprintf(stderr,"----Starting guess %d----\n",(i+1));
			int timer=0;
			while(count<seqlen)
			{			
				//Storing whether the button is pressed or not	
				res = readButton(gpio,pinButton);
				//Checking if the button is pressed or not
				if((pinButton & 0xFFFFFFC0) == 0) 
                {
					//If the button is pressed and the value is 1 means button is pressed again
					if(res!=0 && current==HIGH)
					{
						current=LOW;
						//Incrementing the count
						count++;
						//Printing that the button is pressed
						fprintf(stderr,"==== Button Pressed ====\n");
					}
					//If the button is released then changing the value of current
					else if(res==0 && current==LOW )
					{
						current = HIGH;
					}
				}
                else
				{
                    fprintf(stderr, "only supporting on-board pins\n");
                }
				//Delayig the time
				delay(65);
				//Incrementing the time
				timer++;
				//If the timer is equal to 65
				if(timer==65)
				{
					//Breaking the loop
					break;
				}
			}
			//If the person does'nt press the button then taking the count as 1
			if(count==0)
			{
				count=1;
			}
			//Printing the message
			fprintf(stderr,"----End of guess %d----\n\n",(i+1));
			fprintf(stderr,"You have pressed button %d times.\n",count);
			//Blinking the red light once as input is accepted
			blinkN(gpio,pin2LED2,1);
			//Blinking the green light count times
			blinkN(gpio,pinLED,count);
			//Assinging the code to the variable
			attSeq[i] = count;
		}
		//Blinking red LED twice as input of sequence is completed
        blinkN(gpio,pin2LED2,2);
        showSeq(attSeq);
        int * res_match = countMatches(theSeq,attSeq);
        showMatches(res_match, theSeq, attSeq,1);

		//Blinking red LED exact times
		blinkN(gpio,pinLED,res_match[0]);
		//Blinking red LED once as a separator
        blinkN(gpio,pin2LED2,1);
		//Blinking red LED approx times
		blinkN(gpio,pinLED,res_match[1]);
		//If the eaxct is same as length
        if(res_match[0]==seqlen)
        {
			//Then breaking the loop
            found=1;
            break;
        }
		// fprintf(stderr,"=======================================\n");
		// fprintf(stderr,"==        End Of The Round %d        ==\n",attempts);
		// fprintf(stderr,"=======================================\n\n");
		fprintf(stderr,"End of  The Round %d\n",attempts);
		//Blink Red 3 times as new round starts
		blinkN(gpio,pin2LED2,3);
	}
	//If the code is found then printing the code found
	if (found) 
	{
		fprintf(stderr,"Congratulations!!! You won the game\n");
	} 
	else 
	{
		fprintf(stdout, "Oops! Better Luck Next Time\n");
	}
	return 0;
}