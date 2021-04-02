#include "sh61.hh"
#include <cstring>
#include <cerrno>
#include <vector>
#include <sys/stat.h>
#include <sys/wait.h>


// struct command
//    Data structure describing a command. Add your own stuff.

struct command {
    std::vector<std::string> args;
    pid_t pid;      // process ID running this command, -1 if none
    int oper = TYPE_NORMAL; //default oper of the command
    int cond_oper = TYPE_NORMAL; //defaul oper of the overarching conditional
    command();
    ~command();
    command *next = nullptr;  //Next command or null if nothing next
    command *prev = nullptr;  //Prev command or null if this is the first command
    bool in_background = false; //is the conditional in the background
    int fail = 0;  //If exit failed to happen properly
    int read_end_prev_pipe = -1; //needed to close ends of already open pipe
    std::string input; // < filename 
    std::string output; // > filename
    std::string error;  // 2> filename
    pid_t make_child(pid_t pgid);
};


// command::command()
//    This constructor function initializes a `command` structure. You may
//    add stuff to it as you grow the command structure.

command::command() {
    //set defaults when new command created
    this->pid = -1;
    this->oper = TYPE_NORMAL;
    this->cond_oper = TYPE_NORMAL;
    this->next = nullptr;
    this->prev = nullptr;
    this->in_background = false;
    this->fail = 0;
    this->read_end_prev_pipe = -1;
    this->input = "";
    this->output = "";
    this->error = "";
}


// command::~command()
//    This destructor function is called to delete a command.

command::~command() {
}


// COMMAND EXECUTION

// command::make_child(pgid)
//    Create a single child process running the command in `this`.
//    Sets `this->pid` to the pid of the child process and returns `this->pid`.
//
//    PART 1: Fork a child process and run the command using `execvp`.
//       This will require creating an array of `char*` arguments using
//       `this->args[N].c_str()`.
//    PART 5: Set up a pipeline if appropriate. This may require creating a
//       new pipe (`pipe` system call), and/or replacing the child process's
//       standard input/output with parts of the pipe (`dup2` and `close`).
//       Draw pictures!
//    PART 7: Handle redirections.
//    PART 8: The child process should be in the process group `pgid`, or
//       its own process group (if `pgid == 0`). To avoid race conditions,
//       this will require TWO calls to `setpgid`.

