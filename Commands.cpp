#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <fstream>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";


#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif
#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));
/*========================================================================*/
/*========================General Help Functions==========================*/
/*========================================================================*/

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}
//return true if a char* is numeric
bool is_numeric(char* input) {
    int i = 0;
    bool return_value = true;
    while (input[i] != '\n') {
        if (!(input[i] >= '0' && input[i] <= '9')) {
            return_value = false;
        }
        i++;
    }
    return return_value;
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        if(args[i]==nullptr){
            std::cerr << "smash error: memory allocation failed" << endl;
        }
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void FreeCmdArray(char ** arg_list,int num_of_args){
    for (int i = 0; i < num_of_args;i++) {
        if (arg_list[i] == NULL) break;
        free(arg_list[i]);
    }
}

char* CopyCmd(const char * cmd_line){
    string cmd_s=string(cmd_line);
    unsigned long size= cmd_s.size();
    char * new_cmd_line=new char[size+1];

    for (unsigned long i = 0; i < size;i++) {
        char p=cmd_line[i];
        new_cmd_line[i]=cmd_line[i];
    }
    new_cmd_line[size]=0;
    return new_cmd_line;
}


int FindIfIO(const char* cmd_line){
    int res=REGULAR;
    string cmd_s=string(cmd_line);
    unsigned long ind=cmd_s.find("|&");;
    if(ind!=string::npos){
        res=PIPEERR;
        return res;
    }
    ind=cmd_s.find('|');
    if(ind!=string::npos){
        res=PIPE;
        return res;
    }
    ind=cmd_s.find(">>");
    if(ind!=string::npos){
        res=APPEND;
        return res;

    }
    ind=cmd_s.find('>');
    if(ind!=string::npos){
        res=OVERRIDE;
        return res;

    }
    return res;
}
string* ParseWithIO(const char* cmd_line,int res){
    string cmd_s=string(cmd_line);
    unsigned long ind=0;
    if(res==OVERRIDE){
        ind=cmd_s.find('>');
    }
    else if(res==APPEND){
        ind=cmd_s.find(">>");
    }
    else if(res==PIPE){
        ind=cmd_s.find('|');
    }
    else if(res==PIPEERR){
        ind=cmd_s.find("|&");
    }
    string* cmd1_s = new string;
    if(ind==0){
        return cmd1_s;
    }
    *cmd1_s=cmd_s.substr(0,ind);
    //const char* new_cmd=cmd1_s->c_str();
    return cmd1_s;
}

string* ParseTimeoutCmd(const char* timeout_cmd){
    string timeout_s=string(timeout_cmd);
    if(_isBackgroundComamnd(timeout_cmd)){
        unsigned long idx = timeout_s.find_last_not_of(WHITESPACE);
        timeout_s=timeout_s.substr(0,idx);
    }
    char* arg_list[COMMAND_MAX_ARGS+1];
    int num_of_args=_parseCommandLine(timeout_cmd,arg_list);
    string time_str=arg_list[1];
    unsigned long time_ind=timeout_s.find(time_str);
    unsigned long length=time_str.length();
    string* command= new string(timeout_s);
    *command=command->substr(time_ind+length,string::npos);
    FreeCmdArray(arg_list,num_of_args);
    return command;
}
bool CheckIfBuiltIn(const char* cmd_line){
    int num_of_args;
    char* arg_list[COMMAND_MAX_ARGS+1];
    num_of_args=_parseCommandLine(cmd_line,arg_list);
    string first_word=string(arg_list[0]);
    if(first_word=="chprompt"||first_word=="showpid"||first_word=="pwd"||
       first_word=="cd"|| first_word=="jobs"||first_word=="kill"||first_word=="fg"||
       first_word=="bg" || first_word=="quit"){
        FreeCmdArray(arg_list,num_of_args);
        return true;
    }
    FreeCmdArray(arg_list,num_of_args);
    return false;


}
/*========================================================================*/
/*========================Command Class Functions=========================*/
/*========================================================================*/


Command::Command():cmd_line(""){
    pid=getpid();
    background=false;
    cmd_str= new string("");
    stop_counter=0;
    is_pipe=false;
    is_external=false;
    jobID=-1;
    is_quit=false;
    is_cp=false;
    is_timeout=false;

};
Command::Command(const char* cmd_line) {
    cmd_str = new string(cmd_line);
    this->cmd_line = cmd_str->c_str();
    pid=getpid();
    background=false;
    stop_counter=0;
    is_pipe=false;
    is_external=false;
    jobID=-1;
    is_quit=false;
    is_cp=false;
    is_timeout=false;
}

Command::~Command(){
    delete cmd_str;
}

void Command::PrepForPipeOrTime(){
    is_pipe=true;
};
/*========================================================================*/
/*===========================External Commands============================*/
/*========================================================================*/

ExternalCommand::ExternalCommand(const char* cmd_line, JobsList* jobs):Command(cmd_line){
    this->jobs = jobs;
    int num_of_args;
    char* arg_list[COMMAND_MAX_ARGS + 1];
    num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
    size_t last_arg_size = strlen(arg_list[num_of_args-1]);
    if(_isBackgroundComamnd(cmd_line)){
        ChangeBackground();
    }
    cmd_without_bck=CopyCmd(GetCmdLine()); //drop the &
    _removeBackgroundSign(cmd_without_bck);
    FreeCmdArray(arg_list,num_of_args);
    is_external=true;
}
ExternalCommand::~ExternalCommand() {
    if(GetBackground()){
        delete cmd_without_bck;
    }
}
void ExternalCommand::execute(){
    int num_of_args;
    char* arg_list[COMMAND_MAX_ARGS + 1];
    num_of_args = _parseCommandLine(GetCmdLine(), arg_list);

    //if empty command was sent
    if (arg_list[0]==NULL){
        FreeCmdArray(arg_list,num_of_args);
        return;
    }
    FreeCmdArray(arg_list,num_of_args);
    pid_t pid = fork();
    if(pid==-1){
        perror("smash error: fork failed");
    }
    if (pid == 0) { //child
        if(!is_pipe && !is_timeout){
            setpgrp();
        }
        int res;
        if(GetBackground()){ //suppose to run in the background
            res=execl("/bin/bash", "/bin/bash", "-c", cmd_without_bck, NULL);
            if(res==-1){
                perror("smash error: execl failed");
                exit(0);
            }

        }
        else{//not in the background
            res=execl("/bin/bash", "/bin/bash", "-c", GetCmdLine(), NULL);
            if(res==-1){
                perror("smash error: execl failed");
                exit(0);
            }
        }

    }
    else {//parent - shell
        ChangePID(pid); //of class external command
        if (GetBackground()) {
            if (!is_timeout) {
                jobs->addJob(this, false);
            }
            return;
        }
        int status;
        if(is_pipe){
            waitpid(pid,&status,0);
        }
        else{
            waitpid(pid,&status,WUNTRACED);
            if(WIFSTOPPED(status)){
                ChangeBackground();
            }
        }

    }
}

/*========================================================================*/
/*===========================Special Commands============================*/
/*========================================================================*/

/*-------------------------Command Pipe----------------------------------*/
PipeCommand::PipeCommand(const char* cmd_line,int type,Command* cmd1,Command* cmd2,JobsList* joblst):
        Command(cmd_line),cmd1(cmd1),cmd2(cmd2),type(type),jobs(joblst){
    if(_isBackgroundComamnd(cmd_line)){
        ChangeBackground();
    }
    is_pipe=true;
}
PipeCommand::~PipeCommand(){
    delete cmd1;
    delete cmd2;
}
Command* PipeCommand::GetCmd1() {
    return cmd1;
}
Command* PipeCommand::GetCmd2(){
    return cmd2;
}
/*
void PipeCommand::execute() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) {
        //First child process
        setpgrp();
        int stdin_copy = 0;
        int stdout_copy = 1;
        int stderr_copy = 2;
        int res1, res2;
        if (type == PIPE) {
            stdin_copy = dup(0);
            stdout_copy = dup(1);
            res1 = close(0); //close stdin
            res2 = close(1); //close stdout
        } else {
            stdin_copy = dup(0);
            stderr_copy = dup(2);
            res1 = close(0); //close stdin
            res2 = close(2); //close stderr
        }
        if (res1 == -1 || res2 == -1) {
            perror("smash error: close failed");
            exit(-1);
        }
        int pipe_arr[2];
        int res = pipe(pipe_arr);
        if (res == -1) {
            perror("smash error: pipe failed");
            exit(-1);
        }
        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("smash error: fork failed");
            exit(-1);
        }
        if (pid2 == 0) {
            //Second child process
            int res3 = close(pipe_arr[1]);
            int res4 = 0;
            if (type == PIPE) {
                dup2(stdout_copy, 1);
                res3 = close(stdout_copy);
            } else {
                dup2(stderr_copy, 2);
                res4 = close(stderr_copy);
            }
            if (res3 == -1 || res4 == -1) {
                perror("smash error: close failed");
                exit(-1);
            }

            if(cmd2->GetIsExternal()){
                ExternalCommand* ex_cmd=(ExternalCommand*)cmd2;
                ex_cmd->PrepForPipeOrTime();
            }

            cmd2->execute();
            exit(0);
        } else {
            //First child process
            int res5 = close(pipe_arr[0]);
            dup2(stdin_copy, 0);
            int res6 = close(stdin_copy);
            if (res5 == -1 || res6 == -1) {
                perror("smash error: close failed");
                exit(-1);
            }
            if(cmd1->GetIsExternal()){
                ExternalCommand* ex_cmd=(ExternalCommand*)cmd1;
                ex_cmd->PrepForPipeOrTime();
            }
            cmd1->execute();
            int res7 = close(pipe_arr[1]);
            if(res7==-1){
                perror("smash error: close failed");
                exit(-1);
            }
            int status;
            waitpid(pid2, &status, WUNTRACED);
            exit(0);
        }
    } else {
        //Shell process
        ChangePID(pid);
        if (GetBackground()){
            jobs->addJob(this,false);
            return;
        }
        int status1;
        waitpid(pid,&status1,WUNTRACED);
        if(WIFSTOPPED(status1)){
            ChangeBackground();
        }
        return;
    }

}
 */

void PipeCommand::execute() {
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("smash error: fork failed");
        exit(-1);
    }
    if (pid1 == 0) {
        //First child process

        setpgrp();
        int stdin_copy = 0;
        int stdout_copy = 1;
        int stderr_copy = 2;
        int res1, res2;
        if (type == PIPE) {
            stdin_copy = dup(0);
            stdout_copy = dup(1);
            res1 = close(0); //close stdin
            res2 = close(1); //close stdout
        } else {
            stdin_copy = dup(0);
            stderr_copy = dup(2);
            res1 = close(0); //close stdin
            res2 = close(2); //close stderr
        }
        if (res1 == -1 || res2 == -1) {
            perror("smash error: close failed");
            return;
        }
        int pipe_arr[2];
        int res = pipe(pipe_arr);
        if (res == -1) {
            perror("smash error: pipe failed");
            exit(-1);
        }
        pid_t pid2=fork();
        if(pid2==0){
            //Second child
            int res3 = close(pipe_arr[1]);
            int res4 = 0;
            if (type == PIPE) {
                dup2(stdout_copy, 1);
                res3 = close(stdout_copy);
            } else {
                dup2(stderr_copy, 2);
                res4 = close(stderr_copy);
            }
            if (res3 == -1 || res4 == -1) {
                perror("smash error: close failed");
                exit(-1);
            }
            if(cmd2->GetIsExternal() || cmd2->GetIsCp()){
                cmd2->PrepForPipeOrTime();
            }
            cmd2->execute();
            exit(0);
        }
        else {
            //first child process
            GetCmd2()->ChangePID(pid1);
            pid_t pid3=fork();
            if(pid3==0){
                //Second child process
                int res5 = close(pipe_arr[0]);
                dup2(stdin_copy, 0);
                int res6 = close(stdin_copy);
                if (res5 == -1 || res6 == -1) {
                    perror("smash error: close failed");
                    exit(-1);
                }
                if(cmd1->GetIsExternal()|| cmd1->GetIsCp()){
                    cmd1->PrepForPipeOrTime();
                }
                cmd1->execute();
                exit(0);
            }
            else{
                //first child process
                int res3 = close(pipe_arr[1]);
                int res4 = 0;
                if (type == PIPE) {
                    dup2(stdout_copy, 1);
                    res3 = close(stdout_copy);
                } else {
                    dup2(stderr_copy, 2);
                    res4 = close(stderr_copy);
                }
                int res5 = close(pipe_arr[0]);
                dup2(stdin_copy, 0);
                int res6 = close(stdin_copy);
                if (res3 == -1 || res4 == -1||res5 == -1 || res6 == -1) {
                    perror("smash error: close failed");
                    exit(-1);
                }
                int status,status2;
                waitpid(pid2, &status, 0);
                waitpid(pid3, &status2, 0);
                exit(0);

            }
        }

    }
    else{
        ChangePID(pid1);
        if (GetBackground()){
            jobs->addJob(this,false);
            return;
        }
        int status1;
        waitpid(pid1,&status1,WUNTRACED);
        if(WIFSTOPPED(status1)){
            ChangeBackground();
        }
    }
}

