#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>  
#include <dirent.h> 
#include <sys/stat.h> 
#include <fcntl.h>

//bools
#define TRUE 1
#define FALSE 0

//buffer
#define BUFF_SIZE 512

//problem const values
#define COMMAND_SIZE 3
#define MAX_TITLE 10 //size of topic_name and question_name
#define REPLY_NR 10 //number of replys to send to the user
#define MAX_FILES 99 //max number topics, questions and answers
#define MAX_LEN_SIZE 10 //max length of the size of the file/image
#define ID_SIZE 5
#define MAX_ID power(10,ID_SIZE) - 1
#define MIN_ID 0

#define TOPIC_DIR "./topics"
#define ANSWER_DIR "A_"

extern int errno; 

char* port= "58029";
int topic_nr=0;

//----------
//
// Auxiliar Functions
//
//----------
int power(int x, unsigned int y){ 
    int temp; 
    if( y == 0) 
        return 1; 
    temp = power(x, y/2); 
    if (y%2 == 0) 
        return temp*temp; 
    else
        return x*temp*temp; 
} 

int convert_to_int(char *str){
	int value;

	if(sscanf(str, "%d", &value) != 1) {
		fprintf(stderr, "\nString nao int\n");
		return FALSE;
	}  

	return value;
}

char* parse_string(char *string, char* s){
	char* token;
	token=strtok(string,s);
	return token;
}

int validate_id(int id){
    if( (MIN_ID)>id || id > (MAX_ID) ){
        printf("invalid id\n");
    	return FALSE;
    }
    
    return TRUE;
}

int register_user(int id){
	return validate_id(id);
}

int get_folder_nr(char * dir){
    struct dirent *entry;
	DIR *d = opendir(dir);

	if (d == NULL){ 
		fprintf(stderr, "\nError: cant open dir\n" ); 
		return -1; 
	}
	
	// get each file
    int n=0;
	while ((entry=readdir(d)) != NULL){
        char * extension = strrchr(entry->d_name, '.');
		if (extension==NULL){
            n++;
        }
    }
	closedir(d);
    return n;
}

int duplicate_name(char *dir, char *file_name){
    printf("DUPLICATE: dir:%s\nfilename:%s\n", dir,file_name);
	struct dirent* entry;
	DIR* d = opendir(dir);

	if (d == NULL) {
		fprintf(stderr, "\nError: cant open dir\n");
		return -1;
	}

	// get each file
	while ((entry = readdir(d)) != NULL) {
		if (strcmp(entry->d_name,file_name)==0) {
			closedir(d);
			return TRUE;
		}
	}
	closedir(d);
	return FALSE;
}

int get_uid_from_file(char *uid, char *filepath){
    FILE* file=fopen(filepath, "r");
    if(file==NULL){
		fprintf(stderr, "\nError Opening File\n");
		return -1;
	}
    fscanf(file, "%s", uid);
    fclose(file);
	return TRUE;
}

int write_uid_in_file(char *uid,char *filepath){
    FILE* file=fopen(filepath, "w");
    if(file==NULL){
		fprintf(stderr, "\nError Opening File\n");
		return -1;
	}
	printf("writing id\n");
    fprintf(file, "%s", uid);
    fclose(file);
	return TRUE;
}


//-----------
//
// UDP COMMAND FUNCTIONS
//
//-------------

int propose_topic(char* id, char *topic_name){
    printf("proposing topic \n");
	if (duplicate_name(TOPIC_DIR, topic_name)) {
		return FALSE;
	}
	
    char *topic_path=malloc(sizeof(char)*(MAX_TITLE+32));        
    if(topic_path==NULL){
		fprintf(stderr, "\nError Allocating Memory\n");
		return -1;
	}
	
	sprintf(topic_path,"%s/%s",TOPIC_DIR,topic_name);
        
	if (mkdir(topic_path, 0777) == -1){
		fprintf(stderr, "\nError Creating Directory\n");
        free(topic_path);
        return -1;
    }
    
    printf("\nCreating Directory\n");
    
    strcat(topic_path,"/uid.txt");
    write_uid_in_file(id, topic_path);
    topic_nr++;
    free(topic_path);
    return TRUE;
}
    
