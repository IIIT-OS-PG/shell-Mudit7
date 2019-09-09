#include "headers.h"
#define RCFILE "/home/mudit/OS/Myshell/myrc"
using namespace std;

typedef vector<string> vs;

typedef struct instruction{
	string prog;
	vs params;
	bool bg_flag;
	int fds[2];
}instruction;

typedef vector<instruction> vins;
vs history;
map<int,string> bjob;
int job_count=0;
string arr[7]={"cd","alias","history","alarm","PS1","echo","jobs"};
vector<string> excepts(arr,arr+7);

struct termios orig_termios;

void print_history()
{
	for(int i=0;i<history.size();i++)
		cout<<history[i]<<endl;
}
void backn(int n)
{
	for(int i=0;i<n;i++)
	cout<<"\b \b";
}
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

string getvar(string srchkey)
{
    ifstream  rcfile_r;
    rcfile_r.open(RCFILE);
	string entry,key,value;
    while(rcfile_r)
	{
        getline(rcfile_r,entry);
        stringstream X(entry);
        getline(X,key,'=');
        getline(X,value,'\n');
        if(key==srchkey){
			return value;
		}
    }
	rcfile_r.close();
	value.clear();
    return value;
}

void setvar(string key,string value)
{	
	string val=getvar(key);
	if(val.empty()) 
	{
		ofstream rcfile_w;
		rcfile_w.open(RCFILE, std::ios::app);
		rcfile_w<<key<<"="<<value<<"\n";
		rcfile_w.close();		
	}
	else
	{
		ifstream rcfile_i;
		rcfile_i.open(RCFILE);
		string entry,tempkey,tempval,data;
		while(rcfile_i)
		{
			entry.clear();
			getline(rcfile_i,entry);
			stringstream X(entry);
			getline(X,tempkey,'=');
			getline(X,tempval,'\n');
			if(tempkey==key){
				data=data+key+"="+value+"\n";
				//return;
			}
			else{
				data=data+entry+"\n";
			}	
		}
		rcfile_i.close();
		fstream f;
		f.open(RCFILE,ios::trunc|ios::out);
		f<<data.c_str();
		f.close();
	}	
}

string readcmd()
{
	string ps1=getvar("PS1");
	//cout<<ps1<<" ";	
	string prompt=ps1+" ";
	write(STDOUT_FILENO,prompt.c_str(),prompt.size());
	enableRawMode();
	//atexit(disableRawMode);
	char str[100];
  	char c;
	int i=0;
	int up=0;
  	while (1) {
	int size=read(STDIN_FILENO, &c, 1);
	if(c==27)
	{
		char c=getchar();
		c=getchar();
		if(c=='A'){
		//history.push_back(str);
			if(up== history.size()) {
				continue;
			}
			if(up>0){
				backn(history.at(up-1).size());	
				cout<<history.at(up);
			}
			if(up==0)
				cout<<history.at(up);

			if(up<history.size())
				up++;
		}
		if(c=='B'){
			if(up>0) up--;

			if(up==0)
			 	continue;
			else{
				 backn(history.at(up).size());
				 cout<<history.at(up-1);
			 }			
		 }
	}
	else{
		if (iscntrl(c)) {
	 		//printf("%d\n", c);
			if(c=='\n')
			{//end of command
				str[i]='\0';
				//up--;
				break;
			}
			if((c==127)&&(i>0)){
				backn(1);
				str[--i]='\0';			
			}
		}
		else{
			if(c==34)
			str[i++]=92;
			
			str[i++]=c;
			cout<<c;
		}		
	}
	fflush(0);
  }
  string input(str);
  //cout<<input<<endl;
	if((!input.empty())&&(input!="\n"))
  	history.insert(history.begin(),input);

	cout<<endl;

	if((up==0)&&history.empty()) {  return "";}
	else if((up==0)&&input.empty()) {  return "";}
	else if(up==0) return history.at(0);
	else return history.at(up-1);

}

vins split_input(string input)
{
	// make list of exceptions like if its a shell built-in

	vs args;
	vins instruction_list;
	instruction inst;
	stringstream X(input);
	string S1;
	while(getline(X,S1,'|'))		//seperate on the basis of 'pipe'
	{
		inst.params.clear();
		stringstream Y(S1);
		string S2;

		while(1)
		{	while(Y.peek() == ' ')
			Y.get();

			if(!getline(Y,S2,' ')) break;
			inst.params.push_back(S2);
		}
		inst.prog=inst.params[0];
		if(inst.params[inst.params.size()-1]=="&"){
			inst.bg_flag=1;
			inst.params.pop_back();
			//cout<<"bg";
		}	
		else
		{
			inst.bg_flag=0;	
		}

		//handel for execptions
		vector<string>::iterator it=find(excepts.begin(),excepts.end(),inst.prog);
		if(it==excepts.end()){
		instruction_list.push_back(inst);
		}
		else
		{
			//execute them uniquely
			if(inst.prog=="cd")
			{
				string dir=inst.params[1];
				string home=getvar("HOME");
				size_t n= dir.find("~");
				if(n!=string::npos)
				{
					//cout<<n;
					dir.replace(n,1,home.c_str());			
				}
				chdir(dir.c_str());
			}
			else if(inst.prog=="alias")
			{
				string old,newone,full;
				for(int i=1;i<inst.params.size();i++)
				full=full+inst.params[i]+" ";
				stringstream X(full.c_str());
				getline(X,old,'=');
				getline(X,newone,'\0');
				setvar(old,newone);
				excepts.push_back(old);

			}
			else if(inst.prog=="PS1")
			{
				setvar("PS1",inst.params[1]);
			}
			else if(inst.prog=="echo")
			{
				if(inst.params[1]!="$$"){
					instruction_list.push_back(inst);
				}
				else{
					int pid=(int)getpid();
					cout<<pid<<"\n";
				}
			}
			else if(inst.prog=="history")
			{
				print_history();
			}
			else if(inst.prog=="jobs")
			{
				for(map<int,string>:: iterator i=bjob.begin();i!=bjob.end();i++)
				{
					cout<<i->first<<" "<<i->second<<"\n";
				}
			}
			else if(inst.prog=="alarm")
			{

			}
			else{	// it would mean an alias variable is encountered
				string newval=getvar(inst.prog);
				input.replace(input.begin(),input.begin()+newval.size(),newval);
				vins ins_list=split_input(input);
				return ins_list;
			}
		}
	}
	return instruction_list;
}