/*-------------------------Command Redirection---------------------------*/
RedirectionCommand::RedirectionCommand(const char* cmd_line,int type,Command* cmd):
        Command(cmd_line),type(type),cmd(cmd){
    if(_isBackgroundComamnd(cmd_line)){
        ChangeBackground();
    }
}

RedirectionCommand::~RedirectionCommand(){
    delete cmd;
}
void RedirectionCommand::execute() {
    string cmd_s = string(GetCmdLine());
    string file_name;
    SmallShell &smash = SmallShell::getInstance();
    JobsList *jobs = smash.GetJobList();
    unsigned long ind = 0;
    if (type == APPEND) {
        ind = cmd_s.find(">>");
        file_name = cmd_s.substr(ind + 2, string::npos);

    } else {
        ind = cmd_s.find('>');
        file_name = cmd_s.substr(ind + 1, string::npos);
    }
    //separate the file name from the rest of the command
    //TODO: what if we get two file names?
    unsigned long ind2 = file_name.find_first_not_of(' ');
    file_name = file_name.substr(ind2, string::npos);
    unsigned long ind3 = file_name.find_first_of(' ');
    file_name = file_name.substr(0, ind3);
    unsigned long len=file_name.length();
    if(file_name.back()=='&'){
        file_name=file_name.substr(0,len);
    }
    if (cmd->GetIsExternal()|| cmd->GetIsCp()) {
        SmallShell &smash = SmallShell::getInstance();
        JobsList *jobs = smash.GetJobList();
        pid_t pid = fork();
        if (pid == 0) {
            setpgrp();
            int stdout_copy = dup(1); //save stdout
            close(1);
            int file;
            if (type == APPEND) {
                file = open(file_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, 000666);
            } else {
                file = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 000666);
            }
            if (file == -1) {
                dup2(stdout_copy, 1);
                close(stdout_copy);
                perror("smash error: open failed");
                exit(0);
            } else {
                cmd->PrepForPipeOrTime();
                cmd->execute();
                close(file);
                dup2(stdout_copy, 1);
                close(stdout_copy);
                exit(0);
            }
        } else {
            //Shell process
            ChangePID(pid);
            if (GetBackground()) {
                jobs->addJob(this, false);
                return;
            }
            int status1;
            waitpid(pid, &status1, WUNTRACED);
            if (WIFSTOPPED(status1)) {
                ChangeBackground();
            }
        }

    } else {
        int stdout_copy = dup(1); //save stdout
        close(1);
        int file;
        if (type == APPEND) {
            file = open(file_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, 000666);
        } else {
            file = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 000666);
        }
        if (file == -1) {
            dup2(stdout_copy, 1);
            close(stdout_copy);
            perror("smash error: open failed");
            return;
        } else {
            cmd->execute();
            close(file);
            dup2(stdout_copy, 1);
            close(stdout_copy);
            if (cmd->GetIsQuit()) {
                exit(0);
            }
        }
    }
}



