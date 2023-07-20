#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

int value = 5;

int main()
{
pid_t pid;
 	pid = fork();
 	
if (pid ==0)
 {
        printf("hello from child"); /*LINE A */
 		value +=5;
 		return 0;
 }
 else if (pid > 0)
 {
	wait(NULL);
	printf("PARENT: value= %d", value); /*LINE A */
			return 0;
 }
 return 0;
 }
