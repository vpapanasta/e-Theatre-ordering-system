#ifndef __MY_HEADER__
#define __MY_HEADER__

#define UNIXSTR_PATH "/tmp/unix.str"

#include <stdio.h> /* Header for using input & output methods. */
#include <unistd.h> /* Acces POSIX operating systen API. */
#include <sys/types.h> /* basic system data types. */
#include <sys/socket.h> /* basic socket definitions. */
#include <errno.h> /* for the EINTR constant. */
#include <sys/wait.h> /* for the waitpid() system call. */
#include <sys/un.h> /* for Unix domain sockets. */
#include <string.h> /* Group of functions which allow to use strings. */
#include <fcntl.h> /* For using read and write functions. */
#include <pthread.h> /* Header for implenting types, functions and constants for pthreads. */
#include <stdlib.h> /* For input output processing. */
#include <time.h> /* For the time manipulation functions. */


#define BILLION 1000000000L // Timestamp precision (nanoseconds to seconds).

#define Nt 10 // # of telephones. 
#define Nb 4  // # of bank manchines.
#define t_sf 6000000 // Time for seat find.
#define t_cc 2000000 // Time for card check.
#define t_wait 10 // Time for apologise message.
#define t_trans 30 // Time for money transfer.

/* Declaration of theatre seats */

int A_zone[100] = { 0 }; // Declare table of seats for zone A. 
int B_zone[130] = { 0 }; // Declare table of seats for zone B.
int C_zone[180] = { 0 }; // Declare table of seats for zone C.
int D_zone[230] = { 0 }; // Declare table of seats for zone D.

/********************************/

int trans_table[240]; // Initialize money transfer table. We can have 240 tranfers(= 2 hours).
int number_trans = 0; // Variable for number of transfers.


/****** Declaration mutex proteceted variables. ******/

int total_cnt = 0; // Counter of clients' number.
/* This is a flag, when the theatre is full it will take a value of '1'. */
int full_theatre = 0; 
int company_acc = 0; // Counter for company account.
int theatre_acc = 0; // Counter for theatre account.
int fails_cnt = 0; // Counter for failed orders.
double total_wait_time = 0; // Initialize counter: total_wait_time.
double total_service_time = 0; // Initialize counter: total_service_time.

/*****************************************************/




/* Struct of each clients' info */
struct clnt
{
    int tickets_num; // Number of tickets for each order.
    int zone_name; // Name of zone for each order.
    int card_num; // Number of credit card for each order.

    int id_c; // ID(number) of its client.
    char flag_B; // Flag for bank check process.
    char flag_S; // Flag for find seats process.
    double wait_time; // Waiting time of its client.
    double service_time; // Service time of its client.

    struct timespec start, stop; // Time measure variables (for timestamp).
};

struct clnt cls[800];

/* Declaration of counters for reserved seats in each zone. */

int A_counter; // Counter for zone A.
int B_counter; // Counter for zone B.
int C_counter; // Counter for zone C.
int D_counter; // Counter for zone D.


/***********************************************************/




/***** INITIALIZE MUTEXES *****/
/* Mutexes for variables */
pthread_mutex_t variables_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t transfer_mutex = PTHREAD_MUTEX_INITIALIZER;

/******************************/




/***** INITIALIZE COND. VARIABLES *****/
pthread_cond_t cond_trans = PTHREAD_COND_INITIALIZER;

/**************************************/




/* Struct of variables for locking a critical section, */
/* using my_sema_init(), my_sema_wait(), my_sema_post(). */
struct cond_my_sema
{
  // Current count of the semaphore.
  unsigned int count_;

  // Number of threads that have called <my_sema_wait>.    
  unsigned long waiters_count_;
  
  // Serialize access to <count_> and <waiters_count_>.
  pthread_mutex_t zone_mutex;
  
  // Condition variable that blocks the <count_> 0.
  pthread_cond_t count_nonzero_;
  
};

struct cond_my_sema s_zone[4]; // Variable struct for each zone locking.
struct cond_my_sema s_bank; // Variable struct for card check locking.
struct cond_my_sema s_teleph; // Variable struct for telephones locking.









/********* FUNCTIONS DECLARATION *********/