/*-------------------------Command Copy----------------------------------*/
CopyCommand::CopyCommand(const char* cmd_line, JobsList* jobs): Command(cmd_line){
    this->jobs = jobs;
    if (_isBackgroundComamnd(cmd_line)){
        this->ChangeBackground();
    }
    is_cp=true;

}

//make sure to handle signals and kill option
void CopyCommand::execute(){

    int num_of_args;
    char* arg_list[COMMAND_MAX_ARGS + 1];

    if(_isBackgroundComamnd(GetCmdLine())) {
        char *new_cmd_line = CopyCmd(GetCmdLine());
        _removeBackgroundSign(new_cmd_line);
        num_of_args = _parseCommandLine(new_cmd_line, arg_list);

        delete[] new_cmd_line;
    }
    else{
        num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
    }

    //------------------------------check if both files are the same----------------------------//
    bool same_files = false;
    if (string(arg_list[1])==string(arg_list[0])){
        same_files = true;
    }
    else {
        char *pwd = nullptr;
        pwd = get_current_dir_name();
        int pwd_size = string(pwd).size() + 1;
        char *new_path = nullptr;
        //handling arg[1]
        int arg_size = string(arg_list[1]).size() + 1;
        if (arg_list[1][0] == '.' && arg_list[1][1] == '/') {
            new_path = new char[pwd_size + arg_size - 2];
            for (int i = 0; i < pwd_size - 1; i++) {
                new_path[i] = pwd[i];
            }
            for (int i = 0; i < arg_size - 1; i++) {
                new_path[pwd_size + i] = arg_list[1][i + 1];
            }
        }else if(arg_list[1][0] == '.'){
            new_path = new char[pwd_size + arg_size - 1];
            for (int i = 0; i < pwd_size - 1; i++) {
                new_path[i] = pwd[i];
            }
            new_path[pwd_size-1] = '/';
            for (int i = 0; i < arg_size - 1; i++) {
                new_path[pwd_size + i] = arg_list[1][i + 1];
            }
        } else {

            char* check_if_is_path = new char[pwd_size];
            for (int i = 0; i < pwd_size - 1; i++) {
                check_if_is_path[i] = arg_list[1][i];
            }
            check_if_is_path[pwd_size - 1] = pwd[pwd_size - 1];
            if (string(check_if_is_path) == string(pwd)) {
                new_path = new char[arg_size];
                for (int i = 0; i < arg_size; i++) {
                    new_path[i] = arg_list[1][i];
                }
            } else {
                new_path = new char[pwd_size + arg_size];
                for (int i = 0; i < pwd_size - 1; i++) {
                    new_path[i] = pwd[i];
                }
                new_path[pwd_size-1] = '/';
                for (int i = 0; i < arg_size; i++) {
                    new_path[pwd_size + i] = arg_list[1][i];
                }
            }
            delete[] check_if_is_path;
        }

        //handling arg[2]
        char *new_second_path = nullptr;
        arg_size = string(arg_list[2]).size() + 1;
        if (arg_list[2][0] == '.' && arg_list[2][1] == '/') {
            new_second_path = new char[pwd_size + arg_size - 2];
            for (int i = 0; i < pwd_size - 1; i++) {
                new_second_path[i] = pwd[i];
            }
            for (int i = 0; i < arg_size - 1; i++) {
                new_second_path[pwd_size-1 + i] = arg_list[2][i + 1];
            }
        }else if(arg_list[2][0] == '.'){
            new_second_path = new char[pwd_size + arg_size - 1];
            for (int i = 0; i < pwd_size - 1; i++) {
                new_path[i] = pwd[i];
            }
            new_path[pwd_size-1] = '/';
            for (int i = 0; i < arg_size - 1; i++) {
                new_path[pwd_size + i] = arg_list[2][i + 1];
            }
        } else {

            char* check_if_is_path = new char[pwd_size];
            for (int i = 0; i < pwd_size - 1; i++) {
                check_if_is_path[i] = arg_list[2][i];
            }
            check_if_is_path[pwd_size - 1] = pwd[pwd_size - 1];
            if (string(check_if_is_path) == string(pwd)) {
                new_second_path = new char[arg_size];
                for (int i = 0; i < arg_size; i++) {
                    new_second_path[i] = arg_list[2][i];
                }
            } else {
                new_second_path = new char[pwd_size + arg_size];
                for (int i = 0; i < pwd_size - 1; i++) {
                    new_second_path[i] = pwd[i];
                }
                new_second_path[pwd_size-1] = '/';
                for (int i = 0; i < arg_size; i++) {
                    new_second_path[pwd_size + i] = arg_list[2][i];
                }
            }
            delete[] check_if_is_path;
        }
        if (string(new_path)==string(new_second_path)){
            same_files = true;
        }

        free(pwd);
        delete[] new_path;
        delete [] new_second_path;
    }


    if(same_files) {
        int file = open(arg_list[1], O_RDONLY);
        if (file==-1){
            perror("smash error: open failed");
        }
        else{
            close(file);
            std::cout << "smash: " << arg_list[1] <<" was copied to " << arg_list[2] << endl;
        }
        FreeCmdArray(arg_list, num_of_args);
        return;
    }

    //------------------------------if files are different----------------------------//


    pid_t pid = fork();
    if (pid==-1){
        perror("smash error: fork failed");
        return;
    }
    if (pid>0){ //Shell
        ChangePID(pid);
        if(_isBackgroundComamnd(GetCmdLine())) {
            if (!is_timeout) {
                jobs->addJob(this);
            }
        }
        else{
            int status;
            if(is_pipe){
                waitpid(pid,&status,0);
            }
            else{
                waitpid(pid,&status,WUNTRACED);
                if(WIFSTOPPED(status)){
                    ChangeBackground();
                }
            }

        }
        return;
    }
    else { //copy parent
        if(!is_pipe && !is_timeout){
            setpgrp();
        }

        int* file_to_read = new int;
        int* file_to_write = new int;
        int* status = new int;


        //----------------------------------open reading file-------------------------------//
        *file_to_read = open(arg_list[1], O_RDONLY);
        if (*file_to_read == -1) { //didn't open correctly
            perror("smash error: open failed");
            delete file_to_read;
            delete file_to_write;
            delete status;
            FreeCmdArray(arg_list, num_of_args);
            exit(0);
        }


        //----------------------------------open writing file-------------------------------//

        *file_to_write = open(arg_list[2], O_RDWR | O_CREAT | O_TRUNC, 0600); //TODO: 0666?
        if (*file_to_write == -1) { //didn't open correctly
            perror("smash error: open failed");
            delete file_to_write;
            FreeCmdArray(arg_list, num_of_args);
            close(*file_to_read);
            delete file_to_read;
            delete status;
            exit(0);
        }


        //----------------------------------read and write file-------------------------------//

        int total_read_so_far = 0;
        int current_read = 0;
        int lseek_read_result = 0;
        int lseek_write_result = 0;
        char buf [101];

        while ((current_read = read(*file_to_read, buf, 100)) && current_read > 0 ){
            total_read_so_far += current_read;
            current_read < 100 ? *status = write(*file_to_write, buf, current_read): *status = write(*file_to_write, buf, 100);
            if (*status == -1) {
                perror("smash error: write failed");
                close(*file_to_read);
                close(*file_to_write);
                delete file_to_read;
                delete file_to_write;
                FreeCmdArray(arg_list, num_of_args);
                delete status;
                exit(1);

            }
            lseek_read_result = lseek(*file_to_read, total_read_so_far, SEEK_SET);
            lseek_write_result = lseek(*file_to_write, total_read_so_far, SEEK_SET);
            if (lseek_read_result == -1 || lseek_write_result == -1){
                perror("smash error: lseek failed");
                close(*file_to_read);
                close(*file_to_write);
                delete file_to_read;
                delete file_to_write;
                FreeCmdArray(arg_list, num_of_args);
                delete status;
                exit(1);
            }

        }
        if (current_read == -1){ //reading has failed
            perror("smash error: read failed");
            close(*file_to_read);
            close(*file_to_write);
            delete file_to_read;
            delete file_to_write;
            FreeCmdArray(arg_list, num_of_args);
            delete status;
            exit(1);
        }
        *status = close(*file_to_read);
        if(*status == -1){
            perror("smash error: close failed");
        }
        *status = close(*file_to_write);
        if(*status == -1){
            perror("smash error: close failed");
        }
        delete file_to_read;
        delete file_to_write;
        delete status;
        std::cout << "smash: " << arg_list[1] <<" was copied to " << arg_list[2] << endl;
        FreeCmdArray(arg_list, num_of_args);
        exit(1);

    }

}

