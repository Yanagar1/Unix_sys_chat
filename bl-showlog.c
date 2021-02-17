#include "blather.h"

int main(int argc, char const *argv[]) {
  if (argc < 2){
    printf("usage: %s <name>\n",argv[0]);
    exit(1);
  }
  //
  int in_fd = open(argv[1], O_RDONLY);
  FILE *out_file = stdout;
  int count = 0;
  who_t who;
  int nbytes = 0;

  // First, read the who_t structure
  int nread = read(in_fd, &who, sizeof(who_t));
  nbytes = nread;
  if(nread == -1) {
    perror("ERRNO INDICATES");
    exit(1);
  }

  fprintf(out_file, "%d CLIENTS\n", who.n_clients);
  for(int i = 0; i < who.n_clients; i++) {
    fprintf(out_file, "%d: %s\n", i, who.names[i]);
  }

  fprintf(out_file, "MESSAGES\n");

  while(1)
  {
    // fprintf(out_file, "Here in while loop displaying MSG\n");
    mesg_t msg;
    memset(&msg, 0, sizeof(mesg_t));
    nread = read(in_fd, &msg, sizeof(mesg_t));
    //nbytes += nread;
    if(nread == 0) {
      break;
    }
    switch(msg.kind) {
      case BL_MESG:
        fprintf(out_file, "[%s] : %s\n", msg.name, msg.body);
        break;
      case BL_JOINED:
        fprintf(out_file, "-- %s JOINED --\n", msg.name);
        break;
      case BL_DEPARTED:
        fprintf(out_file, "-- %s DEPARTED --\n", msg.name);
        break;
      case BL_SHUTDOWN:
        fprintf(out_file, "!!! server is shutting down !!!\n");
        break;
      case BL_PING:
        break;
      case BL_DISCONNECTED:
        fprintf(out_file, "-- %s DISCONNECTED --\n", msg.name);
        break;
    }
  }
  // int nread = pread(log_fd, &who, sizeof(who_t), 0);
  // //int nread = pread(log_fd, &who, sizeof(who_t), 0);
  // if(nread == -1) {
  //   perror("ERRNO INDICATES");
  //   exit(1);
  // }
  // int nbytes = nread;
  // printf("bytes read = %d\n", nread);
  // printf("%d CLIENTS\n", who.n_clients);
  // // while(nread != 0) {
  // //   nread = pread(log_fd, &msg, sizeof(who_t), nbytes);
  // //   nbytes += nread;
  // //   printf("%d: %s\n", count, msg.name);
  // //   count++;
  // // }
  // //printf("%d CLIENTS\n", who.n_clients);
  close(in_fd);
  return 0;
}
