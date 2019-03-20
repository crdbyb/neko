#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#define bool char
#define true 1
#define false 0

enum PackType{  //定义值枚举类型
    Login,Register,Logout,FindPass,AddFriend,DelFriend,ListFriends,
	Message,CreateGroup,JoinGroup,QuitGroup,ListGroups,
	ListGroupMembers,DelGroup,KickMember,SendFile
};
struct user{
	char id[16];
	char pass[16];
	int fd;
	int status;   //0: closed 1: online 2: filetransfering  
};
struct group{
	char id[16];
	char creator[16];
	char userId[256][16];
	int userNum;
};
struct message{	
	int type;
	char from[16];
	char to[16];
	char sendTime[48];
	char content[512];
	int tofd;
};
struct FileAttr{
	char fileName[128];
	int len;
	struct user *from;
	struct user *to;
};