/*-------------------------Command Timeout----------------------------------*/
TimeoutCommand::TimeoutCommand(const char* cmd_line, Command* cmd, JobsList* jobslst,TimeoutList* t_list):
        Command(cmd_line),cmd(cmd){
    jobs=jobslst;
    times=t_list;
    is_timeout=true;
}
TimeoutCommand::~TimeoutCommand(){
    delete cmd;
}
Command* TimeoutCommand::GetInternalCommand(){
    return cmd;

};

void TimeoutCommand::execute(){
    int num_of_args;
    char* arg_list[COMMAND_MAX_ARGS+1];
    num_of_args=_parseCommandLine(GetCmdLine(),arg_list);
    if(cmd->GetBackground()){
        ChangeBackground();
    }
    string time=arg_list[1];
    unsigned int duration=(unsigned int)stoi(time);
    if(!cmd->GetIsExternal() && !cmd->GetIsCp()){
        TimeoutList::TimeoutEntry *entry = times->addTimedJob(this, duration);
        cmd->execute();
    }
    else{
        pid_t pid1=fork();
        if(pid1==-1){
            FreeCmdArray(arg_list,num_of_args);
            perror("smash error: fork failed");
            return;
        }
        if(pid1==0){ //Child Process
            setpgrp();
            if(cmd->GetBackground()){
                cmd->ChangeBackground();
            }
            cmd->execute();
            exit(0);
        }
        else{ //Shell
            ChangePID(pid1);
            TimeoutList::TimeoutEntry *entry = times->addTimedJob(this, duration);
            if(GetBackground()){
                JobsList::JobEntry *new_job = jobs->addJob(this, false);
                entry->ChangeJobID(new_job->GetJobID());
                return;
            }
            int status;
            waitpid(pid1,&status,WUNTRACED);
            if(WIFSTOPPED(status)){
                ChangeBackground();
            }

        }
    }
    FreeCmdArray(arg_list,num_of_args);
}

/*========================================================================*/
/*===========================Built In Commands============================*/
/*========================================================================*/

/*---------------------BuiltInCommand Class Functions---------------------*/
BuiltInCommand::BuiltInCommand():Command(){}

BuiltInCommand::BuiltInCommand(const char* cmd_line):Command(cmd_line){}


/*-------------------------BuiltInCommand ChangePrompt--------------------*/
ChangePromptCommand::ChangePromptCommand(std::string* prompt,string new_prompt):
        BuiltInCommand(),prompt_name(prompt),new_prompt(new_prompt){};

void ChangePromptCommand::execute() {
    if (new_prompt == " ") {
        *prompt_name = "smash> ";
    }
    else {
        new_prompt.append(">"); //TODO: what if it end with > already?
        new_prompt.append(" ");
        *prompt_name = new_prompt;
    }

}


/*-------------------------BuiltInCommand ShowPid-------------------------*/
void ShowPidCommand::execute()  {
    std::cout << "smash pid is " << GetPID() << endl;
};


