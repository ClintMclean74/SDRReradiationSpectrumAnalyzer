#include "kerberos_functions.h"
#include <Windows.h>

/*
#define BUFFER_NO 2  // Buffer number
#define CHANNEL_NO 4 // Channel number

int BUFFER_SIZE_SYNC = 0;

/*
*  Signal definitions:
*
*
*
*
*/

/*////
static sem_t trigger_sem;
static int trigger;
static int exit_flag = 0;
static int delays[CHANNEL_NO];
pthread_t fifo_read_thread;
*/

#define CFN "_receiver/C/sync_control_fifo" // Name of the gate control fifo - Control FIFO name



void SetNoiseState(rtlsdr_dev_t *dev, uint8_t state)
{
	if (state == 1)
		rtlsdr_set_gpio(dev, 1, 0);
	else
		rtlsdr_set_gpio(dev, 0, 0);
}

/*////void SetNoiseState(struct rtl_rec_struct *rtl_rec, int state)
{	

	if (state == 1)
		rtlsdr_set_gpio(rtl_rec->dev, 1, 0);
	else
		rtlsdr_set_gpio(rtl_rec->dev, 0, 0);
}*/

void InitializeSynchronization()
{	
	HANDLE hPipe;
	char buffer[1024];
	DWORD dwRead;


	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\Pipe"),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);
	
	



	char args[2][10] = { " ", "256" };

	char *argsPtr = (char *) args;

	char *ptr[2];

	ptr[0] = " ";
	ptr[1] = "256";

	main_sync(2, ptr);



	Sleep(1000);

	DWORD dwWritten;

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		WriteFile(hPipe,
			"d\n",
			2,   // = length of string + terminating '\0' !!!
			&dwWritten,
			NULL);
	}


	/*////while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
		{
			while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				/* add terminating zero */
/*////			buffer[dwRead] = '\0';

				/* do something with data in buffer */
/*////				printf("%s", buffer);
			}
		}
		DisconnectNamedPipe(hPipe);
	}
	*/



	/*////static char buf[262144 * 4 * 30];

	setvbuf(stdout, buf, _IOFBF, sizeof(buf));

	////BUFFER_SIZE_SYNC = (atoi(argv[1]) / 2) * 1024;
	BUFFER_SIZE_SYNC = 128 * 1024;

	int read_size; // Stores the read bytes from stdin
	int first_read = 0;
	int write_index = BUFFER_NO - 1; // Buffer index 0..BUFFER_NO-1
	uint8_t *read_pointer;

	/* Initializing signal thread */
	/*////
	
	
	sem_init(&trigger_sem, 0, 0);  // Semaphore is unlocked
	trigger = 0;

	// Initializing delay buffers
	struct sync_buffer_struct* sync_buffers;
	sync_buffers = malloc(CHANNEL_NO * sizeof(*sync_buffers));
	for (int m = 0; m<CHANNEL_NO; m++)
	{
		(sync_buffers + m * sizeof(*sync_buffers))->circ_buffer = malloc(BUFFER_NO*BUFFER_SIZE_SYNC * 2 * sizeof(uint8_t)); // *2-> I and Q    
		(sync_buffers + m * sizeof(*sync_buffers))->delay = BUFFER_SIZE_SYNC;//BUFFER_SIZE_SYNC/2;  
																			 //fprintf(stderr,"ch: %d, Buff pointer: %p\n",m,(sync_buffers + m * sizeof(*sync_buffers))->circ_buffer);
	}
	*/
}

void Initialize()
{
	
}
