/* Copyright (C) 1987-2002 Free Software Foundation, Inc.
   This file is part of the GNU Readline Library, a library for
   reading lines of text with interactive input and history editing.
   The GNU Readline Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.
   The GNU Readline Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   59 Temple Place, Suite 330, Boston, MA 02111 USA. */

/* fileman.c -- A tiny application which demonstrates how to use the
   GNU Readline library.  This application interactively allows users
   to manipulate files and their modes. */

/* cli.c replaces most of the commands in fileman.c with commands for
   a dialog with sysrepod, a local agent, or a tool.
   cli.c was derived by Mark Baugher in May 2015 for the sysrepo project.

   2015-05: Use parsable lineptr in addition to line.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <srd.h>

extern char **completion_matches PARAMS((char *, rl_compentry_func_t *));

/* The names of functions that actually do the manipulation. */
int com_send(), com_xpath(), com_xslt();
int com_connect(), com_datastore(), com_disconnect();
int com_help(), com_quit();

/* A structure which contains information on the commands this program
   can understand. */
typedef int Function2 ();
typedef void VFunction ();
typedef char *CPFunction (); 
typedef char **CPPFunction2 ();

typedef struct {
  char *name;    /* User printable name of the function. */
  Function2 *func;/* Function to call to do the job. */
  char *doc;     /* Documentation for this function.  */
} COMMAND;

COMMAND commands[] = {
  { "connect", com_connect, "Connect to server IPV4-ADDRESS PORT-NUMBER" },
  { "disconnect", com_disconnect, "Disconnect from sysrepod server" },
  { "open", com_datastore, "Use the datastore named NAME" },
  { "send", com_send, "Send SRD string to the connected server" },
  { "xslt", com_xslt, "Apply string xls FILE-NAME to the open datastore" },
  { "xpath", com_xpath, "Apply string XPATH to the the open datastore" },
  { "help", com_help, "Display this text" },
  { "?", com_help, "Synonym for `help'" },
  { "quit",com_quit,"Quit the sysrepo CLI" },
  { (char *)NULL, (Function2 *)NULL, (char *)NULL }
};

/* Forward declarations. */
char *stripwhite ();
COMMAND *find_command ();
void initialize_readline ();
int execute_line (char *);
static void handle_signal(int sigid, siginfo_t *siginfo, void *context);

/* The name of this program, as taken from argv[0]. */
char *progname;

/* When non-zero, this global means the user is done using this program. */
int done;

int main (int argc, char **argv)
{
  extern int sockfd;
  char *line, *s;
  struct sigaction sig;
  memset(&sig,'\0',sizeof(sig));
 
  /* Use the sa_sigaction field because the handles has two additional parameters */
  sig.sa_sigaction = &handle_signal; 
  /* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
  sig.sa_flags = SA_SIGINFO; 
  if (sigaction(SIGPIPE, &sig, NULL) < 0) {
    fprintf (stderr,"SIGPIPE sigaction failure");
    return 1;
  }
  if (sigaction(SIGINT, &sig, NULL) < 0) {
    fprintf (stderr,"SIGINT sigaction failure");
    return 1;
  }

  progname = argv[0];

  initialize_readline ();/* Bind our completer. */

  char *lineptr = NULL;
  /* Loop reading and executing lines until the user quits. */
  for ( ; done == 0; )
    {
      if (!lineptr)
	{
	  if (!(line = readline("sysrepo> "))) 
	    break;
	  lineptr = strtok(line, ";");
	}

      printf("line is %s\n",line);

      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = stripwhite (lineptr);

      if (*s)
        {
          add_history (s);
          execute_line (s);
        }
      lineptr = strtok(NULL,";");
      if(!lineptr)
	free (line);
    }
  close(sockfd);
  exit (0);
}

/* Execute a command line. */
int execute_line (line)
     char *line;
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = find_command (word);

  if (!command)
    {
      fprintf (stderr, "%s: No such command for sysrepo cli.\n", word);
      return (-1);
    }

  /* Get argument to command, if any. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
COMMAND *
find_command (name)
     char *name;
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *)NULL);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *
stripwhite (string)
     char *string;
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;
    
  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

char *command_generator ();
char **fileman_completion ();

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void initialize_readline ()
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "FileMan";

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = (CPPFunction2 *)fileman_completion;
}

/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char **
fileman_completion (text, start, end)
     char *text;
     int start, end;
{
  char **matches;

  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = completion_matches (text, command_generator);

  return (matches);
}

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator (text, state)
     char *text;
     int state;
{
  static int list_index, len;
  char *name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the command list. */
  while ((name = commands[list_index].name))
    {
      list_index++;

      if (strncmp (name, text, len) == 0)
	{
	  char *s;
	  if (!(s = malloc(sizeof(name))))
	    {
	      fprintf(stderr, "Out of memory for name\n");
	      return NULL;
	    }
	  strcpy(s, name);
	  return (s);
	}
    }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

enum cstate {
  disconnected,
  connected,
  repopen,
};
char serverIPv4[15] = "127.0.0.1";
int  serverPort = SRD_DEFAULTSERVERPORT;
char dataStoreName[SRD_DEFAULT_NAMESIZE] = "running";
int sockfd;
enum cstate state = disconnected;

/* Handle Signals */
static void handle_signal(int sigid, siginfo_t *siginfo, void *context)
{
  fprintf (stderr, "Caught SIGNAL: %d PID: %ld, UID: %ld\n",sigid,
	    (long)siginfo->si_pid, (long)siginfo->si_uid);
  switch (sigid) {
  case SIGPIPE :
    state = disconnected;
    break;
  case SIGINT :
    done = 1;
    break;
  default :
    break;
  }
}

