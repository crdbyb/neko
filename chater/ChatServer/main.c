#include "type.h"

struct user users[MaxUsers];
int userNum;
struct group groups[MaxGroups];
int groupNum;
struct message *buffer[1024];
int buffLen=0;
struct epoll_event ev;
int epfd;
int curfds;

void setnonblocking(int sock)
{
     int opts;
     opts=fcntl(sock,F_GETFL);
     if(opts<0){
		printf("fcntl(sock,GETFL)\n");
		exit(1);
     }
	opts = opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<0){
		printf("fcntl(sock,SETFL,opts)\n");
		exit(1);
	} 
}
int findstr(char *s_str,char *d_str){
	int i,j,k,count=-1;
	char *p=strstr(s_str,d_str);
	if(p!=NULL)
		count=p-s_str;
	return count;
}
void * fileTransfer(void *args)
{
	struct FileAttr *attr=(struct FileAttr *)args;	
	unsigned char bt[1024];
	int count=0;
	bool ret1=true;
	bool ret2=true;
	while(1){
		int n1 = read(attr->from->fd, bt, 1024);
		if(n1<0){
			continue;
			close(attr->from->fd);
			ret1=false;
			break;
		}
		int n2=write(attr->to->fd, bt ,n1);
		if(n2<0){
			close(attr->to->fd);
			ret2=false;
			break;
		}
		count+=n1;
		if(count==attr->len)
			break;		
	}	
	if(ret1)
		attr->from->status=1;				
	else attr->from->status=0;	
	if(ret2)
		attr->to->status=1;				
	else attr->to->status=0;	
	ev.events = EPOLLIN; 
    	ev.data.fd = attr->from->fd; 	
	if(ret1){
		epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd , &ev);
		curfds++; 
	}
	ev.data.fd = attr->to->fd;
	if(ret2){
		epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd , &ev);
		curfds++; 
	}
	free(attr);
}
void sendPack(int sockfd){
	unsigned char reply[1024];
	for(int i=0;i<buffLen;i++){
		struct message *temp=buffer[i];//printf("%s\n",temp->to);
		if(temp->tofd==-1)
			continue;		
		if(temp->tofd==sockfd){
			temp->sendTime[strlen(temp->sendTime)-1]=0;
			int len1=4+strlen(temp->from)+strlen(temp->to)+strlen(temp->sendTime)+3+strlen(temp->content);
			int type=temp->type;
			memcpy(reply,&len1,4);
			memcpy(reply+4,&type,4);
			sprintf(reply+8,"%s\n%s\n%s\n%s",temp->from,temp->to,temp->sendTime,temp->content);//printf("reply=%s %s %sjj %s \n",temp->from,temp->to,temp->sendTime,temp->content);
			FILE* fp=fopen ("log.txt","at+");
			fprintf(fp,"sent:%s %s %s %s\n", temp->from,temp->to,temp->sendTime,temp->content);
			fclose(fp);
			if(write(sockfd,reply,len1+4)!=(len1+4)){
				close(sockfd);
				epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,&ev);
				curfds--;
				for(int j=0;j<userNum;j++){
					if(strcmp(users[j].id,temp->to)==0){
						users[j].status=0;						
						break;
					}
				}
				free(temp);
				for(int k=i;k<buffLen-1;k++)
					buffer[k]=buffer[k+1];						
				buffLen--;	
				continue;
			}			
			if(temp->type==SendFile&&findstr(temp->content,"ok\n")!=-1){
				struct FileAttr *attr=malloc(sizeof(struct FileAttr));
				attr->len=atoi(temp->content+10);
				for(int k=0;k<userNum;k++){
					if(strcmp(temp->from,users[k].id)==0){
						attr->to=&users[k];
						users[k].status=2;						
					}
					if(strcmp(temp->to,users[k].id)==0){
						attr->from=&users[k];
						users[k].status=2;						
					}
				}
				int  pos1=findstr(temp->content,"\n");
				char sizestr[16];
				int pos2=findstr(temp->content+pos1+1,"\n");
				strncpy(sizestr,temp->content+pos1+1,pos2);
				attr->len=atoi(sizestr);
				ev.data.fd = attr->from->fd;
				epoll_ctl(epfd,EPOLL_CTL_DEL,attr->from->fd,&ev);
				ev.data.fd = attr->to->fd;
				epoll_ctl(epfd,EPOLL_CTL_DEL,attr->to->fd,&ev);
				curfds--; 
				curfds--; 				
				pthread_t thread;	
				pthread_create(&thread, NULL, fileTransfer, (void *)attr);
			}else{
				ev.events = EPOLLIN; 
				ev.data.fd = sockfd;
				epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
			}
			free(temp);
			for(int k=i;k<buffLen-1;k++)
				buffer[k]=buffer[k+1];						
			buffLen--;			
		}
	}
}
struct message parsePack(char msg[1024], int len){
	struct message pack;
	char *p=msg+4;
	int pos1=findstr(p,"\n");
	char *p3=p+pos1;
	*p3='\0';
	strcpy(pack.from,p);
	char *p2=p+pos1+1;
	int pos2=findstr(p2,"\n");
	p3=p2+pos2;
	*p3='\0';
	strcpy(pack.to,p2);
	memset(pack.content,0,512);
	strncpy(pack.content,p3+1,len-4-strlen(pack.from)-strlen(pack.to)-2);	//printf("recv=%s %s %s \n",pack.from,pack.to,pack.content);
	char sendTime[64];
	time_t timep;
	time(&timep);
	strcpy(sendTime,ctime(&timep));	
	sendTime[strlen(sendTime)-1]=0;
	FILE* fp=fopen ("log.txt","at+");
	fprintf(fp,"recerived:%s %s %s %s\n", pack.from,pack.to,sendTime,pack.content);
	fclose(fp);
	return pack;
}
void handleLogin(struct message pack,int sockfd){//from to sendtime content	
	bool find=false;
	int i=0;
	for(i=0;i<userNum;i++){
		if(strcmp(users[i].id,pack.from)==0&&strcmp(users[i].pass,pack.content)==0&&users[i].status==0){
			users[i].status=1;
			users[i].fd=sockfd;
			find=true;
			break;
		}
	}
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=Login;
	temp->tofd=sockfd;
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));	
	if(find)	
		strcpy(temp->content,"ok");
	else
		strcpy(temp->content,"error");
	for(int i=buffLen;i>0;i--){
		buffer[i]=buffer[i-1];
	}buffLen++;buffer[0]=temp;
	//buffer[buffLen]=temp;
	//buffLen++;	
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
}
void handleRegister(struct message pack,int sockfd){//from to sendtime content	
	bool find=false;
	int i=0;
	for(i=0;i<userNum;i++){
		if(strcmp(users[i].id,pack.from)==0){
			find=true;
			break;
		}
	}
	if(strcmp(pack.from,"server")==0)
		find=true;
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=Register;
	temp->tofd=sockfd;
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	if(find)	
		strcpy(temp->content,"error");
	else{
		FILE* fp=fopen ("users.txt","at+");
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"%s\t%s\n", pack.from,pack.content);
		fclose(fp);
		char fileName[128];
		sprintf(fileName,"user_%s.txt",pack.from);
		fp=fopen (fileName,"at+");
		fclose(fp);
		strcpy(temp->content,"ok");
		strcpy(users[userNum].id,pack.from);
		strcpy(users[userNum].pass,pack.content);
		users[userNum].fd=-1;
		users[userNum].status=0;
		userNum++;
	}
	buffer[buffLen]=temp;
	buffLen++;
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
}
void handleLogout(struct message pack,int sockfd){//from to sendtime content	
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=Logout;
	temp->tofd=sockfd;
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->content,"ok");
	buffer[buffLen]=temp;
	buffLen++;
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
}
void handleFindPass(struct message pack,int sockfd){//from to sendtime content	
	bool find=false;
	int i=0;
	for(i=0;i<userNum;i++){
		if(strcmp(users[i].id,pack.from)==0){
			find=true;
			break;
		}
	}
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=FindPass;
	temp->tofd=sockfd;
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	if(find)
		sprintf(temp->content,"ok\n%s\n",users[i].pass);
	else sprintf(temp->content,"error");
	buffer[buffLen]=temp;
	buffLen++;
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
}
void handleAddFriend(struct message pack,int sockfd){//from to sendtime content	
	bool find=false;
	int i=0;
	for(i=0;i<userNum;i++){
		if(strcmp(users[i].id,pack.to)==0){
			find=true;
			break;
		}
	}
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=AddFriend;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	if(find){         //printf("4444444444\n");///////////////////
		strcpy(temp->from,pack.from);
		strcpy(temp->to,pack.to);
		strcpy(temp->content,pack.content);//content: request  ok  cancel
		if(strcmp(temp->content,"ok")==0){
			char fileName[128];
			sprintf(fileName,"user_%s.txt",pack.from);
			FILE* fp=fopen (fileName,"at+");
			fseek(fp,0L,SEEK_SET);
			bool find2=false;
			char line[64];
			while(1){
				char *ret=fgets(line,64,fp);
				if(ret==NULL)
					break;
				line[strlen(line)-1]=0;
				if(strcmp(line,pack.to)==0){
					find2=true;
					fclose(fp);
					break;
				}
			}
			if(!find2){
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"%s\n", pack.to);
				fclose(fp);
				sprintf(fileName,"user_%s.txt",pack.to);
				fp=fopen (fileName,"at+");
				fseek(fp,0L,SEEK_END);
				fprintf(fp,"%s\n", pack.from);
				fclose(fp);
			}
		}
		temp->tofd=users[i].fd;
		if(users[i].status!=1)
			temp->tofd=-1;
		else{
			ev.data.fd=temp->tofd;
			ev.events=EPOLLOUT;
			epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);
		}
	}else{
		strcpy(temp->to,pack.from);
		strcpy(temp->from,"server");
		temp->tofd=sockfd;
		sprintf(temp->content,"NotFind");//printf("333333333333\n");
		ev.data.fd=sockfd;
		ev.events=EPOLLOUT;
		epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	}
	buffer[buffLen]=temp;
	buffLen++;	
}
void init(){
	char *ret;
	FILE* fp1=fopen ("users.txt","at+");
	fseek(fp1,0L,SEEK_SET);
	char line[64];
	userNum=0;
	while(1){
		ret=fgets(line,64,fp1);
		if(ret==NULL)
			break;		
		line[strlen(line)-1]=0;
		int pos=findstr(line,"\t");
		line[pos]=0;
		strcpy(users[userNum].id,line);
		strcpy(users[userNum].pass,line+pos+1);//printf("%s\n",users[userNum].pass);
		users[userNum].status=0;
		userNum++;
	}
	fclose(fp1);
	fp1=fopen ("groups.txt","at+");
	fseek(fp1,0L,SEEK_SET);
	while(1){
		ret=fgets(line,64,fp1);
		if(ret==NULL)
			break;
		line[strlen(line)-1]=0;
		int pos=findstr(line,"\t");
		line[pos]=0;
		strcpy(groups[groupNum].id,line);
		strcpy(groups[groupNum].creator,line+pos+1);
		char fileName[64];
		sprintf(fileName,"group_%s.txt",groups[groupNum].id);
		FILE* fp2=fopen (fileName,"at+");
		fseek(fp2,0L,SEEK_SET);
		groups[groupNum].userNum=0;
		while(1){
			ret=fgets(line,64,fp1);
			if(ret==NULL)
				break;
			line[strlen(line)-1]=0;
			strcpy(groups[groupNum].userId[userNum],line);
			groups[groupNum].userNum++;
		}
		fclose(fp2);
		groupNum++;
	}
	fclose(fp1);	
}
void delFriend(char *from,char *to){
	char fileName1[128];
	char fileName2[128];
	sprintf(fileName1,"user_%s.txt",from);
	sprintf(fileName2,"user_%s.bak",from);
	char *ret;
	FILE* fp1=fopen (fileName1,"at+");
	FILE* fp2=fopen (fileName2,"at+");
	fseek(fp1,0L,SEEK_SET);
	fseek(fp2,0L,SEEK_SET);
	char line[64];
	while(1){
		ret=fgets(line,64,fp1);
		if(ret==NULL)
			break;
		line[strlen(line)-1]=0;
		if(strcmp(line,to)==0)
			continue;
		fprintf(fp2,"%s\n", line);
	}
	fclose(fp1);
	fclose(fp2);
	remove(fileName1);
	rename(fileName2,fileName1);	
	sprintf(fileName1,"user_%s.txt",to);
	sprintf(fileName2,"user_%s.bak",to);
	fp1=fopen (fileName1,"at+");
	fp2=fopen (fileName2,"at+");
	fseek(fp1,0L,SEEK_SET);
	fseek(fp2,0L,SEEK_SET);
	while(1){
		ret=fgets(line,64,fp1);
		if(ret==NULL)
			break;
		line[strlen(line)-1]=0;
		if(strcmp(line,from)==0)
			continue;
		fprintf(fp2,"%s\n", line);
	}
	fclose(fp1);
	fclose(fp2);
	remove(fileName1);
	rename(fileName2,fileName1);
}
void handleDelFriend(struct message pack,int sockfd){//from to sendtime content	
	bool find=false;
	int i=0;
	for(i=0;i<userNum;i++){
		if(strcmp(users[i].id,pack.to)==0){
			find=true;
			break;
		}
	}
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=DelFriend;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	temp->tofd=sockfd;
	if(find){
		delFriend(pack.from,pack.to);
		sprintf(temp->content,"ok");
	}else
		sprintf(temp->content,"error");
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	buffer[buffLen]=temp;
	buffLen++;	
}
void handleListFriends(struct message pack,int sockfd){
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=ListFriends;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	char fileName[128];
	sprintf(fileName,"user_%s.txt",pack.from);
	char *ret;
	FILE* fp=fopen (fileName,"at+");
	fseek(fp,0L,SEEK_SET);
	char line[64];
	int pos=0;
	while(1){
		ret=fgets(line,64,fp);
		if(ret==NULL)
			break;
		line[strlen(line)-1]=0;
		bool online=false;
		for(int j=0;j<userNum;j++){
			if(strcmp(users[j].id,line)==0&&users[j].status!=0){
				online=true;					
				break;
			}
		}
		if(online)
			sprintf(temp->content+pos,"%s online\n",line);
		else sprintf(temp->content+pos,"%s offline\n",line);
		pos=strlen(temp->content);
	}
	fclose(fp);
	temp->tofd=sockfd;	
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	buffer[buffLen]=temp;
	buffLen++;	
}
void handleMessage(struct message pack,int sockfd){	
	bool find=false;
	int i=0;
	for(i=0;i<userNum;i++){
		if(strcmp(users[i].id,pack.to)==0){
			find=true;
			break;
		}
	}
	time_t timep;
	time(&timep);
	if(find){
		struct message *temp=(struct message *)malloc(sizeof(struct message));
		temp->type=Message;			
		strcpy(temp->sendTime,ctime(&timep));
		strcpy(temp->from,pack.from);
		strcpy(temp->to,pack.to);
		strcpy(temp->content,pack.content);
		temp->tofd=users[i].fd;
		if(users[i].status!=1)
			temp->tofd=-1;
		else{
			ev.data.fd=temp->tofd;
			ev.events=EPOLLOUT;
			epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);
		}	
		buffer[buffLen]=temp;
		buffLen++;	
	}else{
		for(i=0;i<groupNum;i++){
			if(strcmp(groups[i].id,pack.to)==0){
				find=true;
				break;
			}
		}
		if(find){
			for(int j=0;j<groups[i].userNum;j++){
				struct message *temp=(struct message *)malloc(sizeof(struct message));
				temp->type=Message;	
				strcpy(temp->sendTime,ctime(&timep));
				strcpy(temp->to,groups[i].userId[j]);
				strcpy(temp->from,pack.to);
				sprintf(temp->content,"%s say: %s",pack.from,pack.content);
				for(int k=0;k<userNum;k++){
					if(strcmp(users[k].id,groups[i].userId[j])==0){
						temp->tofd=users[k].fd;
						if(users[k].status!=1)
							temp->tofd=-1;
						else{
							ev.data.fd=temp->tofd;
							ev.events=EPOLLOUT;
							epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);
						}	
						break;
					}
				}
				buffer[buffLen]=temp;
				buffLen++;	
			}
		}
	}
}
void handleCreateGroup(struct message pack,int sockfd){
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=CreateGroup;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	bool find=false;
	int i=0;
	for(i=0;i<groupNum;i++){
		if(strcmp(groups[i].id,pack.to)==0){
			find=true;
			break;
		}
	}
	if(find){
		strcpy(temp->content,"error");
	}else{
		FILE* fp=fopen ("groups.txt","at+");
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"%s\t%s\n", pack.to,pack.from);
		fclose(fp);
		char fileName[128];
		sprintf(fileName,"group_%s.txt",pack.to);
		fp=fopen (fileName,"at+");
		fseek(fp,0L,SEEK_END);
		fprintf(fp,"%s\n", pack.from);
		fclose(fp);
		strcpy(temp->content,"ok");
		strcpy(groups[groupNum].id,pack.to);
		strcpy(groups[groupNum].creator,pack.from);
		strcpy(groups[groupNum].userId[0],pack.from);
		groups[groupNum].userNum=1;
		groupNum++;
	}	
	temp->tofd=sockfd;
	ev.data.fd=temp->tofd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);
	buffer[buffLen]=temp;
	buffLen++;	
}
void handleJoinGroup(struct message pack,int sockfd){
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=JoinGroup;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	bool find=false;
	int i=0;
	for(i=0;i<groupNum;i++){
		if(strcmp(groups[i].id,pack.to)==0){
			find=true;
			break;
		}
	}
	if(find){
		bool find2=false;
		int j=0;
		for(j=0;j<groups[i].userNum;j++){
			if(strcmp(groups[i].userId[j],pack.from)==0){
				find2=true;
				break;
			}
		}
		if(!find2){
			char fileName[128];
			sprintf(fileName,"group_%s.txt",pack.to);
			FILE* fp=fopen (fileName,"at+");
			fprintf(fp,"%s\n", pack.from);
			fclose(fp);
			strcpy(groups[i].userId[groups[i].userNum],pack.from);
			groups[i].userNum++;
		}
		strcpy(temp->content,"ok");
	}else{
		strcpy(temp->content,"error");
	}
	temp->tofd=sockfd;
	ev.data.fd=temp->tofd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);	
	buffer[buffLen]=temp;
	buffLen++;		
}
void handleQuitGroup(struct message pack,int sockfd){
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=QuitGroup;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	bool find=false;
	int i=0;
	for(i=0;i<groupNum;i++){
		if(strcmp(groups[i].id,pack.to)==0){
			find=true;
			break;
		}
	}
	if(find){
		bool find2=false;
		int j=0;
		for(j=0;j<groups[i].userNum;j++){
			if(strcmp(groups[i].userId[j],pack.from)==0){
				find2=true;
				break;
			}
		}
		if(find2){
			for(int k=j;k<groups[i].userNum-1;k++)
				strcpy(groups[i].userId[k],groups[i].userId[k+1]);			
			groups[i].userNum--;
			char fileName[128];
			sprintf(fileName,"group_%s.txt",pack.to);
			FILE* fp=fopen (fileName,"wt+");
			for(j=0;j<groups[i].userNum;j++)
				fprintf(fp,"%s\n", groups[i].userId[j]);				
			fclose(fp);			
		}
		strcpy(temp->content,"ok");
	}else{
		strcpy(temp->content,"error");
	}
	temp->tofd=sockfd;
	ev.data.fd=temp->tofd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);	
	buffer[buffLen]=temp;
	buffLen++;	
}
void handleListGroups(struct message pack,int sockfd){
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=ListGroups;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	int pos=0;
	for(int i=0;i<groupNum;i++){
		for(int j=0;j<groups[i].userNum;j++){
			if(strcmp(groups[i].userId[j],pack.from)==0){
				sprintf(temp->content+pos,"%s\n",groups[i].id);
				pos+=strlen(groups[i].id)+1;
				break;
			}
		}		
	}
	temp->tofd=sockfd;	
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	buffer[buffLen]=temp;
	buffLen++;	
}
void handleListGroupMembers(struct message pack,int sockfd){
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=ListGroupMembers;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	int pos=0;
	for(int i=0;i<groupNum;i++){
		if(strcmp(groups[i].id,pack.to)==0){
			for(int j=0;j<groups[i].userNum;j++){
				sprintf(temp->content+pos,"%s\n",groups[i].userId[j]);
				pos+=strlen(groups[i].userId[j])+1;
			}
			break;
		}
	}
	temp->tofd=sockfd;	
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	buffer[buffLen]=temp;
	buffLen++;
}
void handleDelGroup(struct message pack,int sockfd){//from to sendtime content	
	bool find=false;
	int i=0;
	for(i=0;i<groupNum;i++){
		if(strcmp(groups[i].id,pack.to)==0&&strcmp(groups[i].creator,pack.from)==0){
			find=true;
			break;
		}
	}
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=DelGroup;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	temp->tofd=sockfd;
	if(find){
		for(int j=i;j<groupNum-1;j++)
			groups[j]=groups[j+1];
		groupNum--;		
		FILE* fp=fopen ("groups.txt","wt+");
		for(int j=0;j<groupNum;j++)
			fprintf(fp,"%s\n", groups[j].id);				
		fclose(fp);
		char fileName[128];
		sprintf(fileName,"group_%s.txt",pack.to);
		remove(fileName);
		sprintf(temp->content,"ok");
	}else
		sprintf(temp->content,"error");
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	buffer[buffLen]=temp;
	buffLen++;	
}
void handleKickMember(struct message pack,int sockfd){//from to sendtime content	
	bool find=false;
	int i=0;
	int j=0;
	for(i=0;i<groupNum;i++){
		if(strcmp(groups[i].id,pack.to)==0&&strcmp(groups[i].creator,pack.from)==0){
			for(j=0;j<groups[i].userNum;j++){
				if(strcmp(groups[i].userId[j],pack.content)==0){
					find=true;
					break;
				}
			}
			break;
		}
	}
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=KickMember;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));
	strcpy(temp->to,pack.from);
	strcpy(temp->from,pack.to);
	temp->tofd=sockfd;
	if(find){
		for(int k=j;k<groups[i].userNum-1;k++)
			strcpy(groups[i].userId[k],groups[i].userId[k+1]);			
		groups[i].userNum--;
		char fileName[128];
		sprintf(fileName,"group_%s.txt",pack.to);
		FILE* fp=fopen (fileName,"wt+");
		for(j=0;j<groups[i].userNum;j++)
			fprintf(fp,"%s\n", groups[i].userId[j]);				
		fclose(fp);			
		sprintf(temp->content,"ok\n%s\n",pack.content);
	}else
		sprintf(temp->content,"error\n%s\n",pack.content);
	ev.data.fd=sockfd;
	ev.events=EPOLLOUT;
	epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	buffer[buffLen]=temp;
	buffLen++;	
}
void handleSendFile(struct message pack,int sockfd){//from to sendtime content	 content format:  fileName\nsize\nrequest\n   fileName\nsize\nok\n   fileName\nsize\ncancel\n    error\n
	struct message *temp=(struct message *)malloc(sizeof(struct message));
	temp->type=SendFile;	
	time_t timep;
	time(&timep);
	strcpy(temp->sendTime,ctime(&timep));	
	bool find=false;
	int i=0;
	for(i=0;i<userNum;i++){
		if(strcmp(users[i].id,pack.to)==0&&users[i].status==1){
			find=true;
			break;
		}
	}
	if(find){
		strcpy(temp->from,pack.from);
		strcpy(temp->to,pack.to);
		strcpy(temp->content,pack.content);
		temp->tofd=users[i].fd;		
		ev.data.fd=temp->tofd;
		ev.events=EPOLLOUT;
		epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);
		buffer[buffLen]=temp;
		buffLen++;	
	}else{
		strcpy(temp->to,pack.from);
		strcpy(temp->from,"server");
		temp->tofd=sockfd;
		strcpy(temp->content,"error\n");
		ev.data.fd=temp->tofd;
		ev.events=EPOLLOUT;
		epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);
		buffer[buffLen]=temp;
		buffLen++;	
	}	
}

