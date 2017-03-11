#include "MyHeader.h" // MyHeader: headers, global variables, functions.





/* The use of this functions avoids the generation of "zombie" processes. */
void sig_chld( int signo ){
     pid_t pid;
     int stat;

     while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 ) {
            printf( "Child %d terminated.\n", pid );
     }
}


/* Handler function for SIGALRM & SIGINT */
void general_signal_handler(int sig){
       
     

     if(sig == SIGALRM){ // If signal == ALARM

        alarm(t_trans); // Alarm every 30 seconds
        signal(SIGALRM, general_signal_handler); // Signal for transfer money every 30 seconds        

        trans_money(); // Transfer money from local account to bank account

      }


      signal(SIGINT, general_signal_handler); // Signal for detaching shm and destroying semaphores with Ctrl + C
      

      if(sig == SIGINT){

        /*########## PRINT ALL RESULTS #############*/ 
        Print_Reserves_Seats(); // Print zones with reserved seats
        print_results(); // Print statistics
        /*##########################################*/


        // Unlink & close semaphores
        semaphores_close(); 
        semaphores_unlink();
                        
        // Destroy shared Memories
        shmctl(shmid_1, IPC_RMID, NULL);

        shmctl(shmid_2, IPC_RMID, NULL);

        shmctl(shmid_3, IPC_RMID, NULL);                                              
                            
                   
        exit(0);
      }
 }       









int main( int argc, char **argv )
{      
   int listenfd, connfd; // Socket descriptors.
   char line; 
   pid_t childpid, bank_chilldpid, seats_chilldpid, wait_pid;
   int status = 0; // Wait status.
   socklen_t clilen;
   struct sockaddr_un cliaddr, servaddr; // Structs for the client and server socket addresses.
   int rec_msg[5]; // Buffer for recieved message. 
   int msg_s; // Message from server to client.
   int i = 0; // Iter for saving the recieving message.

   


   /*******************  BIG, SMALL AND STRUCT SHARED MEMORY DEFINITION AREA    *******************/

   key_t key_1 = 9122; /* Big Shared Memory Key */ 
   
   key_t key_2 = 5176; /* Small Shared Memory Key */

   key_t key_3 = 1992;  /* Struct Shared Memory Key */
   
  

                                  /**** Big SHM ****/

   /* Create the segment */
    if ((shmid_1 = shmget(key_1, SHMSZ1, IPC_CREAT | 0666)) < 0)
    {

           perror("shmget");
           exit(1);

    }

    /* Attach the segment to our data space */
    if (( shm_1 = shmat(shmid_1, NULL, 0)) == (int *) - 1) // Shared memory of integers
    {

           perror("shmat");
           exit(1);

    }

    s1 = shm_1; // Copy of big shm's pointer.



                                   /**** Small SHM ****/

     /* Create the segment */
    if ((shmid_2 = shmget(key_2, SHMSZ2, IPC_CREAT | 0666)) < 0) 
    {

           perror("shmget");
           exit(1);

    }

    /* Attach the segment to our data space */
    if (( shm_2 = shmat(shmid_2, NULL, 0)) == (int *) - 1) // Shared memory of integers
    {

           perror("shmat");
           exit(1);

    }

    s2 = shm_2; // Copy of small shm's pointer.


                      /**** Client Structs' SHM ****/


   /* Create the segment */
    if ((shmid_3 = shmget(key_3, SHMSZ3, IPC_CREAT | 0666)) < 0)
    {

           perror("shmget");
           exit(1);

    }

    /* Attach the segment to our data space */
    if (( shm_3 = shmat(shmid_3, NULL, 0)) == (struct clnt *) - 1) // Shared memory of struct clnt
    {

           perror("shmat");
           exit(1);

    }

    s3 = shm_3; // Copy of struct shm's pointer.   
    


    init_small_shm(); // Call initialization function for small shm. 



   /***********************  **********************   ***********************/


   // Signal for killing zombie prosseces with ctrl + C
   signal( SIGCHLD, sig_chld ); 
   // Signal for detaching shm and destroying semaphores with Ctrl + C
   signal( SIGINT, general_signal_handler); 



   listenfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); // Create the server's endpoint. 

   /* ATTENTION!!! THIS ACTUALLY REMOVES A FILE FROM YOUR HARD DRIVE!!! */
   unlink( UNIXSTR_PATH ); // Remove any previous socket with the same filename.

   bzero( &servaddr, sizeof( servaddr ) ); // Zero all fields of servaddr. 
   servaddr.sun_family = AF_LOCAL; // Socket type is local (Unix Domain).
   strcpy( servaddr.sun_path, UNIXSTR_PATH ); // Define the name of this socket. 

   /* Create the file for the socket and register it as a socket. */
   bind( listenfd, ( struct sockaddr* ) &servaddr, sizeof( servaddr ) );

   listen( listenfd, LISTENQ ); // Create request queue.


   open_all_semaphores(); // Call function for opening all semaphores.



  
   alarm(t_trans); // Alarm every 30 seconds.
   signal(SIGALRM, general_signal_handler); // Signal for transfer money every 30 seconds.






