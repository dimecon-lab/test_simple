/*
 ============================================================================
 Name        : test.c
 Author      : Sale
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include "pb_encode.h"
#include "pb_decode.h"
#include "simple.pb.h"
#include "my_global.h"
#include "mysql.h"

#include <stdlib.h>
#include <string.h>    //strlen
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <sys/fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/time.h>
#include "sys/select.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include <time.h>


#define  SERVER_UDP_PORT	5683
#define  MAX_MESSAGE_LENGTH  500

#define INT8U  uint8_t
#define INT16U uint16_t
#define INT32U unsigned long int
#define INT64U uint64_t

#define INT8S  int8_t
#define INT16S int16_t
#define INT32S long int

struct thread_args { int fd[2]; };

typedef struct _pipe_message_ {
	INT16U sender;
	INT16U send_code;
	INT8U  source_ip[16];
	INT16U source_port;
	INT8U  destination_ip[16];
 	INT16U size;						// Velicina poruke u bajtovima
	INT8U  message[MAX_MESSAGE_LENGTH] __attribute__ ((aligned(4)));;
} pipe_message;

typedef struct _udp_struct_ {
	INT16U State; /* Trenutno stanje izvrsavanja */
	INT8U  lmc_ip[16];					 // IP LMCa (string)
	struct sockaddr_in destination_addr; // IP LMCa (socket struktura)
	INT32U destination;					 // Serijski broj brojila (int)
	INT32U source;
	//dl_dlms_message client_msg;
	//dl_dlms_message server_msg;
	time_t receive_timeout;
	INT8U  chunk[MAX_MESSAGE_LENGTH];
	INT16U offset_for_chunk;
	INT8U  Phase, ErrorNb, Retry_count, Disconnect;
} udp_struct;

void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}

void nanopb_test() {
	/* This is the buffer where we will store our message. */
	uint8_t buffer[128];
	size_t message_length;
	bool status;
	SimpleMessage message;

	/* Encode our message */

	/* Allocate space on the stack to store the message data.
	 *
	 * Nanopb generates simple struct definitions for all the messages.
	 * - check out the contents of simple.pb.h! */

	/* Create a stream that will write to our buffer. */
	pb_ostream_t out_stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

	/* Fill in the lucky number */
	message.lucky_number = 13;

	/* Now we are ready to encode the message! */
	status = pb_encode(&out_stream, SimpleMessage_fields, &message);
	message_length = out_stream.bytes_written;

	/* Then just check for any errors.. */
	if (!status)
	{
		printf("Encoding failed: %s\n", PB_GET_ERROR(&out_stream));
		return;
	}

	/* Now we could transmit the message over network, store it in a file or
	 * wrap it to a pigeon's leg.
	 */

	/* But because we are lazy, we will just decode it immediately. */

	/* Create a stream that reads from the buffer. */
	pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);

	/* Now we are ready to decode the message. */
	status = pb_decode(&stream, SimpleMessage_fields, &message);

	/* Check for errors... */
	if (!status)
	{
		printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
		return;
	}

	/* Print the data contained in the message. */
	printf("Your lucky number was %d!\n", message.lucky_number);
}

int open_server_socket(uint16_t port) {
	int sd, res, iSockFlags;
    struct sockaddr_in addr;

    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd == -1) {
         printf("Communicator Server: Could not create socket.");
         sd = -2;
         return sd;
     }

    //The socket must be Non-Blocking to avoid accept to be blocking
    iSockFlags = fcntl(sd, F_GETFL, 0);
    iSockFlags |= O_NONBLOCK;
    fcntl(sd, F_SETFL, iSockFlags);

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if ( (res = bind(sd, (struct sockaddr*)&addr, sizeof(addr))) != 0 ) {
    	printf("Communicator Server: Can't bind port %d, %s.\n", port, strerror(errno));
        close(sd);
        return -3;
    }

    return sd;
}