void parse(char msg[1024], int len,int sockfd)
{
	int type=(*msg);
	struct message pack=parsePack(msg, len);
	switch(type){
		case Login:
			handleLogin(pack, sockfd);
		break;
		case Register:
			handleRegister(pack, sockfd);
		break;
		case Logout:
			handleLogout(pack, sockfd);
		break;
		case FindPass:
			handleFindPass(pack, sockfd);
		break;
		case AddFriend:
			handleAddFriend(pack, sockfd);
		break;
		case DelFriend:
			handleDelFriend(pack, sockfd);
		break;
		case ListFriends:
			handleListFriends(pack, sockfd);
		break;
		case Message:
			handleMessage(pack, sockfd);
		break;
		case CreateGroup:
			handleCreateGroup(pack, sockfd);
		break;
		case JoinGroup:
			handleJoinGroup(pack, sockfd);
		break;
		case QuitGroup:
			handleQuitGroup(pack, sockfd);
		break;
		case ListGroups:
			handleListGroups(pack, sockfd);
		break;
		case ListGroupMembers:
			handleListGroupMembers(pack, sockfd);
		break;
		case DelGroup:
			handleDelGroup(pack, sockfd);
		break;
		case KickMember:
			handleKickMember(pack, sockfd);
		break;
		case SendFile:
			handleSendFile(pack, sockfd);
		break;
	}
}

