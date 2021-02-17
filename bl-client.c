#include "blather.h"
#include "stdio.h"
#include "string.h"

char server_name[MAXNAME];
simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

client_t client_actual;
client_t *client = &client_actual;

pthread_t user_thread;          // thread managing user input
pthread_t background_thread;

void show_last_n(int n) {
  char log_fname[MAXNAME];
  sprintf(log_fname, "%s.log", server_name);
  int in_fd = open(log_fname, O_RDONLY);
  if(in_fd == -1) {
    perror("ERRORNO INDICATES");
    exit(1);
  }
  int pos = lseek(in_fd, -n*sizeof(mesg_t), SEEK_END);
  if(pos < sizeof(who_t)){
    lseek(in_fd, sizeof(who_t), SEEK_SET);
  }
  int nbytes = -1;
  mesg_t msg;
  memset(&msg, 0, sizeof(mesg_t));

  iprintf(simpio, "====================\n");
  iprintf(simpio, "LAST %d MESSAGES\n", n);
  while(nbytes != 0) {
    nbytes = read(in_fd, &msg, sizeof(mesg_t));
    if(nbytes == 0) {
      break;
    }
    switch(msg.kind) {
      case BL_MESG:
        iprintf(simpio, "[%s] : %s\n", msg.name, msg.body);
        break;
      case BL_JOINED:
        iprintf(simpio, "-- %s JOINED --\n", msg.name);
        break;
      case BL_DEPARTED:
        iprintf(simpio, "-- %s DEPARTED --\n", msg.name);
        break;
      case BL_SHUTDOWN:
        iprintf(simpio, "!!! server is shutting down !!!\n");
        pthread_cancel(user_thread);
        break;
      case BL_DISCONNECTED:
        iprintf(simpio, "-- %s DISCONNECTED --\n", msg.name);
        break;
    }
  }
  iprintf(simpio, "====================\n");
  close(in_fd);
}

void *user_worker(void *arg){
  while(!simpio->end_of_input){
    simpio_reset(simpio);
    iprintf(simpio, "");                                          // print prompt
    while(!simpio->line_ready && !simpio->end_of_input) {          // read until line is complete
      simpio_get_char(simpio);
    }
    mesg_t msg;
    memset(&msg, 0, sizeof(mesg_t));
    if(simpio->line_ready){
      int n;
      // Determin if %last n is typed by the user
      char cmd[6];
      const char *usr_arg = &simpio->buf[6];
      n = atoi(usr_arg);
      strncpy(cmd, simpio->buf, 6);
      if(strcmp(cmd, "%last ") == 0) {
        show_last_n(n);
      }
      else {
        iprintf(simpio, "");
        msg.kind = BL_MESG;
        strncpy(msg.name, client_actual.name, MAXNAME);
        strncpy(msg.body, simpio->buf, MAXLINE);
      }
    }
    else if(simpio->end_of_input) {
      strncpy(msg.name, client_actual.name, MAXNAME);
      msg.kind = BL_DEPARTED;
    }
    write(client_actual.to_server_fd, &msg, sizeof(mesg_t));
  }
  pthread_cancel(background_thread); // kill the background thread
  return NULL;
}

void *background_worker(void *arg){
  int done = 0;
  while(!done) {
    mesg_t msg;
    memset(&msg, 0, sizeof(mesg_t));
    read(client_actual.to_client_fd, &msg, sizeof(mesg_t));
    switch(msg.kind) {
      case BL_MESG:
        iprintf(simpio, "[%s] : %s\n", msg.name, msg.body);
        break;
      case BL_JOINED:
        iprintf(simpio, "-- %s JOINED --\n", msg.name);
        break;
      case BL_DEPARTED:
        iprintf(simpio, "-- %s DEPARTED --\n", msg.name);
        break;
      case BL_SHUTDOWN:
        iprintf(simpio, "!!! server is shutting down !!!\n");
        pthread_cancel(user_thread);
        done = 1;
        break;
      case BL_PING:
        write(client_actual.to_server_fd, &msg, sizeof(mesg_t));
        break;
      case BL_DISCONNECTED:
        iprintf(simpio, "-- %s DISCONNECTED --\n", msg.name);
        break;
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  //signal(SIGTERM, sigint_handler);

	if(argc < 3) {
		printf("usage: %s <server_name> <user_name>\n", argv[0]);
		exit(1);
	}
  strcpy(server_name, argv[1]);

  pid_t pid = getpid();

	// construct a join_t structure ready to be sent to server fifo
	join_t join;
  memset(&join, 0, sizeof(join_t));
	char join_fname[MAXPATH];

  strcpy(client_actual.name, argv[2]);
  sprintf(join_fname, "%s.fifo", argv[1]);
	// Sets to_client_actual_fname to the following format
	// pid.client.fifoint signalled = 0;

  sprintf(client_actual.to_client_fname, "%d_client.fifo", pid);

	// Sets to_server_fname to the following format:
	// pid.server.fifo
  sprintf(client_actual.to_server_fname, "%d_server.fifo", pid);

	// Sets the fields of the outgoing join_t structure
	strcpy(join.name, client_actual.name);
	strcpy(join.to_client_fname, client_actual.to_client_fname);
	strcpy(join.to_server_fname, client_actual.to_server_fname);

  // remove existing fifos
  remove(client_actual.to_client_fname);
  remove(client_actual.to_server_fname);

  // make new fifos
  mkfifo(client_actual.to_client_fname, DEFAULT_PERMS);
  mkfifo(client_actual.to_server_fname, DEFAULT_PERMS);

  // Open the two fifos that are created
  client_actual.to_client_fd = open(client_actual.to_client_fname, O_RDWR);
  client_actual.to_server_fd = open(client_actual.to_server_fname, O_RDWR);
	int join_fd = open(join_fname, O_RDWR);

	// Write the join_t structure to <server_name>.fifo
	int check = write(join_fd, &join, sizeof(join_t));
  if(check == -1) {
    perror("ERRORNO indicates: ");
    exit(1);
  }

	// ***************************************************
  char prompt[MAXNAME];
  snprintf(prompt, MAXNAME, "%s>> ", argv[2]); // create a prompt string
  simpio_set_prompt(simpio, prompt);         // set the prompt
  simpio_reset(simpio);                      // initialize io
  simpio_noncanonical_terminal_mode();       // set the terminal into a compatible mode
  pthread_create(&background_thread, NULL, background_worker, NULL);
  pthread_create(&user_thread,   NULL, user_worker,   NULL);     // start user thread to read input

  pthread_join(user_thread, NULL);
  pthread_join(background_thread, NULL);

  simpio_reset_terminal_mode();
  printf("\n");                 // newline just to make returning to the terminal prettier

	// ***************************************************
	return 0;
}
