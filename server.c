#include "blather.h"
// All functions Checked, No errors found

// Checked
client_t *server_get_client(server_t *server, int idx) {
  // Error Checking
  if (idx >= MAXCLIENTS || idx < 0) {
    printf("Requested client out of array bounds. Exiting...");
    exit(1);
  }
  return &server->client[idx];
}

// Checked
void server_start(server_t *server, char *server_name, int perms) {
  // Initialize Parameters
  strcpy(server->server_name, server_name);
  server->join_ready = 0;
  server->n_clients = 0;
  server->time_sec = 0;

  // create a string with ".fifo" appended to server_name
  char join_fname[MAXPATH];
  sprintf(join_fname, "%s.fifo", server_name);

  // remove old fifos and create new fifos
  remove(join_fname);
  int n = mkfifo(join_fname, perms);
  if (n == -1){
    perror("ERRNO INDICATES: ");
    exit(1);
  };

  server->join_fd = open(join_fname, O_RDWR);
  if(server->join_fd == -1){
    perror("ERRNO INDICATES: ");
    exit(1);
  }

  //Make the log file
  char log_fname[MAXPATH];
  sprintf(log_fname, "%s.log", server_name);
  server->log_fd = open(log_fname, O_RDWR | O_CREAT, 0666);


  printf("server_start(): end\n");
  return;
}

/*  Process Logic:
 *  1. Close file descriptors
 *  2. Remove <server_name>.fifo
 *  3. Broadcast shutdown message
 *  4. Remove one client at a time at index 0
 *    (server_remove_client does the shifting)
 */
// Checked
void server_shutdown(server_t *server) {
  char join_fname[MAXPATH];
  char log_fname[MAXPATH];

	sprintf(join_fname, "%s.fifo", server->server_name);
  sprintf(log_fname, "%s.log", server->server_name);

 	//close file descriptors
	close(server->join_fd);
  close(server->log_fd);

	// only remove join fifo and keep log file
	remove(join_fname);

  // broadcast shutdown message
  mesg_t msg;
  memset(&msg, 0, sizeof(mesg_t)); // this line avoids valgrind errors
  msg.kind = BL_SHUTDOWN;
  server_broadcast(server, &msg);

	// remove all existing clients
  while(server->n_clients != 0) {
    server_remove_client(server, 0);
  }

  printf("Shutting down server...\n");
  return NULL;
}

// Checked
int server_add_client(server_t *server, join_t *join) {
  // Error checking : Check if there is space available
	int n = server->n_clients;
	if(server->n_clients >= MAXCLIENTS)	{
		printf("Failed to add client, no space left on server");
		return -1;
	}

	// Initialize client
	strcpy(server->client[n].to_client_fname, join->to_client_fname);
	strcpy(server->client[n].to_server_fname, join->to_server_fname);
	strcpy(server->client[n].name, join->name);

  server->client[n].to_server_fd = open(server->client[n].to_server_fname, O_RDWR);
  server->client[n].to_client_fd = open(server->client[n].to_client_fname, O_RDWR);
  server->client[n].data_ready=0;
  server->client[n].last_contact_time = server->time_sec;
  server->n_clients += 1;

  // Broadcast join message to all users
  mesg_t msg;
  memset(&msg, 0, sizeof(mesg_t));
  strcpy(msg.name, join->name);
  msg.kind = BL_JOINED;
  server_broadcast(server, &msg);

  printf("server_add_client(): %s %s %s\n", join->name, join->to_client_fname, join->to_server_fname);
	return 0;
}

// Checked
int server_remove_client(server_t *server, int idx) {
	if(idx < 0 || idx >= MAXCLIENTS) {
		printf("Cannot aquire client at index %d, array index out of bounds.", idx);
		exit(1);
	}

  printf("Here in server_remove_client... \n");
	// close relative fifos
	close(server->client[idx].to_client_fd);
	close(server->client[idx].to_server_fd);

	// remove fifos
	remove(server->client[idx].to_client_fname);
	remove(server->client[idx].to_server_fname);

	// shift the rest of the clients down by one index
	if(idx < server->n_clients-1) {
		for(int i = idx; i < server->n_clients; i++) {
			server->client[i] = server->client[i+1];
		}
	}
	// decrease client counter in server
	server->n_clients -= 1;

  return 0;
}

// Checked
int server_broadcast(server_t *server, mesg_t *mesg) {
  // Send mesg to every single client
  for (int i=0; i< server->n_clients; i++){
    write(server->client[i].to_client_fd, mesg, sizeof(mesg_t));
  }

  // Log the message
  if(mesg->kind != BL_PING)
  server_log_message(server, mesg);

  // The rest is for debugging purpose
  switch(mesg->kind) {
    case BL_MESG:
      printf("server_broadcast(): %d from %s - %s\n", mesg->kind, mesg->name, mesg->body);
      break;
    case BL_DEPARTED:
      printf("server: depated client %s\n", mesg->name);
      break;
    case BL_DISCONNECTED:
      printf("%s has disconnect from server\n", mesg->name);
      break;
  }

  return 0;
}

