Clarifications:

Two people contributed to this project;
Person 1 is Xingzhe Xin (Me)
Person 2 is Yana Garipova

I am the only person submitting this assignment to avoid duplicate submission.
Both people (I and Yana) have agreed that I submit this assignment.

On submission, the Code has passed all 20 normal tests and 17 valgrind
tests. Those tests have been run extensively on two local machines and one
CSE Lab machine (Machine name: csel-kh4250-03.cselabs.umn.edu) with default
tick times listed as follows:

ticktime_norm=0.05
ticktime_valg=0.5



Advanced Features:
We have implemented 3 out of 4 advanced features within our code.

1. Server Ping and Disconnected Clients
  We created a background thread in bl-server.c to sleep(1) and wake up
  to advance server time_sec, ping all existing clients and remove
  disconnected clients. The disconnect message is broadcast to all existing
  clients. Tested extensively and functions properly.

2. Binary Server Log File
  All messages apart from ping messages are logged into a file called
  <server_name>.log. This file is not removed when server_shutdown is called.
  All additional messages are appended to the end of the file using the
  call to lseek with SEEK_END as argument. Tested extensively and functions
  properly.

3. Last Messages from clients
  Clients can type %last <some_number> to display the last n messages in the
  log file. If the number is greater than the number of existing messages, all
  existing messages within the log file is displayed.

Thank you.
