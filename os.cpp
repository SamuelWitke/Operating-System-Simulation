#include "PCB.h"
#include <map>
#include <queue>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iostream>

static std::map<int,PCB*> Jobtable; 	// Map of jobs (Job Number)->Process Control Block
static std::deque<int> IOqueue; 	   // Queue of I/O requests
static std::vector<std::pair<int,int> > FSTable; // Size,Address (Best Fit Free Space Table)
static std::deque<PCB*> Drumqueue;		 // Queue of Jobs on Drum to be put in memory
static std::deque<int> readyQueue;		// Ready Queue of jobs ready to be processed

// Helper Functions used in memoryManager()
bool pairCompare(const std::pair<int,int>,const std::pair<int,int>); 
int freeSpace(int);

// Helper Function used to put job in memory 
void memoryManager();

// Helper Function used to run a job
void scheduler(int &,int *);

// Helper Function used to join adjacent  jobs
bool consolidate(const std::map<int,PCB*>::iterator);

// Helper Function used to remove a job
void terminate(int);

// Helper Function used to start I/O disk tranfer
void startIO(int jobNum);

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
	FSTable.push_back(std::make_pair(100,0));
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
*/
void Crint(int &a, int p[]) 
{
	PCB *pcb = new PCB(p);
	Jobtable[p[1]] = pcb; 	  // Add to Jobtable (Job Number)->Process Control Block
	Drumqueue.push_back(pcb);// Add to Drum Queue
	memoryManager(); // Handle memory for job
	scheduler(a,p); // Call to run job 
}

/*
 * Indicates Disk interrupt.
 * An I/O transfer between the disk and memory has been completed.
 * p[5] = current time.
*/
void Dskint(int &a, int p[])
{
	PCB *job = Jobtable[IOqueue.front()];
	job->setLatched(false);
	IOqueue.pop_front();
	if(!IOqueue.empty())// Test if any IO jobs left
		siodisk(IOqueue.front());
	else
		job->setBlocked(false);
	memoryManager();
	scheduler(a,p);
}

/*
 * Indicates Drum interrupt.
 * A transfer between the drum and memory has completed a swap (In/Out)
 * p[5] = current time.
 */
void Drmint(int &a, int p[])
{
	if(Jobtable[p[1]]->InCore()){
	 	readyQueue.push_back(p[1]);
		Jobtable[p[1]]->setTimeStart(p[5]);
	}
	memoryManager();
	scheduler(a,p);
}

/*
 * Indicates IntervalTimer's resister has decremented to 0.
 * Either move job back to ready queue or terminate
*/
void Tro(int &a, int p[])
{
	PCB *job=Jobtable[p[1]]; // Get most recent job on ready queue
	std::cout<<job->getMaxCpu()<<" Time remain "<<p[5] - job->getTimeStart()<<std::endl;
	if(job->getMaxCpu() <= p[5] - job->getTimeStart()){
		terminate(job->getNum());
	}
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
	// Keep track of interrupted job (if any)
	switch(a){
		case 5: terminate(p[1]); 
				break;
		case 6: startIO(p[1]);
				break;
		case 7: if(!IOqueue.empty())
					Jobtable[p[1]]->setBlocked(true);
				break;
	}
	memoryManager();
	scheduler(a,p);
}


/*
	  Helper Functions
	  ================
*/

// Sorting algorithm used in Free Space
bool pairCompare(const std::pair<int,int> firstElem, const std::pair<int,int>secondElem) 
{
  return firstElem.first < secondElem.first; // Based On Best Fit Table
}

/* 
 * Free Space Manager
 *  if Job on Drum Queue can fit into memory
 *	return address 
 *	if no more space delete entry
 *  else 
 *  	return -1;
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
 * if enough memory move job
 * else take out a job
 *
*/  
void memoryManager()
{
	std::sort(FSTable.begin(), FSTable.end(), pairCompare);
    if(Drumqueue.empty()) // return if empty
        return;
    PCB *temp= Drumqueue.front();
    int address= freeSpace(temp->getSize());	
    if(address != -1){
         siodrum(temp->getNum(),temp->getSize(),address,0);// Allocate from drum to memory
         temp->setAddress(address);
         Drumqueue.pop_front();
    }else{
    //(Using some Alog) swap from memory to drum 
    }
}

/* 
 * Scheduler 
 * handle readyQueue
 * return (Using some Alog) job to run on CPU
*/
void scheduler(int &a,int *p)
{
	std::map<int,PCB*>::iterator it=Jobtable.find(readyQueue.front()); // Get most recent job on ready queue
	if(readyQueue.empty() || it==Jobtable.end() || it->second->Blocked() ){ // Case test when to get new job
		a=1;
		return;
	}
	PCB *jobToRun= it->second;
	p[1]= jobToRun->getNum(); 
	p[2]= jobToRun->getAddress();
	p[3]= jobToRun->getSize();
	p[4]= 10;
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
 */
void startIO(int jobNum)
{
 	IOqueue.push_back(jobNum);
	siodisk(IOqueue.front());
	Jobtable[jobNum]->setLatched(true);
}