/* Connect to server */
int com_connect (char *arg)
{
  char *s;

  if (state != disconnected) 
    {
      printf("Already connected, please disconnect first\n");
      return 1;
    }
  if (*arg) 
    {
      s = strtok(arg," ");
      if ((strstr( s, ".") == NULL)  
	  && (strspn(s,"0123456789") == strlen(s)))
	{  //This is "send port" or "send port IPaddress" case
	  sscanf(s,"%d",&serverPort);
	  s = strtok(NULL," ");
	  if (s && (strspn(s,"0123456789.") == strlen(s)))
	    { //This is the "send port IPaddress" case
	      strcpy(serverIPv4, s);
	    }
	  else if (s) 
	    {
	      fprintf(stderr,"Invalid address parameter for connect command\n");
	      return 1;
	    }
	}
      else if (strspn(s,"0123456789.") == strlen(s))
	{ //This is the "connect IP-address" or "connect IPaddress Port case"
	  strcpy(serverIPv4,s);
	  s = strtok(NULL, " ");
	  if (s && (strspn(s,"0123456789") == strlen(s)))
	    { //This is the "connect IP-address Port" case
	      sscanf(s,"%d",&serverPort);
	    }
	  else if (s)
	    {
	      fprintf(stderr,"Invalid port parameter for connect command\n");
	      return 1;
	    }
	}
    }
  printf("ipv4 is %s and port is %d\n", serverIPv4, serverPort);

  if (!srd_connect (serverIPv4, serverPort, &sockfd))
    {
    fprintf (stderr, "Error in connecting to server %s at port %d\n", 
	             serverIPv4, 
                     serverPort);
    return 1;
    }
  state = connected;
  return 0;
}

int com_disconnect (char *arg)
{
  if (state == disconnected)
    {
      fprintf(stderr,"There is no connection to sysrepo\n");
      return 1;
    }
  srd_disconnect (sockfd);
  state = disconnected;
  return 0;
}
int com_datastore (arg)
     char *arg;
{
  if (state == disconnected)
    {
      fprintf(stderr, "Please connect to a sysrepo server\n");
      return 1;
    }
  if (!(srd_setDataStore(sockfd, arg)))
    {
      fprintf(stderr, "Failed to access datastore:%s\n", arg);
      return 1;
    }
  memset(dataStoreName, 0, sizeof(dataStoreName));
  strcpy(dataStoreName,arg);
  state = repopen;
  return 0;
}
int com_send (char *arg)
{
  char *buffer;
  int buffsize;

  if (state == disconnected)
    {
      fprintf(stderr, "No server is connected\n");
      return 1;
    }
  if (!(srd_sendServer(sockfd, arg, strlen(arg))))
    {
      fprintf(stderr,"Failed to send message to server\n");
      return 1;
    }
  if (!(buffer = malloc(BUFSIZ)))
    {
      fprintf(stderr,"Failed to allocate memory for server's message\n");
      return 1;
    }
  buffsize = BUFSIZ;
  if (!(srd_recvServer(sockfd, &buffer, &buffsize)))
    {
      fprintf(stderr,"Failed to receive message from server\n");
      return 1;
    }
  printf("%s\n", buffer);
  return 0;
}

int com_xslt (char *arg)
{
  char xslt[BUFSIZ];
  FILE *filep;
  char *p;
  unsigned n, j;

  if (state !=repopen)
    {
      fprintf(stderr, "No datastore is open\n");
      return 1;
    }
  if (!(filep = fopen(arg, "r")))
    {
      fprintf(stderr, "Cannot open file: %s\n",arg);
      return 1;
    }
  strcpy(xslt, "<![CDATA[");
  j = strlen(xslt);
  p = xslt + j;
  *p = '\0';
  if (!(n = fread(p, 1, BUFSIZ-j, filep)))
    {
      fprintf(stderr, "File is empty or an error occurred:  errno is %d\n",errno);
      return 1;
    }
  p +=  n;
  strcpy(p,"]]>");
  srd_applyXSLT (sockfd, xslt, &p);
  free(p);
  return 0;
}
int com_xpath (char *arg)
{
  char *p;
  char *err = strchr(arg,'"');
  if (err)
    {
      fprintf(stderr,"XPATH string contains a quote\n");
      return 1;
    }
  srd_applyXPath(sockfd, arg, &p);
  if (!p)
    {
      fprintf(stderr, "XPATH result is NULL\n");
      return 1;
    }
  printf("%s\n", p);
  return 0;
}
/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int com_help (arg)
   char *arg;
 {
   register int i;
   int printed = 0;

   for (i = 0; commands[i].name; i++)
     {
       if (!*arg || (strcmp (arg, commands[i].name) == 0))
	 {
	   if (strlen(commands[i].name) < 8)
	     printf ("%s\t\t%s.\n", commands[i].name, commands[i].doc);
	   else printf ("%s\t%s.\n", commands[i].name, commands[i].doc);
	   printed++;
	 }
     }

   if (!printed)
     {
       printf ("No commands match `%s'.  Possibilties are:\n", arg);

       for (i = 0; commands[i].name; i++)
	 {
	   /* Print in six columns. */
	   if (printed == 6)
	     {
	       printed = 0;
	       printf ("\n");
	     }

	   printf ("%s\t", commands[i].name);
	   printed++;
	 }

       if (printed)
	 printf ("\n");
     }
   return (0);
 }
/* The user wishes to quit using this program.  Just set DONE non-zero. */
int com_quit (char *arg)
{
  done = 1;
  return (0);
}
