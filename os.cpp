#include "PCB.h"
#include <map>
#include <queue>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iostream>

static const int TIMESLICE [6] = {0,20,40,60,80,1000}; //last queue is FCFS, queue 0 is omitted
static std::map<int,PCB*> Jobtable; 	// Map of jobs (Job Number)->Process Control Block
static std::deque<int> IOqueue; 	   // Queue of I/O requests
static std::vector<std::pair<int,int> > FSTable;   // Size,Address (Best Fit Free Space Table)
static std::deque<std::pair<int,int> > Drumqueue; // Queue of Jobs on Drum to be put in memory job size job num
static std::vector<std::deque<int> > readyQueue (6);	   // Ready Queue of jobs ready to be processed
static bool DrumFlag; 			  	  // Used in Memory Manager as a critical section flag
static int jobRunning;            // Used in to find running job
static int jobSwapping;          // Used to find job begin placed into memory

// Used in free Space to sort by size
bool pairCompare(const std::pair<int,int>,const std::pair<int,int>);

// Free Space manager used to handle finding address for job on free space
int freeSpace(int);

// Helper Function used to put job in memory
void memoryManager();

// Helper Function used to find the best job
int scheduler();

//Helper Function used to run job selected by scheduler
void dispatcher(int&,int*);

// Helper Function used to join adjacent jobs (one case)
bool consolidate(const std::pair<int,int>);

// Helper Function used to remove a job
void terminate(int);

// Helper Function used to start I/O disk transfer
void startIO(int);

// Handles Job Time remaining if interrupted
void bookKeeper(int);

// Send Job Number to see if job in jobtable
bool find(int);

// Used adjoin adjacent jobs (all cases)
void consolidateAll();

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
	DrumFlag=true;
	ontrace();
	jobRunning=0;
	jobSwapping=0;
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
	Drumqueue.push_back(std::make_pair(p[3],p[1]));		 // Add Job to Drum Queue
	memoryManager(); 				// Handle memory for job
	dispatcher(a,p); 			   // Call to run job
}

/*
 * Indicates Disk interrupt.
 * An I/O transfer between the disk and memory has been completed.
 * Mark job currently finished as unlatched.
 * Find another job to perform I/O.
 * If job completed all IO transfers mark job unblocked.
 * If job marked for termination, remove job.
 * p[5] = current time.
*/
void Dskint(int &a, int p[])
{
	bookKeeper(p[5]);
	PCB *job = Jobtable[IOqueue.front()];
	job->setIOJobCount(job->getIOJobCount()-1);
	job->setLatched(false);
	IOqueue.pop_front();
	if(!IOqueue.empty())// Test if any IO jobs left
		siodisk(IOqueue.front());
	if(job->getIOJobCount() <= 0){
		job->setBlocked(false);
		readyQueue[job->getPriorty()].push_back(job->getNum()); //is it more efficient to set timeSlice here?
		if(job->Terminate())
			terminate(job->getNum());
	}
	memoryManager();
	dispatcher(a,p);
}

/*
 * Indicates Drum interrupt.
 * A transfer between the drum and memory has completed a swap (In/Out)
 * Find best job to put into memory
 * Set the flag in memory manager to show drum is not busy
 * p[5] = current time.
 */
void Drmint(int &a, int p[])
{
	bookKeeper(p[5]);
	std::sort(Drumqueue.begin(),Drumqueue.end(), pairCompare);
	/*
	// Used in Debugging
	for(std::deque<std::pair<int,int> >::iterator it=Drumqueue.begin();it != Drumqueue.end();++it)
		std::cout<<"JOB NUM "<<it->second<<" SIZE "<<it->first<<std::endl;
	*/
	PCB *job=Jobtable[jobSwapping];
	DrumFlag=true; // Drum Critical Section ready to be opened
	if(job->InCore()){
		int priority = job->getPriorty();
        readyQueue[priority].push_back(job->getNum());
    }
	memoryManager();
	dispatcher(a,p);
}

