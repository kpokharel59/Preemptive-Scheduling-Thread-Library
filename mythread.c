#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>



//****** Threads Routines *********

//dynamically allocates id to each new thread
static int threadCount=0;
static int semaphoreCount=0;
static int currentThreadId=0;
static int currentThreadParent=0;
static int blockedThreadCount=0;
static int sBlockedThreadCount=0;
static ucontext_t mainInitContext;

// Semaphore strcture
typedef struct semaphore{
  int sid;
  int value;

}MySemaphore;

//******* Ready Queue ********
//Node of Ready Queue
//Store pointer to context
//Each node is a thread
typedef struct Nodes{
    ucontext_t context;
    struct Nodes *nxt;
    int threadId;
    int parentId;
    }MyThread;

//front and rear pointers of the queue
static MyThread *front;
static MyThread *rear;
 

//Insert element in the front of the Ready Queue
void push(ucontext_t context,int threadId,int parentId){
   // printf("Ready Queue Push called\n");
    MyThread *temp=(MyThread *)malloc(sizeof(MyThread));
    temp->context=context;
    temp->threadId=threadId;
    temp->parentId=parentId;
    temp->nxt=NULL;
    if(front==NULL||rear==NULL){
        front=temp;
        rear=temp;
	//ucontext_t firstCase;
	//getcontext(&firstCase);
        //firstCase.uc_link=front->context_ptr;
        //rear->context_ptr->uc_link=NULL;
    }
    else{
	//printf("second push\n");
        rear->nxt=temp;
	//rear->context_ptr->uc_link=temp->context_ptr;
        rear=temp;
    }
}

//pop elements from the Ready Queue
MyThread pop(){
  // printf(" Ready Queue Pop called\n");
    if(front==NULL){
        //printf("Queue is Empty");
        return ;
    }
    else {
     MyThread *tempFront=front;
     front=front->nxt;
     if(front==NULL)
	rear=NULL;
     return *tempFront;
    }
}


//******* Blocked Queue ********
typedef struct BNodes{
    ucontext_t context;
    struct BNodes *nxt;
    int threadId; //thread id start with 1
    int parentId;
    int blockedBy; // whenever a thread is blocked stores its blocking child Id and stores 0 in case of Join All
    }blockedThread;

static blockedThread *bfront;
static blockedThread *brear;

//Insert element in the front of the Blocked Queue
void bpush(ucontext_t context,int threadId,int parentId, int blockingChildId){
  //  printf("B Queue Push called\n");
    blockedThreadCount++;
    blockedThread *temp=(blockedThread *)malloc(sizeof(blockedThread));
    temp->context=context;
    temp->threadId=threadId;
    temp->parentId=parentId;
    temp->blockedBy=blockingChildId;
    temp->nxt=NULL;
    if(bfront==NULL||brear==NULL){
        bfront=temp;
        brear=temp;
	//ucontext_t firstCase;
	//getcontext(&firstCase);
        //firstCase.uc_link=front->context_ptr;
        //rear->context_ptr->uc_link=NULL;
    }
    else{
        brear->nxt=temp;
	//rear->context_ptr->uc_link=temp->context_ptr;
        brear=temp;
    }
}

//pop elements from the Blocked Queue with given tid
blockedThread bpop(int tid){
    //printf(" B Queue Pop called\n");
    blockedThread *tempFront;
    if(bfront==NULL){
        //printf("Queue is Empty");
        return ;
    }
    else if(bfront->threadId==tid){
	 tempFront=bfront;
         bfront=bfront->nxt;
         blockedThreadCount--;
    }
    else{
    // blockedThreadCount--;
     blockedThread *prev;
     tempFront=bfront;
     while(tempFront != NULL){
	if(tempFront->threadId==tid){
		prev->nxt=tempFront->nxt;
		break;
	 }   
	  prev=tempFront;
	  tempFront=tempFront->nxt;
     }
     }
     return *tempFront;
}


//******** Semaphore Blocked Queue ********
typedef struct SBNodes{
    ucontext_t context;
    struct SBNodes *nxt;
    int threadId; //thread id start with 1
    int parentId;
    int blockedBy; // sid of blocking semaphore
    }sBlockedThread;

static sBlockedThread *sbfront;
static sBlockedThread *sbrear;

//Insert element in the front of the Blocked Queue
void sbpush(ucontext_t context,int threadId,int parentId, int blockingChildId){
   // printf("S  B Queue Push called\n");
    sBlockedThreadCount++;
    sBlockedThread *temp=(sBlockedThread *)malloc(sizeof(sBlockedThread));
  //printf("Started  11\n");
    temp->context=context;
    temp->threadId=threadId;
    temp->parentId=parentId;
    temp->blockedBy=blockingChildId;
    temp->nxt=NULL;
  //printf("Started  12\n");
    if(sbfront==NULL||sbrear==NULL){
 // printf("first Started  1\n");
        sbfront=temp;
        sbrear=temp;
    }
    else{
  //printf("secon Started  1\n");
        sbrear->nxt=temp;
        sbrear=temp; 
    }
 // printf("end Started  1\n");
return;
}

