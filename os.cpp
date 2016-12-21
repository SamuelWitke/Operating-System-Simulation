#include "PCB.h"
#include <map>
#include <queue>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iostream>

static const int TIMESLICE [6] = {0,1500,2500,3500,4500,6500}; //Last queue is FCFS, queue 0 is omitted
static std::map<int,PCB*> Jobtable;     // Map of job (Job Number)->Process Control Block
static std::deque<int> IOqueue;                               // Queue of I/O requests
static std::vector<std::pair<int,int> > FSTable;             // Size,Address (Best Fit Free Space Table)
static std::deque<std::pair<int,int> > Drumqueue;           // Queue of Jobs on Drum to be put in memory job size job num
static std::vector<std::deque<int> > readyQueue(6);       // Ready Queue of jobs ready to be processed
static bool DrumFlag;              // Used in Memory Manager as a critical section flag
static int jobRunning;            // Used in to find running job
static int jobSwapping;          // Used to find job begin placed into memory
static int jobBlocked;          // Used in swapper to swapped blocked job
static int jobIO;              // Used in diskint() and startIO for tracking IO job

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

// Swap a job out of memory when too big and blocked
void swapper();

// Retrieves the next IO job based on small cpu time
int getIOJob();

// Returns smallest val from map
int getMapMin(std::map<int,int>);

