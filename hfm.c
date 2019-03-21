#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>

#define N 257
#define READNUM 5

typedef struct
{
    int weight;
    int order;
    int parent,Lchild,Rchild;
}HuffmanTree;

typedef struct Node
{
    int node_order;
    int node_weight;
}node;


void my_err();
int count_node();
node *creat_node();
void open_file();
void creat_HuffManTree();
void select_tree();

void Crt_HuffManCode();
void write_file();

int array[N];
int k = 0;
void my_err(const char *err_string,int line)
{
    fprintf(stderr,"line:%d ",line);
    perror(err_string);
    exit(1);
}

int count_node()
{
    int i = 0, number = 0;
    for(i = 0; i < N; i++)
    {
        if(array[i] > 0)
            number++;
    }
    return number;
}

node* creat_node(int number)
{
    node *NODE = (node *)malloc(sizeof(node *)*number);
    int i = 0;
    memset(NODE,0,sizeof(NODE));
    for( i = 0; i < N; i++ )
    {
        if(array[i] != 0)
        {
            NODE[k].node_order = i;
            NODE[k].node_weight = array[i];
            k++;
        }
    }
    return NODE;
}

void open_file(char *filename[])
{
    int fd;
    int len,ret;
    int i = 0;
    if((fd = open(*filename, O_RDONLY)) == -1)
    {
       my_err("open",__LINE__);
    }
    else
    {
        if(lseek(fd,0,SEEK_END) == -1)
            my_err("lseek",__LINE__);
        if((len = lseek(fd,0,SEEK_CUR)) == -1)
            my_err("lseek",__LINE__);
        if(lseek(fd,0,SEEK_SET) == -1)
            my_err("lseek",__LINE__);
        if(len <= READNUM)
        {
            char str[len];
            memset(str,0,sizeof(str));
            if((ret = read(fd,str,len)) < 0)
                my_err("read",__LINE__);
            str[len-1] = '\0';
            for( i = 0; i < len-1; i++ )
                array[str[i]]++;
        }
        else
        {
            while(len > READNUM)
            {
                char str[READNUM];
                memset(str,0,sizeof(str));
                if((ret = read(fd,str,READNUM)) < 0)
                    my_err("read",__LINE__);
                for( i = 0; i < READNUM; i++ )
                    array[str[i]]++;
                len -= READNUM;
            }
            if(len != 0)
            {
                char str[len];
                memset(str,0,sizeof(str));
                if((ret = read(fd,str,len)) < 0)
                    my_err("read",__LINE__);
                str[len-1]='\0';
                for( i = 0; i < len-1; i++ )
                    array[str[i]]++;

            }
        }
    }
    close(fd);
}
void creat_HuffManTree(node *NODE,HuffmanTree *Tree,int mumber,int number)
{
    int i = 0;
    int s1, s2;
    for(i = 0;i <  number ; i++)
    {
        Tree[i].weight = NODE[i].node_weight;
        Tree[i].order = NODE[i].node_order;
        Tree[i].parent = -1;
        Tree[i].Lchild = -1;
        Tree[i].Rchild = -1;
    }
    for(i = number ; i < mumber; i++)
    {
        Tree[i].weight = -1;
        Tree[i].order = -1;
        Tree[i].parent = -1;
        Tree[i].Lchild = -1;
        Tree[i].Rchild = -1;
    }
    for(i = number ; i < mumber; i++)
    {
        select_tree(Tree, i-1, &s1, &s2);
        Tree[i].weight = Tree[s1].weight + Tree[s2].weight;
        Tree[i].Lchild = s1;
        Tree[i].Rchild = s2;
        Tree[s1].parent = i;
        Tree[s2].parent = i;
    }
}

void select_tree(HuffmanTree *Tree, int number, int *s1, int *s2)
{
    int i = 0, min = 65535, min2 = 65535,k = 0;
    for(i = 0; i <= number; i++)
    {
        if(Tree[i].parent == -1 && Tree[i].weight < min)
        {
            *s1 = i;
            min = Tree[i].weight;
            k = i;
        }
    }
    for(i = 0; i <= number; i++)
    {
        if(Tree[i].parent == -1 && Tree[i].weight < min2 && i != k)
        {
            *s2 = i;
            min2 = Tree[i].weight;
        }
    }
}

