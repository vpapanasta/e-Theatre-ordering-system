#ifndef __MY_HEADER__
#define __MY_HEADER__

#define UNIXSTR_PATH "/tmp/unix.str"
#include <stdio.h>  /* Header for using input & output methods */
#include <unistd.h> /* Acces POSIX operating systen API. */
#include <sys/types.h> /* Basic system data types */ 
#include <sys/ipc.h> /* SHM header */
#include <sys/shm.h> /* SHM header */
#include <sys/socket.h> /* Basic socket definitions */
#include <errno.h> /* For the EINTR constant */
#include <sys/wait.h> /* For the waitpid() system call */
#include <sys/un.h> /* For Unix domain sockets */
#include <string.h> /* Group of functions which allow to use strings */
#include <time.h> /* For the time manipulation functions */
#include <stdlib.h> /* For input output processing */
#include <semaphore.h> /* Header which we use the semaphores with */
#include <fcntl.h> /* For using read and write functions */
#include <sys/stat.h> /* Containing constructs that facilitate information about files attributes */


#define Nt 10 // # of telephones. 
#define Nb 4  // # of bank manchines.
#define t_sf 6 // Time for seat find.
#define t_cc 2 // Time for card check.
#define t_wait 10 // Time for apologise message. 
#define t_trans 30 // Time for money transfer.


/* Size of the request queue. */
#define LISTENQ 400 
//
#define SHMSZ1 sizeof(int)*(100 + 130 + 180 + 230 + 1)  // Big shm size. ( +1: for having enough space).
                                                        // Zone_A (100), Zone_B (130), Zone_C (180), Zone_D (230).  
#define SHMSZ2 sizeof(int)*(11) // Small shm size (11 counters).
#define SHMSZ3 sizeof(struct clnt)*(1000) // Struct shm size. We can have <= 1000 clients. 

#define BILLION 1000000000L // Timestamp precision (nanoseconds to seconds).

/* Semaphores Defines */
#define SEM_SHM2_NAME "semaphore_small_shm"
#define SEM_NAME_Z1 "zone1_semaphore"
#define SEM_NAME_Z2 "zone2_semaphore"
#define SEM_NAME_Z3 "zone3_semaphore"
#define SEM_NAME_Z4 "zone4_semaphore"
#define SEM_BANK_NAME "bankers_semaphore"
#define SEM_TELEPHONES "telephones_semaphore"



/*FUNCTIONS DECLARATION*/
void Print_Reserves_Seats();
char find_seats(int seat, int zn);
void init_small_shm();
char card_check(int card_num);
inline int inc_total_cnt();
void open_all_semaphores ();
void semaphores_close();
void semaphores_unlink();
void charge_order(int num_tick, int zn);
void trans_money();
void increase_if_fail();
void my_sem_zone_wait(int zn);
void my_sem_zone_post(int zn);
void seats_correction(int num_tick, int zn);
void print_results();
int calc_ammount(int num_tick, int zn);
void incr_total_wait_time(double w_time);
void incr_total_service_time(double s_time);



/* Client's struct */ 
/* Every client has each struct. The structs are saved in third share memory */
struct clnt{
      int id_c; // ID(number) of its client.
      char flag1; // Flag for bank check process.
      char flag2; // Flag for find seats process.
      double wait_time; // Waiting time of its client.
      double service_time; // Service time of its client.
      
};
struct clnt cl;


/* GLOBAL VARIABLES */

int *shm_1, *s1; // Big Shared Memory pointers.
int bup_zone[4] = { 1, 101, 231, 411 }; // Backup of start in every zone.
int shmid_1; // Big Shared Memory id.

int shmid_2; // Small Shared Memory id.
int *shm_2, *s2; //Small Shared Memory pointers.

int shmid_3; // Struct Shared Memory id.
struct clnt *shm_3, *s3; // Small Shared Memory pointers.


int trans_table[240]; // Initialize money transfer table. We can have 240 tranfers.
int number_trans = 0; // Variable for number of transfers.

