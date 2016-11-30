#include "PCB.h"
#include <map>
#include <queue>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iostream>

static const int TIMESLICE=10; // Predetermined Time Slice to Handle RR scheduling 

static std::map<int,PCB*> Jobtable; 	// Map of jobs (Job Number)->Process Control Block
static std::deque<int> IOqueue; 	   // Queue of I/O requests
static std::vector<std::pair<int,int> > FSTable; // Size,Address (Best Fit Free Space Table)
static std::deque<int> Drumqueue;		 // Queue of Jobs on Drum to be put in memory
static std::deque<int> readyQueue;		// Ready Queue of jobs ready to be processed
static bool csFlag; 			  // Used in Memory Manager as a critical section flag 
static int jobRunning =0;        // Used in bookKeeper to find running job

// Used in free Space to sort by size  
bool pairCompare(const std::pair<int,int>,const std::pair<int,int>); 

// Free Space manager used to handle finding address for job on free space
int freeSpace(int);

// Helper Function used to put job in memory 
void memoryManager();

// Helper Function used to run a job
void scheduler(int &,int *);

// Helper Function used to join adjacent  jobs
bool consolidate(const std::map<int,PCB*>::iterator);

// Helper Function used to remove a job
void terminate(int);

// Helper Function used to start I/O disk transfer  
void startIO(int,int);

// Handles Job Time remaining if interrupted 
void bookKeeper(int);

/*
	  SOS Functions
 	  ==============
*/

/*
 * Invoked by OS to start disk data transfer.
*/
void siodisk(int jobnum);

/* 
 * Invoked by OS to start drum data transfer.
 * 1 => move from core (memory) to drum
 * 0 => move from drum to core (memory)
 */
void siodrum(int jobnum, int jobsize, int coreaddress,bool direction);

 /* 
  * The 2 trace procedures allow you to turn the tracing mechanism on and off.
  * The default value is off. WARNING: ontrace produces a blow-by-blow description of each event and results in an extremely large amount of output.
  * It should be used only as an aid in debugging
  * Even with the trace off, performance statistics are
  * generated at regular intervals and a diagnostic message appears in case of a crash.
  * In either case, your OS need not print anything.
 */
void ontrace(); 
void offtrace();

/*
 * Allows initialization of static system variables declared above.
 * Called once at start of the simulation.
*/
void startup()
{
	// Size(100) - Address(0)
	FSTable.push_back(std::make_pair(99,0));
	csFlag=true;
	ontrace();
	//offtrace();
}

/*  
 *  Indicates the arrival of a new job on the drum.
 *  p[1] = Job number.
 *  p[2] = Priority.
 *  p[3] = Job size, K bytes.
 *  p[4] = Time Slice
 *  p[5] = Current time.
 *  New information is saved and store in Jobtable 
 *  Job placed on Drum Queue to be handled by memoryManager()
*/
void Crint(int &a, int p[]) 
{
	bookKeeper(p[5]);
	Jobtable[p[1]] = new PCB(p); 	  // Add to Jobtable (Job Number)->Process Control Block
	Drumqueue.push_back(p[1]);		 // Add Job to Drum Queue
	memoryManager(); 				// Handle memory for job
	scheduler(a,p); 			   // Call to run job 
}

/*
 * Indicates Disk interrupt.
 * An I/O transfer between the disk and memory has been completed.
 * Mark job currently finished as unlatched 
 * Find another job to perform I/O
 * if no jobs to perform I/O on mark job unblocked
 * p[5] = current time.
*/
void Dskint(int &a, int p[])
{
	bookKeeper(p[5]);
	PCB *job = Jobtable[IOqueue.front()];
	IOqueue.pop_front();
	if(!IOqueue.empty())// Test if any IO jobs left
		siodisk(IOqueue.front());
	else{
		job->setBlocked(false);
		job->setLatched(false);
	}
	readyQueue.push_back(p[1]);
	memoryManager();
	scheduler(a,p);
}

/*
 * Indicates Drum interrupt.
 * A transfer between the drum and memory has completed a swap (In/Out)
 * Set the flag in memory manager to show drum is not busy
 * p[5] = current time.
 */
void Drmint(int &a, int p[])
{
	bookKeeper(p[5]);
	PCB *job = Jobtable[p[1]];
	csFlag=true; // Drum Critical Section ready to be opened 
	if(job->InCore())
		readyQueue.push_back(p[1]);
	memoryManager();
	scheduler(a,p);
}

/*
 * Indicates IntervalTimer's resister has decremented to 0.
 * Either move job back to ready queue or terminate
*/
void Tro(int &a, int p[])
{
	bookKeeper(p[5]);
	PCB *job=Jobtable[p[1]];
	job->setTimeRemain(TIMESLICE); // Decrement the time remaining by the time slice
	if(job->getTimeRemain() <= 0) // Terminate Job if no time processed remaining 
		terminate(job->getNum());
	else if(job->getTimeRemain() < TIMESLICE){//Handle Case where time remaining is less than time slice
		p[4] = job->getTimeRemain();
		a=2;
	}else
		readyQueue.push_back(p[1]);
	memoryManager();
	scheduler(a,p);
}

/*
 * Indicates the executing job invoked software interrupt instruction.
 * a == 5 -> termination
 * a == 6 -> I/O operation
 * a == 7 -> Blocked until all pending I/O requests are filled
 */
void Svc(int &a, int p[])
{
	bookKeeper(p[5]);
	PCB *job = Jobtable[p[1]]; 
	switch(a){
		case 5: terminate(p[1]); 
				break;
		case 6: startIO(p[1],p[5]);
				readyQueue.push_back(p[1]);
				break;
		case 7:	
				if(!IOqueue.empty())
					job->setBlocked(true);
				readyQueue.push_back(p[1]);
				break;
	}
	memoryManager();
	scheduler(a,p);
}


