#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFSIZE 330000
struct
{
    char *ext;
    char *filetype;
} extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0} };

void handle_socket(int fd)
{
    int j, file_fd, buflen, len;
    long i, ret;
    char * fstr;
    static char buffer[BUFSIZE+1];
    ret = read(fd,buffer,BUFSIZE);   /* 讀取瀏覽器要求 */
	write(1,buffer,BUFSIZE);
    if (ret==0||ret==-1)
        exit(3);
    if (ret>0&&ret<BUFSIZE)
        buffer[ret] = '\0';
    else
        buffer[0] = 0;
    if (!strncmp(buffer,"GET ",4))
    {
        for(i=4;i<BUFSIZE;i++) {
            if(buffer[i] == ' ') {
                buffer[i] = 0;
                break;
                }
            }
        for (j=0;j<i-1;j++)
            if (buffer[j]=='.'&&buffer[j+1]=='.')
                exit(3);
        /* 當客戶端要求根目錄時讀取 index.html */
        if (!strncmp(&buffer[0],"GET /\0",6)||!strncmp(&buffer[0],"get /\0",6) )
            strcpy(buffer,"GET /index.html\0");
        /* 檢查客戶端所要求的檔案格式 */
        buflen = strlen(buffer);
        fstr = (char *)0;
        for(i=0;extensions[i].ext!=0;i++) {
            len = strlen(extensions[i].ext);
            if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
                fstr = extensions[i].filetype;
                break;
            }
        }
        /* 檔案格式不支援 */
        if(fstr == 0) {
            fstr = extensions[i-1].filetype;
        }
        /* 開啟檔案 */
        if((file_fd=open(&buffer[5],O_RDONLY))==-1)
            write(fd, "Failed to open file", 19);
        /* 傳回瀏覽器成功碼 200 和內容的格式 */
        sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
        write(fd,buffer,strlen(buffer));
        /* 讀取檔案內容輸出到客戶端瀏覽器 */
        while ((ret=read(file_fd, buffer, BUFSIZE))>0) {
            	write(fd,buffer,ret);
        }
    }
    else if(!strncmp(buffer,"POST ",5)||!strncmp(buffer,"post ",5))
    {
        char *tmp;
	    char filename[2048];
		int i = 0;
        //get filename
        char *tmp1;
        char body[BUFSIZE];
		char  path[100] = "./file/";
	    tmp1 = strstr(buffer,"filename=");
	    strcpy(filename, tmp1+10);
	    for(int i = 0; i < strlen(filename); i++)
	    {
		    if(filename[i]=='"')
		    {
			    filename[i] = 0;
			    break;
		    }
	   }
		strcat(path,filename);        tmp = strstr(tmp1, "\n\r\n");
       	tmp+=3;
		FILE *fp = fopen(path,"a+");
        while (*tmp!='\n'||*(tmp+1)!='-'||*(tmp+2)!='-')
        {
            fprintf(fp,"%c",*tmp);
            tmp++; 
        }
        fclose(fp);
        write(fd,"Success",7);
		
    }

    exit(1);
}

int main(int argc, char **argv)
{
    int i, pid, listenfd, socketfd;
	int yes =1;
    socklen_t length;
    static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;
    signal(SIGCHLD, SIG_IGN);
    if ((listenfd=socket(AF_INET, SOCK_STREAM,0))<0)
       {
		printf("socket error\n");
 		exit(3);
	}
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(8080);
	if (setsockopt(listenfd ,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) == -1) 
    {
        fprintf(stderr,"setsockopt\n");
        exit(1);
    }
    if (bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
       {
		printf("bind error\n");
 		exit(3);
}
    if (listen(listenfd,64)<0)
	{
		printf("listen error\n");
        exit(3);
	}
    while(1) {
        length = sizeof(cli_addr);
        /* 等待客戶端連線 */
        if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length))<0)
{
printf("accept error\n");
            exit(3);
}
        /* 分出子行程處理要求 */
        if ((pid = fork()) < 0) {
            exit(3);
        } else {
            if (pid == 0) {  /* 子行程 */
                close(listenfd);
                handle_socket(socketfd);
            } else { /* 父行程 */
                close(socketfd);
            }
        }
    }
}