//pop elements from the Blocked Queue with given tid
sBlockedThread sbpop(int tid){
   // printf("S  B Queue Pop called\n");
    sBlockedThread *tempFront;
    if(sbfront==NULL){
        //printf("Queue is Empty");
        return ;
    }
    else if(sbfront->threadId==tid){
	 tempFront=sbfront;
         sbfront=sbfront->nxt;
         sBlockedThreadCount--;
    }
    else{
     sBlockedThreadCount--;
     sBlockedThread *prev;
     tempFront=sbfront;
     while(tempFront != NULL){
	if(tempFront->threadId==tid){
		prev->nxt=tempFront->nxt;
		break;
	 }   
	  prev=tempFront;
	  tempFront=tempFront->nxt;
     }
     }
     return *tempFront;
}



// ****** CALLS ONLY FOR UNIX PROCESS ****** 
// Create and run the "main" thread
//called only once from the unix process at very first
//makecontext with given start_func and start by running that context
//the uc_link in this context points to the front of the ready queue that store context pointers
void MyThreadInit(void(*start_funct)(void *), void *args){
	//printf("My Thread Init called\n");
	ucontext_t mContext;
	MyThread swpContext;
	getcontext(&mContext);
	mContext.uc_stack.ss_sp=malloc(8192);
	mContext.uc_stack.ss_size=8192;
	mContext.uc_link=NULL;
	makecontext(&mContext,(void *)start_funct, 1,args);
	push(mContext,++threadCount,currentThreadId);
	swpContext=pop();
        currentThreadId=swpContext.threadId;
	currentThreadParent=swpContext.parentId;
	//printf("Hello\n");
	swapcontext(&mainInitContext,&swpContext.context);
	//printf("This should always print at last\n");

}



// ****** THREAD OPERATIONS ****** 
// Create a new thread.
MyThread* MyThreadCreate(void(*start_funct)(void *), void *args){
	//printf("My Thread Create called\n");
	ucontext_t mContext;
	getcontext(&mContext);
	mContext.uc_stack.ss_sp=malloc(8192);
	mContext.uc_stack.ss_size=8192;
	mContext.uc_link=NULL;
	makecontext(&mContext,(void(*)(void))start_funct, 1,args);
	//printf("My Thread Create before push\n");
	push(mContext,++threadCount,currentThreadId);
	//printf("My Thread Create after push  ie thread created with id=%d\n",threadCount);
	return rear;
}

// Yield invoking thread
void MyThreadYield(void){
        if(front==NULL)
	      return;
	//printf("yield start\n");
	ucontext_t mContext;
	MyThread swpContext;
        getcontext(&mContext);
	push(mContext,currentThreadId,currentThreadParent);
	//printf("valid pop");
	swpContext=pop();
        currentThreadId=swpContext.threadId;
	currentThreadParent=swpContext.parentId;
	swapcontext(&(rear->context),&swpContext.context);
}

//checks whether a given thread is terminated or not
//returns 1 if terminated otherwise 0
int childTerminted(int threadId){
       
    //checks whether the Thread exists in Thread queue
    MyThread *tempFront=front;
    while(tempFront!=NULL){
         if(tempFront->threadId==threadId)
		return 0;
	 tempFront=tempFront->nxt;
    }

    //checks whether the thread exists in Blocked Thread queue
    blockedThread *btempFront=bfront;
    while(btempFront!=NULL){
         if(btempFront->threadId==threadId)
		return 0;
	 btempFront=btempFront->nxt;
    }	

    //checks whether the thread exists in Semaphore Blocked Thread queue
    sBlockedThread *sbtempFront=sbfront;
    while(sbtempFront!=NULL){
         if(sbtempFront->threadId==threadId)
		return 0;
	 sbtempFront=sbtempFront->nxt;
    }

    return 1;
}

// Join with a child thread
int MyThreadJoin(MyThread* childThread){
	//printf("My Thread Join called\n");
	MyThread thread= *childThread;
	//printf("threadId=%d  parentId=%d  child thread=%d\n",thread.parentId,currentThreadId,thread.threadId);
	if(thread.parentId != currentThreadId)
		return -1;

	if(!childTerminted(thread.threadId)){
	ucontext_t mContext;
	MyThread swpContext;
        getcontext(&mContext);
	bpush(mContext,currentThreadId,currentThreadParent,thread.threadId);
	//printf("valid pop");
	swpContext=pop();
        currentThreadId=swpContext.threadId;
	currentThreadParent=swpContext.parentId;
	swapcontext(&(brear->context),&swpContext.context);
	}	
        return 0;
}

//checks whether any child exists or not for a given parentId
//returns 1 if any child exists otherwise 0
int childExistIfAny(int parentId){
       
    //checks whether any child exists in Thread queue
    MyThread *tempFront=front;
    while(tempFront!=NULL){
         if(tempFront->parentId==parentId)
		return 1;
	 tempFront=tempFront->nxt;
    }

    //checks whether any child exists in Blocked Thread queue
    blockedThread *btempFront=bfront;
    while(btempFront!=NULL){
         if(btempFront->parentId==parentId)
		return 1;
	 btempFront=btempFront->nxt;
    }
	
    return 0;
}