int calc_ammount( int num_tick, int zn );
int inc_total_cnt();
char find_seats(int seat, int zn, int id);
void Print_Reserved_Seats();
char card_check(int card_num);
void seats_correction(int num_tick, int zn);
void charge_order(int num_tick, int zn);
void increase_if_fail();
void my_sema_init (struct cond_my_sema *s, unsigned int initial_count);
void my_sema_wait (struct cond_my_sema *s);
void my_sema_post (struct cond_my_sema *s);
void incr_total_wait_time(double w_time);
void incr_total_service_time(double s_time);
void trans_money();
void print_results();




 

/* Function for total_cnt increase. */
/* The functions returns the number of the current client (total counter). */
int inc_total_cnt() {
  		
  total_cnt = total_cnt + 1; // Increase # of orders
  return total_cnt;
}



/* Function for destroy all mutexes. */
void mutexes_destroy() {
   int i = 0;

   pthread_mutex_destroy( &variables_mutex );

   for( i; i < 4; i++){
      pthread_mutex_destroy( &s_zone[i].zone_mutex );
   }
   
   pthread_mutex_destroy( &s_bank.zone_mutex );   
   pthread_mutex_destroy( &s_teleph.zone_mutex );
   pthread_mutex_destroy( &transfer_mutex );   
   
}



/* Function for calculating ammount of an order. */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* The function returns the ammount of the current order. */
/* This function is used only by the client because of printing the result of an order (in case of success). */
int calc_ammount( int num_tick, int zn ) {

  int ammount = 0; // Buffer for ammount of the order.

  if(zn == 1){ // If zone A
    ammount = 50*num_tick; // Calculate the ammount of the order. 
  } 
  if(zn == 2){ // If zone B
    ammount = 40*num_tick; // Calculate the ammount of the order. 
  } 
  if(zn == 3){ // If zone C
    ammount = 35*num_tick; // Calculate the ammount of the order. 
  } 
  if(zn == 4){ // If zone D
    ammount = 30*num_tick; // Calculate the ammount of the order.
  } 

  return ammount; 
}




/* Function for finding seats. */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* The function returns a letter. */
/* A: Seats reverved successfully. C: The current zone is full OR there is not enough space in the current for your order. D: Theatre is FULL. */
char find_seats(int seat, int zn, int id) {

  int iterc;
  char ret_char; // Returned character from function.



  /* The first time that the theatre is FULL, we return 'D' and set full_theatre flag to 1.  */
  /* Else if threatre isn't full, we find seats for the order. */

  if(A_counter == 100 && B_counter == 130 && C_counter == 180 && D_counter == 230){ // Check if ALL zones are FULL.

    ret_char = 'D';

    /* Only the first time the theatre is FULL Print message for terminating server and printing results using [ctrl+C]. */
    if(full_theatre != 1){

       pthread_mutex_lock( &variables_mutex ); // Lock mutex. 
       full_theatre = 1; 
       /* Print message for terminating server and printing results using [ctrl+C]. */
       printf("\n\n\033[44m---> The theatre is full. You can press { Ctrl + C } in order to view the results. \033[0m\n");
       pthread_mutex_unlock( &variables_mutex ); // Mutex unlock.
    }
        
  }

  else{
      
      if(zn == 1){ // If zone A.
          /* Check if zone A is NOT FULL & if there are enough available seats for the current client. */
          
          if(A_counter < 100 && (100 - A_counter) >= seat ){ // Check if zones A is FULL.
           
            for(iterc = 0 ; iterc < seat ; iterc++){
             A_zone[A_counter] = cls[id].id_c; // Write client's number in zone A table.  
             
             A_counter += 1; // Increase zone's counter for showing to next seat.
             }
           
            ret_char = 'A';

          } 
          else{ // Else if zone A is FULL OR there are not enough available seats for the current client.
            ret_char = 'C'; 
          }
      }

      if(zn == 2){ // If zone B.
          /* Check if zone B is NOT FULL & if there are enough available seats for the current client. */
          if(B_counter != 130 && (130 - B_counter) >= seat){ // Check if  zones B is FULL.

            for(iterc = 0 ; iterc < seat ; iterc++){
             B_zone[B_counter] = cls[id].id_c; // Write client's number in zone B table. 
             
             B_counter += 1; // Increase zone's counter for showing to next seat.             
            }
            
            ret_char = 'A';

          } 
          else{ // Else if zone B is FULL OR there are not enough available seats for the current client.                
            ret_char = 'C'; 
          }
                    
      }

      if(zn == 3){ // If zone C.
          /* Check if zone C is NOT FULL & if there are enough available seats for the current client. */
          if(C_counter != 180 && (180 - C_counter) >= seat){ // Check if  zones C is FULL.

            for(iterc = 0 ; iterc < seat ; iterc++){
              C_zone[C_counter] = cls[id].id_c; // Write client's number in zone C table. 
             
             C_counter += 1; // Increase zone's counter for showing to next seat.             
            }
            
            ret_char = 'A';

          } 
          else{ // Else if zone C is FULL OR there are not enough available seats for the current client.                   
            ret_char = 'C'; 
          }
                    
      }

      if(zn == 4){ // If zone D.
          /* Check if zone D is NOT FULL & if there are enough available seats for the current client. */
          if(D_counter != 230 && (230 - D_counter) >= seat){ // Check if zones D is FULL.

            for(iterc = 0 ; iterc < seat ; iterc++){
             D_zone[D_counter] = cls[id].id_c; // Write client's number in zone D table. 
             
             D_counter += 1; // Increase zone's counter for showing to next seat.             
            }
            
            ret_char = 'A';

          } 
          else{ // Else if zone D is FULL OR there are not enough available seats for the current client.                  
            ret_char = 'C'; 
          }
                    
      }

  }    
  
  
  return ret_char; // Return answer message letter for the client. 
}


