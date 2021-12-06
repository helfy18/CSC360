/*
 * Skeleton code for CSC 360, Summer 2021,  Assignment #4
 *
 * Prepared by: Michael Zastre (University of Victoria) 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

/*
 * Some compile-time constants.
 */

#define REPLACE_NONE 0
#define REPLACE_FIFO 1
#define REPLACE_CLOCK 2
#define REPLACE_LFU  3
#define REPLACE_OPTIMAL 4


#define TRUE 1
#define FALSE 0
#define PROGRESS_BAR_WIDTH 60
#define MAX_LINE_LEN 100


/*
 * Some function prototypes to keep the compiler happy.
 */
int setup(void);
int teardown(void);
int output_report(void);
long resolve_address(long, int);
void error_resolve_address(long, int);


/*
 * Variables used to keep track of the number of memory-system events
 * that are simulated.
 */
int page_faults = 0;
int mem_refs    = 0;
int swap_outs   = 0;
int swap_ins    = 0;


/*
 * Page-table information. You may want to modify this in order to
 * implement schemes such as CLOCK. However, you are not required
 * to do so.
 */
/* In this struct I added the count variable, which keeps track of the
 * number of "hits" the page has, as well as the age, which is used as
 * the tiebreaker in LFU, and the reference bit for the clock implementation.
 */
struct page_table_entry *page_table = NULL;
struct page_table_entry {
    long page_num;
    int dirty;
    int free;
    int count;
    int age;
    int reference;
};


/*
 * These global variables will be set in the main() function. The default
 * values here are non-sensical, but it is safer to zero out a variable
 * rather than trust to random data that might be stored in it -- this
 * helps with debugging (i.e., eliminates a possible source of randomness
 * in misbehaving programs).
 */

int size_of_frame = 0;  /* power of 2 */
int size_of_memory = 0; /* number of frames */
int page_replacement_scheme = REPLACE_NONE;
//for FIFO, which is the next page to replace
int next_replace = 0;
//for LFU, the age to be given to a newly inserted page in the table
int total_age = 0;
//for clock, the two hands
int clock_hand = 0;
int second_clock = 0;


/*
 * Function to convert a logical address into its corresponding 
 * physical address. The value returned by this function is the
 * physical address (or -1 if no physical address can exist for
 * the logical address given the current page-allocation state.
 */

long resolve_address(long logical, int memwrite)
{
    int i;
    long page, frame;
    long offset;
    long mask = 0;
    long effective;

    /* Get the page and offset */
    page = (logical >> size_of_frame);

    for (i=0; i<size_of_frame; i++) {
        mask = mask << 1;
        mask |= 1;
    }
    offset = logical & mask;

    /* Find page in the inverted page table. */
    frame = -1;
    for ( i = 0; i < size_of_memory; i++ ) {
        if (!page_table[i].free && page_table[i].page_num == page) {
            frame = i;
	    if (memwrite == TRUE) {
		page_table[i].dirty = TRUE;
	    }
	    page_table[i].count++;
	    page_table[i].reference = 1;
            break;
        }
    }
    /* If frame is not -1, then we can successfully resolve the
     * address and return the result. */
    if (frame != -1) {
        effective = (frame << size_of_frame) | offset;
        return effective;
    }


    /* If we reach this point, there was a page fault. Find
     * a free frame. */
    page_faults++;

    for ( i = 0; i < size_of_memory; i++) {
        if (page_table[i].free) {
            frame = i;
            break;
        }
    }

    /* if frame is still -1, we need to choose a victim frame.
     * For FIFO, we use a pointer that points to the first in (oldest)
     * element in the page table. Since elements are always put in
     * FIFO, this will go in a continuous circle, represented by
     * adding one to the pointer and modding by size of the array.
     * For clock, we implement two "hands", one pointing at the start
     * of the table (initially) and the other pointing to the middle
     * (size/2). These hands move up the table (wrapping around with
     * modulus by size). The second hand clears reference bits, and the
     * first hand checks for a cleared reference bit. The first cleared
     * reference bit encountered by the first hand is chosen as the victim.
     * For LFU, I kept track of the age of every page inputted to the table
     * as well as the number of times the page has been referenced (count).
     * The comparison is then that lower count has priority, and in case of
     * a tie the higher age (i.e. older frame) is chosen as the victim. Once
     * the victim has been chosen, it's dirty bit is checked, and swapped out
     * if set, then reset if the current instruction is not a write.
     * */
    if (frame == -1) {
	if (page_replacement_scheme == REPLACE_FIFO) {
	    frame = (next_replace++)% size_of_memory;
	} else if (page_replacement_scheme == REPLACE_CLOCK) {
	    for(;;) {
		second_clock = second_clock % size_of_memory;
		page_table[second_clock++].reference = 0;
   		if (page_table[clock_hand].reference == 0) {
		    frame = clock_hand++;
		    clock_hand = clock_hand % size_of_memory;
		    break;
		}
		clock_hand++;
		clock_hand = clock_hand % size_of_memory;
	    }
	} else if (page_replacement_scheme == REPLACE_LFU) {
	    int lowest_index = 0;
	    int lowest_count = page_table[0].count;
	    int lowest_age = page_table[0].age;
	    for (i = 0; i < size_of_memory; i++) {
		if (page_table[i].count < lowest_count || 
				(page_table[i].count == lowest_count 
				 && page_table[i].age < lowest_age)) {
		    lowest_index = i;
		    lowest_count = page_table[i].count;
		    lowest_age = page_table[i].age;
		}
	    }
	    frame = lowest_index;
	}
  	if (page_table[frame].dirty == TRUE) {
	    swap_outs++;
	}
	if (memwrite == FALSE) {
	    page_table[frame].dirty = FALSE;
	}   
    }


    /* If we found a free frame, then patch up the
     * page table entry and compute the effective
     * address. Otherwise return -1.
     */
    /* In this section I reset the new frame's values all to 0, and set
     * the dirty bit if the instruction is a write.
     */
    if (frame != -1) {
	page_table[frame].count = 0;
	page_table[frame].age = total_age++;
	page_table[frame].page_num = page;
        page_table[frame].free = FALSE;
	page_table[frame].reference = 0;
	if (memwrite == TRUE) {
	    page_table[frame].dirty = TRUE;
	}
        swap_ins++;
        effective = (frame << size_of_frame) | offset;
        return effective;
    } else {
        return -1;
    }
}



