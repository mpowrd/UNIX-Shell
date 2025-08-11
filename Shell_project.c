/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include "parse_redir.h"
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */
job * lista;
int shell_pid;
variables * var;

// -----------------------------------------------------------------------
//                  	    THREADS     
// -----------------------------------------------------------------------

void *alarm_thread(void *argumento) {
	
	int seconds = *((int *)argumento);
	int pid = *((int *)((int *)argumento +1));
	
	sleep(seconds);
	
	// Obtener el PID del proceso a aniquilar
	
	
	
	// Enviar la señal SIGKILL al proceso
	kill(pid, SIGKILL);
	// Hacer que el hilo sea detached
return NULL;
}

// -----------------------------------------------------------------------
//                  	FACTORIZACIÓN DEL CÓDIGO          
// -----------------------------------------------------------------------
void ejecucion_comando (char * args[])
{
	//printf("%s \n", args[2]);
	if(execvp(args[0],args)==-1)
	{				
		printf("Error, command not found: %s \n",args[0]);
		exit(EXIT_FAILURE) ;
			
	}
}

void proceso_hijo (int background,char * args[],char * filein,char * fileout)
{
	new_process_group( getpid()); //Crear un grupo de procesos en función dela id del proceso hijo
			
	//Asociamos al foreground del terminal el proceso hijo
	if(background==0)tcsetpgrp(STDIN_FILENO, getpid());
			
	restore_terminal_signals();  // pertimos las señales de job_control.c para así poder mandar señales al hijo
			
	// -----------------------------------------------------------------------
	//                            TAREA 5          
	// -----------------------------------------------------------------------
	if(filein != NULL)
	{	
		int in_fno =open(filein,O_RDONLY); //Te da el numero del pos mem
		//Comprobar si el método no funciono
		if(in_fno==-1)
		{
			perror("open_in");
			exit(EXIT_FAILURE);
		}
				
		//Hacemos una copia del fcihero
		//Hay que controlar que dup2 de -1 sigifica error
		if(dup2(in_fno,STDIN_FILENO)==-1)
		{
			perror("Dup2: file in");
			exit(EXIT_FAILURE) ;
		}
		close(in_fno);
	}
			
	if(fileout !=NULL)
	{
		int out_fno= open (fileout,O_WRONLY | O_CREAT,0666); // 0x666 wrx usuario
		if(out_fno==-1)
		{
			perror("open_out");
			exit(EXIT_FAILURE) ;
		}
				
		//Hacemos una copia del fcihero
		//Hay que controlar que dup2 de -1 sigifica error
		if(dup2(out_fno,STDOUT_FILENO)==-1)
		{
			perror("Dup2: file out");
			exit(EXIT_FAILURE) ;
		}
			
				
		close(out_fno);
	}
	// -----------------------------------------------------------------------			
			
	ejecucion_comando (args);
			
			
}

void background_process (int pid_fork,char * args[],job * nodo, int respawnable_flag)
{
	block_SIGCHLD();
	enum job_state state= BACKGROUND;
	//Creamos un nodo del proceso background y lo añadimos a la lista
	if(respawnable_flag == 1) state= RESPAWNABLE;
	nodo = new_job(pid_fork, args[0],state,var);

	add_job (lista,nodo);
			
	printf("Background job running... pid: %d,command: %s \n",pid_fork,args[0]);
				
	unblock_SIGCHLD() ;
}

void foreground_process(int pid_fork,char * args[],job * nodo)
{
	int pid_wait;
	enum status status_res;
	int status;
	int info;

	//El proceso obtiene el control del terminal
	tcsetpgrp(STDIN_FILENO,pid_fork);
				
	//RETURN: 
	//	*It returns the process ID of the stopped child process.
	//	*If it fails returns -1
	

	pid_wait=waitpid(pid_fork,&status,WUNTRACED);
				
				
				
	if(pid_wait ==-1)
	{
		printf("\nError in waitpid\n");
		exit(EXIT_FAILURE);
	}else
	{
		//Colocamos en foreground el proceso de nuestro terminal
		tcsetpgrp(STDIN_FILENO,shell_pid);
		
		//Evaluamos el motivo de terminación del proceso hijo e imprimimos 
		status_res= analyze_status(status, &info);
		
		//Imprimimos el mensaje con información del proceso		
		printf("Foreground pid: %d, command: %s, %s, info: %d \n",pid_wait,args[0],status_strings[status_res],info);
		//Si el proceso ha sido parado hay que añadirlo a la lista de background

		if (status_res == SUSPENDED){
			
			block_SIGCHLD();
			nodo = new_job(pid_wait, args[0], STOPPED,var);
			add_job(lista, nodo);
			unblock_SIGCHLD();
		}
	}
}