/* Print all reserved seats of every zone (from zone_A to zone_D). */
/* Every zone has in a row seats for each client. */
void Print_Reserved_Seats() {

  int i;

  printf("\n\n\n\033[34m*************************************************************************************************\033[0m\n");
  printf("\033[34m*************************************************************************************************\033[0m\n");
  printf("\033[34m*************************************************************************************************\033[0m\n");  
  
  printf("\n\n\033[32mZone A: [ \033[0m "); //Printing reserved seats in zone A.
  for( i = 0 ; i < A_counter ; i++ ) // For the number of seats the client wants.
    {

      if( A_zone[i] != 0 ) // If seat is not empty, print it.
      {
        printf("C%d, ", A_zone[i] );
          
      }

    }
   printf( "\b\b\033[32m ] \033[0m\n" ); 


  printf("\n\033[32mZone B: [  \033[0m"); //Printing reserved seats in zone B.
  for( i = 0 ; i < B_counter ; i++ ) // For the number of seats the client wants.
    {    
      if( B_zone[i] != 0 ) // If seat is not empty, print it.
      {
        printf("C%d, ", B_zone[i] );
          
      }

    }  
   printf( "\b\b\033[32m ] \033[0m\n" );


  printf("\n\033[32mZone C: [ \033[0m "); //Printing reserved seats in zone C.
  for( i = 0 ; i < C_counter ; i++ ) // For the number of seats the client wants.
    {
      if( C_zone[i] != 0 ) // If seat is not empty, print it.
      { 
        printf("C%d, ", C_zone[i] );
          
      }

    }
   printf( "\b\b\033[32m ] \033[0m\n" );




  printf("\n\033[32mZone D: [ \033[0m "); //Printing reserved seats in zone D.
  for( i = 0 ; i < D_counter ; i++ ) // For the number of seats the client wants.
    {

      if( D_zone[i] != 0 ) // If seat is not empty, print it.
      {
        printf("C%d, ", D_zone[i] );
          
      }

    }  
   printf( "\b\b\033[32m ] \033[0m\n" );

}



/* Function for number card check. */
/* Arguments: the number of card for the current client. */
/* The function returns a letter. R: If card is valid. B: If card is invalid. */
char card_check(int card_num) {

  char wrg_card = 'R'; // Init for (R)right card number.
    
    if(card_num == 0){   // If wrong card number, send message B.  
      wrg_card = 'B';  
    }
   
  return wrg_card; // Return message 'B'.
}




