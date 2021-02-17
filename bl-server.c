#include "blather.h"

int signalled = 0;
int alarmed = 0;
server_t server;

pthread_t ping_thread;
pthread_attr_t attr;
void sigterm_handler(int signum) {
  pthread_cancel(ping_thread);
  server_shutdown(&server);
  sleep(1);
  exit(1);
  return;

}

void sigint_handler(int signum) {
  pthread_cancel(ping_thread);
  server_shutdown(&server);
  sleep(1);
  exit(1);
  return;
}

void *threadproc(void *arg)
{
  pthread_detach(pthread_self());
  while(!signalled)
  {
      sleep(1);
      server_tick(&server);
      server_ping_clients(&server);
      server_remove_disconnected(&server, DISCONNECT_SECS);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  // check inputs
  if (argc < 2){
    printf("usage: %s <name>\n",argv[0]);
    exit(1);
  }

  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigterm_handler);
  server_start(&server, argv[1], DEFAULT_PERMS);
  pthread_create(&ping_thread, NULL, &threadproc, NULL);
  while(!signalled){
    if(signalled) break;
      server_check_sources(&server);
      server_write_who(&server);
      if(server_join_ready(&server)) {
        server_handle_join(&server);
      }

      for (int i=0; i<server.n_clients; i++) {
         if(server_client_ready(&server, i)) {//
           server_handle_client(&server, i);
         }
      }
  }
  pthread_join(&ping_thread, NULL);
  server_shutdown(&server);
  return 0;
}
