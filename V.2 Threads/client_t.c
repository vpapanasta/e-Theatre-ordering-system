#include "MyHeader.h" /* for user-defined constants */


void client_signal_handler(int sig_c) {   
    
    alarm(t_wait); // Alarm every 10 seconds.
    signal(SIGALRM, client_signal_handler); // Signal for printing "SORRY" every 10 seconds.
    
    fprintf(stdout, "\n\033[033m*** SORRY for waiting client. ***\033[0m \n\n "); 
          
}


int main( int argc, char **argv )
{
   int sockfd; // Client's endpoint variable.
   struct sockaddr_un servaddr; // Struct for the server socket address.
   int pid, iter;
   char msg_c[5] = "    "; // Buffer for saving the message for client to server
                                           
   int line; // Message received from server.      

          
   sockfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); // Create the client's endpoint.

   bzero( &servaddr, sizeof( servaddr ) ); // Zero all fields of servaddr. 
   servaddr.sun_family = AF_LOCAL; // Socket type is local (Unix Domain).
   strcpy( servaddr.sun_path, UNIXSTR_PATH ); // Define the name of this socket. 

   

/* Connect the client's and the server's endpoint. */
  if( connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
      perror("connect");
      exit(1);
  }
     
   

       

/* MANUAL ORDER OR RANDOM ORDER */

                /************************* MANUAL ORDER *************************/

   if(argc > 1){ //If there is input manually from user.


      for(iter = 1 ; iter <= argc - 1 ; iter++)  // Save user's input in buffer msg_c.
      {    
            
            msg_c[iter - 1] = *argv[iter];
      } 



      
      /* Check if the user's manual input is valid. */
      /* We use function atoi() for cascading char input to integer. */
      if( (argc == 4 || argc == 1) && atoi(argv[1]) > 0 && atoi(argv[1]) < 5 && \
          atoi(argv[2]) > 0 && atoi(argv[2]) < 5 && \
          atoi(argv[3]) >= 0 && atoi(argv[3]) < 10 ){
            
            write( sockfd, msg_c , sizeof( msg_c ) ); // Client send to server.
            
      }
      else{ // IF user's input is INVALID!

          fprintf(stdout, "\n\033[31m-> Your input is invalid.\033[0m \n\n");

          fprintf(stdout, "\033[45m##********************* < HELPING MENU > *********************##\033[0m\n\n");
          
          fprintf(stdout, "> How many tickets do you want?\nYou can choose between [ 1 | 2 | 3 | 4 ]. (Then press ENTER: )");
          fprintf(stdout, "\nPlease choose the # of seats: ");
          scanf("%c", &msg_c[0]);
          getchar(); // Takes the character putted by ENTER key. 

          while( msg_c[0] < '1' || msg_c[0] > '4'){                
                fprintf(stdout, "\nPlease try again: ");                 
                scanf("%c", &msg_c[0]);
                getchar(); // Takes the character putted by ENTER key.
          }
         

          fprintf(stdout, "\n> Choose the number of the zone you want to reserve to.\n");
          fprintf(stdout, "> You can choose between zones [ 1 | 2 | 3 | 4 ]. (Then press ENTER: )\nPlease choose the # of zone: ");
          scanf("%c", &msg_c[1]);
          getchar(); // Takes the character putted by ENTER key.

          while(msg_c[1] < '1' || msg_c[1] > '4'){
                printf("\n> Please,try again: ");
                scanf("%c", &msg_c[1]);
                getchar(); // Takes the character putted by ENTER key.

          }


          fprintf(stdout, "\n> Enter the number of your credit card. ");
          fprintf(stdout, "\nYou can choose between [ 0 - 9 ]. (Then press ENTER: )\nPlease choose the number of your card: ");
          scanf("%c", &msg_c[2]);
          getchar(); // Takes the character putted by ENTER key.

          while(msg_c[2] < '0' || msg_c[2] > '9'){
              fprintf(stdout, "\n> Please, try again: ");
              scanf("%c", &msg_c[2]);
              getchar(); // Takes the character putted by ENTER key.
          }


           write( sockfd, msg_c , sizeof( msg_c ) ); // Client to server.
      }


      } 
      /************************* RANDOM ORDER *************************/   
    else{ 

             
        srand(time(NULL) ^ (getpid()<<16)); // Seeding fast the rand() function. 

        msg_c[0] = rand()%(53 - 49) + 49; // Generate random # of tickets [1-4].
        
        msg_c[1] = rand()%(58 - 48) + 48; // Generate random # of zone [0->A (10%). 1, 2->B(20%). 3, 4, 5->C (30%). 6, 7, 8, 9->D (40%)].

        if(msg_c[1] == '0'){                                               // 10% chance
              msg_c[1] = '1';
        }
        else if(msg_c[1] == '1' || msg_c[1] == '2'){                       // 20% chance
              msg_c[1] = '2';     
        } 
        else if(msg_c[1] == '3' || msg_c[1] == '4' || msg_c[1] == '5'){    // 30% chance
              msg_c[1] = '3';     
        }
        else{                                                              // 40% chance
              msg_c[1] = '4';  
        }


        msg_c[2] = rand()%(58 - 48) + 48; // Generate random credit card number [0-9]. 0 is the fail number (10%).


        write( sockfd, msg_c , sizeof( msg_c ) ); // Client to server.
                             
    }
  
      
    /************************************/ 
    alarm(t_wait); // Alarm every 10 seconds.
    /* Signal for printing "SORRY" every 10 seconds in client_signal_handler(int sig_c). */ 
    signal(SIGALRM, client_signal_handler); 
    read( sockfd, &line, sizeof( (int*) line ) );
    alarm(0);
    /***********************************/ 




    /* Read from server. */      
    read( sockfd, &line, sizeof( (int*) line ) ); 
    
         
     
     
   /* Print a message for each of the 4 cases.  */

   switch (line) {

    case 66: // 66(Dec) ---> B(Char).
        fprintf(stdout, "\n\n\033[31m-> Your card number is NOT VALID.\033[0m\n\n");            
        break;

    case 67: // 67(Dec) ---> C(Char).
        // Cascade zones' chars (1, 2, 3, 4) to (A, B, C, D) by adding 16.
        fprintf(stdout, "\n\n\033[36m-> There are not available seats at zone %c.\033[0m\n\n", msg_c[1] + 16); 
        break;

    case 68: // 68(Dec) ---> D(Char).
        fprintf(stdout, "\n\n\033[34m-> There are NO seats available (The theatre is FULL).\033[0m \n\n");            
        break;

    default: // // Case: (68 + id), we pass the id in "coded form". Decoding it using (-68) in printing below.
        // Cascade zones' chars (1, 2, 3, 4) to (A, B, C, D) by adding 16.
        // Calculate the ammount of order calling function calc_ammount(int num_tick, int zn) cascading char to int using (-48).
        if(line > 68){ // Security check.      
           fprintf(stdout, "\n\n\033[32m-> Successfull order!"
                   " ID of reservation: #%d, %c seats in zone %c."
                   " Amount of transaction: %d€.\033[0m\n\n", (line - 68), msg_c[0], msg_c[1] + 16, calc_ammount(msg_c[0] - 48, msg_c[1] - 48) );
        }
        else if(line < 68){ // Security check.
           fprintf(stdout, "\n\n\033[34m-> There are NO seats available (The theatre is FULL).\033[0m \n\n"); 
        }
    }
                

     close( sockfd ); // Close socket using file descriptor.
}