/*
	  Helper Functions
	  ================
*/

// Handle Cases where Job is interrupted and time needs to be updated
void bookKeeper(int time)
{
	// Definitely needs to be cleaned up
	int amt=0;
	std::map<int,PCB*>::iterator job=Jobtable.find(jobRunning); // Get most job running
	if(job == Jobtable.end())
		return; 
	if(!job->second->Running()){
		job->second->setStartIntTime(time);	
		return;
	}
	amt = time - job->second->getStartIntTime();
	if(amt == TIMESLICE)
		return;
	job->second->setTimeRemain(amt);
}

// Sorting algorithm used in Free Space
bool pairCompare(const std::pair<int,int> firstElem, const std::pair<int,int>secondElem) 
{
  return firstElem.first < secondElem.first; // Based On Best Fit Table
}

/* 
 * Free Space Manager
 * Handle if Job on Drum Queue can fit into memory return address 
 * Handle if no more space delete entry
*/
int freeSpace(int jobSize)
{
	int address;
	for(std::vector< std::pair<int,int> >::iterator it = FSTable.begin(); it != FSTable.end(); ++it){
 		if(it->first - jobSize >=0){     // Based on Best Fit Table 
            if(it->first - jobSize==0){ // Handle deleted entry
                address=it->second + jobSize;
                FSTable.erase(it);
                return address;
            }
	    	address=it->second;   // Get starting address
            it->second+=jobSize; // Update Free Space Table
            it->first-=jobSize; // ^^^^^^^^^^^^^^^^^^^^^^^^
            return address;
		}        
	}
	return -1;
}
			
/*
 * Memory Manager
 * Handle Drum Queue For Job to be placed in core
 * Handle if enough memory move job
 * Handle case where not enough memory for job therefore take out a job
*/  
void memoryManager()
{
	std::sort(FSTable.begin(), FSTable.end(), pairCompare);
	/*
	Used in Debugging to see how many jobs need to be put into memory
	std::deque<int> temp = Drumqueue;
	while( ! temp.empty()){
		std::cout<<" Job on DQue "<<temp.front()<<std::endl;
		temp.pop_front();
	}
	*/
    if(Drumqueue.empty()) // return if empty
        return;
    PCB *job= Jobtable[Drumqueue.front()];
	if(csFlag){
    	int address= freeSpace(job->getSize());	
    	if(address != -1){
         	siodrum(job->getNum(),job->getSize(),address,0);// Allocate from drum to memory
         	job->setAddress(address);
         	Drumqueue.pop_front();
		 	csFlag=false;
		}else{
			std::cout<<"NOT enough MEMOry"<<std::endl;
    		//(Using some Alog) swap from memory to drum 
    	}
	}
}

/* 
 * Scheduler 
 * handle readyQueue
 * Using some Algorithm job to run on CPU
 * handle cases where job is unable to run
 * handle cycle of jobs running on CPU
*/
void scheduler(int &a,int *p)
{
	// THIS IS NOT FULLY CORRECT JUST MADE TO TEST OTHER CASES
	// Needs to be cleaned up
	std::map<int,PCB*>::iterator it=Jobtable.find(readyQueue.front()); // Get most recent job on ready queue
	if(it == Jobtable.end()){
		a=1;
		return;
	}

	if(readyQueue.empty() || it->second->Blocked() ){ // Case test when to get new job
		a=1;
		it->second->setRunning(false);
		jobRunning=0;
		return;
	}

	PCB *job= it->second;	
	p[1]= job->getNum(); 
	p[2]= job->getAddress();
	p[3]= job->getSize();
	job->setStartIntTime(p[5]);
	if(job->getTimeRemain() < TIMESLICE)
		p[4] = job->getTimeRemain();
	else
		p[4]= TIMESLICE;
	job->setRunning(true);
	readyQueue.pop_front();
	jobRunning = p[1];
	std::cout<<"TIME :"<<job->getTimeRemain()<<std::endl;
	a=2; // set CPU to run mode 
}

/*
 *	Used in terminate() to join adjacent jobs in memory
*/ 
bool consolidate(const std::map<int,PCB*>::iterator it)
{
	for(std::vector<std::pair<int,int> >::iterator itr=FSTable.begin();itr != FSTable.end(); ++itr){
			if(it->second->getSize() + it->second->getAddress() == itr->second){
				FSTable.erase(itr);
				FSTable.push_back(std::make_pair(it->second->getSize()+itr->first,it->second->getAddress()));
				return true;
			}
	}
}

/*
 * Terminate 
 * Free Job Table entry
 * Free Free Space Entry 
*/
void terminate(int jobNum)
{
	std::map<int,PCB*>::iterator it=Jobtable.find(jobNum);
	if(it!=Jobtable.end()){
		if(!(it->second->Blocked()) && !it->second->Latched()){ // Job not blocked and can be deleted
			if(!consolidate(it))
				FSTable.push_back(std::make_pair(it->second->getSize(),it->second->getAddress()));
			readyQueue.erase(std::remove(readyQueue.begin(),readyQueue.end(),jobNum),readyQueue.end());	
			delete it->second;
			Jobtable.erase(it);	
		}
	}
}

 /*
  * Handle I/O 
  * Base on SVC request
  * Place on diskQueue
  * Preform Disk Opp
  * Job running I/O is Latched
 */
void startIO(int jobNum,int time)
{
 	IOqueue.push_back(jobNum);
	siodisk(IOqueue.front());
	Jobtable[IOqueue.front()]->setLatched(true);
}
