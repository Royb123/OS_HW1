#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

class Command {
// TODO: Add your data members
    const char* cmd_line;
    pid_t pid;
 public:
    Command();
  explicit Command(const char* cmd_line);
  virtual ~Command()=default;
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  const char* GetCmdLine(){return cmd_line;};
  const pid_t GetPID(){return pid;}

  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
    BuiltInCommand();
  explicit BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand()=default;
  void execute() override;
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;

};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    ChangePromptCommand():BuiltInCommand(){};
    virtual ~ChangePromptCommand()=default;
    void execute() override{};
};

class ShowPidCommand : public BuiltInCommand {
public:
	ShowPidCommand();
	virtual ~ShowPidCommand() = default;
	void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
	explicit GetCurrDirCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {};
	virtual ~GetCurrDirCommand() {}
	void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
	char** plastPwd;
  public:
  ChangeDirCommand(const char* cmd_line, char** plastPwd);
  virtual ~ChangeDirCommand() {};
  void execute() override;
};

class JobsList;

class JobsList {
public:
	class JobEntry {
		const int jobid;
		bool stopped;
		Command* command;
		time_t start_time;
	public:
		JobEntry(int ID, bool stopped, Command* cmd) : jobid(ID), stopped(stopped) {
			command = cmd;
			start_time = time(nullptr); //TODO: handle errors
		}
		~JobEntry() = default;
		int GetJobID() { return jobid; }
		time_t GetStartTime() { return start_time; }
		Command* GetCommand() { return command; }
		bool isStopped() { return stopped; }
		void Execute_Signal(); //TODO: WRITE THIS

		// TODO: Add your data members
	};
	int max_job_id;
	std::vector<JobEntry*> lst;

public:
	JobsList();
	~JobsList();
	void addJob(Command* cmd, bool isStopped = false);
	void printJobsList();
	void killAllJobs();
	void removeFinishedJobs();
	void ClearJobsFromList();
	void PrintForQuit();
	JobEntry * getJobById(int jobId);
	void removeJobById(int jobId);
	JobEntry * getLastJob(int* lastJobID);
	JobEntry *getLastStoppedJob(int *jobId);
	unsigned long ListSize();
	// TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
	JobsList* job_list;
public:
	JobsCommand(const char* cmd_line, JobsList* jobs);
	virtual ~JobsCommand() = default;
	void execute() override;
};

class KillCommand : public BuiltInCommand {
	JobsList* job_list;
	// TODO: Add your data members
public:
	KillCommand(const char* cmd_line, JobsList* jobs);
	virtual ~KillCommand() {}
	void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
	// TODO: Add your data members
	JobsList* job_list;
public:
	ForegroundCommand(const char* cmd_line, JobsList* jobs);
	virtual ~ForegroundCommand() {}
	void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
	// TODO: Add your data members
public:
	BackgroundCommand(const char* cmd_line, JobsList* jobs);
	virtual ~BackgroundCommand() {}
	void execute() override;
};

class QuitCommand : public BuiltInCommand {
    JobsList* jobs_list;
    bool isKillSpecified;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand()= default;
  void execute() override;
};
/*
class CommandsHistory {
 protected:
  class CommandHistoryEntry {
	  // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  CommandsHistory();
  ~CommandsHistory() {}
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() {}
  void execute() override;
};
*/







// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
 public:
  CopyCommand(const char* cmd_line);
  virtual ~CopyCommand() {}
  void execute() override;
};





// TODO: add more classes if needed 
// maybe chprompt , timeout ?

class SmallShell {
 private:
    JobsList* job_list;
    std::string prompt_name;
	char* old_pwd[HISTORY_MAX_RECORDS+1];
	//int old_pwd_used;
<<<<<<< Updated upstream
    pid_t pid;
=======
	int pid;
>>>>>>> Stashed changes
  // TODO: Add your data members
  SmallShell();
 public:
	~SmallShell();
	Command *CreateCommand(const char* cmd_line);
	SmallShell(SmallShell const&)      = delete; // disable copy ctor
	void operator=(SmallShell const&)  = delete; // disable = operator
	static SmallShell& getInstance() // make SmallShell singleton
	{
	static SmallShell instance; // Guaranteed to be destroyed.
	// Instantiated on first use.
	return instance;
	}
	void executeCommand(const char* cmd_line);
	void ChangePrompt(std::string new_prompt);
	std::string GetPromptName(){return prompt_name;};
	// TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