int list_topics(char *topics){

	struct dirent* entry;
	DIR* d = opendir(TOPIC_DIR);

	// try openning current folder
	if (d == NULL) {
		fprintf(stderr, "\nError: cant open dir");
		return -1;
	}

    char *path=malloc(sizeof(char)*(MAX_TITLE+32));
    if(path==NULL){
		fprintf(stderr, "\nError Allocating Memory\n");
        closedir(d);
		return -1;
	}
    char *uid=malloc(sizeof(char)*(ID_SIZE+1));
    if(uid==NULL){
		fprintf(stderr, "\nError Allocating Memory\n");
		free(path);
        closedir(d);
		return -1;
	}

	int len = strlen(topics);
    while ((entry=readdir(d)) != NULL){
        char * extension = strrchr(entry->d_name, '.');
        
        if (extension==NULL){
			sprintf(path,"%s/%s/uid.txt",TOPIC_DIR,entry->d_name);
                
            get_uid_from_file(uid, path);
            
			len+=sprintf(topics+len," %s:%s",entry->d_name,uid); //appends
        }
    }
    strcat(topics, "\n");
    
    closedir(d);
    free(path);
    free(uid);
    
    return TRUE;
}

int list_questions(char *topic_path, char *questions){
    struct dirent *entry;
        
	DIR *d = opendir(topic_path);
    
     if (d == NULL){ 
        fprintf(stderr, "\nError: cant open dir" ); 
        return -1; 
    }
    
    char *file_path=malloc(sizeof(char)*(MAX_TITLE+64));
    if(file_path==NULL){ //path to open file, ./topics/[TOPICNAME]/[QUESTIONAME]/uid.txt
		fprintf(stderr, "\nError Allocating Memory\n");
        closedir(d);
		return -1;
	}

    char *question_path=malloc(sizeof(char)*(MAX_TITLE+32));
    if(question_path==NULL){ //path to open file, ./topics/[TOPICNAME]/[QUESTIONAME]
		fprintf(stderr, "\nError Allocating Memory\n");
		free(file_path);
        closedir(d);
		return -1;
	}
    
	char *uid=malloc(sizeof(char)*(ID_SIZE+1));
    if(uid==NULL){
		fprintf(stderr, "\nError Allocating Memory\n");
		free(file_path);
		free(question_path);
        closedir(d);
		return -1;
	}

	int len = strlen(questions);
    while ((entry=readdir(d)) != NULL){
        char * extension = strrchr(entry->d_name, '.');
        if (extension==NULL){

			sprintf(question_path,"%s/%s",topic_path,entry->d_name);		
			int file_nr = get_folder_nr(question_path);		
			sprintf(file_path,"%s/uid.txt",question_path);          
			get_uid_from_file(uid,file_path);
            
			len+=sprintf(questions+len," %s:%s:%d",entry->d_name,uid,file_nr); //apend   
        }
    }
    strcat(questions, "\n");    

    closedir(d);
    free(uid);
    free(question_path);
    free(file_path);
    
    return TRUE;
}


// ________________________________________________________________________________
// 
// 										CONNECTIONS
// ________________________________________________________________________________

int open_udp(){
	struct addrinfo hints,*res;
	int fd,errcode;

	memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_DGRAM;//UDP socket
    hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;
    
    if((errcode=getaddrinfo(NULL,port,&hints,&res))!=0){
    	exit(1);     
    }

    if((fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol))==-1){
        exit(1);    
    }

    if(bind(fd,res->ai_addr,res->ai_addrlen)==-1){
        exit(1);    
    }
	
	return fd;
}

