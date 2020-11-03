#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "80" //http port
#define BACKLOG 10 // 有多少個特定的連線佇列（pending connections queue）
#define BUFFERSIZE 8096

struct
{
 char *ext;
 char *filetype;

}

extensions [] =
{
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
 {0,0}
};

void load (char buffer[BUFSIZ+1],int fd,int ret)   {
    char *tmp = strstr (buffer,"filename");
    if (tmp == 0 ) return;
    char filename[BUFSIZ+1],data[BUFSIZ+1],location[BUFSIZ+1];
    memset (filename,'\0',BUFSIZ);
    memset (data,'\0',BUFSIZ);
    memset (location,'\0',BUFSIZ);
    int filesize;

    char *a,*b;
    a = strstr(tmp,"\"");
    b = strstr(a+1,"\"");
    strncpy (filename,a+1,b-a-1);
    strcat (location,"upload/");
    strcat (location,filename);

    a = strstr(buffer,"Content-Length:");
    filesize = atoi (a+15);
    a = strstr(tmp,"\n");
    b = strstr(a+1,"\n");
    a = strstr(b+1,"\n");
    b = strstr(a+1,"---------------------------");

    int download = open(location,O_CREAT|O_WRONLY|O_TRUNC|O_SYNC,S_IRWXO|S_IRWXU|S_IRWXG);

    char t[BUFSIZ+1];
    int last_write,last_ret;
    if (b != 0)
    write(download,a+1,b-a-3);
    else {
    int start = (int )(a - &buffer[0])+1;
    last_write = write(download,a+1,ret -start -61);
    last_ret = ret;
    memcpy (t,a+1+last_write,61);
    int total=0;

    total += ret;
    while ((ret=read(fd, buffer,BUFSIZ))>0) {
        total += ret;
        write(download,t,61);
        last_write = write(download,buffer,ret - 61);
        memcpy (t,buffer+last_write,61);
        last_ret = ret;
        //printf ("ret:%d\n",ret);
        if (total>=filesize-1)
            break;
    }
    }

    close(download);
    printf ("UPLOAD FILE NAME :%s\n",filename);
    return ;
}

void handle_socket(int fd)
{
 int j, file_fd, buflen, len;
 long i, ret;
 char * fstr;
 static char buffer[BUFFERSIZE+1],tmp[BUFSIZ+1];

 ret=read(fd,buffer,BUFFERSIZE); //讀取瀏覽器需求
 if(ret==0 || ret==-1)//連線有問題，結束
 {
  perror("read");
  exit(3);
 }
 
 //處理字串:結尾補0,刪換行
 if(ret>0 && ret<BUFFERSIZE)
  buffer[ret]=0;
 else
  buffer[0]=0;
 for(i=0;i<ret;i++)
 {
  if(buffer[i]=='\r' || buffer[i]=='\n')
   buffer[i] = 0;
 }

 //判斷GET命令
 if ((strncmp(buffer,"GET ",4)==0)||(strncmp(buffer,"get ",4))==0) {
        
        /* 我們要把 GET /index.html HTTP/1.0 後面的 HTTP/1.0 用空字元隔開 */
        for(i=4;i<BUFSIZ;i++) {
            if(buffer[i] == ' ') {
                buffer[i] = 0;
                break;
            }
        }

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
        {	    
        write(fd, "Failed to open file", 19);
        }	 

        /* 傳回瀏覽器成功碼 200 和內容的格式 */
        sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
        write(fd,buffer,strlen(buffer));


        /* 讀取檔案內容輸出到客戶端瀏覽器 */
        while ((ret=read(file_fd, buffer, BUFSIZ))>0) {
            write(fd,buffer,ret);
        }

        exit(1);
    }
    else if((strncmp(tmp,"POST ",5)==0)||((strncmp(tmp,"post ",5))==0)){
        //POST
        load(tmp,fd,ret);


        file_fd = open("sucessful.html", O_RDONLY);  // index.html
        sprintf(tmp,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");

        write(fd,tmp,strlen(tmp));


        /* 讀取檔案內容輸出到客戶端瀏覽器 */
        while ((ret=read(file_fd, buffer, BUFSIZ))>0) {
            write(fd,buffer,ret);
        }
    }
    else
    exit(3);
   
}


void sigchld_handler(int s)
{
    while(waitpid(-1,NULL,WNOHANG)>0);
}

//addrinfo , sockaddr_storage , sigaction

int main(void)
{
  int sockfd, new_fd; // 在 sock_fd 進行 listen，new_fd 是新的連線
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // 連線者的位址資訊 
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;


  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // 使用我的 IP
  hints.ai_protocol=0 ;


  getaddrinfo(NULL, PORT, &hints, &servinfo);


  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol); // int socket(int domain, int type, int protocol);
  //AF_UNIX/AF_LOCAL：用在本機程序與程序間的傳輸
  //AF_INET , AF_INET6 ：讓兩台主機透過網路進行資料傳輸，AF_INET使用的是IPv4協定，而AF_INET6則是IPv6協定。
  //domain 定義了socket要在哪個領域溝通，常用的有2種
  //type 說明這個socket是傳輸的手段為何
  //protocol 設定socket的協定標準，一般來說都會設為0，讓kernel選擇type對應的默認協議。

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)); // closesocket（一般不會立即關閉而經歷TIME_WAIT的過程）後想繼續重用該socket：


  if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)==-1)
  {
    perror("bind");
    exit(1);
  }	  

  freeaddrinfo(servinfo);

  listen(sockfd, BACKLOG);


  sa.sa_handler = sigchld_handler ; // 收拾全部死掉的 processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  sigaction(SIGCHLD, &sa, NULL);


  printf("connect successfully!\n");


  while(1) // 主要的 accept() 迴圈
  {	  
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) 
    {
      perror("accept");
      continue;
    }


    if (!fork()) // child process
    {

      close(sockfd); // child 不需要 listener
      handle_socket(new_fd);
      close(new_fd);

      exit(0);
    }

    close(new_fd); // parent 不需要這個
  }


  return 0 ;

}