/* SEMAPHORES' POINTERS */
sem_t *sem_shm2; // Pointer to small shm semaphore.
sem_t *sem_zone1; // Pointer to zone 1 semaphore.
sem_t *sem_zone2; // Pointer to zone 2 semaphore.
sem_t *sem_zone3; // Pointer to zone 3 semaphore.
sem_t *sem_zone4; // Pointer to zone 4 semaphore.
sem_t *sem_bank; // Pointer to bank machines semaphore.
sem_t *sem_teleph; // Pointer to telephones semaphore.

// OFFSETS for small shm
int local_acc = 0; // Offset for COMPANY account.
int bank_acc = 1;   // Offset for THEATRE account.
int fails_cnt = 2;  // Offset for order fails counter.
int total_cnt = 3;  // Offset for total orders counter.
int zone_0 = 4; // Offset for zone[0].
int zone_1 = 5; // Offset for zone[1].
int zone_2 = 6; // Offset for zone[2].
int zone_3 = 7; // Offset for zone[3].
int total_wait_time = 8; // Offset for total wait time.
int total_service_time = 9; // Offset for total service time.
int full_theatre = 10; // Offset for Flag full theatre.



struct timespec start, stop;// Time measure variables (for timestamp).







/*#################################################################################################################*/

        /* ######################################## FUNCTIONS ######################################## */

/*#################################################################################################################*/




/* Initialize shared memory 2 (small shm) counters. */
void init_small_shm(){ 
  /* Small SHM initializations */
    *(s2 + local_acc) = 0; // Initialize counter: local_acc.
    *(s2 + bank_acc) = 0; // Initialize counter: bank_acc.
    *(s2 + fails_cnt) = 0; // Initialize counter: fail_cnt.
    *(s2 + total_cnt) = 0; // Initialize counter: total_cnt.
    *(s2 + total_wait_time) = 0; // Initialize counter: total_wait_time.
    *(s2 + total_service_time) = 0; // Initialize counter: total_service_time.
    *(s2 + full_theatre) = 0; //Initialize flag: full_theatre.
    
    /* Counters of reserved seats located in small shm (for every zone) */
    *(s2 + zone_0) = 1;   
    *(s2 + zone_1) = 101;
    *(s2 + zone_2) = 231;
    *(s2 + zone_3) = 411;

        
}






/* Print all reserved seats of every zone (from zone_A to zone_D). */
/* Every zone has in a row seats for each client. */
void Print_Reserves_Seats()
{ 
  int i;

  printf("\n\n\n\033[34m*************************************************************************************************\033[0m\n");
  printf("\033[34m*************************************************************************************************\033[0m\n");
  printf("\033[34m*************************************************************************************************\033[0m\n");  
  
  printf("\n\n\033[32mZone A: [ \033[0m "); //Printing reserved seats in zone A.
  for( i = 0 ; i < *(s2 + zone_0) - 1  ; i++ ) // For the number of seats the client wants.
    {

      if( *(s1 + bup_zone[0] + i) != 0 ) // If seat is not empty, print it.
      {
        printf("C%d, ", *(s1 + bup_zone[0] + i) );
          
      }

    }
   printf( "\b\b\033[32m ] \033[0m\n" ); 


  printf("\n\033[32mZone B: [  \033[0m"); //Printing reserved seats in zone B.
  for( i = 0 ; i < (*(s2 + zone_1)  - 101) ; i++ ) // For the number of seats the client wants.
    {    
      if( *(s1 + bup_zone[1] + i) != 0 ) // If seat is not empty, print it.
      {
        printf("C%d, ", *(s1 + bup_zone[1] + i) );
          
      }

    }  
   printf( "\b\b\033[32m ] \033[0m\n" );


  printf("\n\033[32mZone C: [ \033[0m "); //Printing reserved seats in zone C.
  for( i = 0 ; i < (*(s2 + zone_2) - 231)  ; i++ ) // For the number of seats the client wants.
    {
      if( *(s1 + bup_zone[2] + i) != 0 ) // If seat is not empty, print it.
      { 
        printf("C%d, ", *(s1 + bup_zone[2] + i) );
          
      }

    }
   printf( "\b\b\033[32m ] \033[0m\n" );




  printf("\n\033[32mZone D: [ \033[0m "); //Printing reserved seats in zone D.
  for( i = 0 ; i < (*(s2 + zone_3) - 411) ; i++ ) // For the number of seats the client wants.
    {

      if( *(s1 + bup_zone[3] + i) != 0 ) // If seat is not empty, print it.
      {
        printf("C%d, ", *(s1 + bup_zone[3] + i) );
          
      }

    }  
   printf( "\b\b\033[32m ] \033[0m\n" );

}






