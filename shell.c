#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include "tokenizer.h"
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>


typedef struct err_map_node{/*a struct to represent an error number and its meaning*/
    int number;
    char* desc;
}err_map_node_t;
/*a list of errors returned by cd and their corresponding meaning*/
err_map_node_t errs_map[]= {
        {EACCES,"Search permission is denied for one of the components  of  path."},
        {EFAULT,"path points outside your accessible address space."},
        {EIO    ,"An I/O error occurred."},
        {ELOOP ,"Too many symbolic links were encountered in resolving path."},
        {ENAMETOOLONG,"path is too long."},
        {ENOENT, "The file does not exist."},
        {ENOMEM, "Insufficient kernel memory was available."},
        {ENOTDIR,"A component of path is not a directory."}
};

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_uid(struct tokens *tokens);
int cmd_gid(struct tokens *tokens);
int cmd_groups(struct tokens *tokens);
// int cmd_pwd(struct tokens *tokens);
int cmd_run(struct tokens *tokens);



/* Built-in command functions take token array and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_uid, "uid", "display the current user-id"},
  {cmd_gid, "gid", "display the primary group-id"},
  {cmd_groups, "groups", "display the groups that the user is part of"},
  // {cmd_pwd, "pwd", "print the current working directory"},
  {cmd_run, "$", "to run the executable present in the directory given as an argument"}
};

/*looks up the description of the error number passed as argument*/
char *lookup_errno(int number)
{
    for(unsigned int i = 0;i< sizeof(errs_map)/ sizeof(err_map_node_t);i++)
        if(errs_map[i].number == number)
            return errs_map[i].desc;

    return NULL;
}

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

int cmd_uid(unused struct tokens *tokens) {
  struct passwd *p;
  uid_t  uid;

  if ((p = getpwuid(uid = getuid())) == NULL)
    perror("getpwuid() error");
  else {
    // puts("getpwuid() returned the following info for your userid:");
    printf("%s\n",       p->pw_name);
    // printf("  pw_uid   : %d\n", (int) p->pw_uid);
    // printf("  pw_gid   : %d\n", (int) p->pw_gid);
    // printf("  pw_dir   : %s\n",       p->pw_dir);
    // printf("  pw_shell : %s\n",       p->pw_shell);
  }
  return 1;
}

int cmd_gid(unused struct tokens *tokens){
  __uid_t uid = getuid();//you can change this to be the uid that you want

  struct passwd* pw = getpwuid(uid);
  if(pw == NULL){
      perror("getpwuid error: ");
  }

  int ngroups = 0;

  //this call is just to get the correct ngroups
  getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
  __gid_t groups[ngroups];

  //here we actually get the groups
  getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);

  
  struct group* gr = getgrgid(groups[0]);
  if(gr == NULL){
      perror("getgrgid error: ");
  }
  printf("%s\n",gr->gr_name);

  return 1;
}


int cmd_groups(unused struct tokens *tokens){
  __uid_t uid = getuid();//you can change this to be the uid that you want

  struct passwd* pw = getpwuid(uid);
  if(pw == NULL){
      perror("getpwuid error: ");
  }

  int ngroups = 0;

  //this call is just to get the correct ngroups
  getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
  __gid_t groups[ngroups];

  //here we actually get the groups
  getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);


  //example to print the groups name
  for (int i = 0; i < ngroups; i++){
      struct group* gr = getgrgid(groups[i]);
      if(gr == NULL){
          perror("getgrgid error: ");
      }
      printf("%s\n",gr->gr_name);
  }
  return 1;
}

// int cmd_pwd(unused struct tokens *tokens){
//     char cwd[1024];
//     getcwd(cwd, sizeof(cwd));
//     // for(int i=0; i<1024; i++){
//     //   fprintf(stdout, "%c\n",cwd[i]);
//     // }
//     fprintf(stdout, "%s\n",cwd);
//     return 1;
// }

/*sets the current directory*/
int cmd_run(struct tokens *tokens){

    // int result = chdir(tokens_get_token(tokens, 1));
    // if(result == 0){
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        char * argv_list[] = {tokens_get_token(tokens, 1),NULL};
        pid_t pid = fork();
        if (pid == 0) {
        // child process
          // setpgid(pid,pid);
          signal(SIGINT, SIG_DFL);
          int file_desc = open("out.txt",O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
          printf("Hello\n");
          // here the newfd is the file descriptor of stdout (i.e. 1)
          dup2(file_desc, 1) ;
          execv(argv_list[0],argv_list);
        } else {
        // the parent waits for the child
          setpgid(0, 0);
          tcsetpgrp(0, 0);
          wait(NULL);
        }
        return 1;
    // }
    // else {
    //     if(lookup_errno(errno))
    //         fprintf(stdout, "%s\n",lookup_errno(errno));
    //     else
    //         fprintf(stdout,"an unknown error occurred");
    //     return 0;
    // }
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  signal(SIGINT, SIG_IGN);
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is notint cmd_uid(struct tokens *tokens); a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      fprintf(stdout, "This shell doesn't know how to run programs.\n");
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
