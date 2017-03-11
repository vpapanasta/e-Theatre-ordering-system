#include "MyHeader.h" // For user-defined constants, functions and headers.


/* Size of the request queue. */
#define LISTENQ 800



/****** FUNCTIONS DECLARATION ******/
void *sudo_main();
void *reserving_seats( void *arg );
void *checking_card( void *arg );
void *seats_search_time();
void *transfer_wheel();




/* Each new thread has its own connfd. */
int listenfd, connfd[800]; // Socket descriptors.


/* Handler function for SIGINT (Ctrl+C). */
void signal_handler_term(int sig){

  mutexes_destroy(); // Destroy all mutexes.

  
  /*########## PRINT ALL RESULTS #############*/ 
  Print_Reserved_Seats(); // Print zones with reserved seats.
  print_results(); // Print statistics.
  /*##########################################*/

  
  close(listenfd); // Close listening socket. 

  exit(0); // Termination of all threads.
}




int main( int argc, char **argv )
{
   int tmp; // Save pthread_create() returned value.
   int k = 0; // Counting number of accepted clients.

   pthread_t serving_thread[800]; // Storing threads' IDs in this table.
   pthread_t transfer_thread; // ID of transfering thread.
   
   pthread_attr_t attr; // Attribute of thread.
   pthread_attr_init( &attr ); // Initialize the thread attributes object pointed to by attr.
   pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ); // Threads that are created using attr will be created in a detached state.

       
   signal( SIGINT, signal_handler_term); // Signal for detaching shm and destroying my_semaphores with ctrl + C.

   socklen_t clilen;
   struct sockaddr_un cliaddr, servaddr; // Structs for the client and server socket addresses.
   
   
   

   listenfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); // Create the server's endpoint.

   unlink( UNIXSTR_PATH ); // Remove any previous socket with the same filename. 

   bzero( &servaddr, sizeof( servaddr ) ); // Zero all fields of servaddr.
   servaddr.sun_family = AF_LOCAL; // Socket type is local (Unix Domain). 
   strcpy( servaddr.sun_path, UNIXSTR_PATH ); // Define the name of this socket.

   /* Create the file for the socket and register it as a socket. */
   bind( listenfd, ( struct sockaddr* ) &servaddr, sizeof( servaddr ) );

   listen( listenfd, LISTENQ ); // Create request queue.


   /* ## Initialize mutexes, condition variables and counters. ## */ 
   my_sema_init(&s_zone[0], 1); 
   my_sema_init(&s_zone[1], 1);
   my_sema_init(&s_zone[2], 1);
   my_sema_init(&s_zone[3], 1);

   my_sema_init(&s_bank, Nb);
   my_sema_init(&s_teleph, Nt);



   /******   MONEY TRANSFER   ******/

   /* Creating the thread which is going to make the money tranfers while server operates. */
   tmp = pthread_create( &transfer_thread, &attr, transfer_wheel, NULL );
      
   if ( tmp != 0 )
   {
      fprintf(stderr,"Creating thread failed!");
      return 0;
   }

   /*******************************/






   for (  ; ; k++ ) {

      clilen = sizeof( cliaddr );

      /* Copy next request from the queue to connfd and remove it from the queue. */
      connfd[k] = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen );

      if ( connfd[k] < 0 ) {
             if ( errno == EINTR ) /* Something interrupted us. */
                    continue; /* Back to for()... */
             else {
                    fprintf( stderr ,"Accept Error\n" );
                    exit( 0 );
             }
      }

         

      /* For each accepted client, a new thread is creating in order to serve him. */


      /*#########################################################*/
        tmp = pthread_create( &serving_thread[k], &attr, sudo_main, NULL );
        
      
        if ( tmp != 0 )
        {
                fprintf(stderr,"Creating thread failed!");
                return 0;
        }   
      
      /*##########################################################*/

     pthread_attr_destroy(&attr); // Destroy thread's attribute. 
   }

}





