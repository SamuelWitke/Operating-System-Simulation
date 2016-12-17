#ifndef PCB_H
#define PCB_H

class PCB
{
	//friend std::ostream& operator<<(std::ostream & os, const PCB &other);
	private:
		int num,prio,size,maxCpu,timeRemain,address,startIntTime,IOjobcount, CurrentRQ;
		bool inCore,blocked,latched,running,terminate;
	public:
		PCB();
		PCB(const int *data);
		PCB &operator = (const PCB &pcb);
		void setAddress(const int addr);
		void setBlocked(const bool);
		void setTimeRemain(const int);
		void setLatched(const bool);
		void setStartIntTime(const int);
		void setRunning(const bool);
		void setTerminate(const bool);
		void IOCountIN();
		void IOCountDec();
		void setCurrentReadyQ(const int);
		bool Blocked()const;
		int getAddress()const;
		int getIOJobCount()const;
		int getSize()const;
		int getNum()const;
		int getMaxCpu()const;
		int getTimeRemain()const ;
		int getStartIntTime()const;
		int getPriorty()const;
		int getCurrentQ()const;
		bool InCore()const;
		bool Latched()const;
		bool Running()const;
		bool Terminate()const;
};
#endif

void PCB::IOCountIN()
{
	IOjobcount++;
}

void PCB::IOCountDec()
{
	IOjobcount--;
}

void PCB::setTerminate(const bool val)
{
	terminate = val;
}

void PCB::setRunning(const bool val)
{
	running = val;
}

void PCB::setStartIntTime(const int val)
{
	startIntTime = val;
}

void PCB::setLatched(const bool val)
{
	latched =val;
}

void PCB::setBlocked(const bool val)
{
	blocked = val;
}


void PCB::setTimeRemain(const int amt)
{
	timeRemain -= amt;
}

void PCB::setAddress(const int addr)
{
	address = addr;
	inCore = true;
}

void PCB::setCurrentReadyQ(int x){

    CurrentRQ = x;
}

int PCB::getIOJobCount()const
{
	return IOjobcount;
}

bool PCB::Terminate()const
{
	return terminate;
}

int PCB::getStartIntTime()const
{
	return startIntTime;
}

bool PCB::Running()const
{
	return running;
}

bool PCB::Latched()const
{
	return latched;
}

bool PCB::Blocked()const
{
	return blocked;
}

int PCB::getTimeRemain()const
{
	return timeRemain;
}

int PCB::getAddress()const
{
	return address;
}

int PCB::getSize()const
{
	return size;
}

int PCB::getNum()const
{
	return num;
}

bool PCB::InCore()const
{
	return inCore;
}

int PCB::getMaxCpu()const
{
	return maxCpu;
}

int PCB::getPriorty()const
{
    return prio;
}

int PCB::getCurrentQ()const{

    return CurrentRQ;
}

/*
* Used for debugging
std::ostream &operator<<(std::ostream &os,const PCB &other)
{
	os<<"Job Size"<<other.size;
	os<<" Addr "<<other.address;
	return os;
}
*/

// Initial Constructor
PCB::PCB()
{
	num = 0;
	prio = -1;
	size = 0;
	maxCpu = 0;
	timeRemain = 0;
	address = -1;
	IOjobcount =0;
	inCore = false;
	blocked = false;
	latched = false;
	running = false;
	terminate = false;
	startIntTime=0;
}

PCB::PCB(const int *data)
{
	num  =  data[1];
	prio = data[2];
	size = data[3];
	maxCpu = data[4];
	inCore = false;
	blocked = false;
	latched = false;
	running = false;
	timeRemain=maxCpu;
	terminate = false;
	startIntTime=0;
	IOjobcount =0;
}

PCB &PCB::operator=(const PCB &pcb)
{
	num = pcb.num;
	prio = pcb.prio;
	size = pcb.size;
	maxCpu = pcb.maxCpu;
	inCore = pcb.inCore;
	blocked = pcb.blocked;
	latched = pcb.latched;
	running = pcb.running;
	timeRemain=pcb.timeRemain;
	startIntTime=pcb.startIntTime;
	terminate = pcb.terminate;
	IOjobcount =pcb.IOjobcount;
	return *this;
}
