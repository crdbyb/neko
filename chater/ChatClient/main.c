
#include "type.h"

struct message *buffer[1024];
int buffLen=0;
int sockfd;
struct message currMsg;
bool isSent=true;
struct user me;
//bool printMsg=false;
struct message currMsg;
struct message reqMsg;
bool hasReq=false;
bool runEnable=true;
bool transferEnable=false;
bool isChating=false;

int findstr(char *s_str,char *d_str){
	int i,j,k,count=-1;
	char *p=strstr(s_str,d_str);
	if(p!=NULL)
		count=p-s_str;
	return count;
}
bool sendNewPack(){
	unsigned char reply[1024];
	int len1=4+strlen(currMsg.from)+strlen(currMsg.to)+2+strlen(currMsg.content);//printf("len=%d\n",len1);
	memcpy(reply,&len1,4);
	memcpy(reply+4,&currMsg.type,4);
	sprintf(reply+8,"%s\n%s\n%s",currMsg.from,currMsg.to,currMsg.content);
	int n=write(sockfd,reply,len1+4);//printf("7777=%d\n",n);
	if(n!=(len1+4)){
		close(sockfd);		
		return false;
	}
	if(!isChating)
		system("clear");
	//printf("\nsend command ok\n");
	if(currMsg.type==Message){
		char fileName[64];
		sprintf(fileName,"%s.txt",currMsg.to);
		time_t timep;
		time(&timep);
		strcpy(currMsg.sendTime,ctime(&timep));
		currMsg.sendTime[strlen(currMsg.sendTime)-1]=0;
		FILE* fp=fopen (fileName,"at+");
		fprintf(fp,"%s %s %s %s\n",currMsg.from,currMsg.to,currMsg.sendTime,currMsg.content);
		fclose(fp);
	}
	if(currMsg.type==SendFile&&findstr(currMsg.content,"ok\n")!=-1){//and content is ok
		//file transfer
		char fileName[64];
		char size[16];
		int pos1=findstr(currMsg.content,"\n");
		memset(fileName,0,64);
		memset(size,0,16);
		strncpy(fileName,currMsg.content,pos1);
		int pos2=findstr(currMsg.content+pos1+1,"\n");
		strncpy(size,currMsg.content+pos1+1,pos2);
		int len=atoi(size);			
			//mkdir(me.id);
		sprintf(fileName,"%s.rcv",fileName);//printf("len=%d\n",len);
		FILE* fp=fopen (fileName,"w");//if(fp!=NULL)printf("file opened %s\n",fileName);
		unsigned char bt[1024];
		int n;
		while(1){
			if(len>=1024)
			n = read(sockfd, bt, 1024);
			else n = read(sockfd, bt, len);
			fwrite(bt,1,n,fp);
			len=len-n;
			if(len==0)
			break;
		}
		fclose(fp);						
		printf("文件传输成功！\n");
		transferEnable=false;
	}
	return true;
}
void parsePack(unsigned char msg[1024], int len){
	struct message *pack=malloc(sizeof(struct message));
	pack->type=(*msg);
	char *p=msg+4;
	int pos1=findstr(p,"\n");
	char *p3=p+pos1;
	*p3='\0';
	strcpy(pack->from,p);
	char *p2=p3+1;
	int pos2=findstr(p2,"\n");
	p3=p2+pos2;
	*p3='\0';
	strcpy(pack->to,p2);
	int pos3=findstr(p3+1,"\n");
	char *p4=p3+1+pos3;
	*p4='\0';
	strcpy(pack->sendTime,p3+1);
	memset(pack->content,0,512);
	strncpy(pack->content,p4+1,len-4-strlen(pack->from)-strlen(pack->to)-strlen(pack->sendTime)-3);		//printf("recv=%s %s %skk %s \n",pack->from,pack->to,pack->sendTime,pack->content);	
	if(pack->type==SendFile){
		int pos=findstr(pack->content,"request\n");
		if(pos!=-1){
			strcpy(reqMsg.from,pack->from);
			strcpy(reqMsg.to,pack->to);
			strcpy(reqMsg.content,pack->content);
			strcpy(reqMsg.sendTime,pack->sendTime);
			reqMsg.type=SendFile;
			hasReq=true;
			printf("你有一个文件传输的请求！\n");
			free(pack);
			return;
		}
		pos=findstr(pack->content,"ok\n");
		if(pos!=-1){//and content is ok		
			//file transfer			
			char fileName[64];
			char size[16];
			int pos1=findstr(pack->content,"\n");
			memset(fileName,0,64);
			memset(size,0,16);
			strncpy(fileName,pack->content,pos1);
			int pos2=findstr(pack->content+pos1+1,"\n");
			strncpy(size,pack->content+pos1+1,pos2);
			int len=atoi(size);
			FILE* fp=fopen (fileName,"r");
			unsigned char bt[1024];
			int n;
			while(1){
				if(len>=1024)
					n = fread(bt, 1,1024,fp);
				else n = fread(bt, 1,len,fp);
				write(sockfd,bt,n);
				len=len-n;
				if(len==0)
				break;
			}
			fclose(fp);			
			printf("文件传输成功！\n");			
		}
		pos=findstr(pack->content,"cancel\n");
		if(pos!=-1)		
			printf("文件传输已取消！\n");
		pos=findstr(pack->content,"error\n");
		if(pos!=-1)	
			printf("文件传输错误！\n");
		transferEnable=false;
	}
	if(pack->type==AddFriend){//printf("add friend pack\n");
		int pos=findstr(pack->content,"request");
		if(pos!=-1){
			strcpy(reqMsg.from,pack->from);
			strcpy(reqMsg.to,pack->to);
			strcpy(reqMsg.content,pack->content);
			strcpy(reqMsg.sendTime,pack->sendTime);
			reqMsg.type=AddFriend;
			printf("你有一个好友请求\n");
			hasReq=true;
		}else {
			printf("add friend %s\n",pack->content);
		}
	}
	if(pack->type==DelFriend){
		printf("delete friend %s\n",pack->content);
	}
	if(pack->type==ListFriends){
		printf("list friends\n %s\n",pack->content);
	}
	if(pack->type==CreateGroup){
		printf("create group %s\n",pack->content);
	}
	if(pack->type==JoinGroup){
		printf("join group %s\n",pack->content);
	}	
	if(pack->type==QuitGroup){
		printf("quit group %s\n",pack->content);
	}
	if(pack->type==ListGroups){
		printf("list groups\n %s\n",pack->content);
	}
	if(pack->type==ListGroupMembers){
		printf("list group members\n %s\n",pack->content);
	}
	if(pack->type==DelGroup){
		printf("delete group %s\n",pack->content);
	}		
	if(pack->type==KickMember){
		printf("kick group member %s\n",pack->content);
	}			
	if(pack->type==Login){
		printf("Login %s\n",pack->content);
		if(strcmp(pack->content,"ok")==0){
			me.status=1;//printf("3333\n");
		}
	}	
	if(pack->type==Register){
		printf("Register %s\n",pack->content);
	}
	if(pack->type==FindPass){
		printf("FindPass %s\n",pack->content);
	}	
	if(pack->type==Message){
		//printf("Message received\n");
		char fileName[64];
		sprintf(fileName,"%s.txt",pack->from);
		FILE* fp=fopen (fileName,"at+");
		fprintf(fp,"%s %s %s %s\n", pack->from,pack->to,pack->sendTime,pack->content);
		fclose(fp);
		buffer[buffLen]=pack;						
		buffLen++;	
	}else free(pack);
}
bool receivePack(){
	unsigned char msg[1024];
	int n = read(sockfd, msg, 4);
	if(n==0)
		return true;
	if (n < 0) {
		//close(sockfd);printf("closed\n");
		return false;
	}
	int len=(*msg);
	n = read(sockfd, msg, len);
	if (n !=len) {
		//close(sockfd);printf("closed\n");
		return false;
	}
	parsePack(msg, len);
	return true;
}
void friendCmd(){
	char opt[16];
	int option;
	while(1){
		system("clear");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                            好 友 管 理                                              ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    1.添 加 好 友    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    2.好 友 列 表    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    3.删 除 好 友    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    4.好 友 请 求    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    0.返 回 菜 单    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t你 想 干 嘛:");
		scanf("%s",opt);
		option=atoi(opt);
		system("clear");		
		if(option==0&&opt[0]!='0')
			continue;
		if(option==0)
			break;
		if(option<0&&option>3)
			continue;
		if(option==1){
			printf("输 入 好 友 名:\n");
			scanf("%s",currMsg.to);
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"request");
			currMsg.type=AddFriend;
			isSent=false;
		}
		if(option==2){
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.to,"server");
			strcpy(currMsg.content,"");
			currMsg.type=ListFriends;
			isSent=false;
		}
		if(option==3){
			printf("输 入 好 友 名:\n");
			scanf("%s",currMsg.to);
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			currMsg.type=DelFriend;
			isSent=false;
		}
		if(option==4){
			if(hasReq){
				if(reqMsg.type==AddFriend){
					printf("收到来自 %s 的好友请求,输入 1 同意,其他数字拒绝:\n",reqMsg.from);
					int ans;
					scanf("%d",&ans);
					strcpy(currMsg.from,me.id);
					strcpy(currMsg.to,reqMsg.from);
					currMsg.type=AddFriend;
					if(ans==1){						
						strcpy(currMsg.content,"ok");
					}else{
						strcpy(currMsg.content,"cancel");
					}	
					isSent=false;
				}
				hasReq=false;
			}
		}
		usleep(700000);		
		if(option==2){			
			getchar();
			printf("输 入 回 车 返 回\n");
			char ch;
			do{        
			}while((ch=getchar())!='\n');
		}
	}
}
void chatCmd(){
	char opt[16];
	int option;
	printf("输 入 好 友 或 群 组 名：\n");
	scanf("%s",currMsg.to);
	printf("1为发送信息，2为历史记录，0退出聊天\n");
	isChating=true;	
	while(1){
		//printf("1 发送消息\n");		
		//printf("2 历史记录\n");
		//printf("0 return\n");
		scanf("%s",opt);
		option=atoi(opt);
		//system("clear");
		if(option==0&&opt[0]!='0')
			continue;
		if(option==0){
			system("clear");
			isChating=false;
			break;
		}
		if(option<0&&option>3)
			continue;
		if(option==1){
			//printf("please intput message\n");
			scanf("%s",currMsg.content);
			strcpy(currMsg.from,me.id);
			currMsg.type=Message;
			isSent=false;
		}
		if(option==2){
			char fileName[64];
			sprintf(fileName,"%s.txt",currMsg.to);
			FILE* fp=fopen (fileName,"r");
			char line[128];
			char *ret;
			if(fp!=NULL){
				while(1){
					ret=fgets(line,128,fp);
					if(ret==NULL)
						break;
					line[strlen(line)-1]=0;
					printf("%s\n",line);
				}
				fclose(fp);
			}
		}					
		usleep(700000);
	}
}
void groupCmd(){
	char opt[16];
	int option;
	while(1){
		system("clear");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                            群 组 管 理                                              ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    1.建 一 个 群    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    2.加 一 个 群    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    3.退 一 个 群    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    4.群 组 列 表    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    5.成 员 列 表    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    6.解 散 群 组    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    7.踢 一 个 人    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    0.返 回 菜 单    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t你 想 干 嘛:");
		scanf("%s",opt);
		option=atoi(opt);
		system("clear");
		if(option==0&&opt[0]!='0')
			continue;
		if(option==0)
			break;
		if(option<0&&option>7)
			continue;
		if(option==1){
			printf("输 入 群 名:\n");
			scanf("%s",currMsg.to);
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			currMsg.type=CreateGroup;
			isSent=false;
		}
		if(option==2){
			printf("输 入 群 名:\n");
			scanf("%s",currMsg.to);
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			currMsg.type=JoinGroup;
			isSent=false;
		}
		if(option==3){
			printf("输 入 群 名:\n");
			scanf("%s",currMsg.to);
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			currMsg.type=QuitGroup;			
			isSent=false;
		}
		if(option==4){
			strcpy(currMsg.to,"server");
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			currMsg.type=ListGroups;
			isSent=false;
		}
		if(option==5){
			printf("输 入 群 名:\n");
			scanf("%s",currMsg.to);
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			currMsg.type=ListGroupMembers;
			isSent=false;
		}
		if(option==6){
			printf("输 入 群 名:\n");
			scanf("%s",currMsg.to);
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			currMsg.type=DelGroup;
			isSent=false;
		}
		if(option==7){
			printf("输 入 群 名:\n");
			scanf("%s",currMsg.to);
			printf("输 入 想 踢 的 人:\n");
			scanf("%s",currMsg.content);
			strcpy(currMsg.from,me.id);
			currMsg.type=KickMember;
			isSent=false;
		}
		usleep(700000);
		if(option==4||option==5){			
			getchar();
			printf("输 入 回 车 返 回\n");
			char ch;
			do{        
			}while((ch=getchar())!='\n');
		}
	}
}
void fileTransferCmd(){
	char opt[16];
	int option;
	while(1){
		system("clear");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                            文 件 传 输                                              ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    1.发 送 文 件    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    2.文 件 处 理    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||---------------------------------------    0.返 回 菜 单    -----------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t你 想 干 嘛:");
		scanf("%s",opt);
		option=atoi(opt);
		system("clear");
		if(option==0&&opt[0]!='0')
			continue;
		if(option==0)
			break;
		if(option<0&&option>2)
			continue;
		if(option==1){
			printf("输 入 文 件 名：\n");
			char fileName[64];
			scanf("%s",fileName);
			printf("发 送 的 人：\n");
			scanf("%s",currMsg.to);
			if(strcmp(currMsg.to,me.id)==0){
				printf("你 不 能给 自 己 发 送！\n");
				continue;
			}
			FILE* fp=fopen(fileName,"r");
			if(fp==NULL){
				printf("文 件 不 存 在！\n");
				continue;
			}else{
				fseek(fp,0,SEEK_END);
				int size=ftell(fp);				
				fclose(fp);				
				sprintf(currMsg.content,"%s\n%d\nrequest\n",fileName,size);
			}			
			currMsg.type=SendFile;			
			printf("请 稍 等...\n");
			transferEnable=true;
			isSent=false;
			while(transferEnable)
				usleep(100000);			
		}
		if(option==2){
			if(hasReq){
				if(reqMsg.type==SendFile){
					char fileName[64];
					char size[16];
					int pos1=findstr(reqMsg.content,"\n");
					memset(fileName,0,64);
					memset(size,0,16);
					strncpy(fileName,reqMsg.content,pos1);
					int pos2=findstr(reqMsg.content+pos1+1,"\n");
					strncpy(size,reqMsg.content+pos1+1,pos2);
					printf("收到了来自 %s 的文件传输,文件名为 %s,输入 1 接受,其他数字取消\n",reqMsg.from,fileName);
					int ans;
					scanf("%d",&ans);
					strcpy(currMsg.from,me.id);
					strcpy(currMsg.to,reqMsg.from);
					currMsg.type=SendFile;
					if(ans==1){				
						sprintf(currMsg.content,"%s\n%s\nok\n",fileName,size);
						transferEnable=true;
						isSent=false;	
						while(transferEnable)
							usleep(100000);	
					}else{
						sprintf(currMsg.content,"%s\n%s\ncancel\n",fileName,size);					
						isSent=false;	
					}									
				}
				hasReq=false;				
			}
		}
		usleep(700000);
	}
}
void * threadFunction(void *args)
{
	while(runEnable){
		if(!isSent){//printf("thread\n");			
			sendNewPack();
			isSent=true;
		}
		receivePack();
		for(int i=0;i<buffLen;i++){
			printf("%s %s %s\n%s\n",buffer[i]->from,buffer[i]->to,buffer[i]->sendTime,buffer[i]->content);
			free(buffer[i]);
		}
		buffLen=0;
	}
}
int getachar()
{
	struct termios oldt,
	newt;
	int ch;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return ch;
}
int getpasswd(char* passwd, int size) 
{ 
   int c; 
   int n = 0; 
   do{ 
		c=getachar(); 
		if (c != '\n'&& c!='\r' && c!=127){ 
			passwd[n] = c; 
			printf("*");
			n++;
		} 
		else if ((c != '\n'|c!='\r')&&c==127)
		{
			if(n>0){
				n--;
				printf("\b \b");
			}
		} 
   }while(c != '\n' && c !='\r' && n < (size - 1)); 
   passwd[n] = '\0';  
   return n; 
}
int main(int argc, char *argv[])
{
	struct sockaddr_in sockaddr;
	char *servInetAddr = "127.0.0.1";	
	memset(&sockaddr,0,sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(9999);
	inet_pton(AF_INET,servInetAddr,&sockaddr.sin_addr);	
	struct timeval timeout = {0, 500000};
	char opt[16];
	int option;
	me.status=0;
	while(1){	
		system("clear");	
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                     这 大 概 是 一 个 聊 天 室                                      ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||------------------------------------------    1.登 陆    --------------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||------------------------------------------    2.注 册    --------------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||------------------------------------------    0.退 出    --------------------------------------------||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t你想干嘛:");
		scanf("%s",opt);
		option=atoi(opt);
		//system("clear");
		if(option==0&&opt[0]!='0')
			continue;
		if(option==0)
			return 0;
		if(option<0||option>3)
			continue;
		sockfd = socket(AF_INET,SOCK_STREAM,0);
		setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
		setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
		if(connect(sockfd,(struct sockaddr*)&sockaddr,sizeof(sockaddr))<0)
			continue;				
		if(option==1){
			printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
			printf("||虽然不温馨但确实是个提示：若用户名或密码错误则将自动返回！                                           ||\n");
			printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
			printf("输 入 用 户 名:");
			scanf("%s",me.id);
			char ch;
			scanf("%c",&ch);;
			printf("输 入 密 码:");
			getpasswd(me.pass, 15);
			printf("\n");  
			currMsg.type=Login;
			strcpy(currMsg.to,"server");
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,me.pass);
			bool ret=sendNewPack();
			unsigned char msg[1024];
			if(ret){
				ret=receivePack();
				if(me.status==1)
					break;
			}		
		}
		if(option==2){
			printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
			printf("||虽然不温馨但确实是个提示：若用户名已经存在，你注册了也登陆不上去，用户名请个性化一些！               ||\n");
			printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
			printf("输 入 用 户 名:");
			scanf("%s",me.id);
			char ch;
			scanf("%c",&ch);;
			printf("输 入 密 码:");
			getpasswd(me.pass, 15);
			currMsg.type=Register;
			strcpy(currMsg.to,"server");
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,me.pass);
			bool ret=sendNewPack();
			unsigned char msg[1024];
			if(ret){
				ret=receivePack();
			}	
		}
		if(option==3){
			printf("please input name\n");
			scanf("%s",me.id);
			currMsg.type=FindPass;
			strcpy(currMsg.to,"server");
			strcpy(currMsg.from,me.id);
			strcpy(currMsg.content,"");
			bool ret=sendNewPack();
			unsigned char msg[1024];
			if(ret){
				ret=receivePack();
			}
		}
		close(sockfd);		
	}
	pthread_t thread;	
	pthread_create(&thread, NULL, threadFunction, NULL);
	while(1){
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                            主 菜 单                                                 ||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||-------------------------------------    1.好 友 管 理    -------------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||-------------------------------------    2.聊 天 管 理    -------------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||-------------------------------------    3.群 组 管 理    -------------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||-------------------------------------    4.文 件 传 输    -------------------------------------------||\n");
		printf("\t\t\t\t||                                                                                                     ||\n");
		printf("\t\t\t\t||-------------------------------------    0.退 出 登 陆    -------------------------------------------||\n");
		printf("\t\t\t\t||*****************************************************************************************************||\n");
		printf("\t\t\t\t= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
		printf("\t\t\t\t你想干啥:");
		scanf("%s",opt);
		option=atoi(opt);
		//system("clear");
		if(option==0&&opt[0]!='0')
			continue;
		if(option==0){
			runEnable=false;
			break;
		}		
		if(option==1)
			friendCmd();		
		if(option==2)
			chatCmd();		
		if(option==3)
			groupCmd();		
		if(option==4)
			fileTransferCmd();		
	}
	return 0;
}