void proceso_padre (int pid_fork,int background,char * args[],int respawnable_flag,job * nodo)
{
	//RETURN: La variable background tomará los valores 0 y 1 los cuales informan de:
	//	* 0: El proceso está en foreground
	//	* 1: El proceso está en background
	
	new_process_group( getpid());
	
	if(var->timeout){
		int pid= pid_fork;
		pthread_t pid_t;
				
		int argumento[2];
		argumento[0]= var->timeout;
		argumento[1]= pid;
		
		//printf("Entra en alarm-thread \n");

		pthread_create(&pid_t, NULL, alarm_thread, (void *)argumento);
		
		pthread_detach(pid_t); 
		var->timeout =0;
	}
	
	
	if(background!=0)
	{
		background_process(pid_fork,args, nodo, respawnable_flag);
	}else
	{
		foreground_process(pid_fork,args,nodo);
	}
	
}

void proceso_fork (int background, char ** args,int argc,char * filein,char * fileout, int respawnable_flag,int alarm)
{
	int pid_fork;
	
	/*RETURN:
		On success, the PID of the child process is returned in the
       parent, and 0 is returned in the child.  On failure, -1 is
       returned in the parent, no child process is created, and errno is
       set to indicate the error.
	*/
	
	
	pid_fork = fork(); 

	if(pid_fork==-1)
	{
		//fork fallido
		perror("Fork have not succeeded");
	}else if (pid_fork==0)
	{
		//Proceso hijo
		
		proceso_hijo(background,args,filein,fileout);

	}else
	{

		job * nodo;
		var= copia_variable(background,args,argc,filein, fileout,respawnable_flag,&pid_fork,alarm,0);
		//Proceso padre
		proceso_padre(pid_fork,background,args,respawnable_flag,nodo);
	}


}


// -----------------------------------------------------------------------
//				MASK FORK
// -----------------------------------------------------------------------
void proceso_hijo_mask (int background,char * args[],char * filein,char * fileout, sigset_t mask)
{
	new_process_group( getpid()); //Crear un grupo de procesos en función dela id del proceso hijo
			
	//Asociamos al foreground del terminal el proceso hijo
	if(background==0)tcsetpgrp(STDIN_FILENO, getpid());
			
	restore_terminal_signals();  // pertimos las señales de job_control.c para así poder mandar señales al hijo
	sigprocmask(SIG_SETMASK, &mask, NULL);	
	// -----------------------------------------------------------------------
	//                            TAREA 5          
	// -----------------------------------------------------------------------
	if(filein != NULL)
	{	
		int in_fno =open(filein,O_RDONLY); //Te da el numero del pos mem
		//Comprobar si el método no funciono
		if(in_fno==-1)
		{
			perror("open_in");
			exit(EXIT_FAILURE);
		}
				
		//Hacemos una copia del fcihero
		//Hay que controlar que dup2 de -1 sigifica error
		if(dup2(in_fno,STDIN_FILENO)==-1)
		{
			perror("Dup2: file in");
			exit(EXIT_FAILURE) ;
		}
		close(in_fno);
	}
			
	if(fileout !=NULL)
	{
		int out_fno= open (fileout,O_WRONLY | O_CREAT,0666); // 0x666 wrx usuario
		if(out_fno==-1)
		{
			perror("open_out");
			exit(EXIT_FAILURE) ;
		}
				
		//Hacemos una copia del fcihero
		//Hay que controlar que dup2 de -1 sigifica error
		if(dup2(out_fno,STDOUT_FILENO)==-1)
		{
			perror("Dup2: file out");
			exit(EXIT_FAILURE) ;
		}
			
				
		close(out_fno);
	}
	// -----------------------------------------------------------------------			
			
	ejecucion_comando (args);
			
			
}