int udp(){
	struct sockaddr_in addr;
	socklen_t addrlen;
	ssize_t nread;
	int fd=open_udp();

    char *buffer = malloc( sizeof(char) *(BUFF_SIZE));
    if(buffer==NULL){
		fprintf(stderr,"\nError Allocating Memory\n");
		exit(-1);
	}

    char *command= malloc( sizeof(char) *(COMMAND_SIZE+1));
    if(command==NULL){
		fprintf(stderr,"\nError Allocating Memory\n");
		exit(-1);
	}
    
	char *string= malloc( sizeof(char) *(BUFF_SIZE-COMMAND_SIZE+1));
    if(string==NULL){
		fprintf(stderr,"\nError Allocating Memory\n");
		exit(-1);
	}
    int id;
    char *uid; //uid is id but in char

    while(1){
        addrlen=sizeof(addr);
        nread = recvfrom(fd,buffer,BUFF_SIZE,0,(struct sockaddr*)&addr,&addrlen);
        buffer[nread]='\0';
        printf("AFTR READ, buffer:%s\n",buffer);
        
        if(nread!=-1){
       
        command= parse_string(buffer, " \n");
        
        if(strlen(command)!=COMMAND_SIZE){
            sendto(fd,"ERR\n",strlen("ERR\n"),0,(struct sockaddr*)&addr,addrlen);
        	printf("ERR\n");
        }
                
        if(strncmp(command, "REG",COMMAND_SIZE) ==0){ //RGR ID, Answer:  RGR STATUS ->reg user
            string=parse_string(NULL, "\n"); //string=id
                
            if(nread>(COMMAND_SIZE+ID_SIZE+2) || strlen(string)!=5 || !(id=convert_to_int(string))){ //"COMMAND ID\n"
                sendto(fd,"RGR NOK\n",strlen("RGR NOK\n"),0,(struct sockaddr*)&addr,addrlen);
                printf("RGR NOK\n");
            }
                
            else{
                if(register_user(id)){ //Tudo OK           
                    sendto(fd,"RGR OK\n",strlen("RGR OK\n"),0,(struct sockaddr*)&addr,addrlen);
                    printf("RGR OK\n");
                }
                        
                else{ //Not Valid ID
                    sendto(fd,"RGR NOK\n",strlen("RGR NOK\n"),0,(struct sockaddr*)&addr,addrlen);
                    printf("RGR NOK\n"); 
                }
            }
        }

        else if(strncmp(command, "LTP",COMMAND_SIZE)==0){ //LTP, Answer: LTR N (topic:userID )* ->topic list
            if(nread!=COMMAND_SIZE+1){// +1\n
			    sendto(fd,"ERR\n",strlen("ERR\n"),0,(struct sockaddr*)&addr,addrlen);
				printf("ERR\n");
			}
			
			else{
				
				char* topics = malloc(sizeof(char) * topic_nr * (MAX_TITLE+ID_SIZE+2) + 7);
                if(topics!=NULL){ //7=LTR N \n

                    sprintf(topics,"LTR %d",topic_nr);                
                    
                    list_topics(topics);   
                    printf("TOPICS:%s\n", topics);            
                    sendto(fd,topics,strlen(topics),0,(struct sockaddr*)&addr,addrlen);
                    
                    free(topics);
				}
			}
        }
        
        else if(strncmp(command, "PTP",COMMAND_SIZE)==0){ //PTP ID topic, Answer: PTR STATUS ->topic propose
            if(nread>COMMAND_SIZE+ID_SIZE+MAX_TITLE+3){
				sendto(fd,"ERR\n",strlen("ERR\n"),0,(struct sockaddr*)&addr,addrlen);
				printf("ERR\n");
			}
			else{
				
				if(topic_nr==MAX_FILES){  //FULL
					sendto(fd,"PTR FUL\n",strlen("PTR FUL\n"),0,(struct sockaddr*)&addr,addrlen);
					printf("PTR FULL\n");
				}
				
				else{                
					if(strlen(uid=parse_string(NULL," "))!=ID_SIZE){
						sendto(fd,"PTR NOK\n",strlen("PTR NOK\n"),0,(struct sockaddr*)&addr,addrlen);
					}
					string=parse_string(NULL, "\n"); //string=topic
					
					int status=propose_topic(uid,string);
					if(status==TRUE){
							//topic created
							sendto(fd,"PTR OK\n",strlen("PTR OK\n"),0,(struct sockaddr*)&addr,addrlen);
							printf("PTR status: %d\n", status);
					}
					else if(status==FALSE){
						//Duplicate topic
						sendto(fd,"PTR DUP\n",strlen("PTR DUP\n"),0,(struct sockaddr*)&addr,addrlen);
						printf("PTR status: %d\n", status);
					}
								
					else{
						//??
						sendto(fd,"PTR NOK\n",strlen("PTR NOK\n"),0,(struct sockaddr*)&addr,addrlen);
						printf("PTR status: %d\n", status);
					}
				}
			}

        }
        
        else if(strncmp(command, "LQU",COMMAND_SIZE)==0){ //LQU topic, Answer:LQR N (question:userID:NA )* ->question titles for the topic
			if(nread>COMMAND_SIZE+MAX_TITLE+2){
				sendto(fd,"ERR\n",strlen("ERR\n"),0,(struct sockaddr*)&addr,addrlen);
			}
			else{
				string=parse_string(NULL, "\n"); //messagic=topic;
				
				char *topic_path=malloc(sizeof(char)*MAX_TITLE+2+strlen(TOPIC_DIR));
				if(topic_path!=NULL){

					sprintf(topic_path,"%s/%s",TOPIC_DIR,string);
										   
					int question_nr=get_folder_nr(topic_path);
											
					char * questions=malloc(sizeof(char)*MAX_FILES*(MAX_TITLE+ID_SIZE+5)+7);
                    if(questions!=NULL){ //5= :+:+NA+" ",7="LQR N \n"

						sprintf(questions, "LQR %d",question_nr);
					
						list_questions(topic_path, questions);    
						printf("QUESTIONS: %s\n", questions);                
						sendto(fd,questions,strlen(questions),0,(struct sockaddr*)&addr,addrlen);

						free(questions);
					}
				free(topic_path);				
				}
				
			}
        }
        
        
        else{ //Invalid Command
        	sendto(fd,"ERR\n",strlen("ERR\n"),0,(struct sockaddr*)&addr,addrlen);
        	printf("ERR\n");
        }
    }
	}

    //free(buffer)
    //free(string)
    //free(command)
    //freeaddrinfo(res);
    //close(fd);
    //exit(0);
	return -1;
}