// Checked
void server_check_sources(server_t *server) {
  // Add all the fds, including the join_fd to readset
  int maxfd = server->join_fd;
  fd_set readset;
  memset(&readset, 0, sizeof(fd_set));
  FD_ZERO(&readset);
  for (int i=0; i<server->n_clients; i++){
    if (server->client[i].to_server_fd > maxfd){
      maxfd = server->client[i].to_server_fd;
    }
    FD_SET(server->client[i].to_server_fd, &readset);
  }
  FD_SET(server->join_fd, &readset);

  // After calling select(), the program gose to sleep and wakes up whenever
  // one of the file descriptors has an update
  select(maxfd+1, &readset, NULL, NULL, NULL);

  // Wakes up, checks all fds
  for(int i=0; i<server->n_clients; i++) {
      if (FD_ISSET(server->client[i].to_server_fd, &readset)){
          // printf("FD_ISSET value for clients: %d\n", FD_ISSET(server->client[i].to_server_fd, &readset));
          server->client[i].data_ready = 1;
      }
  }

  // If there is a join available, set the join_ready flag
  if(FD_ISSET(server->join_fd, &readset)) {
    server->join_ready = 1;
  }
  return;
}

// Checked
int server_join_ready(server_t *server) {
  return server->join_ready;
}

// Checked
// This function is called in main when server_join_ready returns true
int server_handle_join(server_t *server) {
  join_t join;
  memset(&join, 0, sizeof(join_t));
  int nread = read(server->join_fd, &join, sizeof(join_t));
  if(nread == -1) {
    perror("ERRORNO Indicates: \n");
  }

  server_add_client(server, &join);
  server->join_ready = 0;
  return server->join_ready;

  printf("server_process_join()\n");
}

// Checked
int server_client_ready(server_t *server, int idx) {
  return server->client[idx].data_ready;
}

// Checked
// This function is only called when server_client_ready returns true
int server_handle_client(server_t *server, int idx) {
  server->client[idx].last_contact_time = server->time_sec;
  server->client[idx].data_ready = 0;
  mesg_t mesg;
  memset(&mesg, 0, sizeof(mesg_t));
  int n = read(server->client[idx].to_server_fd, &mesg, sizeof(mesg_t));
  //printf("%s just sent mesg.kind = %d\n", server->client[idx].name, mesg.kind);
  switch(mesg.kind) {
    case BL_MESG:
      printf("mesg received from client %d %s : %s\n", idx, mesg.name, mesg.body);
      server_broadcast(server, &mesg);
      break;
    case BL_DISCONNECTED:
      printf("%s has disconnected from server\n", mesg.name);
      server_remove_client(server, idx);
      server_broadcast(server, &mesg);
      break;
    case BL_DEPARTED:
      printf("%s has depated from chat\n", mesg.name);
      server_remove_client(server, idx);
      server_broadcast(server, &mesg);
      break;
    case BL_PING:
      server->client[idx].last_contact_time = server->time_sec;
      // printf("Client Number %d, Last contacted time: %d.\n", idx, server->client[idx].last_contact_time);
      break;
  }

  return 0;
}

// Checked
void server_remove_disconnected(server_t *server, int disconnect_secs) {
  // printf("Checking for disconnected users...\n");
  for(int i = 0; i < server->n_clients; i++) {
    // printf("The current server time : %d\n", server->time_sec);
    // printf("Client %d's last contact time : %d\n", i, server->client[i].last_contact_time);
    if(server->time_sec - server->client[i].last_contact_time >= disconnect_secs) {
      mesg_t msg;
      memset(&msg, 0, sizeof(mesg_t));
      msg.kind = BL_DISCONNECTED;
      strcpy(msg.name, server->client[i].name);
      server_remove_client(server, i);
      server_broadcast(server, &msg);
    }
  }
  return NULL;
}

// Checked
void server_tick(server_t *server) {
  server-> time_sec += 1;
  return NULL;
}

// Checked
void server_ping_clients(server_t *server) {
  // printf("Pinging all existing clients...\n");
  mesg_t msg;
  memset(&msg, 0, sizeof(mesg_t));
  msg.kind = BL_PING;
  server_broadcast(server, &msg);
  return NULL;
}

// Checked
void server_write_who(server_t *server){
    who_t who;
    memset(&who, 0, sizeof(who_t));
    who.n_clients = server->n_clients;
    for (int i=0; i<who.n_clients; i++){
      strcpy(who.names[i], server->client[i].name);
    }

    // Always write this to the start of the log file
    pwrite(server->log_fd, &who, sizeof(who_t), 0);
    return;
}

// Checked
void server_log_message(server_t *server, mesg_t *mesg){
    lseek(server->log_fd, 0, SEEK_END);
    write(server->log_fd, mesg, sizeof(mesg_t));
    return;
}