/* The function called by each client's pthread_create(). */
/* In this function includes the "main" part of the server. */                                    
void *sudo_main(){


   char line; // Variable in which read() puts the message from client.
   char rec_msg[5] = ""; // Buffer for recieved message.
   int i = 0; 
   int local_temp = 0; // Variable for temporary save the # of client (total_cnt).
  
   int msg_s = 0; // Message from server to client
   int temp_B, temp_S; // Save returned value for each of the two calls of pthread_create().

   pthread_t seats_time_thread, card_thread, seats_thread; // Variables of pthread_t type.

   pthread_attr_t attr2; // Attribute of threads.
   pthread_attr_init( &attr2 ); // Initialize the thread attributes object pointed to by attr2.
   pthread_attr_setdetachstate( &attr2, PTHREAD_CREATE_JOINABLE ); // Threads that are created using attr will be created in a joinable state.


   pthread_mutex_lock( &variables_mutex ); // Mutex lock.

   /******INCRESE COUNTER******/  
   local_temp = inc_total_cnt(); // Increase total_cnt.
   cls[local_temp].id_c = local_temp;
   /***************************/
   
   pthread_mutex_unlock( &variables_mutex ); // Mutex unlock.



   /************************ WAITING TIME - START **************************/  
             if( clock_gettime( CLOCK_REALTIME, &cls[local_temp].start) == -1 ) { // Get time of start.                    
                  perror( "clock gettime" ); 
                  return EXIT_FAILURE;
             }
   /************************************************************************/


   /* Lock critical section using my_sema_wait().*/
   /* There are only 10 telephones available. */
   /* We allow only 10 threads to execute this critical section simultaneously. */
   my_sema_wait( &s_teleph ); 
        
   

   /** When client stops waiting in queue, send message for stop printing SORRY. **/           
   msg_s = '@';             
   write( connfd[local_temp - 1], &msg_s, sizeof((char*) msg_s) );
   /*****************************************************************************/


   /************************ WAITING TIME - STOP **************************/ 
   if( clock_gettime( CLOCK_REALTIME, &cls[local_temp].stop) == -1 )  { // Get stop time.
           perror( "clock gettime" );
           return EXIT_FAILURE;
   }
   cls[local_temp].wait_time = ( cls[local_temp].stop.tv_sec - cls[local_temp].start.tv_sec ) // Result of time stamps.
                                  + (double)( cls[local_temp].stop.tv_nsec - cls[local_temp].start.tv_nsec )
                                  / (double)BILLION;
   
    incr_total_wait_time( cls[local_temp].wait_time ); // Increase total_wait_time by clients' wait time.
   /***********************************************************************/

  


   /************************ SERVICE TIME - START **************************/  
   if( clock_gettime( CLOCK_REALTIME, &cls[local_temp].start) == -1 ) { // Get time of start.                    
       perror( "clock gettime" ); 
       return EXIT_FAILURE;
   }
   /***********************************************************************/ 

  




   while(read( connfd[local_temp - 1], &line, sizeof( char ) ) > 0){ // Read from client.
              
                 
         rec_msg[i] = (int) line - 48; // Saving message from client cascading from char to int.
         i++;
         

         if(line == '\0') { // If reach to end of string BREAK.
                fprintf(stdout, "\n");
                break;
         }
                 
   }

   
   fprintf(stdout, "\nClient \033[36m#%d\033[0m requests \033[36m%d\033[0m"
                   " seats in zone \033[36m%c\033[0m.\n\n", cls[local_temp].id_c, rec_msg[0], rec_msg[1] + 64);



   

   /* Saving order's values to current clients struct. */
   cls[local_temp].tickets_num = rec_msg[0];
   cls[local_temp].zone_name = rec_msg[1];
   cls[local_temp].card_num = rec_msg[2];


  
   if( full_theatre != 1 ){ // If NOT FULL THEARTE, then check card and find seats.

       
       /* Create one new thread. */           
       /* This thread simulates the seats finding time. */
       temp_B = pthread_create( &seats_time_thread, &attr2, seats_search_time, NULL );

       if ( temp_B != 0 ){
          fprintf(stderr,"Creating thread failed!");
          return 0;
        }



       
       /* Create a new thread. */
       /* In this thread card is checked by a bank machine. */
       temp_B = pthread_create( &card_thread, &attr2, checking_card, (void *)&local_temp );

       if ( temp_B != 0 ){
          fprintf(stderr,"Creating thread failed!");
          return 0;
        } 




   /*############*/
       /* Lock telephones' critical section using my_sema_wait(). */ 
       /* Only one thread can write in each at the same time. */
       /* Selects which zone is going to be locked. */ 
       my_sema_wait( &s_zone[ cls[local_temp].zone_name - 1 ] ); 
       

       
        
       /* Create a new thread. */
       /* In this thread seats are found to the selected zone. */
       temp_S = pthread_create( &seats_thread, &attr2, reserving_seats, (void *)&local_temp );

       if ( temp_S != 0 ){
          fprintf(stderr,"Creating thread failed!");
          return 0;
        } 

        /* Joining of card checking and finding seats threads. */
       temp_B = pthread_join( card_thread, NULL );

       if ( temp_B != 0 ){
         fprintf(stderr,"Joining thread failed!");
          return 0;
       }

       temp_S = pthread_join( seats_thread, NULL );

       if ( temp_S != 0 ){
         fprintf(stderr,"Joining thread failed!");
          return 0;
       }

       
  
       /* If card = INVALID && seats were reserved, go and delete the wrong ordered seats */
       /* We delete ONLY if the card is invalid and seats were reserverd */
       if(cls[local_temp].flag_B == 'B' && cls[local_temp].flag_S == 'A')
         seats_correction(cls[local_temp].tickets_num, cls[local_temp].zone_name);

       
       /* Unlock critical section of locked zone using my_sema_post().*/ 
       my_sema_post( &s_zone[ cls[local_temp].zone_name - 1 ] ); 
  /*############*/       
                   


       /* seats_time_thread joins the two created threads for the
          card checking and finding seats.
       */
       temp_S = pthread_join( seats_time_thread, NULL );

       if ( temp_S != 0 ){
         fprintf(stderr,"Joining thread failed!");
          return 0;
       }

       



       /************************************** CHECKINGS **************************************/

       /* If card = INVALID. */
       if( cls[local_temp].flag_B == 'B' ){ 

          increase_if_fail(); // Increase fails_cnt, because of failing to reserve seats.
          msg_s = 'B'; // Pass 'B' as integer.

       }
       /* If card = VALID and seats FOUND. */
       else if( cls[local_temp].flag_B == 'R' && cls[local_temp].flag_S == 'A'){

          // If seats found, calculate the price of order and increase local_acc.
          charge_order( cls[local_temp].tickets_num, cls[local_temp].zone_name ); 
          msg_s = 68 + cls[local_temp].id_c; // Pass client's ID (incresed by 68 for client decode).


       }
       /* If card = VALID and seats NO FOUND (FULL ZONE). */
       else if( cls[local_temp].flag_B == 'R' && cls[local_temp].flag_S == 'C'){ 

          increase_if_fail(); // Increase fails_cnt, because of failing to reserve seats.
          msg_s = 'C'; // Pass 'C' as integer.

       }
       /* If card = VALID and FULL THEATRE (for first time theatre is FULL). */
       else if( cls[local_temp].flag_B == 'R' && cls[local_temp].flag_S == 'D'){ 

          increase_if_fail(); // Increase fails_cnt, because of failing to reserve seats.
          msg_s = 'D'; // Pass 'D' as integer.
          
          
       }

       /****************************************************************************************/


   }    
   

          
   /* Server writes to client. */
   write( connfd[local_temp - 1], &msg_s, sizeof((char*) msg_s) );




   /************************ SERVICE TIME - STOP **************************/ 
   if( clock_gettime( CLOCK_REALTIME, &cls[local_temp].stop) == -1 )  { // Get stop time.
           perror( "clock gettime" );
           return EXIT_FAILURE;
   }
   cls[local_temp].service_time = ( cls[local_temp].stop.tv_sec - cls[local_temp].start.tv_sec ) // Result of time stamps.
                                  + (double)( cls[local_temp].stop.tv_nsec - cls[local_temp].start.tv_nsec )
                                  / (double)BILLION;
   /* Increase total_wait_time by clients' wait time. */
               
   incr_total_service_time( cls[local_temp].service_time );             
   
   /***********************************************************************/





   /* Unlock telephones' critical section using my_sema_post(). */ 
   my_sema_post( &s_teleph ); 

   pthread_attr_destroy(&attr2); // Destroy threads' attribute. 

   /* Take the ID of the current thread and mark this as detached.
   When a detached thread terminates, its resources are automatically
   released back to the system without the need for another thread to join with the terminated thread. */
   pthread_detach(pthread_self()); 
   close( connfd[local_temp - 1] ); // Closes connected socket.
   pthread_exit( NULL ); // Terminate the calling thread.

}










