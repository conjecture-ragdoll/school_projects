// Flora Afroza 23690682

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <inttypes.h>
#include <errno.h>
#include <stdint.h>


// SET FILE ENCODING TO UTF-8


// **********Compile using: 
// gcc Afroza_23690682.c -o Afroza_23690682.exe -lpthread -lm

// I also used "./Afroza_23690682.exe Prj2InpSect24.txt Afroza_23690682.txt"

int *InpArray; 


struct child_comp_thd { // worker computation threads
    pthread_t comp_ID;
    pthread_attr_t compthd_attr;
};

struct child_thd {      // Should have 12
    pthread_t child_ID;
    pthread_attr_t childthd_attr;
    struct child_comp_thd computations[3];  
    // Every worker child thread will have 3 computation threads
};

/* Worker Computation Arrays */
float *sums;        // threads will contribute to these arrays later
float *sum_prod_squares;        // *EDIT: I replaced sum_prod_squares to find "root of the sum of the squares" instead
float *geometric_avgs;


// Worker threads
struct child_thd *worker;

        // Worker computation thread functions, which are put into their own segment in their 12 indexed array,
        // See comment that says "/* Worker Computation Arrays */" 
        // ^^^ Those are the arrays where the values of the 1000 integer computed segments are stored
        void *sum(void *arg) {
            // int slice is the partition among the 12 sections the function starts indexing in the InpArray[] 
            int slice = (int)(intptr_t) arg;
            for(int i = 1000 * slice; i < 1000 * (slice + 1); i++) {
                sums[slice] += (float) InpArray[i];     // In the segment of 1000 consecutive integers, add the numbers continously
            }
            pthread_exit(0);
        }

        void *sqrt_sum_squares(void *arg) {
            int slice = (int)(intptr_t) arg;
            for(int i = 1000 * slice; i < 1000 * (slice + 1); i++) {
                sum_prod_squares[slice] += (InpArray[i] * InpArray[i]);     // Square the value before adding to the total sum
            }
            sum_prod_squares[slice] = (float) pow(sum_prod_squares[slice], (float) 0.5);  // After the squares are done adding, take the square root of the total sum
            pthread_exit(0);
        }

        void *geometric_avg(void *arg) {
            int slice = (int)(intptr_t) arg;
            geometric_avgs[slice] = 1;      
            // Multiply 10 numbers at a time and take the thousandth root
            float geo = 1;
            for(int i = (1000 * slice); i < 1000 * (slice + 1); i++) {
                    if(InpArray[i] == 0) {
                        geometric_avgs[slice] = 0;
                    break;     // Having a 0 in the array will multiply all values to 0 no matter if we continue, so stop the loop here if this happens
                    } 
                        geo = (geo * InpArray[i]);       // Multiply every value in this 1000 segment with eachother
                if(((i+1) % 10) == 0) {   // for every 10th number starting from 0th index, multiply the thousandth root of geo to geometric_avgs
                    geometric_avgs[slice] *= (float) pow(geo, (float) 0.001);
                    geo = 1;    // reset to 1
                }
            }
            pthread_exit(0);    
        }

        // Pointers for the pointer functions, each child thread will create 1 worker computation thread for each of these functions
        void* (*compute_ptr[3]) (void *args) =  {     
                                                    sum,
                                                    sqrt_sum_squares,
                                                    geometric_avg
                                                };


        void *create_grandchilds(void *args) {  // For creating worker child computation threads
            // Create 3 Computation threads
            int part = (int)(intptr_t) args;    // part is the thread number, and part is also equal to the one of the 12 (value 0-11) partitions to be processed in InpArray

            for(int i = 0; i < 3; i++) {
                pthread_attr_init(&(worker[part].computations[i].compthd_attr));
            }

                /* 36 Worker Computation Threads do a computation (as specified as sum, sqrt_sum_squares and geometric_avg) for each 
                1000 chunk in an array for the 3 computations*/

                for(int i = 0; i < 3; i++) {
                    pthread_create(&(worker[part].computations[i].comp_ID), &(worker[part].computations[i].compthd_attr), compute_ptr[i], (void *) (intptr_t) part);
                }
            
               //  wait for 3 computation threads to finish
                for(int i = 0; i < 3; i++) {
                    pthread_join(worker[part].computations[i].comp_ID, NULL);
                }
            pthread_exit(0);
        }