void* communicator_server(void *arg) {
	int curr_slot = 0;
	int receive_fd, send_fd, client_fd, max_sockets;
	pipe_message pipe_msg;
	fd_set fds, rfds;
	struct timeval timeout = {1, 0};
    struct thread_args *sent_fd = arg;
    struct sockaddr_in client;
    socklen_t addr_len = sizeof(struct sockaddr);
    INT8U client_request[MAX_MESSAGE_LENGTH];
    INT16S result;

    receive_fd = sent_fd->fd[0];
    send_fd = sent_fd->fd[1];
	client_fd = open_server_socket(SERVER_UDP_PORT);

	printf("Communicator Server: Started, Pipes (fd): rcv %d, send %d, socket %d\n", receive_fd, send_fd, client_fd);

    FD_ZERO(&fds);
    FD_SET(client_fd, &fds);  // Client UDP socket
    FD_SET(receive_fd, &fds); // Pipe from Communicator Server
    max_sockets = client_fd > receive_fd ? client_fd : receive_fd;

    // if (connect(client_fd , (struct sockaddr *)&client , sizeof(client)) < 0) {

<<<<<<< Upstream, based on origin/master
        /* Print the data contained in the message. */
        printf("Your lucky number was %d!\n", message.lucky_number);
        printf("5th commit!\n");
=======
    while (1) {

		rfds = fds;
    	curr_slot = select(max_sockets+1, &rfds, NULL, NULL, &timeout);
    	if (curr_slot == -1)
    	    printf("Communicator Server: ERROR Function select() returned -1, errno %d, %s.\n", errno, strerror(errno));

       	if (FD_ISSET(receive_fd, &rfds)) {
			// Data from Main is available now
       		if (read(receive_fd, &pipe_msg, sizeof(pipe_message)) != sizeof(pipe_message)) {
				printf("Communicator Server: Error in reading pipe, function read().\n");
			} else {
				printf("Communicator Server: Received pipe message from Main.\n");
			}
       	}

       	if (FD_ISSET(client_fd, &rfds)) {
        	result = recvfrom(client_fd, client_request, MAX_MESSAGE_LENGTH, 0, (struct sockaddr *)&client, &addr_len);
			strcpy(pipe_msg.source_ip, inet_ntoa(client.sin_addr));
			pipe_msg.source_port = ntohs(client.sin_port);
        	if (result > 0) {
        		//result = parse_udp_message(client_request, result, (struct sockaddr *)&client);
                //if (result == 1) {
                	//inet_ntoa(client_ip, &client.sin_addr);
                	//client.sin_port = htons(UNIT_DLMS_PORT_UDP);
                	//					client_data->slot[curr_slot].destination_addr.sin_family = AF_INET;
        			pipe_msg.send_code = 0;
        			if (result < MAX_MESSAGE_LENGTH) {
        				pipe_msg.size = result;
        				memmove(pipe_msg.message, client_request, result);
                    	printf("Communicator Server: Received UDP packet (IP %s:%d, Size %d) sent to MAIN.\n", pipe_msg.source_ip, pipe_msg.source_port, pipe_msg.size);
                    	write(send_fd, &pipe_msg, sizeof(pipe_message));
        			} else {
        				printf("Communicator Server: Received UDP packet (IP %s:%d, Size %d), TOO BIG - DROPED PACKET.\n", pipe_msg.source_ip, pipe_msg.source_port, pipe_msg.size);
        			}
        	} else if (result == 0) {
				printf("Communicator Server: Received UDP packet (IP %s:%d, Size %d), ZERO CONTENT - DROPED PACKET.\n", pipe_msg.source_ip, pipe_msg.source_port, pipe_msg.size);
        	} else {
        		printf("Communicator Server: ERROR Function recvfrom() returned result %d, errno %d, %s.\n", result, errno, strerror(errno));
        	}
       	}

>>>>>>> 83b7f5e NB IoT serverska aplikacija
    }

    printf("Communicator Server: Done.\n");
    pthread_exit(NULL);
    return NULL;
}