// Find Lv of job on readyQueue
int findReadyQ();

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
	jobBlocked = -1;
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
    Jobtable[p[1]] = new PCB(p);      // Add to Jobtable (Job Number)->Process Control Block
    Drumqueue.push_back(std::make_pair(p[4],p[1]));          // Add Job to Drum Queue (CPU size,number)
    memoryManager();                                		// Handle memory for job
    dispatcher(a,p);                           			   // Call to run job
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
    PCB *job = Jobtable[jobIO];
    job->IOCountDec();
    job->setLatched(false);
    std::deque<int>::iterator it =
	std::find(IOqueue.begin(),IOqueue.end(),job->getNum());
	if(it != IOqueue.end())
		IOqueue.erase(it);
    if(!IOqueue.empty()){
		jobIO=getIOJob();
        siodisk(jobIO);
	}
    if(job->getIOJobCount() == 0){
    	job->setBlocked(false);
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
 * Place job on ready queue if inCore
 * p[5] = current time.
 */
void Drmint(int &a, int p[])
{
    bookKeeper(p[5]);
    int priority,jobTime,q=5;
    std::sort(Drumqueue.begin(),Drumqueue.end(),pairCompare);
    PCB *job=Jobtable[jobSwapping];
    DrumFlag=true;
    if(job->InCore()){
        //find ready queue according to Max CPU time
        jobTime = job->getMaxCpu();
        for(int i= 1; i < 5;i++)
            if(jobTime < TIMESLICE[i]){
                q = i;
                break;
            }
        readyQueue[q].push_back(job->getNum());
        job->setCurrentReadyQ(q);
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
    job->setTimeRemain(TIMESLICE[job->getCurrentQ()]);
    if(job->getTimeRemain() <= 0)
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
 * Handle if big job is blocked should be swapped to disk
 */
void Svc(int &a, int p[])
{
    bookKeeper(p[5]);
    PCB *job = Jobtable[jobRunning];
    switch(a){
    	case 5:
        	terminate(job->getNum());
        break;
    	case 6:
        	startIO(jobRunning);
        break;
    	case 7:
        	if(job->getIOJobCount() != 0){
            	job->setBlocked(true);
				if(job->getMaxCpu() == 65000)
					jobBlocked = job->getNum();
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

// Used to compare two pairs first element
typedef std::pair<int, int> MyPairType;
struct CompareSecond
{
    bool operator()(const MyPairType& left, const MyPairType& right) const
    {
        return left.first < right.first;
    }
};

// Return smallest value from map
int getMapMin(std::map<int,int> temp)
{
	std::pair<int, int> min = *std::min_element(temp.begin(),temp.end(), CompareSecond());
	return min.second;
}

//Return next IO job based on smallest time remaining
int getIOJob()
{
	std::map<int,int> temp;
    for(std::deque<int>::iterator it = IOqueue.begin();it!= IOqueue.end();++it)
		temp[(Jobtable[*it]->getTimeRemain())] = *it;
	return getMapMin(temp);
}

// Return true if job found
bool find(int job)
{
    std::map<int,PCB*>::iterator it=Jobtable.find(job);
    return(it == Jobtable.end()? false : true);
}

// Handle Cases where Job is interrupted and time needs to be updated
void bookKeeper(int time)
{
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
        if(it->first - jobSize >=0){      // Based on Best Fit Table
            if(it->first - jobSize==0){  // Handle deleted entry
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

/* When job to be swapped is found and not latched
 * Take job out of memory back on drum queue
 * Swap job to disk
*/
void swapper()
{
	if(find(jobBlocked)){
		std::map<int,PCB*>::iterator it=Jobtable.find(jobBlocked);
    	PCB *job =it->second;
		if(!job->Latched()){
           	std::pair<int,int> p = std::make_pair(job->getSize(),job->getAddress());
           	if(!consolidate(p))
            	FSTable.push_back(std::make_pair(job->getSize(),job->getAddress()));
           	std::deque<int>::iterator v =
			std::find(readyQueue[job->getCurrentQ()].begin(),readyQueue[job->getCurrentQ()].end(),job->getNum());
           	if(v != readyQueue[job->getCurrentQ()].end())
              		readyQueue[job->getCurrentQ()].erase(v);
			siodrum(job->getNum(),job->getSize(),job->getAddress(),1);
   			Drumqueue.push_back(std::make_pair(job->getSize(),job->getNum()));
          	jobSwapping = job->getNum();
			job->setInCore(false);
       		DrumFlag=false;
			jobBlocked = -1;
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
			job->setInCore(true);
        }else
			swapper();
	}
}

/*
 * Called from every function
 * See if job is available to run
 * Else have CPU idle
*/
void dispatcher(int &a,int* p)
{
    if(find(scheduler())){
        PCB *job= Jobtable[scheduler()];
        p[2]= job->getAddress();
        p[3]= job->getSize();
        job->setStartIntTime(p[5]);
        if(job->getTimeRemain() < TIMESLICE[job->getCurrentQ()])//will tell us what time slice to use
            p[4] = job->getTimeRemain();
        else
            p[4]= TIMESLICE[job->getCurrentQ()];
        job->setRunning(true);
        jobRunning = job->getNum();
        a=2; // set CPU to run mode
    }else
        a=1;
}

/*
 * Start from Lv 1
 * test every job to see if it can run
 * if job can run
 * update currentQ
 * return job
*/
int getNextJob()
{
	for(int i=1;i<readyQueue.size();i++){
    	for(std::deque<int>::iterator it=readyQueue[i].begin();it != readyQueue[i].end(); ++it){
            PCB *job= Jobtable[*it];
           	if(!job->Blocked() && !job->Terminate())
            	  return job->getNum();
    	}
  	}
	return -1;
}

//finds which ready queue is not empty
int findReadyQ()
{
    int x = 1;
    while(readyQueue[x].empty()&& x < 6)
    	x++;
    if(x == 6) x = 5;//if x = 6 then all queues are empty. return 6 to mark all empty queues
    return x;
}

/*
 * Using some MLFQ select job to run
*/
int scheduler()
{
    PCB* job;
    int jobToRun=0;
    int x = findReadyQ(); //tells you which queue to get process from
    if(find(readyQueue[x].front()) && !readyQueue[x].empty()){// Get most recent job on ready queue
        job= Jobtable[readyQueue[x].front()];
        if(job->Blocked() || job->Terminate()){ // Cases where job Can't Run
			readyQueue[x].pop_front();
			readyQueue[x].push_back(job->getNum());
          	jobToRun=getNextJob();
       	}else
        	jobToRun = job->getNum();
    }
    return jobToRun;
}

/*
 * Used to join adjacent job in memory
*/
bool consolidate(const std::pair<int,int> it)
{
    int temp=0;
    for(std::vector<std::pair<int,int> >::iterator itr=FSTable.begin(); itr != FSTable.end(); ++itr){
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
 * Free Ready Queue entry
 * Free Free Space entry
*/
void terminate(int jobNum)
{
    std::map<int,PCB*>::iterator it=Jobtable.find(jobNum);
    if(find(jobNum)){
        PCB *job=it->second;
        if(!job->Blocked() && !job->Latched()){// Job not blocked or running I/O and can be deleted
            std::pair<int,int> p = std::make_pair(job->getSize(),job->getAddress());
            if(!consolidate(p))
                FSTable.push_back(std::make_pair(job->getSize(),job->getAddress()));
			// Remove from Ready queue
            std::deque<int>::iterator v =
			std::find(readyQueue[job->getCurrentQ()].begin(),readyQueue[job->getCurrentQ()].end(),job->getNum());
            if(v != readyQueue[job->getCurrentQ()].end())
                readyQueue[job->getCurrentQ()].erase(v);
             delete it->second;
            Jobtable.erase(it);
        }else{
            it->second->setTerminate(true);
            std::deque<int>::iterator v =
			std::find(readyQueue[job->getCurrentQ()].begin(),readyQueue[job->getCurrentQ()].end(),job->getNum());
            if(v != readyQueue[job->getCurrentQ()].end())
                readyQueue[job->getCurrentQ()].erase(v);
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
	PCB *job = Jobtable[jobRunning];
    job->setLatched(true);
    job->IOCountIN();
    IOqueue.push_back(job->getNum());
    if(IOqueue.size() == 1){
        siodisk(IOqueue.front());
		jobIO=IOqueue.front();
	}
}
