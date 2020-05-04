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


int FindIfIO(char** arg_list,int num_of_args){
	int res=REGULAR;
	for(int i=0;i<num_of_args;i++){
		if(string(arg_list[i])==">"){
			res=OVERRIDE;
			break;
		}
		else if(string(arg_list[i])==">>"){
			res=APPEND;
			break;
		}
		else if(string(arg_list[i])=="|"){
			res=PIPE;
			break;
		}
		else if(string(arg_list[i])=="|&"){
			res=PIPEERR;
			break;
		}

	}
	return res;
}
/*========================================================================*/
/*========================Command Class Functions=========================*/
/*========================================================================*/


// TODO: Add your implementation for classes in Commands.h
Command::Command():cmd_line(""){
	pid=getpid();
	background=false;

};
Command::Command(const char* cmd_line) {
    cmd_str = new string(cmd_line);
    this->cmd_line = cmd_str->c_str();
    pid=getpid();
    background=false;
}

Command::~Command(){
	delete cmd_str;
}

/*========================================================================*/
/*===========================External Commands============================*/
/*========================================================================*/

ExternalCommand::ExternalCommand(const char* cmd_line, JobsList* jobs):Command(cmd_line){
    this->jobs = jobs;
	int num_of_args;
	char* arg_list[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
	size_t last_arg_size = strlen(arg_list[num_of_args-1]);
	if(string(arg_list[num_of_args-1])=="&" || arg_list[num_of_args-1][last_arg_size-1]=='&'){
	    ChangeBackground();
	}
	if(GetBackground()){
		cmd_without_bck=CopyCmd(GetCmdLine()); //drop the &
		_removeBackgroundSign(cmd_without_bck);
	}
	else{
		cmd_without_bck= nullptr;
	}
	FreeCmdArray(arg_list,num_of_args);
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
    pid_t pid = fork();
    if(pid==-1){
        perror("smash error: fork failed\n");
    }
    if (pid == 0) { //child
        setpgrp();
        if(GetBackground()){ //suppose to run in the background
			execl("/bin/bash", "bash", "-c", cmd_without_bck, NULL);

    	}
    	else{//not in the background
			execl("/bin/bash", "bash", "-c", GetCmdLine(), NULL);
    	}

    }
    else { //parent - shell
		ChangePID(pid); //of class external command
        if (GetBackground()){
			FreeCmdArray(arg_list,num_of_args);
            jobs->addJob(this,false);
            return;
        }
        int status;
        waitpid(pid,&status,0);
    }

    FreeCmdArray(arg_list,num_of_args);
}

/*========================================================================*/
/*===========================Special Commands============================*/
/*========================================================================*/

/*-------------------------Command Pipe----------------------------------*/
PipeCommand::PipeCommand(const char* cmd_line,int type,Command* cmd1,Command* cmd2):
        Command(cmd_line),cmd1(cmd1),cmd2(cmd2),type(type){}

void PipeCommand::execute() {

    pid_t pid = fork(); //TODO: add setgrp()!
    if (pid == -1) {
        perror("smash error: fork failed\n");
        return;
    }
    if (pid == 0) {
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
            perror("smash error: close failed\n");
            exit(-1); //TODO: is this ok
        }
        int pipe_arr[2];
        int res = pipe(pipe_arr);
        if (res == -1) {
            perror("smash error: pipe failed\n");
            exit(-1);
        }
        pid_t pid1 = fork();
        if (pid1 == -1) {
            perror("smash error: fork failed\n");
            exit(-1);
        }
        if (pid1 == 0) {
            setpgrp();
            int res3 = close(pipe_arr[1]);
            int res4;
            if (type == PIPE) {
                dup2(stdout_copy, 1);
                res4 = close(stdout_copy);
            } else {
                dup2(stderr_copy, 2);
                res4 = close(stderr_copy);
            }
            if (res3 == -1 || res4 == -1) {
                perror("smash error: close failed\n");
                exit(-1);
            }

            pid_t pid = fork(); //TODO: add setgrp()!
            if (pid == -1) {
                perror("smash error: fork failed\n");
                return;
            }
            if (pid == 0) {
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
                    perror("smash error: close failed\n");
                    exit(-1); //TODO: is this ok
                }
                int pipe_arr[2];
                int res = pipe(pipe_arr);
                if (res == -1) {
                    perror("smash error: pipe failed\n");
                    exit(-1);
                }
                pid_t pid1 = fork();
                if (pid1 == -1) {
                    perror("smash error: fork failed\n");
                    exit(-1);
                }
                if (pid1 == 0) {
                    int res3 = close(pipe_arr[1]);
                    int res4;
                    if (type == PIPE) {
                        dup2(stdout_copy, 1);
                        res4 = close(stdout_copy);
                    } else {
                        dup2(stderr_copy, 2);
                        res4 = close(stderr_copy);
                    }
                    if (res3 == -1 || res4 == -1) {
                        perror("smash error: close failed\n");
                        exit(-1);
                    }


                    cmd2->execute();
                    exit(0);
                } else {
                    int res5 = close(pipe_arr[0]);
                    dup2(stdin_copy, 0);
                    int res6 = close(stdin_copy);
                    if (res5 == -1 || res6 == -1) {
                        perror("smash error: close failed\n");
                        exit(-1);
                    }
                    cmd1->execute();
                    int status;
                    waitpid(pid1, &status, 0); //TODO: check this!!!
                    exit(0);
                }
            } else {
                //TODO: add support for &

                delete cmd1;
                delete cmd2;
                return;
            }

        }
    }
}