/*-------------------------BuiltInCommand GetCurrDir-----------------------*/
void GetCurrDirCommand::execute() {
    char * pwd = nullptr;
    pwd=get_current_dir_name();
    if(!pwd){
        perror("smash error: getcwd failed");
        return;
    }
    std::cout << pwd << endl;
    free(pwd);
}


/*-------------------------BuiltInCommand ChangeDirCommand-----------------*/
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char* old_pwd, char* curr_pwd) :BuiltInCommand(cmd_line) {

    this->old_pwd = old_pwd;
    this->curr_pwd = curr_pwd;
}

void ChangeDirCommand::execute() {
    int num_of_args;
    char* arg_list[COMMAND_MAX_ARGS + 1];
    num_of_args = _parseCommandLine(GetCmdLine(), arg_list);

    SmallShell& smash = SmallShell::getInstance();
    if(arg_list[1]== nullptr){
        FreeCmdArray(arg_list,num_of_args);
        return;
    }
    //if there are more than 2 arguments
    if (arg_list[2] != NULL) {
        std::cerr << "smash error: cd: too many arguments" << endl;
        FreeCmdArray(arg_list,num_of_args);
        return;
    }

    //in case you want to go back
    if (string(arg_list[1]) == "-") {
        //in case there is no available history
        if (old_pwd == nullptr) {
            std::cerr << "smash error: cd: OLDPWD not set" << endl;

        }
            //in case there is available history
        else {
            int state = chdir(old_pwd);
            if (state == 0) { //succeeded
                char* new_curr_pwd= CopyCmd(old_pwd);
                smash.ChangeCurrPwd(new_curr_pwd);
            }
            else { //failed
                perror("smash error: chdir failed");
            }
        }
    }

    else if(string(arg_list[1]) == ".."){
        int state;
        if (string(curr_pwd) == "/"){
            char* new_curr_pwd= CopyCmd(curr_pwd);
            state = chdir(new_curr_pwd);
            if(state==-1){
                FreeCmdArray(arg_list,num_of_args);
                perror("smash error: chdir failed");
                return;
            }
            smash.ChangeCurrPwd(new_curr_pwd);
        }
        else{
            std::size_t found = string(curr_pwd).find_last_of('/');
            string new_curr_string = string(curr_pwd).substr(0,found);
            char* new_curr= CopyCmd(new_curr_string.c_str());
            state = chdir(new_curr);
            if(state==-1){
                FreeCmdArray(arg_list,num_of_args);
                perror("smash error: chdir failed");
                return;
            }
            smash.ChangeCurrPwd(new_curr);
        }
    }

    else {//normal change direction
        int state = chdir(arg_list[1]);
        if (state == 0) { //succeeded
            char* full_curr_pwd=get_current_dir_name();
            smash.ChangeCurrPwd(full_curr_pwd);
        }
        else { //failed
            perror("smash error: chdir failed");
        }

    }

    FreeCmdArray(arg_list,num_of_args);
}


/*-------------------------BuiltInCommand JobsCommand-----------------------*/
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
    job_list = jobs;

}

void JobsCommand::execute() {
    job_list->printJobsList();
}


/*-------------------------BuiltInCommand KillCommand------------------------*/
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
    job_list = jobs;
}

void KillCommand::execute() {
    int num_of_args;
    int res,sig,jobID;
    string num;
    char* arg_list[COMMAND_MAX_ARGS + 1];
    num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
    try {
        if(num_of_args!=3){
            std::cerr << "smash error: kill: invalid arguments" << endl;
            FreeCmdArray(arg_list,num_of_args);
            return;
        }
        jobID=stoi(arg_list[2]);
        num=string(arg_list[1]).substr(1,string::npos);
        sig=stoi(num);
    }
    catch(exception &e){
        std::cerr << "smash error: kill: invalid arguments" << endl;
        FreeCmdArray(arg_list,num_of_args);
        return;
    }
    if (jobID<1){
        std::cerr << "smash error: kill: job-id " << arg_list[2] << " does not exist" << endl;
        FreeCmdArray(arg_list,num_of_args);
        return;
    }

    //checking if jobID is valid
    JobsList::JobEntry* temp = this->job_list->getJobById(jobID);
    if (temp == nullptr) {
        std::cerr << "smash error: kill: job-id " << arg_list[2] << " does not exist" << endl;
    }
    if ((31<sig || sig<0)){
        std::cerr << "smash error: kill failed: invalid argument" << endl;
        FreeCmdArray(arg_list,num_of_args);
        return;
    }

    if (arg_list[1][0] != '-') { //checking input
        std::cerr << "smash error: kill: invalid arguments" << endl;
        FreeCmdArray(arg_list,num_of_args);
        return;
    }


    else {
        //everything is OK, execute signal
        res = job_list->ExecuteSignal(jobID,sig);
        if (res == OK) {
            std::cout << "signal number " << num << " was sent to pid " << arg_list[2] << endl;

        }
    }

    FreeCmdArray(arg_list,num_of_args);

}


/*-------------------------BuiltInCommand ForegroundCommand------------------*/
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
    job_list = jobs;
}

void ForegroundCommand::execute() {
    int num_of_args,status = 0;
    int jobID;
    pid_t jobPID;
    int res=0;
    char* args[COMMAND_MAX_ARGS + 1];
    SmallShell& smash = SmallShell::getInstance();
    num_of_args = _parseCommandLine(GetCmdLine(), args);
    switch (num_of_args) {
        case 1: {
            unsigned long list_size = job_list->ListSize();
            if (list_size == 0) {
                std::cerr << "smash error: fg: jobs list is empty" << endl;
                break;
            }
            else {
                //TODO: add function to remove last job so no error occurs!
                JobsList::JobEntry* last_job = job_list->getLastJob(&jobID);
                jobPID = last_job->GetCommand()->GetPID();
                std::cout << last_job->GetCommand()->GetCmdLine() << " : " << jobPID << endl;
                smash.ChangeCurrCmd(last_job->GetCommand());
                if(smash.GetCurrCmd()->GetIsPipe()){
                    res=killpg(smash.GetCurrCmd()->GetPID(),SIGCONT);
                }
                else{
                    res=kill(jobPID, SIGCONT);
                }

                if(res==-1) {
                    perror("smash error: kill failed");
                    break;
                }
                res=waitpid(jobPID, &status, WUNTRACED);
                if(!WIFSTOPPED(status)){ //job finished, wasn't stopped again
                    job_list->removeJobById(jobID);
                }
                if(res==-1) {
                    perror("smash error: waitpid failed");

                }

                break;
            }
        }
        case 2: {
            try {
                jobID = stoi(args[1]);
            }
            catch (exception &e) {
                std::cerr << "smash error: fg: invalid arguments" << endl;
                break;
            }
            if (jobID<1){
                std::cerr << "smash error: fg: job-id " << args[1] << " does not exist" << endl;
                break;
            }
            JobsList::JobEntry* job = job_list->getJobById(jobID);
            if (!job) {
                std::cerr << "smash error: fg: job-id " << jobID << " does not exist" << endl;
                break;
            }
            jobPID = job->GetCommand()->GetPID();
            smash.ChangeCurrCmd(job->GetCommand());
            std::cout << job->GetCommand()->GetCmdLine() << " : " << jobPID << endl;
            if(smash.GetCurrCmd()->GetIsPipe()){
                res=killpg(smash.GetCurrCmd()->GetPID(),SIGCONT);
            }
            else{
                res=kill(jobPID, SIGCONT);
            }

            if(res==-1) {
                perror("smash error: kill failed");
                break;
            }

            res=waitpid(jobPID, &status, WUNTRACED);
            if(res==-1) {
                perror("smash error: waitpid failed");
            }
            if(!WIFSTOPPED(status)){ //job finished, wasn't stopped again
                job_list->removeJobById(jobID);
            }
            break;
        }
        default:
            std::cerr << "smash error: fg: invalid arguments" << endl;
            break;

    }
    FreeCmdArray(args,num_of_args);
}