void proceso_fork_mask (int background, char ** args,int argc,char * filein,char * fileout, int respawnable_flag,int alarm, sigset_t mask,int mask_flag)
{
	int pid_fork;
	
	/*RETURN:
		On success, the PID of the child process is returned in the
       parent, and 0 is returned in the child.  On failure, -1 is
       returned in the parent, no child process is created, and errno is
       set to indicate the error.
	*/
	
	
	pid_fork = fork(); 

	if(pid_fork==-1)
	{
		//fork fallido
		perror("Fork have not succeeded");
	}else if (pid_fork==0)
	{
		//Proceso hijo
		
		proceso_hijo_mask(background,args,filein,fileout,mask);

	}else
	{

		job * nodo;
		var= copia_variable(background,args,argc,filein, fileout,respawnable_flag,&pid_fork,alarm,0);
		//Proceso padre
		proceso_padre(pid_fork,background,args,respawnable_flag,nodo);
	}


}
// -----------------------------------------------------------------------
//                            TAREA 3          
// -----------------------------------------------------------------------

void manejador (int senal)
{
    block_SIGCHLD();
	pid_t pid;
	int status_pid=0, info=0;
	enum status status_res;
	job * proceso;
	int borrar ;
	
	
	while((pid= waitpid(-1, &status_pid,(WNOHANG | WUNTRACED | WCONTINUED)))>0)
	{
		status_res= analyze_status(status_pid, &info);
		
		
		proceso= get_item_bypid(lista,pid);
		if(proceso!=NULL)
		{
			if (status_res == SUSPENDED )
			{
				proceso->state = STOPPED;
			}else if (status_res == CONTINUED )
			{
				proceso->state = BACKGROUND;
			} else if(status_res == EXITED || status_res == SIGNALED ) //Eliminar proceso de la lista
			{		
				if(proceso->state == RESPAWNABLE)
				{
					//llamar al proceso fork para volver a empezar
					proceso_fork((proceso->var)->background,(proceso->var)->argumentos,(proceso->var)->num_argu,(proceso->var)->filein,(proceso->var)->fileout,(proceso->var)->respawnable_flag,(proceso->var)->timeout);
					free(proceso->var);
				}
					
				borrar = delete_job(lista, proceso);
				if(borrar==0)
				{
					perror("No se pudo eliminar el proceso de la lista de procesos background");
				}
			
			}
			
		}
		//printf("Manejador,pid: %d, command: %s, %s, info: %d \n",proceso->pgid ,proceso->command ,status_strings[status_res],info);
		
	}
	unblock_SIGCHLD() ;
	
	

}

int longitud_arg (char ** argumentos){
	int lon=0;
	
	while(argumentos[lon]!=NULL)
	{
		lon++;
	}
	return lon++;
}

// -----------------------------------------------------------------------
//                  	  AMPLIACIONES          
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
//                  	    THREADS     
// -----------------------------------------------------------------------