int read_tcp_space(int newfd, char *buffer, int max){ //reads till space or \n (if reads max chars, error)
    int nread;
    char aux[2];
    nread=read(newfd,aux,1);
    aux[1]='\0';
    strcpy(buffer,aux);
    
    while(strcmp(aux," ")!=0 && strcmp(aux,"\n")!=0){       
        nread+=read(newfd,aux,1);
        aux[1]='\0';
        
        if(nread==-1){
            printf("Exiting nread=-1\n");
            exit(1);
        }
        if(nread!=0 && strcmp(aux," ")!=0 && strcmp(aux,"\n")!=0){
            strcat(buffer,aux);
        } 
		
		if(nread==max){ //ERROR
			fprintf(stderr,"\nError: max chars read from socket\n");
			exit (-1);
		}
    }
    printf("tcp buffer:(%s)\nEND\n", buffer);
    return nread;
}
    
    
int read_tcp_size(int newfd, char *buffer, int n){ //read exactly n chars to buffer
    int nread;
    int nleft=n;
	int total_read=0;
    
    char* ptr=buffer;
    
    while(nleft>0){
        nread=read(newfd,ptr,nleft);
        ptr[nread]='\0';

        if(nread==-1){
            printf("Exiting nread=-1\n");
            exit(1);
        }
        
        ptr+=nread;
        nleft-=nread; 
		total_read+=nread;
        
    }
    printf("\nEND\n");
	if(total_read!=n){
		fprintf(stderr,"\nError reading from socket\n");
		exit(-1);
	}
	
    if(total_read<BUFF_SIZE-2){
        printf("tcp buffer:(%s)\n", buffer);
    }
    return total_read; //total number of read chars*/
}

int read_tcp_file(int newfd, char *path, int size){
	char buffer[BUFF_SIZE];
	FILE *file=fopen(path,"w");
    int nleft=size;  
	int nread;
	int nwrite=0;
    printf("nleft:%d\n",nleft);
    while(nleft>0){
        if(nleft<BUFF_SIZE){
			nread=read_tcp_size(newfd,buffer,nleft);
        }
        else{
            nread=read_tcp_size(newfd,buffer,BUFF_SIZE-1);
        }
        nwrite+=fwrite(buffer,1,nread,file);
        nleft-=nread; 
	}
	fclose(file);
	if(nwrite!=size){
		fprintf(stderr,"\nError writing in file\n");
		exit(-1);
	}
	return nwrite;
}


int write_tcp(int newfd, char* buffer, int n){
    int nleft=n;
    int nwrite;
    int sum=0;
    char *ptr=buffer;
    while(nleft>0){
        nwrite=write(newfd,ptr,nleft);
        sum+=nwrite;
        ptr+=nwrite;
        nleft-=nwrite;        
    }
	
	if(sum!=n){
		fprintf(stderr,"\nError writing in socket\n");
		exit(-1);
	}
    if(sum<BUFF_SIZE-2){
        printf("Write tcp:(%s)\n", buffer);
    }
    return sum;
}