/*-------------------------Command Redirection---------------------------*/
RedirectionCommand::RedirectionCommand(const char* cmd_line,int type,Command* cmd):
        Command(cmd_line),type(type),cmd(cmd){}

void RedirectionCommand::execute() {
    char* arg_list[COMMAND_MAX_ARGS+1];
    int num_of_args=_parseCommandLine(GetCmdLine(),arg_list);
    string redirection_sign;
    string file_name;
    if(type==APPEND){
        redirection_sign=">>";
    }
    else{
        redirection_sign=">";
    }
    for (int i = 1; i < num_of_args;i++) {
        if (arg_list[i-1] == redirection_sign){
            file_name=arg_list[i];
            break;
        }
    }
    ofstream out_file;
    //TODO: make sure the file exist!
    if(type==APPEND){
        out_file=ofstream(file_name,ofstream::out|ofstream::app);
    }
    else{
        out_file=ofstream(file_name,ofstream::out|ofstream::trunc);
    }
    streambuf* cout_buffer = cout.rdbuf();
    cout.rdbuf(out_file.rdbuf());
    cmd->execute();
    cout.rdbuf(cout_buffer);
    FreeCmdArray(arg_list,num_of_args);
    free(cmd);
}


/*-------------------------Command Copy----------------------------------*/
CopyCommand::CopyCommand(const char* cmd_line, JobsList* jobs): Command(cmd_line){
    this->jobs = jobs;
    if (_isBackgroundComamnd(cmd_line)){
        this->ChangeBackground();
    }

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

    pid_t pid = fork();
    if (pid==-1){
        perror("smash error: fork failed");
        return;
    }
    if (pid>0){ //Shell - TODO: make sure this is ok
        /*int last_arg_size = strlen(arg_list[num_of_args-1]);
        if (string(arg_list[num_of_args-1])=="&" || arg_list[num_of_args-1][last_arg_size-1]=='&'){
            //CommandForJobList* temp = new CommandForJobList(GetCmdLine(), pid); //run in the background
            jobs->addJob(this);
        }*/
        if(_isBackgroundComamnd(GetCmdLine())){
            jobs->addJob(this);
        }
        else{
            int status;
            //waitpid(pid, nullptr, 0);
            waitpid(pid,&status,WUNTRACED);
            if(WIFSTOPPED(status)){
                ChangeBackground();
                jobs->addJob(this,true);
            }
        }
        //FreeCmdArray(arg_list, num_of_args);
        return;
    }
    else { //copy parent

        setpgrp();

        int* file_to_read = new int;
        int* file_to_write = new int;
        int* status = new int;


        //----------------------------------open reading file-------------------------------//
        *file_to_read = open(arg_list[1], O_RDONLY);
        if (*file_to_read == -1) { //didn't open correctly
            std::cerr<<"file_to_read didn't open correctly\n";
            perror("smash error: open failed");
            delete file_to_read;
            delete file_to_write;
            delete status;
            FreeCmdArray(arg_list, num_of_args);
            exit(0);
        }
        else{
            std::cerr<<"file_to_read opened correctly\n";
        }


        //----------------------------------open writing file-------------------------------//

        *file_to_write = open(arg_list[2], O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (*file_to_write == -1) { //didn't open correctly
            std::cerr<<"file_to_write didn't open correctly\n";
            perror("smash error: open failed");
            delete file_to_write;
            FreeCmdArray(arg_list, num_of_args);
            /**status = */close(*file_to_read);
            delete file_to_read;
            /*if (*status == -1) {
                delete status;
                std::cerr<<"file_to_read didn't close correctly in -open writing file-\n";
                exit(2);

            }*/
            delete status;
            exit(0);
        }
        else{
            std::cerr<<"file_to_write opened correctly\n";
        }


        //----------------------------------read and write file-------------------------------//

        int total_read_so_far = 0;
        int current_read = 0;
        int lseek_read_result = 0;
        int lseek_write_result = 0;
        char buf [101];

        while ((current_read = read(*file_to_read, buf, 100)) && current_read > 0 ){
            total_read_so_far += current_read;
            std::cerr<<"current_read is " << current_read << "\n";
            std::cerr<<"total_read_so_far is " << total_read_so_far << "\n";
            current_read < 100 ? *status = write(*file_to_write, buf, current_read): *status = write(*file_to_write, buf, 100);
            if (*status == -1) {
                perror("smash error: write failed");
                close(*file_to_read);
                close(*file_to_write);
                delete file_to_read;
                delete file_to_write;
                FreeCmdArray(arg_list, num_of_args);
                delete status;
                std::cerr<<"write failed after reading" << total_read_so_far << "bytes\n";
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
                std::cerr<<"lseek failed after reading" << total_read_so_far << "bytes.\n lseek_read_result == "<< lseek_read_result << " and lseek_write_result == " << lseek_write_result << "\n";
                exit(1);
            }

        }
        std::cerr<<"total_read_so_far is " << total_read_so_far << "\n";
        if (current_read == -1){ //reading has failed
            perror("smash error: read failed");
            std::cerr<<"read failed after reading" << total_read_so_far << "bytes\n";
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
        std::cerr<<"close file_to_read failed\n";
    }
    *status = close(*file_to_write);
    if(*status == -1){
        perror("smash error: close failed");
        std::cerr<<"close file_to_write failed\n";
    }
    delete file_to_read;
    delete file_to_write;
    delete status;
    std::cout << "smash: " << arg_list[1] <<" was copied to " << arg_list[2] << "\n";
    FreeCmdArray(arg_list, num_of_args);
    std::cerr<<"finished successfully\n";
    exit(1);

    }

}

/*-----------------------BuiltIn Command Timeout---------------------------*/


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
	if (new_prompt == "") {
		*prompt_name = "smash>";
	}
	else {
		new_prompt.append(">"); //TODO: what if it end with > already?
		*prompt_name = new_prompt;
	}

}