void * delay_thread(void *argumento)
{
	struct variables *datos = (struct variables *) argumento;
	sleep(datos->delay);
	
	proceso_fork(datos->background,datos->argumentos,datos->num_argu,datos->filein,datos->fileout,datos->respawnable_flag,datos->timeout);
}
// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(int argc, char*env[])
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	
	

	// -----------------------------------------------------------------------
	//                            TAREA 3          
	// -----------------------------------------------------------------------
	//Inicializamos la lista de procesos background
	lista= new_list("lista");
	ignore_terminal_signals();
	signal(SIGCHLD,manejador);
	
	shell_pid= getpid();

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		// -----------------------------------------------------------------------
		//                            TAREA 5          
		// -----------------------------------------------------------------------
		//Obtenemos los nombres de fichero para pasarlos por la funcion procesos_fork
		char * filein, * fileout;
		parse_redirections(args,&filein,&fileout);
		// -----------------------------------------------------------------------
		
		if(args[0]==NULL) continue;   // if empty command
		argc = longitud_arg(args);
		// -----------------------------------------------------------------------
		//                            TAREA 2          
		// -----------------------------------------------------------------------
		//Implementar el comando interno cd para permitir cambiar el directorio
		//de trabajo actual (usar la función chdir).
		
		if(strcmp(args[0],"cd")==0)
		{
			//Se ejecuta el comando y si devuelve -1 devuelve un error
			if(chdir(args[1])== -1)
			{
				perror("Path");
			}
			
			continue;
		}
		
		// -----------------------------------------------------------------------
		//                            TAREA 4          
		// -----------------------------------------------------------------------
		//Se debe implementar de forma segura la gestión de tareas en
		//segundo plano o background

		job * nodo;
		job * tarea;
		//int foregrnd= 0;
		//Imprimimos la lista de procesos background
		if(strcmp(args[0],"jobs")==0)
		{
				block_SIGCHLD();
				//Utiliza la función print_list implementada en job_control.h
				print_list(lista,print_item);
				unblock_SIGCHLD();

				continue;
		}

		//Pasamos un proceso foreground a background
		if(strcmp(args[0], "bg")==0)
		{
			block_SIGCHLD();
			//Diferenciamos si nos pansan bg o bg num
			int pos =(args[1]!=NULL)?atoi(args[1]):1;
			
			//Buscamos el nodo en la lista de procesos background
			nodo=get_item_bypos(lista,pos);
			

			// Si lo hemos encontrado y su estado es parado se pasa a Background
			if(nodo!=NULL && nodo->state == STOPPED)
			{
				nodo->state = BACKGROUND;
				killpg(nodo->pgid,SIGCONT);
				
			}
			unblock_SIGCHLD();

			continue;
		}
		//Pasamos un proceso background a foreground
		if (strcmp(args[0],"fg")==0)
		{
			block_SIGCHLD();
			//Diferenciamos si nos pansan fg o fg num
			int pos =(args[1]!=NULL)?atoi(args[1]):1;
			
			//Buscamos el nodo en la lista de procesos background
			nodo = get_item_bypos(lista,pos);

			// Si lo hemos encontrado
			if(nodo !=NULL)
			{
				//foregrnd=1;
				//Colocanos el proceso en foreground 
				tcsetpgrp(STDIN_FILENO,nodo->pgid);

				
				if(nodo->state == STOPPED)
				{
					killpg(nodo->pgid,SIGCONT);
				}
				
				
				//hacemos una copia del proceso para el proceso padre
				pid_fork= nodo->pgid;
				strcpy(args[0],nodo->command);

				//Guardamos los datos correspondientes para mandarselos al proceso padre
				tarea= new_job(pid_fork,args[0],nodo->state,var);

				delete_job(lista, nodo);

				//LLAMAR AL PROCESO PADRE//
				proceso_padre(pid_fork,0,args,0,tarea);
			
			}
			unblock_SIGCHLD();
			continue;
			
		}
		
		// -----------------------------------------------------------------------
		//                        RESPAWNABLE: Ampl1          
		// -----------------------------------------------------------------------
		//Para los procesos respawnables debemos de hacer una variable extra que almacene 
		//dicha información
		//Debo de modificar el manejador para volver a lanzar el proceso.
		/*
		1.Activar la flag respawnable y background
		2. proceso fork
		3. si termina 
			3.1 lanza otro fork
			3.2 sin file in file out 
			3.4 
		*/
		int respawnable_flag= 0;
		
		if(strcmp(args[argc-1],"+")==0)
		{
			block_SIGCHLD();
			printf("+ \n");
			background=1;
			respawnable_flag= 1;
			
			argc--;
			args[argc]=NULL;
			unblock_SIGCHLD();
		}
		
		// -----------------------------------------------------------------------
		//                        ALARM-THREAD: Ampl2         
		// -----------------------------------------------------------------------
		/*
		- se lanzará el proceso xclock con los argumentos indicados;
		
		- el shell creará un thread que dormirá (sleep()) los segundos especificados
		como primer argumento (30 segundos en el ejemplo), tras lo cual
		aniquilará (SIGKILL) el proceso lanzado (xclock –update 1, en el ejemplo)
		si éste aún no hubiera terminado;
		
		- dicho thread terminará ahí (lo recomendable es que el thread esté
		declarado como detached para que libere sus recursos sin más espera).
		
		
		if(strcmp(args[0],"alarm-thread")==0)
		{
			block_SIGCHLD();
			int sec=atoi(args[1]);
			if(sec==0)
			{
				perror("The second argument is not a number");
				exit(EXIT_FAILURE);
			}
			
			unblock_SIGCHLD();
		}
		*/
		int alarm=0;
		if(strcmp(args[0],"alarm-thread")==0 && atoi(args[1])!=0)
		{
			if(args[1]!=NULL && args[2]!=NULL){
				block_SIGCHLD();
				alarm=1;
				alarm = atoi(args[1]);
				
				if(alarm ==0){
					perror("ERROR: al traducir el string a numero");
				}				
				
				for(int i=2;i<argc;i++){
					args[i-2]= strdup(args[i]);
				}
				
				args[argc-1]=NULL;
				args[argc-2]=NULL;
				
				argc -=2;
				//printf("%s  %d %d \n",args[0],argc,alarm);
				unblock_SIGCHLD();
				
			}
		}
		
		
		// -----------------------------------------------------------------------
		//                        DELAY-THREAD: Ampl3        
		// -----------------------------------------------------------------------
		
		int delay =0;
		if(strcmp(args[0],"delay-thread")==0)
		{
			if(args[1]!=NULL && args[2]!=NULL)
			{
				block_SIGCHLD();
				delay=1;
				delay = atoi(args[1]);
				background =1;
				if(delay<0) perror("ERROR:Formato inválido");
				
				for(int i=2;i<argc;i++){
					args[i-2]= strdup(args[i]);
				}
				
				args[argc-1]=NULL;
				args[argc-2]=NULL;
				
				argc -=2;
				
				pthread_t pid_t;
						
				struct variables * argumento = copia_variable(background,args,argc,filein,fileout,respawnable_flag,&pid_fork,alarm,delay);
				
				pthread_create(&pid_t, NULL, delay_thread, argumento);
				
				pthread_detach(pid_t); 
				unblock_SIGCHLD();
				continue;
				
				
				
			
				
			}
		}
		
		// -----------------------------------------------------------------------
		//                            MASK: Ampl4        
		// -----------------------------------------------------------------------
		/*Debemos de almacenar en un array de enteros los números de las señales*/
		sigset_t mask;
		int mask_flag=0;
		int *signals = NULL;
		
		if(strcmp(args[0],"mask")==0)
		{
			block_SIGCHLD();
			int numSignals = 0;
			int i;
			mask_flag =1;
			for (i = 1; i < argc; i++) {
				if (strcmp(args[i], "-c") == 0) {
				    break;
				}

				if (atoi(args[i]) > 0) {
				    signals = realloc(signals, (numSignals + 1) * sizeof(int));
				    signals[numSignals] = atoi(args[i]);
				    numSignals++;
				} else {
				    fprintf(stderr, "Error: Argumento inválido: %s\n", args[i]);
				    free(signals);
				    unblock_SIGCHLD();
				    continue;
				}
			}
			
			
			if(numSignals ==0 )
			{
				fprintf(stderr, "Error: Al menos de pasar una señal");
				free(signals);
				unblock_SIGCHLD();
				continue;
			}
			
			if(i == argc)
			{
				fprintf(stderr, "Error: Faltan argumentos");
				free(signals);
				unblock_SIGCHLD();
				continue;
			}
			int index=0;
			for(int j = i+1;j<argc;j++)
			{
				args[index]= args[j];
				index++;
				
				
			}
			
			argc-= numSignals+2;
			
			
			sigset_t mask;
			sigemptyset(&mask);
			
			for (i = 0; i < numSignals; i++) {
				sigaddset(&mask, signals[i]);
			}
			printf("%s %s",args[0],args[argc-1]);
			sigprocmask(SIG_BLOCK, &mask, NULL);
			unblock_SIGCHLD();
			
			proceso_fork_mask(background,args,argc,filein,fileout,respawnable_flag,alarm,mask,mask_flag);
			continue;
		}
		
		
		proceso_fork(background,args,argc,filein,fileout,respawnable_flag,alarm);
		
		free(signals);
	} // end while
}