/*-------------------------BuiltInCommand BackgroundCommand------------------*/
BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
    job_list = jobs;
}

void BackgroundCommand::execute() {
    int num_of_args,status;
    int jobID;
    pid_t jobPID;
    int res=0;
    char* args[COMMAND_MAX_ARGS + 1];
    num_of_args = _parseCommandLine(GetCmdLine(), args);
    switch (num_of_args) {
        case 1: {
            unsigned long list_size = job_list->ListSize();
            if (list_size == 0) {
                std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
                break;
            }
            else {
                JobsList::JobEntry* last_job = job_list->getLastStoppedJob(&jobID);

                if (last_job == nullptr){
                    std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
                    break;
                }

                jobPID = last_job->GetCommand()->GetPID();
                std::cout << last_job->GetCommand()->GetCmdLine() << " : " << jobPID << endl;
                if(last_job->GetCommand()->GetIsPipe()){
                    res=killpg(last_job->GetCommand()->GetPID(),SIGCONT);
                }
                else{
                    res=kill(jobPID, SIGCONT);
                }
                if(res==-1) {
                    perror("smash error: kill failed");
                    break;
                }
                else{
                    if(last_job->isStopped()){
                        last_job->ChangeStoppedStatus();
                    }
                }
                break;
            }
        }
        case 2: {
            try {
                jobID = stoi(args[1]);
            }
            catch (exception &e) {
                std::cerr << "smash error: bg: invalid arguments" << endl;
                break;
            }
            if (jobID<1){
                std::cerr << "smash error: bg: job-id " << args[1] << " does not exist" << endl;
                break;
            }
            JobsList::JobEntry* job = job_list->getJobById(jobID);
            if (!job) {
                std::cerr << "smash error: bg: job-id " << jobID << " does not exist" << endl;
                break;
            }
            jobPID = job->GetCommand()->GetPID();

            if(!(job->isStopped())){
                std::cerr << "smash error: bg: job-id " << jobID << " is already running in the background" << endl;
                break;
            }

            std::cout << job->GetCommand()->GetCmdLine() << " : " << jobPID << endl;
            if(job->GetCommand()->GetIsPipe()){
                res=killpg(job->GetCommand()->GetPID(),SIGCONT);
            }
            else{
                res=kill(jobPID, SIGCONT);
            }

            if(res==-1) {
                perror("smash error: kill failed");
                break;
            }
            else{
                if(job->isStopped()){
                    job->ChangeStoppedStatus();
                }
            }
            break;
        }
        default:
            std::cerr << "smash error: bg: invalid arguments" << endl;
            break;

    }
    FreeCmdArray(args,num_of_args);
}

/*
void BackgroundCommand::execute() {
	unsigned long list_size = job_list->ListSize();
	if (list_size == 0) {
		std::cerr << "smash error: bg: there is no stopped jobs to resume\n";
		return;
	}
	int num_of_args;
	int res;
	char *arg_list[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
	if ((num_of_args != 3 && num_of_args != 2) || !is_numeric(arg_list[1]) ||
		(num_of_args == 3 && !is_numeric(arg_list[2]))) { //checking input
		std::cerr << "smash error: bg: invalid arguments\n";
        FreeCmdArray(arg_list,num_of_args);
		return;
	}
	int jobID;
	JobsList::JobEntry *job = nullptr;
	//if a specific job was stated
	if (num_of_args == 3) {
		//checking if jobID is valid
		try {
			jobID = stoi(arg_list[1]);
		}
		catch (out_of_range &e) {
			std::cerr << "smash error: bg: invalid arguments\n";
            FreeCmdArray(arg_list,num_of_args);
			return;
		}
        if (jobID<1){
            std::cerr << "smash error: bg: job-id " << arg_list[2] << " does not exist\n";
            FreeCmdArray(arg_list,num_of_args);
            return;
        }
		job = job_list->getJobById(jobID);
		if (!job) {
			std::cerr << "smash error: bg: job-id " << jobID << " does not exist\n";
            FreeCmdArray(arg_list,num_of_args);
		}
		if (!(job->isStopped())) {
			std::cerr << "smash error: bg: job-id " << jobID << " is already running in the background\n";
            FreeCmdArray(arg_list,num_of_args);
		}
	}

		//if a specific job was NOT stated
	else {
		job = job_list->getLastStoppedJob(&jobID);
		if (job == nullptr) {
			std::cerr << "smash error: bg: there is no stopped jobs to resume\n";
            FreeCmdArray(arg_list,num_of_args);
		}

		res = job_list->ExecuteSignal(jobID, SIGCONT);
		if (res == OK) {
			pid_t jobPID = job->GetCommand()->GetPID();
			std::cout << job->GetCommand()->GetCmdLine() << " : " << jobPID << "\n";
		}
	}
	//free allocated memory from arg_list
	FreeCmdArray(arg_list,num_of_args);
}
*/

/*-------------------------BuiltInCommand QuitCommand------------------------*/

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
    jobs_list = jobs;
    char * args[COMMAND_MAX_ARGS + 1];
    is_quit=true;
    int num_of_args = _parseCommandLine(cmd_line,args);
    isKillSpecified=false;
    if(num_of_args >= 2){
        string second_word;
        for(int i=1;i<num_of_args;i++){
            second_word=string(args[i]);
            if (second_word == "kill") {
                isKillSpecified = true;
                break;
            }
        }
    }
    FreeCmdArray(args,num_of_args);
}

void QuitCommand::execute() {
    if (isKillSpecified) {
        jobs_list->removeFinishedJobs();
        jobs_list->PrintForQuit();
        jobs_list->killAllJobs();
    }
    else {
        jobs_list->ClearJobsFromList();
    }
}







/*========================================================================*/
/*=============================Shell Functions============================*/
/*========================================================================*/