/*
 * Super-simple progress bar.
 */
void display_progress(int percent)
{
    int to_date = PROGRESS_BAR_WIDTH * percent / 100;
    static int last_to_date = 0;
    int i;

    if (last_to_date < to_date) {
        last_to_date = to_date;
    } else {
        return;
    }

    printf("Progress [");
    for (i=0; i<to_date; i++) {
        printf(".");
    }
    for (; i<PROGRESS_BAR_WIDTH; i++) {
        printf(" ");
    }
    printf("] %3d%%", percent);
    printf("\r");
    fflush(stdout);
}


int setup()
{
    int i;

    page_table = (struct page_table_entry *)malloc(
        sizeof(struct page_table_entry) * size_of_memory
    );

    if (page_table == NULL) {
        fprintf(stderr,
            "Simulator error: cannot allocate memory for page table.\n");
        exit(1);
    }

    for (i=0; i<size_of_memory; i++) {
        page_table[i].free = TRUE;
    }

    return -1;
}


/* In this function I added a free of the page table to ensure no memory
 * leaks.
 */
int teardown()
{
    free(page_table);
    return -1;
}


void error_resolve_address(long a, int l)
{
    fprintf(stderr, "\n");
    fprintf(stderr, 
        "Simulator error: cannot resolve address 0x%lx at line %d\n",
        a, l
    );
    exit(1);
}


int output_report()
{
    printf("\n");
    printf("Memory references: %d\n", mem_refs);
    printf("Page faults: %d\n", page_faults);
    printf("Swap ins: %d\n", swap_ins);
    printf("Swap outs: %d\n", swap_outs);

    return -1;
}

/* In the main I added an initialization of the second clock hand at half
 * the size of the page table.
 */
int main(int argc, char **argv)
{
    /* For working with command-line arguments. */
    int i;
    char *s;

    /* For working with input file. */
    FILE *infile = NULL;
    char *infile_name = NULL;
    struct stat infile_stat;
    int  line_num = 0;
    int infile_size = 0;

    /* For processing each individual line in the input file. */
    char buffer[MAX_LINE_LEN];
    long addr;
    char addr_type;
    int  is_write;

    /* For making visible the work being done by the simulator. */
    int show_progress = FALSE;

    /* Process the command-line parameters. Note that the
     * REPLACE_OPTIMAL scheme is not required for A#4.
     */
    for (i=1; i < argc; i++) {
        if (strncmp(argv[i], "--replace=", 9) == 0) {
            s = strstr(argv[i], "=") + 1;
            if (strcmp(s, "fifo") == 0) {
                page_replacement_scheme = REPLACE_FIFO;
            } else if (strcmp(s, "lfu") == 0) {
                page_replacement_scheme = REPLACE_LFU;
            } else if (strcmp(s, "clock") == 0) {
                page_replacement_scheme = REPLACE_CLOCK;
            } else if (strcmp(s, "optimal") == 0) {
                page_replacement_scheme = REPLACE_OPTIMAL;
            } else {
                page_replacement_scheme = REPLACE_NONE;
            }
        } else if (strncmp(argv[i], "--file=", 7) == 0) {
            infile_name = strstr(argv[i], "=") + 1;
        } else if (strncmp(argv[i], "--framesize=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_frame = atoi(s);
        } else if (strncmp(argv[i], "--numframes=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_memory = atoi(s);
        } else if (strcmp(argv[i], "--progress") == 0) {
            show_progress = TRUE;
        }
    }

    if (infile_name == NULL) {
        infile = stdin;
    } else if (stat(infile_name, &infile_stat) == 0) {
        infile_size = (int)(infile_stat.st_size);
        /* If this fails, infile will be null */
        infile = fopen(infile_name, "r");  
    }


    if (page_replacement_scheme == REPLACE_NONE ||
        size_of_frame <= 0 ||
        size_of_memory <= 0 ||
        infile == NULL)
    {
        fprintf(stderr, 
            "usage: %s --framesize=<m> --numframes=<n>", argv[0]);
        fprintf(stderr, 
            " --replace={fifo|lfu|clock|optimal} [--file=<filename>]\n");
        exit(1);
    }


    setup();

    //implement the second clock hand for clock page replacement scheme
    second_clock = size_of_memory / 2;

    while (fgets(buffer, MAX_LINE_LEN-1, infile)) {
        line_num++;
        if (strstr(buffer, ":")) {
            sscanf(buffer, "%c: %lx", &addr_type, &addr);
            if (addr_type == 'W') {
                is_write = TRUE;
            } else {
                is_write = FALSE;
            }

            if (resolve_address(addr, is_write) == -1) {
                error_resolve_address(addr, line_num);
            }
            mem_refs++;
        } 

        if (show_progress) {
            display_progress(ftell(infile) * 100 / infile_size);
        }
    }
    

    teardown();
    output_report();

    fclose(infile);

    exit(0);
}