/*
 * Indicates IntervalTimer's resister has decremented to 0.
 * Either move job back to ready queue or terminate
*/
void Tro(int &a, int p[])
{
	bookKeeper(p[5]);
	PCB *job=Jobtable[jobRunning];
	job->setTimeRemain(TIMESLICE[job->getCurrentQ()]); // Decrement the time remaining by the time slice
	if(job->getTimeRemain() <= 0) // Terminate Job if no time processed remaining
		terminate(job->getNum());
	else if(job->getTimeRemain() < TIMESLICE[job->getCurrentQ()]){//Handle Case where time remaining is less than time slice
		p[4] = job->getTimeRemain();
		a=2;
	}
	memoryManager();
	dispatcher(a,p);
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
	PCB *job = Jobtable[jobRunning];
	switch(a){
		case 5: terminate(job->getNum());
				break;
		case 6: startIO(job->getNum());
				break;
		case 7:	if(!IOqueue.empty()){
				  job->setBlocked(true);
				  //removes blocked process from ready queue, what if process is not in front, implement find
				  readyQueue[job->getCurrentQ()].pop_front();
				}
				break;
	}
	memoryManager();
	dispatcher(a,p);
}

/*
	  Helper Functions
	  ================
*/

// Return true if job found
bool find(int job)
{
	std::map<int,PCB*>::iterator it=Jobtable.find(job);
	return(it == Jobtable.end()? false : true);
}

// Handle Cases where Job is interrupted and time needs to be updated
void bookKeeper(int time)
{
	// Definitely needs to be cleaned up
	int amt=0;
	if(!find(jobRunning))
		return;
	PCB *job= Jobtable[jobRunning];
	if(!job->Running() || job->Blocked() || job->Terminate()){ // Cases where job cant run
		job->setStartIntTime(time);
		return;
	}
	amt = time - job->getStartIntTime();
	if(amt == TIMESLICE[job->getCurrentQ()])
		return;
	job->setTimeRemain(amt); // Update Time Remaing
}