/*-------------------------BuiltInCommand ShowPid-------------------------*/
void ShowPidCommand::execute()  {
	std::cout << "smash pid is " << GetPID() << "\n";
};


/*-------------------------BuiltInCommand GetCurrDir-----------------------*/
void GetCurrDirCommand::execute() {
	char * pwd = nullptr;
	pwd=get_current_dir_name();
	if(!pwd){
		perror("smash error: getcwd failed\n");
		return;
	}
	std::cout << pwd << "\n";
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

	//if there are more than 2 arguments
	if (arg_list[2] != NULL) {
		std::cout << "smash error: cd: too many arguments\n";
	}

	//in case you want to go back
	if (string(arg_list[1]) == "-") {
		//in case there is no available history
		if (old_pwd == nullptr) {
			std::cout << "smash error: cd: OLDPWD not set\n";
		}
		//in case there is available history
		else {
			int state = chdir(old_pwd);
			if (state == 0) { //succeeded
				char* temp = curr_pwd;
				curr_pwd = old_pwd;
				old_pwd = temp;
			}
			else { //failed
				std::cout << "smash error: > " << this->GetCmdLine() << endl;
			}
		}
	}

	else {//norml change direction
		int state = chdir(arg_list[1]);
		if (state == 0) { //succeeded
			old_pwd = curr_pwd;
			curr_pwd = arg_list[1];
		}
		else { //failed
			std::cout << "smash error: > " << this->GetCmdLine() << endl;
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
	int res;
	char* arg_list[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
	if (num_of_args != 3 || arg_list[1][0] != '-' || !is_numeric(arg_list[2]) || !is_numeric((arg_list[1] + 1 * sizeof(char)))) { //checking input
		std::cout << "smash error: kill: invalid arguments\n";
        FreeCmdArray(arg_list,num_of_args);
        return;
	}
	//checking if jobID is valid
	int jobID=stoi(arg_list[2]);
	int sig=stoi(arg_list[1]);
	JobsList::JobEntry* temp = this->job_list->getJobById(jobID);
	if (temp == nullptr) {
		std::cout << "smash error: kill: job-id " << arg_list[2] << " does not exist\n";
	}
	else {
		//everything is OK, execute signal
		res = job_list->ExecuteSignal(jobID,sig);
		if (res == OK) {
			std::cout << "signal number " << arg_list[1] << "was sent to pid " << arg_list[2] << endl;

		}
	}

    FreeCmdArray(arg_list,num_of_args);

}


/*-------------------------BuiltInCommand ForegroundCommand------------------*/
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
	job_list = jobs;
}

void ForegroundCommand::execute() {
	int num_of_args;
	int jobID;
	pid_t jobPID;
	int res=0;
	char* args[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), args);
	switch (num_of_args) {
		case 1: {
			unsigned long list_size = job_list->ListSize();
			if (list_size == 0) {
				std::cout << "smash error: fg: jobs list is empty\n";
				break;
			}
			else {
				//TODO: add function to remove last job so no error occurs!
				//TODO: change so that the option the process get Ctrl+Z
				JobsList::JobEntry* last_job = job_list->getLastJob(&jobID);
				job_list->removeJobById(jobID);
				jobPID = last_job->GetCommand()->GetPID();
				std::cout << last_job->GetCommand()->GetCmdLine() << " : " << jobPID << "\n";
				res=kill(jobPID, SIGCONT);
				if(res==-1) {
					perror("smash error: kill failed\n");
					break;
				}
				res=waitpid(jobPID, nullptr, 0);
				if(res==-1) {
					perror("smash error: waitpid failed\n");
				}
				break;
			}
		}
		case 2: {
			try {
				jobID = stoi(args[1]);
			}
			catch (out_of_range &e) {
				std::cout << "smash error: fg: invalid arguments\n";
				return;
			}
			JobsList::JobEntry* job = job_list->getJobById(jobID);
			if (!job) {
				std::cout << "smash error: fg: job-id " << jobID << " does not exist\n";
				return;
			}
			job_list->removeJobById(jobID);
			jobPID = job->GetCommand()->GetPID();
			std::cout << job->GetCommand()->GetCmdLine() << " : " << jobPID << "\n";
			res=kill(jobPID, SIGCONT);
			if(res==-1) {
				perror("smash error: kill failed\n");
				break;
			}
			res=waitpid(jobPID, nullptr, 0);
			if(res==-1) {
				perror("smash error: waitpid failed\n");
			}
			break;
		}
		default:
			std::cout << "smash error: fg: invalid arguments\n";
			break;

	}
}


/*-------------------------BuiltInCommand BackgroundCommand------------------*/
BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
	job_list = jobs;
}