void Crt_HuffManCode(HuffmanTree *Tree, int number, char *filename[])
{
    char *hc[number];
    int i;
    int last,front;
    char *cd;
    char ptr[N];
    int start;
    cd = (char *)malloc(number * sizeof(char));
    cd[number -1] = '\0';
    for(i = 0; i < number; i++)
    {
        start = number - 1;
        last = i;
        front = Tree[i].parent;
        while(front != -1)
        {
            --start;
            if(Tree[front].Lchild == last)
                cd[start] = '0';
            else
                cd[start] = '1';
            last = front;
            front = Tree[front].parent;
        }
        hc[i] = (char *)malloc((number - start) * sizeof(char));
        strcpy(hc[i],&cd[start]);
    }
    free(cd);

    int j = 0;
    int fd2;
    int fd1;
    int len;
    int ret;
    if((fd2 = open(*filename, O_RDONLY)) == -1)
    {
        my_err("open",__LINE__);
    }
    else
    {
        printf("解码存到（文件名）: ");
        scanf("%s",ptr);
        getchar();
        if((fd1 = open(ptr, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
        {
            my_err("open", __LINE__);
        }
        else
        {
            if(lseek(fd2, 0, SEEK_END) == -1)
                my_err("lseek",__LINE__);
            if((len = lseek(fd2, 0, SEEK_CUR)) == -1)
                my_err("lseek", __LINE__);
            if(lseek(fd2,0,SEEK_SET) == -1)
                my_err("lseek", __LINE__);
            if(len <= READNUM)
            {
                char str[len];
                memset(str, 0, sizeof(str));
                if((ret = read(fd2, str, len)) < 0)
                    my_err("read", __LINE__);
                str[len-1] = '\0';
                for(i = 0; i < len -1; i++)
                {
                    int flag = str[i];
                    for(j = 0; j < number; j++)
                    {
                        if(Tree[i].order == flag)
                        {
                            if(write(fd1, hc[j], strlen(hc[j])) != strlen(hc[j]))
                                my_err("write", __LINE__);
                        }
                    }
                }
            }
            else
            {
                while(len > READNUM)
                {
                    char str[READNUM];
                    memset(str,0,sizeof(str));
                    if((ret = read(fd2, str, READNUM)) < 0)
                        my_err("read",__LINE__);
                    for(i = 0; i < READNUM; i++)
                    {
                        int flag = str[i];
                        for(j = 0; j < number; j++)
                        {
                            if(Tree[j].order == flag)
                            {
                                if(write(fd1, hc[j], strlen(hc[j])) != strlen(hc[j]))
                                    my_err("write", __LINE__);
                            }
                        }
                    }
                    len -= READNUM;
                }
                if(len != 0)
                {
                    char str[len];
                    memset(str, 0, sizeof(str));
                    if((ret = read(fd2, str, len)) < 0)
                        my_err("read",__LINE__);
                    str[len-1] = '\0';
                    for(i = 0; i < (len-1); i++)
                    {
                        int flag = str[i];
                        for(j = 0; j < number; j++)
                        {
                            if(Tree[j].order == flag)
                            {
                                if(write(fd1, hc[j], strlen(hc[j])) != strlen(hc[j]))
                                    my_err("write", __LINE__);
                            }
                        }
                    }
                }
            }
        }
    }
    close(fd2);
    close(fd1);
}

void Slove_HuffManCode(HuffmanTree *Tree, int mumber, int number)
{
    FILE *fp;
    int fd;
    char str[N];
    char ptr[N];
    memset(str, 0 ,sizeof(str));
    printf("上次输入的解码的文件名: ");
    scanf("%s",str);
    getchar();
    int len;
    if((fd = open(str, O_RDONLY)) == -1)
        my_err("open", __LINE__);
    else
    {
        if(lseek(fd, 0, SEEK_END) == -1)
            my_err("lseek", __LINE__);
        if((len = lseek(fd, 0, SEEK_CUR)) == -1)
            my_err("lseek", __LINE__);
        if(lseek(fd, 0, SEEK_SET) == -1)
            my_err("lseek", __LINE__);
    }
    close(fd);
    printf("译码存在（文件名）：");
    scanf("%s",ptr);
    getchar();
    FILE *fp2;
    char c2;
    int flag2 = 0;
    char temp;
    int q;
    if(fp2 = fopen(ptr, "w+"))
    {
        if(fp = fopen(str, "r"))
        {
            int sum = 0;
            char ch;
            int i = 0;
            int p;
            int c;
            for(i = 0; i <mumber; i++)
            {
                if(Tree[i].parent == -1)
                {
                    p = i;
                    q = p;
                }
            }
            while(sum < len)
            {
                if(flag2 == 0)
                {
                    ch = fgetc(fp);
                }
                else
                {
                    ch = temp;
                }
                if(ch == '0' && Tree[p].Lchild != -1)
                {
                    sum++;
                    p = Tree[p].Lchild;
                    flag2 = 0;
                }
                else if(ch == '1' && Tree[p].Rchild != -1)
                {
                    sum++;
                    p = Tree[p].Rchild;
                    flag2 = 0;
                }
                else
                {
                    c2 = Tree[p].order;
                    fputc(c2, fp2);
                    flag2 = 1;
                    temp = ch;
                    p = q;
                }
            }
            c2 = Tree[p].order;
            fputc(c2, fp2);
        }
        else
        {
            my_err("fopen", __LINE__);
        }
        fclose(fp);
        fclose(fp2);
    }
    else
    {
        my_err("fopen",__LINE__);
    }
}

int main(int argc,char *argv[])
{
    int number = 0,mumber;
    memset(array,0,sizeof(array));
    open_file(&argv[1]);
    number = count_node();
    mumber = number * 2 -1;
    node *NODE = (node *)malloc(sizeof(node *)* number);
    NODE = creat_node(number);
    HuffmanTree Tree[mumber];
    creat_HuffManTree(NODE,Tree,mumber,number);
    Crt_HuffManCode(Tree, number, &argv[1]);
    Slove_HuffManCode(Tree,mumber,number);
}