/*##################################################################################################*/                             
/*##################################################################################################*/
   
   for ( ; ; ) {


          



      clilen = sizeof( cliaddr ); // Size of client's address.

      /* Copy next request from the queue to connfd and remove it from the queue. */
      connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen );

      if ( connfd < 0 ) {
             if ( errno == EINTR ) // Something interrupted us.
                    continue; //Back to for()... 
             else {
                    fprintf( stderr, "Accept Error\n" );
                    exit( 0 );
             }
      }


    


      childpid = fork(); // Spawn a child -> SERVING CHILD


      /************************ WAITING TIME - START **************************/  
             if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) { // Get time of start                    
                  perror( "clock gettime" ); 
                  return EXIT_FAILURE;
             }
      /************************************************************************/


      /**************************** Starting of serving process - child *****************************/    
                         
             
      if ( childpid == 0 ) { /* SERVING Child process. */




             close( listenfd ); /* Close listening socket. */


            
             sem_wait(sem_teleph); // Lock semaphore (size 10) for telephone center


             /** When client stops waiting in queue, send message for stop printing SORRY **/           
             msg_s = 'S';             
             write( connfd, &msg_s , sizeof((char*) msg_s) );
             /*****************************************************************************/


             
             /************************ WAITING TIME - STOP **************************/ 
             if( clock_gettime( CLOCK_REALTIME, &stop) == -1 )  { // Get stop time.
                     perror( "clock gettime" );
                     return EXIT_FAILURE;
             }
             (*s3).wait_time = ( stop.tv_sec - start.tv_sec ) // Result of time stamps
                                            + (double)( stop.tv_nsec - start.tv_nsec )
                                            / (double)BILLION;
             
             incr_total_wait_time((*s3).wait_time); // Increase total_wait_time by clients' wait time.
             
             /***********************************************************************/



            
             /************************ SERVICE TIME - START **************************/  
             if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) { // Get time of start                    
                  perror( "clock gettime" ); 
                  return EXIT_FAILURE;
             }
             /***********************************************************************/
             



             while(read( connfd, &line, sizeof( char ) ) > 0){ // Read from client                    
                    
                rec_msg[i] = (int) line - 48; // Saving message from client cascading from char to int.
                i++;
                

                if(line == '\0') {  // If reach to end of string then BREAK.
                       printf("\n");
                       break;
                }                 

             }



/* Changing permititions of shared memories so read & write are only allowed on them. */
/***************************************Change Permitions*************************************/       

                                /**** Big SHM ****/
             
              if ((shmid_1 = shmget(key_1, SHMSZ1, 0666)) < 0)
              {

                     perror("shmget");
                     exit(1);

              }

              /* Attach the segment to our data space */
              if (( shm_1 = shmat(shmid_1, NULL, 0)) == (int *) - 1) 
              {

                     perror("shmat");
                     exit(1);

              }
              s1 = shm_1; // Copy of big shm's pointer.


                                /**** Small SHM ****/
               
              if ((shmid_2 = shmget(key_2, SHMSZ2, 0666)) < 0)
              {

                     perror("shmget");
                     exit(1);

              }

              /* Attach the segment to our data space */
              if (( shm_2 = shmat(shmid_2, NULL, 0)) == (int *) - 1)
              {

                     perror("shmat");
                     exit(1);

              }

              s2 = shm_2; // Copy of big shm's pointer.

                                /**** Client Structs SHM ****/

              if ((shmid_3 = shmget(key_3, SHMSZ3, 0666)) < 0)
              {

                     perror("shmget");
                     exit(1);

              }

              /* Attach the segment to our data space */
              if (( shm_3 = shmat(shmid_3, NULL, 0)) == (struct clnt *) - 1)   
              {

                     perror("shmat");
                     exit(1);

              }

              s3 = shm_3; // Copy of struct shm's pointer.    