/* Function for DELETE wrong reserved seats. */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* If seats have reserved and the card number is invalid.*/
/* We delete the "wrong" reserved seats before other client wants to take this seats. */
void seats_correction(int num_tick, int zn) {

  if( zn == 1 ){ // If zone A.
      /* Counting the # of reserved seats we want to delete. */
      for( ; num_tick > 0 ; num_tick-- ){

        pthread_mutex_lock( &variables_mutex ); // Lock mutex. 
        A_counter -= 1; // Decrease zone's counter for showing to previous seat.
        pthread_mutex_unlock( &variables_mutex ); // Lock mutex.

        A_zone[A_counter] = 0; // DELETE wrong seat (make it zero entry). 
      }
  }  

  if( zn == 2 ){ // If zone B.
      /* Counting the # of reserved seats we want to delete. */
      for( ; num_tick > 0 ; num_tick-- ){

        pthread_mutex_lock( &variables_mutex ); // Lock mutex. 
        B_counter -= 1; // Decrease zone's counter for showing to previous seat.
        pthread_mutex_unlock( &variables_mutex ); // Lock mutex. 

        B_zone[B_counter] = 0; // DELETE wrong seat (make it zero entry).           
      }
  }

  if( zn == 3 ){ // If zone C.
      /* Counting the # of reserved seats we want to delete. */
      for( ; num_tick > 0 ; num_tick-- ){ 

        pthread_mutex_lock( &variables_mutex ); // Lock mutex. 
        C_counter -= 1; // Decrease zone's counter for showing to previous seat.
        pthread_mutex_unlock( &variables_mutex ); // Lock mutex. 

        C_zone[C_counter] = 0; // DELETE wrong seat (make it zero entry).  
      }
  }

  if( zn == 4 ){ // If zone D.
      /* Counting the # of reserved seats we want to delete. */
      for( ; num_tick > 0 ; num_tick-- ){ 

        pthread_mutex_lock( &variables_mutex ); // Lock mutex. 
        D_counter -= 1; // Decrease zone's counter for showing to previous seat.
        pthread_mutex_unlock( &variables_mutex ); // Lock mutex. 

        D_zone[D_counter] = 0; // DELETE wrong seat (make it zero entry). 
      }
  }

}



/* Function for calculate the order's value. */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* The function increase the account of company (company_acc) by the ammount of the current order. */
void charge_order(int num_tick, int zn) {

  int value_order = 0; // Buffer for value of each order.

  if(zn == 1){ // If zone A
    value_order = 50*num_tick; // Calculate the value of the order. 
  } 
  if(zn == 2){ // If zone B
    value_order = 40*num_tick; // Calculate the value of the order. 
  } 
  if(zn == 3){ // If zone C
    value_order = 35*num_tick; // Calculate the value of the order. 
  } 
  if(zn == 4){ // If zone D
    value_order = 30*num_tick; // Calculate the value of the order. 
  } 

  pthread_mutex_lock( &variables_mutex ); // Mutex lock.
  /* Increase local account with the money of new order. */
  company_acc = company_acc + value_order;  
  pthread_mutex_unlock( &variables_mutex ); // Mutex unlock.
  
}


/* Function for increase failure counter. */
/* If card number is invalid OR zone is full or without enough
   space OR the theatre is full, increase counter by 1.       */
void increase_if_fail() {
    
  pthread_mutex_lock( &variables_mutex ); // Mutex lock.
  fails_cnt = fails_cnt + 1;
  pthread_mutex_unlock( &variables_mutex ); // Mutex unlock.
}



/* Initialize mutex, condition variable and counters. */
/* Input: pointer of struct, current counter of my_semaphore. */
void my_sema_init (struct cond_my_sema *s, unsigned int initial_count) {

  pthread_mutex_init (&s->zone_mutex, NULL);
  pthread_cond_init (&s->count_nonzero_, NULL);
  s->count_ = initial_count;
  s->waiters_count_ = 0;
}