pid_t command::make_child(pid_t pgid) {
    assert(this->args.size() > 0);
    (void) pgid; 
    // You won’t need `pgid` until part 8.
    // Your code here!

    int r;
    //create array for pipe and if the operator for the command is pipe, create new pipe
    int pfd[2] = {-1,-1};
    if (this->oper == TYPE_PIPE){
        r = pipe(pfd);
    }

    pid_t p1 = fork();
    assert(p1 >= 0);

    //in the forked child
    if (p1 ==0){
        //check for previous pipe. If there is, make it the input and close the previous pipe
        if (this->read_end_prev_pipe >= 0){
            dup2(this->read_end_prev_pipe, STDIN_FILENO);
            close(this->read_end_prev_pipe);
            
         }
        
        //check for new pipe, write standard output to it. Then close both ends of pipe for this child
        if (pfd[0] >= 0){
            dup2(pfd[1],STDOUT_FILENO);
            close(pfd[1]);
            close(pfd[0]);
        }

        //Checks if this command is taking input from a file. Open the file and make it the input
        int g;
        if (this->input.size() > 0){
            g = open(this->input.c_str(), O_RDONLY, S_IRUSR);
            if (g  == -1){
                fprintf(stderr, "%s\n", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            dup2(g, STDIN_FILENO);
            close(g);
        }

        //Check if this command's output is being sent to a file. Open it and make it the output
        if (this->output.size() > 0){
            g = open(this->output.c_str(), O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
            if (g == -1){
                fprintf(stderr, "%s\n", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            dup2(g,STDOUT_FILENO);
            close(g);
        }

        //Check if this file's error should be sent to a file. Open it and direct errors to it. 
        if (this->error.size() > 0){
            g = open(this->error.c_str(), O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
            if (g == -1){
                fprintf(stderr, "%s\n", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            dup2(g, STDERR_FILENO);
            close(g);
        }


        //Put this commands arguments into an array
    	int a = this->args.size();
    	char *my_args[a+1];
    	for (int i = 0; i < a; i++){
    		my_args[i] = (char*) this->args[i].c_str();	
    	}
        //exec this file with my array of arguments. execvp will return -1 if there's an error
    	my_args[a] = nullptr;
    	int e = execvp(my_args[0], my_args);
    	if (e == -1) {
    		printf("ERROR EXECVP \n");
    	}
    }
    
    else if (p1 == -1){
    	_exit(1);
    }

    //Closes unnecessary open end of pipes in the parent.
    this->pid = p1;
    if (this->read_end_prev_pipe >= 0){
        close(this->read_end_prev_pipe);
    }
    if (pfd[0] >= 0){
        close(pfd[1]);
        this->next->read_end_prev_pipe = pfd[0];
    }

    //fprintf(stderr, "command::make_child not done yet\n");
    return this->pid;
}


// run(c)
//    Run the command *list* starting at `c`. Initially this just calls
//    `make_child` and `waitpid`; you’ll extend it to handle command lists,
//    conditionals, and pipelines.
//
//    PART 1: Start the single command `c` with `c->make_child(0)`,
//        and wait for it to finish using `waitpid`.
//    The remaining parts may require that you change `struct command`
//    (e.g., to track whether a command is in the background)
//    and write code in `run` (or in helper functions).
//    PART 2: Treat background commands differently.
//    PART 3: Introduce a loop to run all commands in the list.
//    PART 4: Change the loop to handle conditionals.
//    PART 5: Change the loop to handle pipelines. Start all processes in
//       the pipeline in parallel. The status of a pipeline is the status of
//       its LAST command.
//    PART 8: - Choose a process group for each pipeline and pass it to
//         `make_child`.
//       - Call `claim_foreground(pgid)` before waiting for the pipeline.
//       - Call `claim_foreground(0)` once the pipeline is complete.


command* run_conditional(command* c){
    pid_t a;
    pid_t p;
    int status;
    bool last = false;

    //Loop runs until we hit "last" which is the end of a conditional
    while (last == false){
        if (c->prev == nullptr || (c->prev->cond_oper == TYPE_AND && c->prev->fail == 0) || (c->prev->cond_oper == TYPE_OR && c->prev->fail != 0) || c->prev->cond_oper == TYPE_BACKGROUND || c->prev->cond_oper == TYPE_SEQUENCE){
            if (c->oper != TYPE_PIPE){
                a = c->make_child(0);
                p  = waitpid(a, &status, 0);
            }

            //loop for pipes in case of multiple pipes in a row
            else if (c->oper == TYPE_PIPE){
                while (c->oper == TYPE_PIPE){
                    a = c->make_child(0);
                    c = c->next;
                }
                a = c->make_child(0);
                p = waitpid(a, &status, 0);
            }

            //Set fail based on status from waitpid
            if (WIFEXITED(status)){
                c->fail = WEXITSTATUS(status);
            }
        }
        //Checks the overaching conditional operator for the whole conditional. Sets fail accordingly if type AND or OR based on fail status
        else if ((c->prev->cond_oper == TYPE_AND && c->prev->fail != 0) || (c->prev->cond_oper == TYPE_OR && c->prev->fail == 0)) {
                c->fail = c->prev->fail;

        }

        //one of these operators tells us we're at the end of the conditional
        if (c->oper == TYPE_BACKGROUND || c->oper == TYPE_SEQUENCE || c->oper == TYPE_NORMAL) {
            last = true;
        }
        
        c = c->next;
    }
    //Returns immediate next command after conditional
    return c;        
}


void run(command* c) {

    //run until we hit nullptr which is end of the entire line 
    while (c != nullptr) {
        
        //If in background, create a child shell and run the conditional in the child, then close child
        if (c->in_background == true){
            pid_t new_pid = fork();
            if (new_pid == 0){
                c = run_conditional(c);
                _exit(0);
            }
            //In the parent, just skip to the next conditional
            if (new_pid != 0){
                while (c->cond_oper == TYPE_AND || c->cond_oper == TYPE_OR){
                    c = c->next;
                }
                c = c->next;
            }
        }

        //If not in background, just run the conditional in the foreground on the main shell
        else {
            c = run_conditional(c);
        }    
    }
}


// parse_line(s)
//    Parse the command list in `s` and return it. Returns `nullptr` if
//    `s` is empty (only spaces). You’ll extend it to handle more token
//    types.

command* parse_line(const char* s) {
    shell_parser parser(s);
    // Your code here!

    // Build the command
    // The handout code treats every token as a normal command word.
    // You'll add code to handle operators.

    //Head will be the first command. c will be what we iterate through
    command* c = nullptr;
    command* head = nullptr;

    //Iterate through entire line 
    for (shell_token_iterator it = parser.begin(); it != parser.end(); ++it) {

        if (!c) {
        	c = new command;
        	head = c;
        } 
        
        //Set operator and cond_oper to the operator if we get one of these operators, then create a new command for anything afgerwards
        if (it.type() == TYPE_BACKGROUND || it.type() == TYPE_SEQUENCE || it.type() == TYPE_AND || it.type() == TYPE_OR || it.type() == TYPE_PIPE){
        	c->oper = it.type();
            c->cond_oper = it.type();

        	command* new_c = new command;
        	c->next = new_c;
        	new_c->prev = c;
        	c = new_c;
            
        }

        //For normal type, put into the args, don't make a new command
        else if (it.type() == TYPE_NORMAL){
            c->args.push_back(it.str());
        }
         
        //Redirects. Fill in the error/input/output accordingly with what is immediately after the operator
        else if (it.type() == TYPE_REDIRECT_OP){

            if (it.str() == "2>"){
                ++it;
                c->error = it.str();
            }
            if (it.str() == "<"){
                ++it;
                c->input = it.str();
            }
            if (it.str() == ">"){
                ++it;
                c->output = it.str();
            }

        }
        
    }
    
    //this will happen if nothing entered by user
    if (c == nullptr){
        return head;
    }

    if (c->args.size() == 0) {
    	c->prev->next = nullptr;
        c = c->prev;
    }

    //Reverses back from the very end until we hit the beginning. bg (background) sent to false at first.
    bool bg = false;
    while (c != nullptr){
        //change bg to true, if we see a background operator
        if (c->oper == TYPE_BACKGROUND){
            bg = true;
        }
        //change bg back to false if we see a type sequence
        if (c->oper == TYPE_SEQUENCE){
            bg = false;
        }
        //Pipe cond_oper should be equal to the cond_oper of the first non-pipe following the pipe
        if (c->oper == TYPE_PIPE){
            c->cond_oper = c->next->cond_oper;
        }
        //set background to bg for each command
        c->in_background = bg;
        c = c->prev;

    }

    //Return first command
    return head;
}


int main(int argc, char* argv[]) {
    FILE* command_file = stdin;
    bool quiet = false;

    // Check for '-q' option: be quiet (print no prompts)
    if (argc > 1 && strcmp(argv[1], "-q") == 0) {
        quiet = true;
        --argc, ++argv;
    }

    // Check for filename option: read commands from file
    if (argc > 1) {
        command_file = fopen(argv[1], "rb");
        if (!command_file) {
            perror(argv[1]);
            exit(1);
        }
    }

    // - Put the shell into the foreground
    // - Ignore the SIGTTOU signal, which is sent when the shell is put back
    //   into the foreground
    claim_foreground(0);
    set_signal_handler(SIGTTOU, SIG_IGN);

    char buf[BUFSIZ];
    int bufpos = 0;
    bool needprompt = true;

    while (!feof(command_file)) {
        // Print the prompt at the beginning of the line
        if (needprompt && !quiet) {
            printf("sh61[%d]$ ", getpid());
            fflush(stdout);
            needprompt = false;
        }

        // Read a string, checking for error or EOF
        if (fgets(&buf[bufpos], BUFSIZ - bufpos, command_file) == nullptr) {
            if (ferror(command_file) && errno == EINTR) {
                // ignore EINTR errors
                clearerr(command_file);
                buf[bufpos] = 0;
            } else {
                if (ferror(command_file)) {
                    perror("sh61");
                }
                break;
            }
        }

        // If a complete command line has been provided, run it
        bufpos = strlen(buf);
        if (bufpos == BUFSIZ - 1 || (bufpos > 0 && buf[bufpos - 1] == '\n')) {
            if (command* c = parse_line(buf)) {
                run(c);
                delete c;
            }
            bufpos = 0;
            needprompt = 1;
        }

        // Handle zombie processes and/or interrupt requests
        // Your code here!
        
        int ret = 1;
        int status;

        //Close zombie processes
        while (ret > 0) {
            ret = waitpid(-1, &status, WNOHANG);    
        }
        
    }

    return 0;
}
