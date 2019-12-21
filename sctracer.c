#define MIN_ARGS 2

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <string.h>

int do_trace(pid_t child);
int wait_for_syscall(pid_t child);
void split(char *sentence, char *delim, char **result);
struct syscall_node *newNode(int syscall_num);
struct syscall_node* insert(struct syscall_node* node, int num);
void printNodes(struct syscall_node *root);
void freeNodes(struct syscall_node *root);

struct syscall_node
{
    int num;
    int count;
    struct syscall_node *left, *right;
};

struct syscall_node *root = NULL;
FILE *out_file = NULL;

int main(int argc, char **argv)
{
  // takes values from argv based on the format:
  // ./sctracer "./program..." filename
  //  argv[0]    argv[1]        argv[2]
  char *program, *outputfile;
  if (argc > MIN_ARGS) {
    program = strdup(argv[1]);
    outputfile = strdup(argv[MIN_ARGS]);
  } else {
		fprintf(stderr, "Improper usage.\n");
		return 0;
	}

  out_file = fopen(outputfile, "w");
  if (out_file == NULL) printf("Error opening file: %s", outputfile);

  // process arguments in char *program
  char *args[strlen(program)];
  split(program, " ", args);

  pid_t child;
  if ((child = fork()) == 0) {
    ptrace(PTRACE_TRACEME);
    execvp(args[0], args);
    fprintf(stderr, "Failed to execute program: %s", args[0]);
  } else {
    do_trace(child);
    printNodes(root);
    freeNodes(root);
  }

  return 0;
}

// what the parent process does to trace the child process
int do_trace(pid_t child)
{
  int status, syscall, retval;
  waitpid(child, &status, 0);
  ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);

  while(1) {
    // get initial syscall
    if (wait_for_syscall(child) != 0) break;
    syscall = ptrace(PTRACE_PEEKUSER, child, sizeof(long)*ORIG_RAX);
    root = insert(root, syscall);

    // get return from syscall
    if (wait_for_syscall(child) != 0) break;
    retval = ptrace(PTRACE_PEEKUSER, child, sizeof(long)*RAX);
    // retval is being ignored so that each syscall is not counted twice
  }
  return 0;
}

// helper process to wait for a syscall from the child
// or to cause the parent to exit if the child process exits
int wait_for_syscall(pid_t child)
{
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);
        // 0x80 : uses bit 7 as required by PTRACE_O_TRACESYSGOOD
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        if (WIFEXITED(status))
            return 1;
    }
}

// divides a sentence string into an array of strings using delimiter delim
// #sentence = result[0] o delim o result[1] o delim o result[2] o ...
// (multiple consecutive instances of delim will be treated as one instance)
void split(char *sentence, char *delim, char **result)
{
  char *token = strtok(sentence, delim);
  int index = 0;
  while (token != NULL) {
    result[index] = token;
    token = strtok(NULL, delim);
    index++;
  } result[index] = NULL;
}

// Create a new node and return it
struct syscall_node *newNode(int syscall_num)
{
  struct syscall_node *temp =  (struct syscall_node *)malloc(sizeof(struct syscall_node));
  temp->num = syscall_num;
  temp->count = 1;
  temp->left = temp->right = NULL;
  return temp;
}

// Recursive function to insert a new node at a location based on num
struct syscall_node* insert(struct syscall_node* root, int num)
{
  // Base case: If root has not been initialized yet, return a new node
  if (root == NULL) return newNode(num);

  // Else, recursively search the tree for root->num = num
  if (num < root->num)
    root->left  = insert(root->left, num);
  else if (num > root->num)
    root->right = insert(root->right, num);
  else if (num == root->num)
    root->count++;

  /* return the (unchanged) root pointer */
  return root;
}

// Print all nodes in order
void printNodes(struct syscall_node *root)
{
  if (root != NULL)
  {
    printNodes(root->left);
    fprintf(out_file, "%d\t%d\n", root->num, root->count);
    printNodes(root->right);
  }
}

// Free all nodes
void freeNodes(struct syscall_node *root)
{
  if (root != NULL)
  {
    freeNodes(root->left);
    free(root);
    printNodes(root->right);
  }
}