int write_tcp_file(int newfd, char* path, int n){
	int nleft=n;
	char buffer[BUFF_SIZE];
	int nread;
	int nwrite=0;
	FILE *f=fopen(path,"r");                  
    while (nleft>0){
        if(nleft<BUFF_SIZE){
            nread=fread(buffer, 1, nleft, f);
            buffer[nread]='\0';
        }
        else{
            nread=fread(buffer, 1, BUFF_SIZE-1, f);
            buffer[nread]='\0';
        }
                                
        nwrite+=write_tcp(newfd,buffer,nread);
        nleft-=nread;
    }
	
	if(nwrite!=n){
		fprintf(stderr,"\nError writing to socket\n");
		exit(-1);
	}
	return nwrite;
}
   
int image_exits(char *path,char *image_name, char *ext){
    struct dirent *entry;
	DIR *d = opendir(path);
    int len=strlen(image_name);
	
	if(d==NULL){
		fprintf(stderr,"\nError opening dir\n");
		exit(-1);
	}
    
    while ((entry=readdir(d)) != NULL){
        if(strncmp(entry->d_name,image_name,len)==0){
            char * extension = strrchr(entry->d_name, '.');
            if(strcmp(extension,".txt")!=0){
                strcpy(ext,extension+1);
                closedir(d);
                return TRUE;
            }
        }
    }
    closedir(d);
    return FALSE;
}

int open_tcp(){
	
	struct sigaction act;
	struct addrinfo hints, *res;
	int fd, ret; 

	memset(&act,0,sizeof act);

	act.sa_handler=SIG_IGN;

	if(sigaction(SIGCHLD,&act,NULL)==-1){
        exit(1);
    }
    
	memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;		//IPv4
    hints.ai_socktype=SOCK_STREAM;		//TCP socket
    hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;
    
    if((ret=getaddrinfo(NULL,port,&hints,&res))!=0){
    	exit(1);
    }

    if((fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol))==-1){
        exit(1);
    }
    
    if(bind(fd,res->ai_addr,res->ai_addrlen)==-1){
        printf("exiting tcp bind\n");
        exit(1);
    }
    
    if(listen(fd,5)==-1){	
        exit(1);
    }
    
    freeaddrinfo(res);	//frees the memory allocated by getaddrinfo
	return fd;
}