// Sorting algorithm used in std::sort
bool pairCompare(const std::pair<int,int> firstElem, const std::pair<int,int>secondElem)
{
  return firstElem.first < secondElem.first;
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
                address=it->second;
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
 * Test All Jobs on Freespace Table
 * To see if free space can be combined
*/
void consolidateAll()
{
	for(std::vector< std::pair<int,int> >::iterator it = FSTable.begin(); it != FSTable.end(); ++it){
		if(consolidate((std::make_pair(it->first,it->second)))){
			FSTable.erase(it);
			return;
		}
	}
}

/*
 * Memory Manager
 * Handle Drum Queue For Job to be placed in core
 * Handle if enough memory move job
 * Handle case where not enough memory for job therefore take out a job (Not implemented)
*/
void memoryManager()
{
	consolidateAll();
	std::sort(FSTable.begin(), FSTable.end(), pairCompare);
	/*
	std::cout<<"FREE SPACE"<<std::endl;
	for(std::vector< std::pair<int,int> >::iterator it = FSTable.begin(); it != FSTable.end(); ++it)
		std::cout<<"size "<<it->first<<" address "<<it->second<<std::endl;
	*/
	/*
	Used in Debugging to see how many jobs need to be put into memory
	std::deque<int> temp = Drumqueue;
	while( ! temp.empty()){
		std::cout<<" Job on DQue "<<temp.front()<<std::endl;
		temp.pop_front();
	}
	*/
    if(Drumqueue.empty())
        return;
    PCB *job= Jobtable[Drumqueue.front().second];
	if(DrumFlag){
    	int address= freeSpace(job->getSize());
    	if(address != -1){
         	siodrum(job->getNum(),job->getSize(),address,0);// Allocate from drum to memory
         	job->setAddress(address);
			jobSwapping = job->getNum();
         	Drumqueue.pop_front();
		 	DrumFlag=false;
		}else{
    		//(Using some Alog) swap from memory to drum
		}
    }
}

void dispatcher(int &a,int* p)
{
	if(find(scheduler())){
		PCB *job= Jobtable[scheduler()];
		//std::cout<<"RUNNING job"<<job->getNum()<<std::endl;
		p[2]= job->getAddress();
		p[3]= job->getSize();
		job->setStartIntTime(p[5]);

		if(job->getTimeRemain() < TIMESLICE[job->getCurrentQ()])//will tell us what time slice to use
			p[4] = job->getTimeRemain();
		else
			p[4]= TIMESLICE[job->getCurrentQ()];
		job->setRunning(true);
		jobRunning = job->getNum();
		//std::cout<<"TIME :"<<job->getTimeRemain()<<std::endl;
		a=2; // set CPU to run mode
	}
	else{
		a=1;
	}
}

/*
int getNextJob()
{
	if(readyQueue.size() > 1){ // Case test when to get new job
		for(std::deque<int>::iterator it=readyQueue.begin();it != readyQueue.end(); ++it){
			PCB *job= Jobtable[*it];
			//std::cout<<"Examine job "<<job->getNum()<<std::endl;
			if(!job->Blocked() && !job->Terminate()){
				return job->getNum();
			}
		}
	}
	return -1;
}
*/

//finds which ready queue is not empty
int findReadyQ(){
	int x = 1;
    while(readyQueue[x].empty()&& x < 6)
            x++;
    if(x == 6) x = 5;//if x = 6 then all queues are empty. return 6 to mark all empty queues
    return x;
}


/*
 * Using some Algorithm select job to run
*/
int scheduler()
{
	/*
	std::cout<<" READY QUEUE "<<std::endl;
	for(std::deque<int>::iterator it = readyQueue.begin();it!= readyQueue.end();++it)
		std::cout<<*it<<std::endl;
	*/
	PCB* job;
	int jobToRun=-1;
	int x = findReadyQ(); //tells you which queue to get process from
	if(find(readyQueue[x].front()) && !readyQueue[x].empty()){ // Get most recent job on ready queue
		job= Jobtable[readyQueue[x].front()];
		job->setCurrentReadyQ(x);
    //Finding which readyQueue to use

    /*
		if(job->Blocked() || job->Terminate()){ // Cases where job Can't Run
			readyQueue.pop_front();
			//job->setStartIntTime(time);
			readyQueue.push_back(job->getNum());
			job->setRunning(false);
			jobToRun=getNextJob();
		}else
		*/
		jobToRun = job->getNum();
	}
	return jobToRun;
}

/*
 *	Used to join adjacent job in memory
*/
bool consolidate(const std::pair<int,int> it)
{
	int temp=0;
    for(std::vector<std::pair<int,int> >::iterator itr=FSTable.begin();itr != FSTable.end(); ++itr){
        if(it.first + it.second == itr->second){
            temp = itr->first;
            FSTable.erase(itr);
            FSTable.push_back(std::make_pair(it.first+temp,it.second));
            return true;
        }
    }
    return false;
}

/*
 * Terminate
 * Handle Cases where job is currently doing I/O maked for termination
 * Free Job Table entry
 * Free Free Space entry
*/
void terminate(int jobNum)
{
	std::map<int,PCB*>::iterator it=Jobtable.find(jobNum);
	if(find(jobNum)){
		PCB *job=it->second;
		if(!job->Blocked() && !job->Latched()){ // Job not blocked or running I/O and can be deleted
			std::pair<int,int> p = std::make_pair(job->getSize(),job->getAddress());
            if(!consolidate(p))
				FSTable.push_back(std::make_pair(job->getSize(),job->getAddress()));
			readyQueue.erase(std::remove(readyQueue.begin(),readyQueue.end(),jobNum),readyQueue.end());	//???
			delete it->second;
			Jobtable.erase(it);
		}else{
			it->second->setTerminate(true);
			readyQueue.erase(std::remove(readyQueue.begin(),readyQueue.end(),jobNum),readyQueue.end());
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
void startIO(int jobNum)
{
 	IOqueue.push_back(jobNum);
	Jobtable[jobNum]->setLatched(true);
	Jobtable[jobNum]->setIOJobCount(Jobtable[jobNum]->getIOJobCount()+1);
/*
	std::cout<<"IO QUEUE"<<std::endl;
	for(std::deque<int>::iterator it = IOqueue.begin();it!= IOqueue.end();++it)
		std::cout<<*it<<std::endl;
*/
	if(IOqueue.size() == 1)
		siodisk(IOqueue.front());
}