void BackgroundCommand::execute() {
	unsigned long list_size = job_list->ListSize();
	if (list_size == 0) {
		std::cout << "smash error: bg: there is no stopped jobs to resume\n";
		return;
	}
	int num_of_args;
	int res;
	char *arg_list[COMMAND_MAX_ARGS + 1];
	num_of_args = _parseCommandLine(GetCmdLine(), arg_list);
	if ((num_of_args != 3 && num_of_args != 2) || !is_numeric(arg_list[1]) ||
		(num_of_args == 3 && !is_numeric(arg_list[2]))) { //checking input
		std::cout << "smash error: bg: invalid arguments\n";
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
			std::cout << "smash error: bg: invalid arguments\n";
            FreeCmdArray(arg_list,num_of_args);
			return;
		}
		job = job_list->getJobById(jobID);
		if (!job) {
			std::cout << "smash error: bg: job-id " << jobID << " does not exist\n";
            FreeCmdArray(arg_list,num_of_args);
		}
		if (!(job->isStopped())) {
			std::cout << "smash error: bg: job-id " << jobID << " is already running in the background\n";
            FreeCmdArray(arg_list,num_of_args);
		}
	}

		//if a specific job was NOT stated
	else {
		job = job_list->getLastStoppedJob(&jobID);
		if (job == nullptr) {
			std::cout << "smash error: bg: there is no stopped jobs to resume\n";
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


/*-------------------------BuiltInCommand QuitCommand------------------------*/

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line) {
	jobs_list = jobs;
	char * args[COMMAND_MAX_ARGS + 1];
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
	exit(0); //TODO:maybe we shouldn't put exit here
}







/*========================================================================*/
/*=============================Shell Functions============================*/
/*========================================================================*/

/*-------------------------Shell Class Functions--------------------------*/
SmallShell::SmallShell():prompt_name("smash>") {
	old_pwd = nullptr;
	char * buf = nullptr;
	buf=get_current_dir_name();
	curr_pwd = buf;
	job_list=new JobsList();
	pid=getpid();
	current_cmd= nullptr;
    std::vector<Command*>* timeout_commands= new vector<Command*>();

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
void JobsList::addJob(Command* cmd, bool isStopped){
	removeFinishedJobs();
	max_job_id+=1;
	JobEntry* job= new JobEntry(max_job_id,isStopped,cmd);
	lst.push_back(job);

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
		pid=(*iter)->GetCommand()->GetPID();
		res=waitpid(pid,&status,WNOHANG);
		if(res==-1){
			perror("smash error: waitpid failed\n");
			++iter;
			continue;
		}
		if(res==pid){
			iter=lst.erase(iter);
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
	removeFinishedJobs();
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
			perror("smash error: time failed\n");
		}
		duration=difftime(now,start_time);
		std::cout << "[" << jobID << "] " << cmd << " : " << pid << " ";
		std::cout<<duration << " secs";
		if((*iter)->isStopped()){
			std::cout << " (stopped)";
		}
		std::cout<< "\n";
	}
}

void JobsList::ClearJobsFromList(){
	lst.clear();
}
void JobsList::killAllJobs(){
	int res;
	int jobID;//TODO: use to check return value of kill?
	//TODO: remove finished jobs?
	for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
		jobID=(*iter)->GetJobID();
		ExecuteSignal(jobID,SIGKILL);
	}
	lst.clear();
}
void JobsList::PrintForQuit(){
	pid_t pid;
	unsigned long size=lst.size();
	std::cout << "smash: sending SIGKILL signal to " << size <<" jobs:\n";
	for (vector<JobEntry*>::iterator iter=lst.begin(); iter!=lst.end(); iter++){
		pid=(*iter)->GetCommand()->GetPID();
		std::cout << pid << ": " << (*iter)->GetCommand()->GetCmdLine() << "\n";
	}
	std::cout <<"Linux-shell:" << "\n";

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
	int res=kill(pid,signal);
	if(res==-1){
		perror("smash error: kill failed\n");
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
			job->ChangeStoppedStatus();
			break;

		default:
			break;
	}
	return OK;

}
/*------------------------Commands In Shell Functions-----------------------*/
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	Command* cmd= nullptr;
	bool builtin=true;
	int num_of_args,res;
	char* arg_list[COMMAND_MAX_ARGS+1];
	num_of_args=_parseCommandLine(cmd_line,arg_list);
	if(num_of_args==0){
	    FreeCmdArray(arg_list,num_of_args);
	    return nullptr;
	}
	res=FindIfIO(arg_list,num_of_args);
    if(res==PIPE || res==PIPEERR){
    	string cmd_s=string(cmd_line);
    	string cmd1_s;
    	string cmd2_s;
    	unsigned long ind;
    	if(res==PIPE){
    		ind=cmd_s.find('|');
    		cmd1_s=cmd_s.substr(0,ind-1);
			cmd2_s=cmd_s.substr(ind+1,string::npos);
    	}
    	else{
    		ind=cmd_s.find("|&");
			cmd1_s=cmd_s.substr(0,ind-1);
			cmd2_s=cmd_s.substr(ind+2,string::npos);
    	}
    	Command* cmd1=CreateCommand(cmd1_s.c_str());
    	Command* cmd2=CreateCommand(cmd2_s.c_str());
    	cmd=new PipeCommand(cmd_line,res,cmd1,cmd2);
    	FreeCmdArray(arg_list,num_of_args);
    	return cmd;
    }

	string first_word=string(arg_list[0]);
	if(first_word=="chprompt"){
		string new_prompt;
		if(num_of_args==1){
			new_prompt="";
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
		cmd=new GetCurrDirCommand(cmd_line);
	}
	else if(first_word=="cd"){
		cmd=new ChangeDirCommand(cmd_line,old_pwd,curr_pwd);
	}
	else if(first_word=="jobs"){
		cmd=new JobsCommand(cmd_line,job_list);
	}
	else if(first_word=="kill"){
		cmd=new KillCommand(cmd_line,job_list);
	}
	else if(first_word=="fg"){
		cmd=new ForegroundCommand(cmd_line,job_list);
	}
	else if(first_word=="bg"){
	    cmd=new BackgroundCommand(cmd_line,job_list);
	}
	else if(first_word=="quit"){
		cmd=new QuitCommand(cmd_line,job_list);
	}
	else if(first_word=="cp") {
        cmd = new CopyCommand(cmd_line, job_list);
    }
	/*else if(first_word=="timeout"){
		cmd=new TimeoutCommand(cmd_line);
	}*/
	else{
	    //external command will include the case that there is no command to execute
		builtin=false;
		cmd=new ExternalCommand(cmd_line, job_list);
	}


	if((res==OVERRIDE || res==APPEND) && builtin){
		FreeCmdArray(arg_list,num_of_args);
		RedirectionCommand* reder_cmd = new RedirectionCommand(cmd_line,res,cmd);
		return reder_cmd;
	 	//TODO: redirection needs to support external commands??
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
	current_cmd->execute();
	current_cmd= nullptr;
	if(cmd->GetBackground()){
		return;
	}
	delete cmd;
	// Please note that you must fork smash process for some commands (e.g., external commands....)
}