/* Function for simulating sem_wait, using condition variables & mutexes. */
/* Input: Pointer to struct cond_my_sema. */
void my_sema_wait (struct cond_my_sema *s) {

  // Acquire mutex to enter critical section.
  pthread_mutex_lock (&s->zone_mutex); // Mutex lock.

  // Keep track of the number of waiters so that <my_sema_post> works correctly.
  s->waiters_count_++;

  // Wait until the semaphore count is > 0, then atomically release
  // <zone_mutex> and wait for <count_nonzero_> to be signaled. 
  while (s->count_ == 0)
    pthread_cond_wait (&s->count_nonzero_, &s->zone_mutex);
  
  // <s->zone_mutex> is now held.
  // Decrement the waiters count.
  
  s->waiters_count_--;

  // Decrement the semaphore's count.
  s->count_--;

  // Release mutex to leave critical section.
  pthread_mutex_unlock (&s->zone_mutex); // Mutex unlock.
}



/* Function for simulating sem_post, using condition variables & mutexes. */
/* Input: Pointer to struct cond_my_sema. */ 
void my_sema_post (struct cond_my_sema *s) {

  pthread_mutex_lock (&s->zone_mutex); // Mutex lock.

  /* Always allow one thread to continue if it is waiting. */
  if (s->waiters_count_ > 0)
    pthread_cond_signal (&s->count_nonzero_);
    
  /* Increment the semaphore's count. */
  s->count_++;

  pthread_mutex_unlock (&s->zone_mutex); // Mutex unlock.
}




/* Function for increasing total_wait_time by clients' wait time.  */
/* Arguments: waiting time on hold for the current client. */
void incr_total_wait_time(double w_time) {

  pthread_mutex_lock( &variables_mutex ); // Mutex lock.

  /* Increase total_wait_time by clients' wait time. */
  total_wait_time = total_wait_time + w_time; 
  
  pthread_mutex_unlock( &variables_mutex ); // Mutex unlock.

}



/* Function for increasing total_service_time by clients' service time.  */
/* Arguments: service time for the current client. */
void incr_total_service_time(double s_time) {

  pthread_mutex_lock( &variables_mutex ); // Mutex lock.
  
  /* Increase total_service_time by clients' service time. */
  total_service_time = total_service_time + s_time; 
  
  pthread_mutex_unlock( &variables_mutex ); // Mutex unlock.

}


/* Function for transfering money from company account to theatre account every 30". */
void trans_money() {

  pthread_mutex_lock( &variables_mutex ); // Mutex lock.

  theatre_acc = theatre_acc + company_acc; // Add company_acc money to bank_acc.
  trans_table[number_trans] = company_acc; // Save company_acc into table.
  number_trans++; // Increase number of transfers.
  company_acc = 0; // Company account is now empty.

  pthread_mutex_unlock( &variables_mutex ); // Mutex unlock.
   
}




/* Function for printing final results. */
/* Print: Percentage of failed orders, average service time, average waiting time, */
/*        all the money transfers from company account to theatre account and total sum of money. */
void print_results() {
  int i;

  fprintf(stdout, "\n\033[42m## INFORMATION - STATISTICS ##\033[0m\n\n");

              /********* PRINT PERCENTAGE OF FAILS  *********/
  /* Divide the # of failures with the # of total clients. */
  fprintf(stdout, "  The overall percentage of failed orders is ---> %0.1f%% \n", ( (float)(fails_cnt) / (total_cnt) ) * 100 );
              /**********************************************/

              /********* PRINT AVERAGE SERVICE TIME *********/
  // Divide the total waiting time with the # of total clients.
  fprintf(stdout, "  The average waiting time is ---> %0.1f sec\n", (float)(total_wait_time) / (total_cnt) );
              /**********************************************/

              /********* PRINT AVERAGE SERVICE TIME *********/
  // Divide the total service time with the # of total clients.
  fprintf(stdout, "  The average service time is ---> %0.1f sec\n\n", (float)(total_service_time) / (total_cnt) );
              /**********************************************/



              /********* PRINT TRANSFER TABLE *********/
  fprintf(stdout, "\033[34mMoney Transfers: \033[0m\n");
  for(i=0 ; i < number_trans ; i++ ){ // For number of transfers.
    fprintf(stdout, "  Transfer #%d ==========> %d€ from company account to threatre account\n\n", i, trans_table[i]); // Print each result.
  }
              /***************************************/

              /********* PRINT TOTAL MONEY EARNED *********/
  fprintf(stdout, "  Total money earned by selling all tickets ---> %d€\n", (theatre_acc));


              /********************************************/
}


#endif