int main() {

	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char   *hostinfo;
	char   *serverinfo;
	int    protoinfo;

	char *server = "localhost";
	char *user = "root";
	char *password = "soa"; /* set me first */
	char *database = "wordpress";
	char *db_table = "test_table";
	char query_str[500];
	char temp_str[20];

	int  max_sockets, curr_slot;
	int *iPtr;
	unsigned char keyb_input[20];
	int send_pipe_fd[2], receive_pipe_fd[2];
	int receive_server_fd, send_server_fd;
	pipe_message pipe_msg;
	fd_set fds, rfds;
	struct timeval timeout = {1, 0};
    time_t curr_time;
    struct tm ltime;
    int prev_recv_sec = -1;
    float test_value, fvalue0, fvalue1, fvalue2, fvalue3;
    INT16U value0, value1, value2, value3;
    int valid_for_mysql_push;

	pthread_t server_tid;

	conn = mysql_init(NULL);

	/* Connect to database */
	if (!mysql_real_connect(conn, server,
			user, password, database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	/* Get host info */
	hostinfo = mysql_get_host_info(conn);

	/* Get server info */
	serverinfo = mysql_get_server_info(conn);

	/* Get protocol info */
	protoinfo = mysql_get_proto_info(conn);

	/* Output get info */
	printf("MAIN: MySQL Host     %s\n", hostinfo);
	printf("MAIN: MySQL Server   %s\n", serverinfo);
	printf("MAIN: MySQL Protocol %d\n", protoinfo);
	printf("MAIN: MySQL Client   %s\n\n", mysql_get_client_info());

	curr_slot = pipe(send_pipe_fd);
    if (curr_slot == 0) {
    	send_server_fd = send_pipe_fd[1];
    	printf("MAIN: Sending Pipe -> Communicator Server thread, created ok (fd %d).\n", send_server_fd);
	} else {
		printf("MAIN: Sending Pipe -> Communicator Server thread, error creating: %s\n", strerror(curr_slot));
	}

    curr_slot = pipe(receive_pipe_fd);
    if (curr_slot == 0) {
    	receive_server_fd = receive_pipe_fd[0];
    	printf("MAIN: Receiving Pipe <- Communicator Server thread, created ok (fd %d).\n", receive_server_fd);
	} else {
		printf("MAIN: Receiving Pipe <- Communicator Server thread, error creating: %s", strerror(curr_slot));
	}

    send_pipe_fd[1] = receive_pipe_fd[1];
    curr_slot = pthread_create(&server_tid, NULL, &communicator_server, &send_pipe_fd);
    if (curr_slot == 0) {
        printf("MAIN: Communicator Server thread, created ok.\n");
    } else {
    	printf("MAIN: Communicator Server thread, error creating: %s", strerror(curr_slot));
    }

    FD_ZERO(&fds);
    FD_SET(0, &fds); // Std input
    FD_SET(receive_server_fd, &fds); // Pipe from Communicator Server
    max_sockets = receive_server_fd;

    while (1) {

    	rfds = fds;
    	curr_slot = select(max_sockets+1, &rfds, NULL, NULL, &timeout);
    	if (curr_slot == -1)
    	    printf("MAIN ERROR: Function select() returned -1, errno %d.\n", errno);

    	if (FD_ISSET(receive_server_fd, &rfds)) {
    	    // Data from Communicator Server is available now
    		if (read(receive_server_fd, &pipe_msg, sizeof(pipe_message)) != sizeof(pipe_message)) {
        	    printf("MAIN: Error in reading server pipe, function read().\n");
    		} else {
    			if (pipe_msg.send_code != 0) {
    			} else if (pipe_msg.send_code == 0) {
    			}
    			time(&curr_time);
    			localtime_r(&curr_time, &ltime);
    			value0 = (INT16U)pipe_msg.message[1]+(INT16U)pipe_msg.message[2]*256;
    			value1 = (INT16U)pipe_msg.message[3]+(INT16U)pipe_msg.message[4]*256;
    			value2 = (INT16U)pipe_msg.message[5]+(INT16U)pipe_msg.message[6]*256;
    			value3 = (INT16U)pipe_msg.message[7]+(INT16U)pipe_msg.message[8]*256;

    			printf("MAIN: Client Request Received (Size %u, Time %02u.%02u.%02u %02u:%02u:%02u): %c %05u %05u %05u %05u %c.\n",
    				   pipe_msg.size, ltime.tm_mday, ltime.tm_mon + 1, ltime.tm_year + 1900,
    								  ltime.tm_hour, ltime.tm_min, ltime.tm_sec, pipe_msg.message[0],
									  value0, value1, value2, value3,
									  pipe_msg.message[9]);

    			if (prev_recv_sec != ltime.tm_sec) {
        			valid_for_mysql_push = ((value0 > 0) && (pipe_msg.message[0] == '!') && (pipe_msg.message[9] == '#'));
        			prev_recv_sec = ltime.tm_sec;
    			} else {
    				valid_for_mysql_push = 0;
    				printf("MAIN: Retransmission detected!\n");
    			}

    			if (valid_for_mysql_push) {
    				fvalue1 = (float)value1/(float)value0;
    				fvalue2 = (float)value2/(float)value0;
    				fvalue3 = (float)value3/(float)value0;

    				sprintf(query_str, "INSERT INTO `%s` (`ts`, `value1`, `value2`, `value3`) VALUES (CURRENT_TIMESTAMP,'", db_table);
    				sprintf(temp_str, "%f", fvalue1);
    				strcat(query_str, temp_str);
    				strcat(query_str, "','");
    				sprintf(temp_str, "%f", fvalue2);
    				strcat(query_str, temp_str);
    				strcat(query_str, "','");
    				sprintf(temp_str, "%f", fvalue3);
    				strcat(query_str, temp_str);
    				strcat(query_str, "');");

    				printf("MAIN: MySQL %s query generated (time %02u:%02u:%02u):\n \"%s\".\n", db_table,
    						ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
							query_str);
    				if (mysql_query(conn, query_str)) {
    					finish_with_error(conn);
    				}
    			} else {
    				if (value0 > 0) {
    					fvalue1 = (float)value1/(float)value0;
    					fvalue2 = (float)value2/(float)value0;
    					fvalue3 = (float)value3/(float)value0;
    				} else {
    					fvalue1 = 0; fvalue2 = 0; fvalue3 = 0;
    				}

    				printf("MAIN: MySQL %s query NOT generated (time %02u:%02u:%02u), INVALID data FV1 %f, FV2 %f FV3 %f.\n", db_table,
    						ltime.tm_hour, ltime.tm_min, ltime.tm_sec, fvalue1, fvalue2, fvalue3);
    			}
    		}
    	}

    	if (FD_ISSET(fileno(stdin), &rfds)) {
    		read(fileno(stdin), keyb_input, sizeof(keyb_input));
			if (keyb_input[0] == 't') {
				time(&curr_time);
				localtime_r(&curr_time, &ltime);
				test_value = ltime.tm_sec/60.0*0.8;
				sprintf(temp_str, "%f", test_value);
				sprintf(query_str, "INSERT INTO `%s` (`ts`, `value1`, `value2`, `value3`) VALUES (CURRENT_TIMESTAMP,'", db_table);
				strcat(query_str, temp_str);
				strcat(query_str, "','");
				sprintf(temp_str, "%f", test_value+0.1);
				strcat(query_str, temp_str);
				strcat(query_str, "','");
				sprintf(temp_str, "%f", test_value+0.2);
				strcat(query_str, temp_str);
				strcat(query_str, "');");

				printf("MAIN: TEST MySQL %s query generated (time %02u:%02u:%02u):\n \"%s\".\n", db_table,
						ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
						query_str);
				if (mysql_query(conn, query_str)) {
					finish_with_error(conn);
				}


			} else if (keyb_input[0] == 'i') {
				/* send SQL query table names // get_template_part( 'template-parts/footer/site', 'info' ); */
				if (0) {
					if (mysql_query(conn, "show tables")) {
						finish_with_error(conn);
					}

					res = mysql_use_result(conn);

					printf("MAIN: MySQL Tables in %s database:\n", database);
					while ((row = mysql_fetch_row(res)) != NULL)
						printf("%s \n", row[0]);
				}

				printf("\nMAIN: MySQL test_table, last 20 records:\n");
				sprintf(query_str, "(SELECT * FROM %s ORDER BY ts DESC LIMIT 20) ORDER BY ts ASC;", db_table);
				if (mysql_query(conn, query_str)) {
					finish_with_error(conn);
				}

				res = mysql_store_result(conn);
				if (res == NULL) {
					finish_with_error(conn);
				}

				int num_fields = mysql_num_fields(res);

				while ((row = mysql_fetch_row(res))) {
					for(int i = 0; i < num_fields; i++) {
						printf("%s ", row[i] ? row[i] : "NULL");
					}
					printf("\n");
				}

				mysql_free_result(res);
			}
    	}
    }
    mysql_close(conn);
    pthread_join(server_tid, (void**)&iPtr);

	{
		sprintf(query_str, "INSERT INTO `%s` (`ts`, `value`) VALUES (CURRENT_TIMESTAMP, '17.7');", db_table);
		if (mysql_query(conn, query_str))
		{
			finish_with_error(conn);
		}

		printf("MySQL measurement inserted into %s.\n", db_table);

		printf("MySQL test_table records:\n");
		sprintf(query_str, "(SELECT * FROM %s ORDER BY ts DESC LIMIT 20) ORDER BY ts ASC;", db_table);
		if (mysql_query(conn, query_str))
		{
			finish_with_error(conn);
		}

		res = mysql_store_result(conn);

		if (res == NULL)
		{
			finish_with_error(conn);
		}

		int num_fields = mysql_num_fields(res);

		while ((row = mysql_fetch_row(res)))
		{
			for(int i = 0; i < num_fields; i++)
			{
				printf("%s ", row[i] ? row[i] : "NULL");
			}
			printf("\n");
		}

		/* close connection */
		mysql_free_result(res);
		mysql_close(conn);
	}
	while (1) {
		sleep(1);
	}


return 0;
}