int execute(vins instructions){
	
	//if no commands
	if(instructions.size()==0) return 0;
	//cout<<"parent "<<getpgrp()<<"\n";
	//if only one cmd,execute it
	if(instructions.size()==1){
			//handel for redirection operator
			instructions[0].fds[1]=1;
			for(int i=1;i<instructions[0].params.size();i++)
			{
				if((instructions[0].params[i]==">")||(instructions[0].params[i]==">>"))
				{
					string filename=instructions[0].params[i+1];
					string op=instructions[0].params[i];
					instructions[0].params.erase(instructions[0].params.begin()+i+1);
					instructions[0].params.erase(instructions[0].params.begin()+i);
					//open file 
					if(op==">")
					instructions[0].fds[1]=open(filename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0777);
					if(op==">>")
					instructions[0].fds[1]=open(filename.c_str(),O_WRONLY|O_CREAT|O_APPEND,0777);
				}
			}

			//build the args
			char *argv[1000];
			int j;
			for(j=0;j<instructions[0].params.size();j++){
				argv[j]=new char[instructions[0].params[j].size()+1];
				strcpy(argv[j],instructions[0].params[j].c_str());
			}
			argv[j]=(char*)NULL;

			//execute it as a child
			pid_t pid = fork();  
		    if (pid == 0) { 
				if(instructions[0].fds[1]!=1){
				dup2(instructions[0].fds[1],1);
				close(instructions[0].fds[1]);
				}
				if(instructions[0].bg_flag==1)
				{
					setpgid(0,0);
					int devnullfd=open("/dev/null",O_CREAT);
					dup2(devnullfd,1);
					close(devnullfd);
					//bjob.insert({job_count++,instructions[0].prog});
				}
		        if (execvp(argv[0], (char* const*)argv) < 0) { 
		        	perror("error");
		            return -1;
		        } 
		        exit(0); 
		    }
			wait(NULL); 
	}
	else{ //logic for PIPED commands
		int count=instructions.size();
		int (*pipes)[2]=new int[count-1][2];

		//set up pipe connections beforehand
		instructions[0].fds[0]=0;	//first program to take input from stdin
		for(int i=0;i<count-1;i++){
			
			if(pipe(pipes[i])==-1)
			{
				perror("pipe");
			}
			instructions[i].fds[1]=pipes[i][1];
			instructions[i+1].fds[0]=pipes[i][0];			
		}
		instructions[count-1].fds[1]=1;  //for final output to stdout
		//check for redirection and handle seperately

		for(int i=0;i<count;i++){
			//build args for each command
			char *argv[1000];
			int j;
			for(j=0;j<instructions[i].params.size();j++){
				argv[j]=new char[instructions[i].params[j].size()+1];
				strcpy(argv[j],instructions[i].params[j].c_str());
			}

			argv[j]=(char*)NULL;

			pid_t pid = fork();  

		    if (pid == 0) { //in child
				//make pipe connections as actual input and output
				
				if(instructions[i].fds[0]!=0){
				dup2(instructions[i].fds[0],0);	
				close(instructions[i].fds[0]);
				}
				if(instructions[i].fds[1]!=1){
				dup2(instructions[i].fds[1],1);
				close(instructions[i].fds[1]);
				}
				for(i=0;i<count-1;i++){
					close(pipes[i][1]);
					close(pipes[i][0]);
				}
				if((instructions[i].bg_flag==1)&&(i==count-1))
				{
					setpgid(0,0);
					int devnullfd=open("/dev/null",O_NONBLOCK);
					dup2(devnullfd,1);
					close(devnullfd);
					//bjob.insert({job_count++,instructions[i].prog});
				}
		        execvp(argv[0], (char* const*)argv);
				perror("error");
				for(int i=0;i<instructions[i].params.size();i++)
				//delete[] argv;
				
				abort();
		    }
	    }
		for(int i=0;i<count-1;i++){
			close(pipes[i][1]);
			close(pipes[i][0]);
			wait(NULL);
		}
		wait(NULL);
		//free(pipes);		
	}
    return 0;
}