/*-------------------------Shell Class Functions--------------------------*/
SmallShell::SmallShell():prompt_name("smash> ") {
    old_pwd = nullptr;
    char * buf = nullptr;
    buf=get_current_dir_name(); //TODO: count as sysetm call? check errno?
    curr_pwd = buf;
    job_list=new JobsList();
    pid=getpid();
    current_cmd= nullptr;
    timeout_commands= new TimeoutList();

}
SmallShell::~SmallShell() {
    delete job_list;
    delete timeout_commands;
}


/*--------------------------------Jobs Functions--------------------------*/
JobsList::JobsList():max_job_id(0) {
    vector<JobEntry*> lst=vector<JobEntry*>();
}
JobsList::~JobsList() {
    lst.clear();
}
JobsList::JobEntry* JobsList::addJob(Command* cmd, bool isStopped){
    removeFinishedJobs();
    max_job_id+=1;
    JobEntry* job= new JobEntry(max_job_id,isStopped,cmd);
    cmd->setJobID(max_job_id);
    lst.push_back(job);
    return job;

}

//this function need to make sure it's input are digits
JobsList::JobEntry* JobsList::getJobById(int jobId){
    if(lst.empty()){
        return nullptr;
    }
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        if((*iter)->GetJobID()==jobId){
            return *iter;
        }
    }
    return nullptr;
}

void JobsList::removeFinishedJobs(){
    int status;
    pid_t pid;
    pid_t res=0;
    vector<JobEntry*>::iterator iter=lst.begin();
    while(iter!=lst.end()){
        if((*iter)->isStopped()){
            ++iter;
            continue;
        }
        pid = (*iter)->GetCommand()->GetPID();
        res = waitpid(pid, &status, WNOHANG);
        if (res == -1) {
            perror("smash error: waitpid failed");
            ++iter;
            continue;
        }
        if (res == pid) {
            iter = lst.erase(iter);
            continue;
        }
        ++iter;
    }


    if(lst.empty()){
        max_job_id=0;
    }
    else {
        JobEntry* last_job=lst.back();
        max_job_id = last_job->GetJobID(); //the last job's ID is the maximum ID
    }
}
void JobsList::printJobsList(){
    pid_t pid;
    time_t start_time,now;
    double duration;
    const char* cmd;
    int jobID;
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        cmd=(*iter)->GetCommand()->GetCmdLine();
        pid=(*iter)->GetCommand()->GetPID();
        start_time=(*iter)->GetStartTime();
        jobID=(*iter)->GetJobID();
        now=time(nullptr);
        if(now==-1) {
            perror("smash error: time failed");
        }
        duration=difftime(now,start_time);
        std::cout << "[" << jobID << "] " << cmd << " : " << pid << " ";
        std::cout<<duration << " secs";
        if((*iter)->isStopped()){
            std::cout << " (stopped)";
        }
        std::cout<< endl;
    }
}

void JobsList::ClearJobsFromList(){
    lst.clear();
}
void JobsList::killAllJobs(){
    int res;
    int jobID;//TODO: use to check return value of kill?
    //TODO: remove finished jobs?
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end();){
        jobID=(*iter)->GetJobID();
        ExecuteSignal(jobID,SIGKILL); //here the pointer gets incremented
    }
    lst.clear();
}
void JobsList::PrintForQuit(){
    pid_t pid;
    unsigned long size=lst.size();
    std::cout << "smash: sending SIGKILL signal to " << size <<" jobs:" << endl;
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        pid=(*iter)->GetCommand()->GetPID();
        std::cout << pid << ": " << (*iter)->GetCommand()->GetCmdLine() << endl;
    }

}
JobsList::JobEntry* JobsList::getLastJob(int* lastJobID){//TODO: is that what they want with the ptr?
    //TODO: check for finished jobs?
    if(lst.empty()){
        return nullptr;
    }
    JobEntry* last_job=lst.back();
    *lastJobID=last_job->GetJobID();
    return last_job;
}
JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId){
    //TODO: check for finished jobs?
    if(lst.empty()){
        return nullptr;
    }
    for (vector<JobEntry*>::reverse_iterator iter=lst.rbegin(); iter!=lst.rend(); iter++){
        if((*iter)->isStopped()){
            *jobId=(*iter)->GetJobID();
            return (*iter);
        }
    }
    return nullptr;
}
void JobsList::removeJobById(int jobId){
    int removed_job_ID=0;
    for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
        if((*iter)->GetJobID()==jobId){
            removed_job_ID=(*iter)->GetJobID();
            lst.erase(iter);
            if(removed_job_ID==max_job_id){
                if(lst.empty()){
                    max_job_id=1;
                }
                else {
                    max_job_id = lst.back()->GetJobID();
                }
            }
            break;
        }
    }
}
unsigned long JobsList::ListSize() {
    return lst.size();
}

int JobsList::ExecuteSignal(int jobID,int signal){
    //before using this func jobID should be checked
    JobEntry* job=getJobById(jobID);
    pid_t pid=job->GetCommand()->GetPID();
    int res;
    if(job->GetCommand()->GetIsPipe()){
        res=killpg(pid,signal);
    }
    else{
        res=kill(pid,signal);
    }
    if(res==-1){
        perror("smash error: kill failed");
        return ERROR;
    }
    switch(signal){
        case SIGCONT:
            removeJobById(jobID);
            break;

        case SIGKILL:
            removeJobById(jobID);
            break;

        case SIGSTOP:
            if(job->isStopped()){
                job->ChangeStoppedStatus();
            }
            break;

        default:
            break;
    }
    return OK;

}

/*------------------------Commands In TimeoutList Functions-----------------------*/
TimeoutList::TimeoutList(){
    timeout_list=std::vector<TimeoutEntry*>();

}
TimeoutList::~TimeoutList(){}

TimeoutList::TimeoutEntry* TimeoutList::addTimedJob(TimeoutCommand* cmd, int duration){
    TimeoutEntry* entry= new TimeoutEntry(cmd,duration,cmd->GetBackground());
    time_t new_finish_time=entry->GetFinishTime();
    vector<TimeoutEntry*>::iterator iter;
    bool placed=false;
    time_t curr_finish_time;
    if(timeout_list.empty()){
        timeout_list.push_back(entry);
        alarm((unsigned int)duration);

    }
    else {
        int ind=0;
        for (iter = timeout_list.begin(); iter != timeout_list.end(); iter++) {
            curr_finish_time = (*iter)->GetFinishTime();
            if (new_finish_time <= curr_finish_time) {
                timeout_list.insert(iter, entry);
                placed = true;
                break;
            }
            else{
                ind++;
            }
        }
        if (!placed) {
            timeout_list.push_back(entry);
        }
        if(ind==0){
            alarm((unsigned int)duration);
        }

    }
    return entry;
}
void TimeoutList::removeTimedJob(){
    vector<TimeoutEntry*>::iterator it=timeout_list.begin();
    delete (*it)->GetCommand();
    vector<TimeoutEntry*>::iterator next=timeout_list.erase(it);
    if(next==timeout_list.end()){
        return;
    }
    time_t finish=(*next)->GetFinishTime();
    time_t now=time(nullptr);
    time_t alarm_time=finish-now;
    alarm((unsigned int)alarm_time);
}