int main(int argc, char *argv[]) {

    if(argc < 2) {     // Program looks for 2 arguments with textfile as argv[1] to be opened as filename
        exit(EXIT_FAILURE);
    }

    FILE *stream = fopen(argv[1], "r"); // open specified text file

        if (stream == NULL) {
           perror("fopen");
           exit(EXIT_FAILURE);
        }

InpArray = malloc(12000 * sizeof(int));

        char *line = NULL;    
        size_t line_length = 0;
        ssize_t num_read;

        int index = 0;         
        // Reading the textfile's integers into InpArray until you reach the end of the textfile
        while((num_read = getline(&line, &line_length, stream)) != -1) {
            int integer = atoi(line);
            InpArray[index] = integer;
            index++;
        }
    free(line);

   fclose(stream);     // close file


sums = malloc(12 * sizeof(float));
sum_prod_squares = malloc(12 * sizeof(float));
geometric_avgs = malloc(12 * sizeof(float));

worker = malloc(12 * sizeof(struct child_thd));

    for(int i = 0; i < 12; i++) {
        pthread_attr_init(&(worker[i].childthd_attr));
    }

    // Create the 12 worker threads, create_grandchilds creates 3 computation threads
    for(int k = 0; k < 12; k++) {
        pthread_create(&(worker[k].child_ID), &(worker[k].childthd_attr), create_grandchilds, (void *) (intptr_t) k);   
    }

    // Worker child threads exit when worker computations threads finish
    for(int i = 0; i < 12; i++) {
        pthread_join(worker[i].child_ID, NULL); 
    }


free(InpArray);


    int output_file;
    char *output_file_name;


    // Output file
    if(argc == 3) {
        output_file_name = argv[2];
    } else {
        // Create output file if none exists
        output_file_name = "Afroza_23690682.txt";
    }

    // EDIT** I had issues creating the file so I used this instead of creat()
        //creat(output_file_name, O_RDWR);
        output_file = open(output_file_name, O_CREAT|O_WRONLY|O_TRUNC);
    
//****************** WRITE IN OUTPUT FILE***************************


    // Write pthread worker child results to textfile
    for(int i = 0; i < 12; i++) {
        write(output_file, "Worker Child Pthread Number = ", 30);
        char thd_num[2];
        sprintf(thd_num, "%d", i);      /* Replaced what was previously the value of thread id number with thread number */
        write(output_file, thd_num, 2);
        write(output_file, " : \t  Sum = ", 12);

        char sum_str[7] = "       ";
        sprintf(sum_str, "%d", (int) sums[i]);
        write(output_file, sum_str, 6);

        write(output_file, " \t Root of the Sum of Squares = ", 32);
        
        char power_str[16] = "                ";
        sprintf(power_str, "%f", sum_prod_squares[i]);
        write(output_file, power_str, 15);

        write(output_file, " \t Geometric Average = ", 23);

        char geo_str[16] = "                ";
        sprintf(geo_str, "%f", geometric_avgs[i]);
        write(output_file, geo_str, 15);

        write(output_file, "  \n", 3);

    }


free(worker);

    int max_sum = (int) sums[0];
    for(int i = 1; i < 12; i++) {   // Find max of sum
        if(max_sum < sums[i]) {
            max_sum = sums[i];
        }
    }
free(sums);

    float max_sp = sum_prod_squares[0];
    for(int i = 1; i < 12; i++) {
        if(max_sp < sum_prod_squares[i]) {  // Find max of sum of products squares
            max_sp = sum_prod_squares[i];
        }
    }
free(sum_prod_squares);

    float max_geo = geometric_avgs[0];
    for(int i = 1; i < 12; i++) {
        if(max_geo < geometric_avgs[i]) {   // Find max of geometric average
            max_geo = geometric_avgs[i];
        }
    }
free(geometric_avgs);


        // write the max values to output file as well

        write(output_file, "\nMain program thread: Max of the Sums = ", 40);

        char max_sum_str[7] = "       ";
        sprintf(max_sum_str, "%d", max_sum);
        write(output_file, max_sum_str, 6);

        write(output_file, "\nMain program thread: Max of the root of the sum of the squares = ", 66);

        char max_square_str[16] = "                ";
        sprintf(max_square_str, "%f", max_sp);
        write(output_file, max_square_str, 15);

        write(output_file, "\nMain program thread: Max of the Geometric Averages = ", 54);

        char max_geo_str[16] = "                ";
        sprintf(max_geo_str, "%f", max_geo);
        write(output_file, max_geo_str, 15);

        write(output_file, "\nMain program thread: Terminating. \n", 36);

    close(output_file);     // close output file

    

    return 0;

}