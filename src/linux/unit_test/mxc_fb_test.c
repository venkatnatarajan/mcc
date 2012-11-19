/*
 * @file mvf_linux_mcc.c
 *
 * @brief Unit test of mvf_shared_iram, mvf_ipc and mvf_sema4 modules.
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include "mvf-ipc.h"
#include "mvf-sema4.h"
#include <unistd.h>

#include <signal.h>
#include <string.h>
#include <fcntl.h>

#define MVF_IPC_TEST
//#define MVF_SHARED_IRAM_TEST
//#define MVF_SEMA4_TEST

#ifdef MVF_IPC_TEST
unsigned int wait = 0;
siginfo_t sig_info;

void receiveData(int n, siginfo_t *info, void *unused) {
	wait = 1;
	sig_info.si_signo = info->si_signo;
	sig_info.si_code = info->si_code;
	sig_info.si_int = info->si_int;	
	printf("In receiveData user space handler\n");
}
#endif

int main(int argc, char **argv)
{

#ifdef MVF_SEMA4_TEST
{		
		int ret_code;
		int fd_mvf_sema4 = 0;
		unsigned char gate; 
		
		if ((fd_mvf_sema4 = open("/dev/mvf_sema4", O_RDWR, 0)) < 0)
        {
            printf("Unable to open /dev/mvf_sema4\n");                
            goto err0;
        }
                
        gate =  1;
        
        ret_code = ioctl(fd_mvf_sema4, MVF_SEMA4_UNLOCK, &gate);
        if(ret_code != 0)
        {
			printf("Error unlocking. Error code:%d\n",ret_code);			
		}
		else
		{
			printf("Unlocked\n");	
		}
		
        ret_code = ioctl(fd_mvf_sema4, MVF_SEMA4_LOCK, &gate);
        if(ret_code != 0)
        {
			printf("Error obtaining lock. Error code:%d\n",ret_code);
			goto err1;
		}
		else
		{
			printf("Lock obtained\n");	
		}
				
		ret_code = ioctl(fd_mvf_sema4, MVF_SEMA4_TRYLOCK, &gate);
        if(ret_code != 0)
        {
			printf("Error obtaining lock. Error code:%d\n",ret_code);			
		}
		else
		{
			printf("Lock obtained\n");	
		}
		
		ret_code = ioctl(fd_mvf_sema4, MVF_SEMA4_UNLOCK, &gate);
        if(ret_code != 0)
        {
			printf("Error unlocking. Error code:%d\n",ret_code);			
		}
		else
		{
			printf("Unlocked\n");	
		}
		
		ret_code = ioctl(fd_mvf_sema4, MVF_SEMA4_TRYLOCK, &gate);
        if(ret_code != 0)
        {
			printf("Error obtaining lock. Error code:%d\n",ret_code);
			goto err1;
		}
		else
		{
			printf("Lock obtained\n");	
		}        
        
        ret_code = ioctl(fd_mvf_sema4, MVF_SEMA4_UNLOCK, &gate);
        if(ret_code != 0)
        {
			printf("Error unlocking. Error code:%d\n",ret_code);			
		}
		else
		{
			printf("Unlocked\n");	
		}
err1:
        close(fd_mvf_sema4);
}       
err0:
		return 1;
#endif	

#ifdef MVF_IPC_TEST
{		
		int ret_code;
		int fd_mvf_ipc = 0;
		cpu_interrupt_info_t interrupt_info;
		
		struct sigaction sig;
		unsigned short interrupt_id;							

		if ((fd_mvf_ipc = open("/dev/mvf_ipc", O_RDWR, 0)) < 0)
        {
            printf("Unable to open /dev/mvf_ipc\n");                
            goto err0;
        }
        	
		interrupt_id = CPU_INT1_ID;
        
        ret_code = ioctl(fd_mvf_ipc, MVF_REGISTER_PID_WITH_CPU_INTERRUPT, &interrupt_id);
        if(ret_code != 0)
        {
			printf("Error registering PID with interrupt. Error code:%d\n",ret_code);
			goto err1;
		}
		
		printf("User app realtime signal number:%d\n",interrupt_id);
		sig.sa_sigaction = receiveData;
		sig.sa_flags = SA_SIGINFO;
		ret_code = sigaction(interrupt_id, &sig, NULL);		
		if(ret_code != 0)
        {
			printf("Error registering signal. Error code:%d\n",ret_code);
			goto err1;
		}		
		
		interrupt_info.interrupt_type = CP0_INTERRPUT_ONLY;
		interrupt_info.interrupt_id = CPU_INT1_ID;
		
		ret_code =  ioctl(fd_mvf_ipc, MVF_GENERATE_CPU_TO_CPU_INTERRUPT, &interrupt_info);
        if(ret_code != 0)
        {
			printf("Error requesting interrupt generation. Error code: %d\n",ret_code);
			goto err1;
		}
        sleep(10);
        //while(wait == 0);
        if(wait == 1)
        {
			printf("Signal recieved. Siginfo:\n Number:%d, Code:%d, Data:%d\n",sig_info.si_signo, sig_info.si_code, sig_info.si_int);             
		}
		else
		{
			printf("Signal not recieved\n");
		}
        
err1:
        close(fd_mvf_ipc);
}       
err0:
		return 1;
#endif	
	
#ifdef MVF_SHARED_IRAM_TEST
{
		int fd_shared_iram = 0;
		unsigned short * shared_iram_pointer;

		#define SHARED_IRAM_SIZE (64*1024)

		if ((fd_shared_iram = open("/dev/mvf_shared_iram", O_RDWR, 0)) < 0)
        {
                printf("Unable to open /dev/mvf_shared_iram\n");                
                goto err0;
        }
        else
        {
				printf("Open successfull\n");
		}
        
        shared_iram_pointer = (unsigned short *)mmap(0, SHARED_IRAM_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, fd_shared_iram, 0);
        if ((int)shared_iram_pointer <= 0)
        {
                printf("Error: failed to map shared_iram device to memory.\n");                
                goto err1;
        }
        else
        {
				printf("Memory map successfull, pointer at address %x\n",(unsigned int)shared_iram_pointer);
		}
        
        *shared_iram_pointer = 0xAABB;
        *(shared_iram_pointer+8) = 0x1234;
        
        printf("Write did not crash\n");
   
		munmap(shared_iram_pointer, SHARED_IRAM_SIZE);
err1:
        close(fd_shared_iram);
}       
err0:
		return 1;
#endif
        
}

