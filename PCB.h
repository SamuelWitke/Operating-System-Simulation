#ifndef PCB_H
#define PCB_H

class PCB
{
	//friend std::ostream& operator<<(std::ostream & os, const PCB &other);
	private:
		int num,prio,size,maxCpu,timeRemain,address,startIntTime;
		bool inCore,blocked,latched,running;
	public:
		PCB();
		PCB(const int *data);
		PCB &operator = (const PCB &pcb);
		// Setter Methods
		void setAddress(const int addr);
		void setBlocked(const bool);	
		void setTimeRemain(const int); 
		void setLatched(const bool);
		void setStartIntTime(const int);
		void setRunning(const bool);
		// Getter Methods
		bool Blocked()const;
		int getAddress()const;
		int getSize()const;
		int getNum()const;
		int getMaxCpu()const;
		int getTimeRemain()const ;
		int getStartIntTime()const;
		bool InCore()const;
		bool Latched()const;
		bool Running()const;
};
#endif

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

/*
* Used for debugging 
std::ostream &operator<<(std::ostream &os,const PCB &other)
{
	os<<"Job size"<<other.size;
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
	inCore = false;
	blocked = false;
	latched = false;
	running = false;
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
	startIntTime=0;
}

PCB &PCB::operator = (const PCB &pcb)
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
	startIntTime=0;
	return *this;
}