/* Function of thread which make the seats reservations. */
/* Input: The client's numeber-ID. */
void *reserving_seats( void *arg ) {
    int id;
    id = *((int *)arg);

    /* Call function find_seats() to do the seats reservation. */
    cls[id].flag_S = find_seats( cls[id].tickets_num, cls[id].zone_name, cls[id].id_c );
   
    pthread_exit( NULL ); // Terminate the calling thread.
}




/* Function of thread which make the card checking. */
/* Input: The client's numeber-ID. */
void *checking_card( void *arg ) {
    int id;
    id = *((int *)arg);

    
    /* Lock card check critical section using my_sema_wait().*/
    /* There are only 4 bank machines available. */
    /* We allow only 4 threads to execute this critical section simultaneously. */
    my_sema_wait( &s_bank ); 

    /* Call function card_check() to do the card checking. */ 
    cls[id].flag_B = card_check( cls[id].card_num ); 
    
    /* Unlock bank check critical section using my_sema_post(). */ 
    my_sema_post( &s_bank ); 

    usleep(t_cc); // Sleep for 2 seconds.   

    pthread_exit( NULL ); // Terminate the calling thread.

}

/* Function of thread simulating the demanding time of each client's servining. */
void *seats_search_time() {    

     usleep(t_sf); // Sleep for 6 seconds.
     pthread_exit( NULL ); // Terminate the calling thread.
}  


/* Function of thread which makes the transfer from company account to theatre account. */
void *transfer_wheel() {
  
  int tmp_1 = 1;
  int rtn = 0;

  /* Clear-initialize the two timespec variables. */
  struct timespec transfer_every = {0, 0}; 
  
  /*
     This while loops ONLY once every 30 seconds. The number of loops is equal to the
     number of transfers.
  */
  while(tmp_1){   

    transfer_every.tv_sec = time(NULL) + t_trans;

    /* 
    Wait until 30 secs pass. 
    After 30 secs do the money transfer. 
    */ 

    if (rtn == 0 || rtn == ETIMEDOUT){
      rtn = pthread_cond_timedwait(&cond_trans, &transfer_mutex, &transfer_every);
      /* After the passing of 30 secs, make the money transfer. */
      trans_money();
    }
    else continue;

  }


  pthread_exit( NULL ); // Terminate the calling thread.
}