TimeoutList::TimeoutEntry* TimeoutList::GetFirstEntry(){
    return (*timeout_list.begin());

};
/*------------------------Commands In Shell Functions-----------------------*/
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    Command* cmd= nullptr;
    int num_of_args,res;
    char* arg_list[COMMAND_MAX_ARGS+1];
    res=FindIfIO(cmd_line); //determines if an IO character is present
    const char* new_cmd_line;
    if(res!=REGULAR){
        string* new_cmd_line_s=ParseWithIO(cmd_line,res); //takes only the first part of the IO command,
        new_cmd_line=new_cmd_line_s->c_str();    //determines if it's valid and seperates
        // the IO character from the rest of the command
    }
    else{
        string cmd_s_p=string(cmd_line);
        if(cmd_s_p.empty()){
            return nullptr;
        }
        if(_isBackgroundComamnd(cmd_line)){
            char * cmd_without_bck=CopyCmd(cmd_line); //drop the &
            char* arg_list_temp[COMMAND_MAX_ARGS+1];
            int num_of_args1=_parseCommandLine(cmd_without_bck,arg_list_temp);
            _removeBackgroundSign(cmd_without_bck);
            if(CheckIfBuiltIn(cmd_without_bck)){
                new_cmd_line=(const char*)cmd_without_bck;
            }
            else{
                new_cmd_line = cmd_line;
            }
            FreeCmdArray(arg_list_temp,num_of_args1);
        }
        else {
            new_cmd_line = cmd_line;
        }
    }
    num_of_args=_parseCommandLine(new_cmd_line,arg_list);
    if(num_of_args==0){
        FreeCmdArray(arg_list,num_of_args);
        return nullptr;
    }

    if(res==PIPE || res==PIPEERR){
        //if we reach this, the IO command is valid and we can use the original cmd_line
        string cmd_s=string(cmd_line);
        string cmd1_s;
        string cmd2_s;
        unsigned long ind;
        if(res==PIPE){
            ind=cmd_s.find('|');
            cmd1_s=cmd_s.substr(0,ind);
            cmd2_s=cmd_s.substr(ind+1,string::npos);
        }
        else{
            ind=cmd_s.find("|&");
            cmd1_s=cmd_s.substr(0,ind);
            cmd2_s=cmd_s.substr(ind+2,string::npos);
        }
        if(_isBackgroundComamnd(cmd_line)){
            char *cmd_without_bck=CopyCmd(cmd2_s.c_str()); //drop the &
            _removeBackgroundSign(cmd_without_bck);
            cmd2_s=string(cmd_without_bck);
        }
        Command* cmd1=CreateCommand(cmd1_s.c_str());
        Command* cmd2=CreateCommand(cmd2_s.c_str());
        cmd=new PipeCommand(cmd_line,res,cmd1,cmd2,job_list);
        FreeCmdArray(arg_list,num_of_args);
        return cmd;
    }

    string first_word=string(arg_list[0]);
    if(first_word=="chprompt"){
        string new_prompt;
        if(num_of_args==1){
            new_prompt=" ";
        }
        else{
            new_prompt=string(arg_list[1]);
        }
        cmd=new ChangePromptCommand(&prompt_name,new_prompt);
    }
    else if(first_word=="showpid"){
        cmd=new ShowPidCommand();
    }
    else if(first_word=="pwd"){
        cmd=new GetCurrDirCommand(new_cmd_line);
    }
    else if(first_word=="cd"){
        cmd=new ChangeDirCommand(new_cmd_line,old_pwd,curr_pwd);
    }
    else if(first_word=="jobs"){
        cmd=new JobsCommand(new_cmd_line,job_list);
    }
    else if(first_word=="kill"){
        cmd=new KillCommand(new_cmd_line,job_list);
    }
    else if(first_word=="fg"){
        cmd=new ForegroundCommand(new_cmd_line,job_list);
    }
    else if(first_word=="bg"){
        cmd=new BackgroundCommand(new_cmd_line,job_list);
    }
    else if(first_word=="quit"){
        cmd=new QuitCommand(new_cmd_line,job_list);
    }
    else if(first_word=="cp"){
        cmd=new CopyCommand(new_cmd_line,job_list);
    }
    else if(first_word=="timeout"){
        try{
            stoi(arg_list[1]);
            if(num_of_args<3 || stoi(arg_list[1])<=0){
                std::cerr << "smash error: timeout: invalid arguments" << endl;
                cmd = nullptr;
                FreeCmdArray(arg_list,num_of_args);
                return cmd;
            }
        }
        catch(exception &e){
            std::cerr << "smash error: timeout: invalid arguments" << endl;
            cmd = nullptr;
            FreeCmdArray(arg_list,num_of_args);
            return cmd;

        }
        string* timeout_cmd_s=ParseTimeoutCmd(new_cmd_line);
        Command* cmd3;
        if(CheckIfBuiltIn(arg_list[2])&&_isBackgroundComamnd(new_cmd_line)){
            char* new_cmd_line_for_timeout = CopyCmd(cmd_line);
            _removeBackgroundSign(new_cmd_line_for_timeout);
            cmd3=CreateCommand(new_cmd_line_for_timeout);
        }
        else{//copy or external
            cmd3=CreateCommand(timeout_cmd_s->c_str());
            if (_isBackgroundComamnd(new_cmd_line)){
                cmd3->SetIsTimeout(true);
            }
        }
        cmd=new TimeoutCommand(new_cmd_line,cmd3,job_list,timeout_commands);
        delete timeout_cmd_s;
    }
    else{
        //external command will include the case that there is no command to execute
        cmd=new ExternalCommand(new_cmd_line, job_list);
    }

    if(res==OVERRIDE || res==APPEND){
        RedirectionCommand* reder_cmd;
        FreeCmdArray(arg_list,num_of_args);
        if(CheckIfBuiltIn(new_cmd_line) && _isBackgroundComamnd(cmd_line)) {
            char *cmd_without_bck = CopyCmd(cmd_line); //drop the &
            _removeBackgroundSign(cmd_without_bck);
            const char* alt_cmd_line = (const char *) cmd_without_bck;
            reder_cmd = new RedirectionCommand(alt_cmd_line,res,cmd);
        }
        else{
            reder_cmd = new RedirectionCommand(cmd_line,res,cmd);
        }
        return reder_cmd;
    }

    FreeCmdArray(arg_list,num_of_args);
    return cmd;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if(!cmd){
        return; //TODO: throw error?
    }
    current_cmd=cmd;
    job_list->removeFinishedJobs();
    current_cmd->execute();
    current_cmd= nullptr;
    if(cmd->GetBackground()|| cmd->GetIsTimeout()){
        return;
    }
    if(cmd->GetIsQuit()){
        exit(0);
    }
    delete cmd;
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