int tcp(){
    struct sockaddr_in addr;
    socklen_t addrlen;

	int newfd, ret;
	int fd=open_tcp();    
	
    while(1){
        printf("waiting tcp\n");

    	addrlen=sizeof(addr);
        newfd=accept(fd,(struct sockaddr*)&addr,&addrlen);//wait for a connection
        printf("accepting tcp\n");
        if(newfd==-1){
            exit(1);
        }

        pid_t pid = fork();
        
        if(pid<0){  
            exit(1);    //error
        }
        
        else if(pid==0){ //if child process
        	close(fd);
                       
            char *buffer=malloc(sizeof(char)*BUFF_SIZE);
            if(buffer==NULL){
				fprintf(stderr,"\nError Allocating Memory\n");
				exit(-1);
			}
            //char buffer[BUFF_SIZE];

        	char *command=malloc(sizeof(char)*(COMMAND_SIZE+1));
            if(command==NULL){
				fprintf(stderr,"\nError Allocating Memory\n");
				exit(-1);	
			}
            
            char *topic_name=malloc(sizeof(char)*(MAX_TITLE+1));
            if(topic_name==NULL){
				fprintf(stderr,"\nError Allocating Memory\n");
				exit(-1);
			}

            char *question_name=malloc(sizeof(char)*(MAX_TITLE+1));
            if(question_name==NULL){
				fprintf(stderr,"\nError Allocating Memory\n");
				exit(-1);
			}

            char *uid=malloc(sizeof(char)*ID_SIZE+1);
            if(uid==NULL){
				fprintf(stderr,"\nError Allocating Memory\n");
				exit(-1);
			}
            
            char *path=malloc(sizeof(char)*(MAX_TITLE*2+32));
            if(path==NULL){
				fprintf(stderr,"\nError Allocating Memory\n");
				exit(-1);
			}
            
            read_tcp_space(newfd,command,COMMAND_SIZE+2);
            
            if(strlen(command)!=3){
                write_tcp(newfd,"ERR\n", strlen("ERR\n"));
                printf("ERR\n");
            }
            
            else if(strncmp(command, "GQU",COMMAND_SIZE)==0){//GQU topic question, Answer: QGR qUserID  qsize  qdata  qIMG [qiext  qisize  qidata]  N (AN aUserID asize adata aIMG [aiext aisize aidata])*->question get
                                
                read_tcp_space(newfd,topic_name,MAX_TITLE+2);
                read_tcp_space(newfd,question_name,MAX_TITLE+2); 

                sprintf(path,"%s/%s/%s",TOPIC_DIR,topic_name,question_name);
                //path=./topics/topic_name/question_name
                
                char *uid_path=malloc(sizeof(char)*(MAX_TITLE*2+64));
                if(uid_path==NULL){
					fprintf(stderr,"\nError Allocating Memory\n");
					exit(-1);
				}
				sprintf(uid_path,"%s/uid.txt",path);         
                //uid_path=[path]/uid.txt
				
                get_uid_from_file(uid,uid_path);

                char *q_file=malloc(sizeof(char)*(MAX_TITLE*2+32));
                if(q_file==NULL){
					fprintf(stderr,"\nError Allocating Memory\n");
					exit(-1);
				}
				sprintf(q_file,"%s/%s.txt",path,question_name);
                //q_file=[path]/question_name.txt

                struct stat size;
                stat(q_file, &size);
				
				//Writes GQR qUserID qsize
				sprintf(buffer,"QGR %s %ld ",uid, size.st_size);                               
                write_tcp(newfd, buffer, strlen(buffer));
                
				//Writes qData
                write_tcp_file(newfd, q_file,size.st_size);
                
                char img_ext[4];                            
                if(image_exits(path,question_name,img_ext)){ //if image exists, returns true and fills img_ext                    
                    char qi_path[64];
					sprintf(qi_path,"%s/%s.%s",path,question_name,img_ext);
                    
					//get image size
                    stat(qi_path, &size);
    
					sprintf(buffer," 1 %s %ld ",img_ext, size.st_size);
                    write_tcp(newfd, buffer, strlen(buffer));
                    
					//sends data
                    write_tcp_file(newfd, qi_path, size.st_size);
                }
                
                else{
                    write_tcp(newfd, " 0", strlen(" 0"));
                }

                int answer_nr=get_folder_nr(path);
                
                int get_answer=1;
                //only sends 10replys
                if(answer_nr>REPLY_NR){
                    get_answer=answer_nr-REPLY_NR+1;
                    answer_nr=REPLY_NR;
                }

                if(answer_nr==0){
                    write_tcp(newfd, " 0\n", strlen(" 0\n"));
                }
				
                else{
					sprintf(buffer," %d ",answer_nr);
                    write_tcp(newfd, buffer, strlen(buffer));
                    
                    char *answer_folder_path=malloc(sizeof(char)*(MAX_TITLE*2+64));
                    if(answer_folder_path==NULL){
						fprintf(stderr,"\nError Allocating Memory\n");
						exit(-1);
					}
                    char *answer_file_path=malloc(sizeof(char)*(MAX_TITLE*2+64));
                    if(answer_file_path==NULL){
						fprintf(stderr,"\nError Allocating Memory\n");
						exit(-1);
					}

                    
                    for(int i=0;i<answer_nr;i++){
						sprintf(answer_folder_path,"%s/%s%d",path,ANSWER_DIR,get_answer);
                                            
                        //get answer uid
						sprintf(answer_file_path,"%s/uid.txt",answer_folder_path);
                        get_uid_from_file(uid, answer_file_path);
                        
                        //get answer file path
                        sprintf(answer_file_path,"%s/%s%d.txt",answer_folder_path,ANSWER_DIR,get_answer);
                        
						//get answer size
                        stat(answer_file_path, &size);
                        
                        //sends AN aUserID asize
                        if(get_answer>=10){ //answer number has 2 digits
							sprintf(buffer,"%d %s %ld ",get_answer,uid,size.st_size);
                        }
                        else{
							sprintf(buffer,"0%d %s %ld ",get_answer,uid,size.st_size);
						}
                        printf("I:%d\nAN:%d\n",i,get_answer);
                        write_tcp(newfd, buffer, strlen(buffer));
                        
                        //sends adata                        
                        write_tcp_file(newfd, answer_file_path,size.st_size);
                        
                        //sends aIMG                        
                        char img_name[8];
						sprintf(img_name,"%s%d",ANSWER_DIR,get_answer);
                                               
                        if(image_exits(answer_folder_path,img_name,img_ext)){ //if image exists, returns true and fills img_ext
                            
							//get img path
                            char ai_path[64];
							sprintf(ai_path,"%s/%s.%s",answer_folder_path,img_name,img_ext);
                            
							//get img size
							stat(ai_path, &size);
                            
							//send iFLAG iSIZE
							sprintf(buffer," 1 %s %ld ",img_ext,size.st_size);
                            write_tcp(newfd, buffer, strlen(buffer));
							
							//sends iDATA
                            write_tcp_file(newfd, ai_path,size.st_size);
                        }
                        
                        else{
                            write_tcp(newfd, " 0",strlen(" 0"));                    

                        }
                        
                        if(i<(answer_nr-1)){
                            write_tcp(newfd, " ",strlen(" "));                    

                        }
                        else{
							write_tcp(newfd,"\n",strlen("\n"));                    
                        }
                        
                        get_answer++;
                    }
                    free(answer_file_path);
                    free(answer_folder_path);
                }
                free(q_file);
                free(uid_path);
            }
    
            else if(strcmp(command, "QUS")==0){//QUS  qUserID  topic  question  qsize  qdata  qIMG  [iext isize idata], Answer: QUR status -> submit question
                printf("QUS\n");
                  
                read_tcp_space(newfd,uid,ID_SIZE+2);
                read_tcp_space(newfd,topic_name,MAX_TITLE+2);
                read_tcp_space(newfd,question_name,MAX_TITLE+2);
                
                printf("uid: %s\n", uid);
                printf("topic: %s\n", topic_name);
                printf("question: %s\n", question_name);
                
                char qsize[MAX_LEN_SIZE];                
                read_tcp_space(newfd,qsize,MAX_LEN_SIZE+2);
                int nleft=convert_to_int(qsize);
                
				sprintf(path,"%s/%s",TOPIC_DIR,topic_name);
                //path=./topics/topic_name


				if(duplicate_name(path,question_name)){
                    printf("QUR DUP\n");
					write_tcp(newfd,"QUR DUP\n",strlen("QUR DUP\n"));
				}
				
				else if(get_folder_nr(path)==MAX_FILES){
                    printf("QUR FUL\n");
					write_tcp(newfd, "QUR FUL\n", strlen("QUR FUL\n"));
				}
				
				else{
                    strcat(path, "/");
                    strcat(path, question_name);
                    //path=./topics/topic_name(question_name
					if (mkdir(path, 0777) == -1){
						exit(-1);
					}
					
					else{    
						
						//write uid.txt    
						
						char *path_uid=malloc(sizeof(char)*(strlen(path)+16));
                        if(path_uid==NULL){
							fprintf(stderr,"\nError Allocating Memory\n");
							exit(-1);
						}
							
						sprintf(path_uid,"%s/uid.txt",path);
						write_uid_in_file(uid,path_uid);
						free(path_uid);
						
						//read from socket and writes in question_name.txt
						
						strcat(path,"/");
						strcat(path,question_name);
						strcat(path,".txt");        
						read_tcp_file(newfd, path,nleft);
						
						read_tcp_size(newfd,buffer,3);//get " IMGFLAG "
						
						if(strcmp(buffer," 1 ")==0){ //if image
							
							char ext[4];
							read_tcp_size(newfd,ext,3);//get "img_extension" 
							read_tcp_size(newfd,buffer,1);//get space
							read_tcp_space(newfd,buffer,MAX_LEN_SIZE+2); //get imgsize                      
							nleft=convert_to_int(buffer);
							
							char img_path[64];
							sprintf(img_path,"%s/%s/%s/%s.%s",TOPIC_DIR,topic_name,question_name,question_name,ext);
							
							read_tcp_file(newfd, img_path,nleft);
							read_tcp_size(newfd,buffer,1); //get "\n"
						}
						printf("QUR OK\n");
						write_tcp(newfd, "QUR OK\n", strlen("QUR OK\n"));                        
					}
				}
                    
            }
            
            else if(strcmp(command, "ANS")==0){//ANS aUserID topic  question  asize  adata  aIMG  [iext isize idata], Anser: ANR status ->submit answer
                printf("ANS\n");
                    
                read_tcp_space(newfd,uid,ID_SIZE+2);
                read_tcp_space(newfd,topic_name,MAX_TITLE+2);
                read_tcp_space(newfd,question_name,MAX_TITLE+2);
                
                printf("uid: %s\n", uid);
                printf("topic: %s\n", topic_name);
                printf("question: %s\n", question_name);
                
                char asize[MAX_LEN_SIZE];
                read_tcp_space(newfd,asize,MAX_LEN_SIZE+2);
                int nleft=convert_to_int(asize);
                
				sprintf(path,"%s/%s/%s",TOPIC_DIR,topic_name,question_name);
                //path=./topics/topic_name/question_name
                
                int answer_nr=get_folder_nr(path);
				
				if(answer_nr==MAX_FILES){
					write_tcp(newfd, "ANR FUL\n", strlen("ANR FUL\n"));
				}
				
				else{

					answer_nr++;
                    int len=strlen(path);
                    len+=sprintf(path+len,"/%s%d",ANSWER_DIR,answer_nr); //append             
					//path=./topics/topic_name/question_name/A_N

					if (mkdir(path, 0777) == -1){
						exit(-1);
					}
					
					else{                   
						
						//write uid.txt
						
						char *path_uid=malloc(sizeof(char)*(len+16));
                        if(path_uid==NULL){
							fprintf(stderr,"\nError Allocating Memory\n");
							exit(-1);
						}
							
						sprintf(path_uid,"%s/uid.txt",path); 
						write_uid_in_file(uid,path_uid);
						free(path_uid);
						
						//reads from socket and writes in A_N.txt
						
                        sprintf(path+len,"/%s%d.txt",ANSWER_DIR,answer_nr);

						read_tcp_file(newfd, path,nleft);                    
						read_tcp_size(newfd,buffer,3);//get " IMGFLAG "
						
						if(strcmp(buffer," 1 ")==0){ //if image
							
							char ext[4];
							read_tcp_size(newfd,ext,3);//get "img_extension" 
							read_tcp_size(newfd,buffer,1);//get space
							read_tcp_space(newfd,buffer,MAX_LEN_SIZE+2); //get imgsize                        
							nleft=convert_to_int(buffer);
							
							char img_path[64];
							sprintf(img_path,"%s/%s/%s/%s%d/%s%d.%s",TOPIC_DIR,topic_name,question_name,ANSWER_DIR,answer_nr,ANSWER_DIR,answer_nr,ext);
                            
                            printf("IMG PATH:(%s),nleft:%d\n",img_path,nleft);
							
							read_tcp_file(newfd, img_path,nleft);
							read_tcp_size(newfd,buffer,1); //get "\n"
						}
						
						printf("ANR OK\n");
						write_tcp(newfd, "ANR OK\n", strlen("ANR OK\n"));                        
					}
				}
            }
           
           
           else{ //??
                write_tcp(newfd,"ERR\n", strlen("ERR\n"));
                printf("ERR\n");
            }
            
            printf("ending tcp\n");
            
            free(buffer);
            free(command);
            free(topic_name);
            free(question_name); 
            free(uid);
            free(path);

        	close(newfd); 
            printf("killing child\n");
        	exit(0);
        }
        
        //parent process
        printf("parent\n");
        ret=close(newfd);
        if(ret==-1){
            exit(1);
        }
    }
    
}