// Join with all children
void MyThreadJoinAll(void){
	//printf("Join All\n" );
	if(childExistIfAny(currentThreadId)){
	ucontext_t mContext;
	MyThread swpContext;
        getcontext(&mContext);
	bpush(mContext,currentThreadId,currentThreadParent,0);
	//printf("valid pop");
	swpContext=pop();
        currentThreadId=swpContext.threadId;
	currentThreadParent=swpContext.parentId;
	swapcontext(&(brear->context),&swpContext.context);
	}

}


//checks whether a thread is blocked or not
// returns 1 if blocked otherwise 0

int isBlocked(int threadId){
    blockedThread *btempFront=bfront;
    while(btempFront!=NULL){
         if(btempFront->threadId==threadId)
		return 1;
	 btempFront=btempFront->nxt;
    }
    return 0;
}

// Terminate invoking thread
void MyThreadExit(void){
	//printf("End Context ThreadId= %d  ThreadCount=%d  BThreadCount=%d\n",currentThreadId,threadCount,blockedThreadCount);
        //If any other thread blocked on this exiting thread unblock it
        if(blockedThreadCount > 0 || bfront != NULL){
	         blockedThread *btempFront=bfront;
   		 while(btempFront!=NULL){
    	     	 	if(btempFront->blockedBy==currentThreadId){
				blockedThread tempBlocked=bpop(btempFront->threadId);
				push(tempBlocked.context,tempBlocked.threadId,tempBlocked.parentId);
				break;
		 	}		
	 	 	btempFront=btempFront->nxt;
    		 }
	}
	if(isBlocked(currentThreadParent) && !childExistIfAny(currentThreadParent)){
        	blockedThread tempBlocked=bpop(currentThreadParent);
		push(tempBlocked.context,tempBlocked.threadId,tempBlocked.parentId);
		//printf("Join All released\n");
	}
	
        ucontext_t mainContext;
	MyThread swpContext;
	if(front == NULL){
		ucontext_t tempCC;
		swapcontext(&tempCC,&mainInitContext);
	}
        else{
	       	swpContext=pop();
        	currentThreadId=swpContext.threadId;
		currentThreadParent=swpContext.parentId;
		swapcontext(&mainContext,&swpContext.context);
	}
}


//********** Semaphore Routines **********

// ****** SEMAPHORE OPERATIONS ****** 
// Create a semaphore
MySemaphore* MySemaphoreInit(int initialValue){
 // printf("Sem Create Started\n");
 if(initialValue<0)
	return NULL;
  MySemaphore *sem =(MySemaphore *)malloc(sizeof(MySemaphore));
 // printf("semaphore created\n");
  sem->sid=++semaphoreCount;
  sem->value=initialValue;
  //printf("Sen Created sid=%d value=%d\n",sem->sid,sem->value);
  return sem;
}

// Signal a semaphore
void MySemaphoreSignal(MySemaphore *sem){
 // printf("Sem Signal Called By Thread=%d and sem value=%d \n",currentThreadId,sem->value);
  if(sem->value++<0){
	        sBlockedThread *btempFront=sbfront;
   		 while(btempFront!=NULL){
    	     	 	if(btempFront->blockedBy==sem->sid){
				sBlockedThread tempBlocked=sbpop(btempFront->threadId);
				push(tempBlocked.context,tempBlocked.threadId,tempBlocked.parentId);
				//printf("Thread=%d waiting for semid=%d ublocked",tempBlocked.threadId,sem->sid);
				break;
		 	}		
	 	 	btempFront=btempFront->nxt;
    		 }
	}
  //  printf("Calling thread should be bocked");
  //printf("Sem signal End\n");
}

// Wait on a semaphore
void MySemaphoreWait(MySemaphore *sem){
   // printf(" Sem Wait Called and Sem value= %d \n",sem->value);
  sem->value-=1;
  if(sem->value<0){
      //  printf("Thread=%d waiting for semid=%d",currentThreadId,sem->sid);
	ucontext_t mContext;
	MyThread swpContext;
        getcontext(&mContext);
	sbpush(mContext,currentThreadId,currentThreadParent,sem->sid);
	//printf("valid pop");
	swpContext=pop();
        currentThreadId=swpContext.threadId;
	currentThreadParent=swpContext.parentId;
	swapcontext(&(sbrear->context),&swpContext.context);
	}
   
}

// Destroy on a semaphore
int MySemaphoreDestroy(MySemaphore *sem){
  // if any thread blocked on this sem return -1 as failure
      //  printf("Sem Destroy Started  %d\n",sem->sid);
      sBlockedThread *btempFront=sbfront;
      while(btempFront!=NULL){
         if(btempFront->blockedBy==sem->sid)
	    return -1;
         btempFront=btempFront->nxt;
      }

  //otherwise destroy the semaphore
 free(sem);

  return 0;

}

 
 







 
