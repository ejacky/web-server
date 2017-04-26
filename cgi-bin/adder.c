#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLINE 8192

int main(void)
{
	char *buf, *p;
	char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
	int n1 = 0, n2 = 0;
	
	fprintf(stderr, "||QUERY_STRING=%s", getenv("QUERY_STRING"));	
	if ((buf = getenv("QUERY_STRING")) != NULL) { 

		p = strchr(buf, '&');
		*p = '\0';/* something wrong */
		strcpy(arg1, buf);
		strcpy(arg2, p+1);
		n1 = atoi(arg1);
		n2 = atoi(arg2);
	}
	
	/*  body */
	//sprintf(content, " Welcome:");
	sprintf(content, "%s The answer is : <p>%d + %d = %d \r\n</p>", content, n1, n2, n1+n2);
	//sprintf(content, "%s Thanks for your visiting!\r\n", content);
	
	/* http */
	printf("Content-length: %d\r\n", (int)strlen(content));
	printf("Content-type: text/html\r\n\r\n");
	printf("%s", content);
	fflush(stdout);
	exit(0); 
}