/* Function for finding seats. */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* The function returns a letter. */
/* A: Seats reverved successfully.*/
/* C: The current zone is full OR there is not enough space in the current for your order. D: Theatre is FULL. */
char find_seats(int seat, int zn)
{
  int iterc;
  char ret_char; // Returned character from function


  /* The first time that the theatre is FULL, we return 'D' and set full_theatre flag(in shm2) to 1.  */
  /* Else if threatre isn't full, we find seats for the order. */

  if(*(s2 + zone_0) == 101 && *(s2 + zone_1) == 231 && *(s2 + zone_2) == 411 && *(s2 + zone_3) == 641){ // Check if ALL zones are FULL.
   // *(s2 + full_theatre) = 1; // If ALL zone are FULL set flag to 1.   
    ret_char = 'D';

    // Only the first time the theatre is FULL Print message for terminating server and printing results using [Ctrl+C].
    if(*(s2 + full_theatre) != 1){

       sem_wait(sem_shm2); // Lock semaphore for small shm.
       *(s2 + full_theatre) = 1; 
       // Print message for terminating server and printing results using [Ctrl+C].
       printf("\n\n\033[44m---> The theatre is full. You can press { Ctrl + C } in order to view the results. \033[0m\n");
       sem_post(sem_shm2); // Unlock semaphore for small shm.

    }
        
  }

  else{

      if(zn == 1){ // If zone A.
          // Check if zone A is NOT FULL & if there are enough available seats for the current client.
          if(*(s2 + zone_0) < 101 && (101 - *(s2 + zone_0)) >= seat ){ 

            for(iterc = 0 ; iterc < seat ; iterc++){
             *(s1 + *(s2 + zone_0)) = (*s3).id_c; // Write client's number in big SHM.  

             sem_wait(sem_shm2); // Lock semaphore for small shm.
             *(s2 + zone_0) = *(s2 + zone_0) + 1; // Increase zone's counter for showing to next seat.
             sem_post(sem_shm2); // Unlock semaphore for small shm.
            }
           
            ret_char = 'A';

          } 
          else{ // Else if zone A is FULL OR there are not enough available seats for the current client.
            ret_char = 'C'; 
          }
      }

      if(zn == 2){ // If zone B.
          /* Check if zone B is NOT FULL & if there are enough available seats for the current client. */
          if(*(s2 + zone_1) != 231 && (231 - *(s2 + zone_1)) >= seat){ // Check if  zones B is FULL.

            for(iterc = 0 ; iterc < seat ; iterc++){
             *(s1 + *(s2 + zone_1)) = (*s3).id_c; // Write client's number in big SHM.  

             sem_wait(sem_shm2); // Lock semaphore for small shm.
             *(s2 + zone_1) = *(s2 + zone_1) + 1; // Increase zone's counter for showing to next seat.
             sem_post(sem_shm2); // Unlock semaphore for small shm.
            }
            
            ret_char = 'A';

          } 
          else{ // Else if zone B is FULL OR there are not enough available seats for the current client.                  
            ret_char = 'C'; 
          }
                    
      }

      if(zn == 3){ // If zone C.
          /* Check if zone C is NOT FULL & if there are enough available seats for the current client. */
          if(*(s2 + zone_2) != 411 && (411 - *(s2 + zone_2)) >= seat){ // Check if  zones C is FULL.

            for(iterc = 0 ; iterc < seat ; iterc++){
             *(s1 + *(s2 + zone_2)) = (*s3).id_c; // Write client's number in big SHM . 

             sem_wait(sem_shm2); // Lock semaphore for small shm.
             *(s2 + zone_2) = *(s2 + zone_2) + 1; // Increase zone's counter for showing to next seat.
             sem_post(sem_shm2); // Unlock semaphore for small shm.
            }
            
            ret_char = 'A';

          } 
          else{ // Else if zone C is FULL OR there are not enough available seats for the current client.                 
            ret_char = 'C'; 
          }
                    
      }

      if(zn == 4){ // If zone D.
          /* Check if zone D is NOT FULL & if there are enough available seats for the current client. */
          if(*(s2 + zone_3) != 641 && (641 - *(s2 + zone_3)) >= seat){ // Check if  zones D is FULL.

            for(iterc = 0 ; iterc < seat ; iterc++){
             *(s1 + *(s2 + zone_3)) = (*s3).id_c; // Write client's number in big SHM. 

             sem_wait(sem_shm2); // Lock semaphore for small shm.
             *(s2 + zone_3) = *(s2 + zone_3) + 1; // Increase zone's counter for showing to next seat.
             sem_post(sem_shm2); // Unlock semaphore for small shm.
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



/* Function for total_cnt increase. */
/* The functions returns the number of the current client (total counter). */
int inc_total_cnt(){

  *(s2 + total_cnt) = *(s2 + total_cnt) + 1; // Increase # of orders.
  return *(s2 + total_cnt);
}




/* Function for semaphores close. */
void semaphores_close(){

  sem_close(sem_shm2); 
  sem_close(sem_zone1);
  sem_close(sem_zone2);
  sem_close(sem_zone3);
  sem_close(sem_zone4);
  sem_close(sem_bank);
  sem_close(sem_teleph);

}



/* Function for semaphores unlink. */
void semaphores_unlink(){

  sem_unlink(SEM_SHM2_NAME); 
  sem_unlink(SEM_NAME_Z1);
  sem_unlink(SEM_NAME_Z2);
  sem_unlink(SEM_NAME_Z3);
  sem_unlink(SEM_NAME_Z4);
  sem_unlink(SEM_BANK_NAME);
  sem_unlink(SEM_TELEPHONES);

}



/* Function for calculate the order's value. */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* The function increase the account of company (local_acc) by the ammount of the current order. */
void charge_order(int num_tick, int zn){

  int value_order = 0; // Buffer for value of each order.

  if(zn == 1){ // If zone A.
    value_order = 50*num_tick; // Calculate the value of the order. 
  } 
  if(zn == 2){ // If zone B.
    value_order = 40*num_tick; // Calculate the value of the order. 
  } 
  if(zn == 3){ // If zone C.
    value_order = 35*num_tick; // Calculate the value of the order.
  } 
  if(zn == 4){ // If zone D.
    value_order = 30*num_tick; // Calculate the value of the order. 
  } 

  sem_wait(sem_shm2); // Lock semaphore for small shm.

  *(s2 + local_acc) = *(s2 + local_acc) + value_order; // Increase local account with the money of new order.
  
  sem_post(sem_shm2); // Unlock semaphore for small shm.
}


/* Function for transfering money from company account to bank account every 30". */
void trans_money(){
  sem_wait(sem_shm2); // Lock semaphore for small shm.

  *(s2 + bank_acc) = *(s2 + bank_acc) + *(s2 + local_acc); // Add local_acc money to bank_acc.
  trans_table[number_trans] = *(s2 + local_acc); // Save local_acc into table.
  number_trans++; // Increase number of transfers.
  *(s2 + local_acc) = 0; // Local account is now empty.

  sem_post(sem_shm2); // Unlock semaphore for small shm.

  
  
}


/* Function for increase failure counter. */
/* If card number is invalid OR zone is full or without enough space OR the theatre is full, increase counter by 1. */
void increase_if_fail(){
    
    sem_wait(sem_shm2); // Lock semaphore for small shm.
    *(s2 + fails_cnt) = *(s2 + fails_cnt) + 1;
    sem_post(sem_shm2); // Unlock semaphore for small shm.

}


/* Function for contitional ZONE semaphore locking. */
/* Arguments: the number of zone for the current order. */
void my_sem_zone_wait(int zn){

  if(zn == 1){
    sem_wait(sem_zone1);
  }
  else if(zn == 2){ 
    sem_wait(sem_zone2);
  }  
  else if(zn == 3){ 
    sem_wait(sem_zone3);
  }
  else{ 
    sem_wait(sem_zone4);
  }
}


/* Function for contitional semaphore unlocking. */
/* Arguments: the number of zone for the current order. */
void my_sem_zone_post(int zn){

  if(zn == 1){
    sem_post(sem_zone1);
  }
  else if(zn == 2){ 
    sem_post(sem_zone2);
  }  
  else if(zn == 3){ 
    sem_post(sem_zone3);
  }
  else{ 
    sem_post(sem_zone4);
  }
}


/* Function for DELETE wrong reserved seats. */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* If seats have reserved and the card number is invalid.*/
/* We delete the "wrong" reserved seats before other client wants to take this seats. */
void seats_correction(int num_tick, int zn){

  if( zn == 1){ // If zone A

      for( ; num_tick > 0 ; num_tick-- ){

          sem_wait(sem_shm2); // Lock semaphore for small shm.
          *(s2 + zone_0) = *(s2 + zone_0) - 1; // Decrease zone's counter for showing to previous seat.
          sem_post(sem_shm2); // Unlock semaphore for small shm.

          *(s1 + *(s2 + zone_0)) = 0; // DELETE wrong seat (make it zero entry). 
      }

  }  

  if( zn == 2){ // If zone B.

      for( ; num_tick > 0 ; num_tick-- ){

          sem_wait(sem_shm2); // Lock semaphore for small shm.
          *(s2 + zone_1) = *(s2 + zone_1) - 1; // Decrease zone's counter for showing to previous seat.
          sem_post(sem_shm2); // Unlock semaphore for small shm.

          *(s1 + *(s2 + zone_1)) = 0; // DELETE wrong seat (make it zero entry).
      }

  }

  if( zn == 3){ // If zone C.

      for( ; num_tick > 0 ; num_tick-- ){ 

          sem_wait(sem_shm2); // Lock semaphore for small shm.
          *(s2 + zone_2) = *(s2 + zone_2) - 1; // Decrease zone's counter for showing to previous seat.
          sem_post(sem_shm2); // Unlock semaphore for small shm.

          *(s1 + *(s2 + zone_2)) = 0; // DELETE wrong seat (make it zero entry). 
      }

  }

  if( zn == 4){ // If zone D.

      for( ; num_tick > 0 ; num_tick-- ){ // Counting the # of reserved seats we want to delete.

          sem_wait(sem_shm2); // Lock semaphore for small shm.
          *(s2 + zone_3) = *(s2 + zone_3) - 1; // Decrease zone's counter for showing to previous seat.
          sem_post(sem_shm2); // Unlock semaphore for small shm.

          *(s1 + *(s2 + zone_3)) = 0; // DELETE wrong seat (make it zero entry). 
      }

  }

}





/* Function for number card check. */
/* Arguments: the number of card for the current client. */
/* The function returns a letter. R: If card is valid. B: If card is invalid. */
char card_check(int card_num){

	char wrg_card = 'R'; // Init for (R)right card number.

  sem_wait(sem_bank); // Lock bank semaphore.

  
		if(card_num == 0){   // If wrong card number, send message B.   
			wrg_card = 'B';  
		}

  
	sem_post(sem_bank); // Unlock bank semaphore. 
	return wrg_card; // Return message 'B'.
}




/* Function for opening all semaphores. */
void open_all_semaphores(){
  /* Semaphores openings */

     /* Open semaphore (size 1) for small SHM. */
     sem_shm2 = sem_open(SEM_SHM2_NAME, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 1); 
     if (sem_shm2 == SEM_FAILED){
        printf("Could not open semaphore!\n");
        exit(1);
     }

     /* Open semaphore (size 1) for zone_A. */
     sem_zone1 = sem_open(SEM_NAME_Z1, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 1); 
     if (sem_zone1 == SEM_FAILED){
        printf("Could not open semaphore!\n");
        exit(1);
     }

     /* Open semaphore (size 1) for zone_B. */
     sem_zone2 = sem_open(SEM_NAME_Z2, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 1); 
     if (sem_zone2 == SEM_FAILED){
        printf("Could not open semaphore!\n");
        exit(1);
     }

     /* Open semaphore (size 1) for zone_C. */
     sem_zone3 = sem_open(SEM_NAME_Z3, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 1); 
     if (sem_zone3 == SEM_FAILED){
        printf("Could not open semaphore!\n");
        exit(1);
     }

     /* Open semaphore (size 1) for zone_D. */
     sem_zone4 = sem_open(SEM_NAME_Z4, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 1); 
     if (sem_zone4 == SEM_FAILED){
        printf("Could not open semaphore!\n");
        exit(1);
     }

     /* Open semaphore (size 4) for bank. */
     sem_bank = sem_open(SEM_BANK_NAME, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 4); 
     if (sem_bank == SEM_FAILED){
        printf("Could not open semaphore!\n");
        exit(1);
     }

     /* Open semaphore (size 10) for telephone center. */
     sem_teleph = sem_open(SEM_TELEPHONES, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 10); 
     if (sem_teleph == SEM_FAILED){
        printf("Could not open semaphore!\n");
        exit(1);
     }

}



/* Function for printing final results. */
/* Print: Percentage of failed orders, average service time, average waiting time, */
/*        all the money transfers from company account to bank account and total sum of money. */
void print_results(){
  int i;

  printf("\n\033[42m## INFORMATION - STATISTICS ##\033[0m\n\n");

              /********* PRINT PERCENTAGE OF FAILS  *********/
  // Divide the # of failures with the # of total clients.
  printf("  The overall percentage of failed orders is ---> %0.1f%% \n", ( (float)*(s2 + fails_cnt) / *(s2 + total_cnt) ) * 100 );
              /**********************************************/

              /********* PRINT AVERAGE SERVICE TIME *********/
  // Divide the total waiting time with the # of total clients.
  printf("  The average waiting time is ---> %0.1f sec\n", (float)*(s2 + total_wait_time) / *(s2 + total_cnt) );
              /**********************************************/

              /********* PRINT AVERAGE SERVICE TIME *********/
  // Divide the total service time with the # of total clients.
  printf("  The average service time is ---> %0.1f sec\n\n", (float)*(s2 + total_service_time) / *(s2 + total_cnt) );
              /**********************************************/



              /********* PRINT TRANSFER TABLE *********/
  printf("\033[34mMoney Transfers: \033[0m\n");
  for(i=0 ; i < number_trans ; i++ ){ // For number of transfers.
    printf("  Transfer #%d ==========> %d€ from company account to bank account\n\n", i, trans_table[i]); //Print each result.
  }
              /***************************************/

              /********* PRINT TOTAL MONEY EARNED *********/
  printf("  Total money earned by selling all tickets ---> %d€\n", *(s2 + bank_acc));


              /********************************************/

}


/* Function for calculating ammount of an order */
/* Arguments: the # of seats & the number of the zone the client wants. */
/* The function returns the ammount of the current order. */
/* This function is used only by the client because of printing the result of an order (in case of success). */
int calc_ammount(int num_tick, int zn){

  int ammount = 0; // Buffer for ammount of the order.

  if(zn == 1){ // If zone A.
    ammount = 50*num_tick; // Calculate the ammount of the order.
  } 
  if(zn == 2){ // If zone B.
    ammount = 40*num_tick; // Calculate the ammount of the order.
  } 
  if(zn == 3){ // If zone C.
    ammount = 35*num_tick; // Calculate the ammount of the order. 
  } 
  if(zn == 4){ // If zone D.
    ammount = 30*num_tick; // Calculate the ammount of the order. 
  } 

  return ammount;  
}


/* Function for increasing total_wait_time by clients' wait time  */
/* Arguments: waiting time on hold for the current client. */
void incr_total_wait_time(double w_time){

  sem_wait(sem_shm2); // Lock semaphore for small shm.
  /* Increase total_wait_time by clients' wait time. */
  *(s2 + total_wait_time) = *(s2 + total_wait_time) + w_time; 
  sem_post(sem_shm2); // Unlock semaphore for small shm.

}


/* Function for increasing total_service_time by clients' service time  */
/* Arguments: service time for the current client. */
void incr_total_service_time(double s_time){

  sem_wait(sem_shm2); // Lock semaphore for small shm.
  /* Increase total_service_time by clients' service time. */
  *(s2 + total_service_time) = *(s2 + total_service_time) + s_time; 
  sem_post(sem_shm2); // Unlock semaphore for small shm.

}



#endif