/********************************************************************************************/
          
	                                 
                                     

             sem_wait(sem_shm2); // Lock semaphore to increase counter in small shm.

             /******INCRESE COUNTER******/  
              s3 += inc_total_cnt(); // Increase total_cnt.
             (*s3).id_c = *(s2 + total_cnt); // Initialize client's id in struct.
            /***************************/

             sem_post(sem_shm2); // Unlock semaphore.      
             
                               
             fflush( stdout ); // Flush stdout to output.
             printf("\nClient # %d with pid = %d \n\n", (*s3).id_c, getpid());



             if(*(s2 + full_theatre) != 1){ // If NOT FULL THEARTE, then check card and find seats.


                 /****** CREATE PARALLEL PROCESSES FOR CARD CHECK & FIND SEATS ******/

                 for(i = 0 ; i < 2 ; i++){ // For two times.

                    if(i == 0){  // Create child in order to check if card in valid or invalid.

                      bank_chilldpid = fork(); // Spawn the card check child - process.
                      if ( bank_chilldpid == 0 ) { // Child process.

                        (*s3).flag1 = card_check(rec_msg[2]); // Check card number.
                        
                        sleep(t_cc); // Sleep process for 2 secs.

                        exit(0); // Terminate child process.
                      }

                    }
                  else{ // Create child in order to find available seats. 

                      seats_chilldpid = fork(); // Spawn the seats find child - process.
                      if ( seats_chilldpid == 0 ) { // Child process.

                        sleep(t_sf); // Sleep process for 6 secs.

                        /*^^^^^^^^^^^ Conditionally lock semaphore ^^^^^^^^^^^*/
                        my_sem_zone_wait(rec_msg[1]); 
                        
                        (*s3).flag2 = find_seats(rec_msg[0], rec_msg[1]); // Find seats for client.                                                                                        
                         
                        exit(0); // Terminate child process.
                      }

                    }


                 }



                 /* We wait for both processes to end  and then go on...... */
                 /* wait(&status) is used like a "barrier" */                 
                   do{  

                    wait_pid = wait(&status);

                   } while(wait_pid > 0);                  
                 

                 /********************************************/

                  /* If card = INVALID && seats were reserved, go and delete the wrong ordered seats */
                  /* We delete ONLY if the card is invalid and seats were reserverd */
                  if( (*s3).flag1 == 'B' && (*s3).flag2 == 'A' ){ 
                    // In case of INVALID CARD and FULL ZONE, we DON'T DELETE.

                    seats_correction(rec_msg[0], rec_msg[1]); // DELETE wrong reserved seats.
                    
                 } 
                 
                 /*******************************************/

                 /*^^^^^^^^^^^ Conditionally unlock semaphore ^^^^^^^^^^^*/
                 my_sem_zone_post(rec_msg[1]);   

                 


                 /*************************/
                // If card = INVALID.
                 if( (*s3).flag1 == 'B' ){ 

                    increase_if_fail(); // Increase fails_cnt, because of failing to reserve seats.
                    msg_s = 'B'; // Pass 'B' as integer.

                 }
                 // If card = VALID and seats FOUND
                 else if( (*s3).flag1 == 'R' && (*s3).flag2 == 'A'){  
                    /* If seats found, calculate the price of order and increase local_acc. */
                    charge_order(rec_msg[0], rec_msg[1]); 
                    msg_s =  68 + (*s3).id_c; // Pass client's ID (incresed by 68 for client decode).


                 }
                 /* If card = VALID and seats NO FOUND (FULL ZONE) */
                 else if( (*s3).flag1 == 'R' && (*s3).flag2 == 'C'){ 

                    increase_if_fail(); // Increase fails_cnt, because of failing to reserve seats.
                    msg_s = 'C'; // Pass 'C' as integer.

                 }
                 /* If card = VALID and FULL THEATRE (for first time theatre is FULL) */
                 else if( (*s3).flag1 == 'R' && (*s3).flag2 == 'D'){ 

                    increase_if_fail(); // Increase fails_cnt, because of failing to reserve seats.
                    msg_s = 'D'; // Pass 'D' as integer.
                    
                    
                 }
                 /*************************/


              }
              else{ // In case of FULL THEARTE send D to the rest of clients.

                // Increase fails_cnt, because of failing to reserve seats.
                increase_if_fail(); 
                msg_s = 'D'; // Pass 'D' as integer.               

              }     
            









		         /* Server send to client */
		         write( connfd, &msg_s , sizeof((char*) msg_s) );


             /************************ SERVICE TIME - STOP **************************/ 
             if( clock_gettime( CLOCK_REALTIME, &stop) == -1 )  { // Get stop time 
                     perror( "clock gettime" );
                     return EXIT_FAILURE;
             }
             (*s3).service_time = ( stop.tv_sec - start.tv_sec ) // Result of time stamps.
                                            + (double)( stop.tv_nsec - start.tv_nsec )
                                            / (double)BILLION;
             // Increase total_wait_time by clients' wait time.
             incr_total_service_time((*s3).service_time);             
             
             /***********************************************************************/                                  
             

             if(shmdt(shm_1) == -1)  // Detach the 1st shared memory segment. 
             {
                perror("shmop: shmdt failed");
             } 

                                            
                        
             if(shmdt(shm_2) == -1)  // Detach the 2st shared memory segment. 
             {
                perror("shmop: shmdt failed");
             }



             sem_post(sem_teleph); // Unlock semaphore for telephone center.

             exit( 0 ); // Terminate SERVING child process.    
              

    } // End of child section.
             


     close(connfd); // Parent closes connected socket.

        
   }
   /*##########################################################################################*/
   /*##########################################################################################*/            

} // End of main.