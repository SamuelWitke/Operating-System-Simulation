#ifndef PCB_H
#define PCB_H

class PCB
{
	//friend std::ostream& operator<<(std::ostream & os, const PCB &other);
	private:
		int num,prio,size,maxCpu,timeStart,address;
		bool inCore,blocked,latched,term;
	public:
		PCB();
		PCB(const int *data);
		PCB &operator = (const PCB &pcb);
		// Setter Methods
		void setAddress(const int addr);
		void setBlocked(const bool);	
		void setTimeStart(int); 
		void setLatched(const bool);
		// Getter Methods
		bool Blocked()const;
		int getAddress()const;
		int getSize()const;
		int getNum()const;
		int getMaxCpu()const;
		int getTimeStart()const;
		bool InCore()const;
		bool Latched()const;
};
#endif

void PCB::setLatched(const bool val)
{
	latched =val;
}

void PCB::setBlocked(const bool val)
{
	blocked = val;
}


void PCB::setTimeStart(int amt)
{
	timeStart += amt;
}

void PCB::setAddress(const int addr)
{
	address = addr;	
	inCore = true;	
}

bool PCB::Latched()const
{
	return latched;
}

bool PCB::Blocked()const
{
	return blocked;
}

int PCB::getTimeStart()const
{
	return timeStart;
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
	timeStart = 0;
	address = -1;
	inCore = false;
	blocked = false;
	latched = false;
	term = false;
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
	term = false;
	timeStart=0;
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
	term = pcb.term;
	timeStart=pcb.timeStart;
	return *this;
}