// ________________________________________________________________________________
// 
// 										MAIN
// ________________________________________________________________________________

int main (int argc, char** argv) { /* FS [-p port]*/

    printf("NR ARGs: %d\n", argc);

    if(argc!=1){
    	if(argc!=3){
    		fprintf(stderr, "\nErro: Numero de argumentos invalidos.\n");
    		return -1;
    	}

    	if(strcmp("-p",argv[1])!=0){
    		fprintf(stderr, "\nErro: Argumentos invalidos.\n");
    		return -1;
    	}

    	if(convert_to_int(argv[2])){
    		port=argv[2];
    		printf("port: %s\n",port);
    	}

    	else{
    		fprintf(stderr, "\nErro: Argumentos invalidos.\n");
    		return -1;
    	}
    }
    else{
        printf("1 argument, port: %s\n",port);	//default argument for port
    }
    
    topic_nr=get_folder_nr(TOPIC_DIR); //a.out and main.c


    // ________________________________________________________________________________

    // 1ST FORK  
    // after processing args we begin services

 
    pid_t pid = fork();

    if (pid<0){        
    	fprintf(stderr, "\nErro: no Fork.\n");	//Error
		exit(-1);
    }

    else if(pid==0){ //child
        tcp();	//open for any tcp requests (file transfers will be in yet another fork)
        exit(-1); //Should never reach this
    }

    else{ // parent. the pid is child's pid for parent to use
        udp();	//open for requests like listings,tp, etc
        exit(-1); //Should never reach this
    }


    printf("END OF PROGRAM\n"); //Should never reach this
    return 0;    
}



 