int main(int argc, char *argv[])
{
	int listenfd,connfd,nfds;
	int newfd,sockfd;
	int n;
	unsigned char msg[1024];
	struct epoll_event ev,events[1024];
    epfd=epoll_create(1024);
    struct sockaddr_in serveraddr,cliaddr;
	socklen_t clientAddrLen;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);    
    setnonblocking(listenfd);//把socket设置为非阻塞方式
    ev.data.fd=listenfd;    
    ev.events=EPOLLIN;       //设置要处理的事件类型     
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);    //注册epoll事件
    bzero(&serveraddr, sizeof(struct sockaddr_in)); 
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_port=htons(9999);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    bind(listenfd,(struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in));
    listen(listenfd, 20);
	init();
	curfds=1;
	while (1){ 
		nfds = epoll_wait(epfd, events, curfds, 1000); 
		for (int i = 0; i < nfds; i++){
            if (events[i].data.fd == listenfd) { 
                newfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clientAddrLen); 
                setnonblocking(newfd); 
                ev.events = EPOLLIN; 
                ev.data.fd = newfd; 
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, newfd, &ev) < 0)
                    close(newfd);                
                curfds++; 
            }else if(events[i].events&EPOLLIN)
            {
				if ( (sockfd = events[i].data.fd) < 0)
					continue;
				n = read(sockfd, msg, 4);//printf("received pack\n");
				if (n <= 0) {
					close(sockfd);//printf("closed\n");
					epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,&ev);
					int i=0;
					for(i=0;i<userNum;i++){
						if(users[i].fd==sockfd){
							users[i].status=0;
							break;
						}
					}
					continue;
				}
				int len=(int)(*msg);
				n = read(sockfd, msg, len);
				if (n !=len) {
					close(sockfd);//printf("closed\n");
					epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,&ev);
					int i=0;
					for(i=0;i<userNum;i++){
						if(users[i].fd==sockfd){
							users[i].status=0;
							break;
						}
					}
					continue;
				}
				parse(msg,len,sockfd);				
            }else if(events[i].events&EPOLLOUT)
            {
                sockfd = events[i].data.fd;//printf("server send\n");
				sendPack(sockfd);
            }
        }//printf("bufflen=%d\n",buffLen);
		for(int i=0;i<buffLen;i++){
			struct message *temp=buffer[i];//printf("%s\n",temp->to);
			for(int k=0;k<userNum;k++){
				if(strcmp(temp->to,users[k].id)==0&&users[k].status==1){//printf("now on line\n");
					temp->tofd=users[k].fd;			
					ev.events = EPOLLOUT; 
					ev.data.fd = temp->tofd;
					epoll_ctl(epfd,EPOLL_CTL_MOD,temp->tofd,&ev);
					break;
				}
			}
		}		
    }
    close(listenfd); 
	return 0;
}











