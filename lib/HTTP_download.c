#include "HTTP_download.h"

#include <stdio.h>//字符串处理
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>//文件
#include <fcntl.h>

#include <errno.h>

#include <sys/types.h>//socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//线程锁解锁任务
static void handler(void *arg)
{
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}

//tcp连接服务器
static int HTTP_Tcp_Client_IPv4(const char *ip, unsigned int port)
{
	//创建socket
	int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_socket < 0)
	{
		perror("socket");
		return -1;
	}

    //填写服务器IP信息
    struct sockaddr_in caddr;
    memset(&caddr, 0, sizeof(struct sockaddr_in));
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(port);
    caddr.sin_addr.s_addr = inet_addr(ip);

	//连接服务器
	int ret = connect(tcp_socket, (struct sockaddr *)&caddr, sizeof(caddr));
	if (ret < 0)
	{
		perror("connect");
		close(tcp_socket);
		return -1;
	}

	return tcp_socket;
}

//获取当前线程池缓冲区大小
void HTTP_Download_get_bufsize(struct Http_Down_t *ctrl, uint32_t *size)
{
	*size = ctrl->download_buf_size;
}

//设置当前线程池缓冲区大小
void HTTP_Download_set_bufsize(struct Http_Down_t *ctrl, uint32_t size)
{
	pthread_mutex_lock(&(ctrl->mutex));
	ctrl->download_buf_size = size;
	pthread_mutex_unlock(&(ctrl->mutex));
}

//获取当前线程池设置线程数
void HTTP_Download_get_setthreadnum(struct Http_Down_t *ctrl, uint32_t *num)
{
	*num = ctrl->set_thread_num;
}

//设置当前线程池设置线程数
void HTTP_Download_set_setthreadnum(struct Http_Down_t *ctrl, uint32_t num)
{
	pthread_mutex_lock(&(ctrl->mutex));
	ctrl->set_thread_num = ((num<(ctrl->max_thread_num))?(num):(ctrl->max_thread_num));
	pthread_mutex_unlock(&(ctrl->mutex));
}

//初始化新的任务节点
struct down_task *init_down_task(const char *host, const char *url, const char *filepath, uint32_t buf_size, func_t cb_func)
{
	//创建新的节点
	struct down_task *newtask = (struct down_task *)malloc(sizeof(struct down_task));
	if (newtask == NULL)
	{
		perror("init_down_task");
		return NULL;
	}
	
	//分配内存空间
	newtask->host = (char *)malloc(strlen(host));
	if (newtask->host == NULL)
	{
		perror("newtask->host");
		free(newtask);
		return NULL;
	}
	
	newtask->url = (char *)malloc(strlen(url));
	if (newtask->url == NULL)
	{
		perror("newtask->url");
		free(newtask);
		free(newtask->host);
		return NULL;
	}
	
	newtask->filepath = (char *)malloc(strlen(filepath));
	if (newtask->filepath == NULL)
	{
		perror("newtask->filepath");
		free(newtask);
		free(newtask->host);
		free(newtask->url);
		return NULL;
	}
	
	//初始化新节点
	newtask->buf_size = buf_size;//缓冲区大小
	newtask->cb_func = cb_func;//回调函数
	newtask->next = NULL;
	strcpy(newtask->host, host);
	strcpy(newtask->url, url);
	strcpy(newtask->filepath, filepath);
	
	return newtask;
}

//释放任务结构体
void free_down_task(struct down_task *task)
{
	free(task->host);
	free(task->url);
	free(task->filepath);
}

//从http服务器下载文件
int HTTP_Download(struct down_task *task)
{
	//连接http服务器
	int socket = HTTP_Tcp_Client_IPv4(task->host, 80);
	if (socket < 0)
	{
		return -1;
	}
	
	//判断负载的长度
	int buf_len = strlen(HTTP_FORM) + strlen(task->host) + strlen(task->url);
	
	//分配内存空间
	char *buf = (char *)malloc(sizeof(char) * buf_len);
	if (buf == NULL)
	{
		perror("malloc");
		close(socket);
		return -1;
	}
	
	//获取http负载
	memset(buf, 0, buf_len);
	snprintf(buf, buf_len, HTTP_FORM, task->url, task->host);
	
	//发送http请求
	int ret = write(socket, buf, buf_len);
	if (ret < 0)
	{
		perror("write");
		close(socket);
		free(buf);
		buf = NULL;
		return -1;
	}
	
	//释放空间
	free(buf);
	buf = NULL;
	
	//打开文件
	int fd = open(task->filepath, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		perror("open");
		close(socket);
		return -1;
	}
	
	//获取缓存区
	buf = (char *)malloc(sizeof(char) * task->buf_size);
	if (buf == NULL)
	{
		perror("malloc buf");
		close(socket);
		close(fd);
		return -1;
	}
	
	#ifdef DEBUG
		printf("buf size = %d\n", task->buf_size);
	#endif
	
	//先读一次，获取报头
	memset(buf, 0, task->buf_size);//清空缓冲区
	ret = read(socket, buf, task->buf_size);

	//获取"Content-Length"的位置
	char *p = strstr(buf, "Content-Length");
	if (p == NULL)
	{
		close(socket);
		close(fd);
		free(buf);
		buf = NULL;
		return -1;
	}

	//获取文件的字节数
	int file_bytes;
	sscanf(p, "Content-Length: %d\r\n", &file_bytes);
	printf("download file bytes : %d\n", file_bytes);

	//找到有效数据的头
	p = strstr(buf, "\r\n\r\n");
	if (p == NULL)
	{
		close(socket);
		close(fd);
		free(buf);
		buf = NULL;
		return -1;
	}

	//计算偏移量
	p = p + 4;//"\r\n\r\n"

	//将有效数据写入文件并计算写入大小
	int send_bytes = write(fd, p, (ret - (p - buf)));
	
	//读取后续字节
	while(1)
	{
		memset(buf, 0, task->buf_size);
		ret = read(socket, buf, task->buf_size);
		if (ret <= 0)
		{
			//网络终止
			close(fd);
			close(socket);
			free(buf);
			buf = NULL;
			return -2;
		}
		
		#ifdef DEBUG
			printf("ret = %d\n", ret);
		#endif

		//写入文件并计算写入大小
		send_bytes += write(fd, buf, ret);

		if (file_bytes == send_bytes)
		{
			//文件已经接收完
			break;
		}
	}

	//关闭文件,关闭socket
	close(fd);
	close(socket);
	free(buf);
	buf = NULL;
	
	return 0;
}

//工作线程
void *routine(void *arg)
{
	//打桩
	printf("%s init success\n", __FUNCTION__);
	
	//获取线程池
	struct Http_Down_t 	*ctrl = (((struct routine_arg *)arg)->ctrl);
	
	//获取当前线程的节点
	struct down_thread 	*self = (((struct routine_arg *)arg)->self);

	//释放参数
	free((struct routine_arg *)arg);

	//任务节点
	struct down_task 	*task = NULL;
	
	//接返回值
	int ret = 0;
	
	while(1)
	{
		//解锁任务压栈
		pthread_cleanup_push(handler, (void *)&(ctrl->mutex));
		pthread_mutex_lock(&(ctrl->mutex));

		//如果没有任务,且线程池未关闭
		while(ctrl->task_num == 0 && ctrl->shut_down == 0)
		{
			//用条件变量阻塞
			pthread_cond_wait(&(ctrl->cond), &(ctrl->mutex));
		}

		//如果没有任务,且线程池关闭
		while(ctrl->task_num == 0 && ctrl->shut_down == 1)
		{
			//解锁当前线程,并退出
			pthread_mutex_unlock(&(ctrl->mutex));
			pthread_exit(NULL);
		}
		
		//判断设置线程数是否高于当前线程数
		if (ctrl->set_thread_num < ctrl->now_thread_num)
		{
			//找到当前线程的前一个线程消息结构体
			struct down_thread *prev = ctrl->thread_head;
			while(prev->next != self)
			{
				prev = prev->next;
			}
			
			//出队列
			prev->next = self->next;
			self->next = NULL;
			
			//当前线程数减一
			ctrl->now_thread_num--;
			
			//设置分离属性
			pthread_detach(pthread_self());
			
			//释放线程信息结构体
			free(self);
			
			//解锁当前线程,并退出
			pthread_mutex_unlock(&(ctrl->mutex));
			pthread_exit(NULL);
		}

		//有任务取出任务
		task = ctrl->task_head->next;
		ctrl->task_head->next = task->next;
		ctrl->task_num--;

		//释放线程锁,解锁函数弹出但不执行
		pthread_mutex_unlock(&(ctrl->mutex));
		pthread_cleanup_pop(0);

		//执行任务期间,线程不会被取消
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		
		ret = HTTP_Download(task);//执行下载任务
		(task->cb_func)(task, ret);//回调函数
	
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		//释放任务结构体
		free_down_task(task);
		
		task = NULL;
	}
}

//添加一条下载线程
static int HTTP_Download_add_thread(struct Http_Down_t *ctrl)
{
	//创建新节点
	struct down_thread *newthread = (struct down_thread *)malloc(sizeof(struct down_thread));
	if (newthread == NULL)
	{
		perror("malloc");
		return -1;
	}
	
	//初始化新节点
	struct routine_arg *targ = (struct routine_arg *)malloc(sizeof(struct routine_arg));
	if (targ == NULL)
	{
		perror("malloc");
		free(newthread);
		return -1;
	}
	
	targ->ctrl = ctrl;
	targ->self = newthread;
	
	//启动新的线程
	newthread->next = NULL;
	pthread_create(&(newthread->d_tid), NULL, routine, (void *)targ);
	
	//找到尾节点
	struct down_thread *p = ctrl->thread_head;
	while(p->next != NULL)
	{
		p = p->next;
	}
	
	//加入链表
	p->next = newthread;
	
	//线程数加一
	ctrl->now_thread_num++;
	
	return 0;
}

//管理线程
static void *admin_rout(void *arg)
{
	//打桩
	printf("%s init success\n", __FUNCTION__);
	
	//获取线程池
	struct Http_Down_t *ctrl = (struct Http_Down_t *)arg;
	
	int num;
	
	while(1)
	{	
		//线程池的退出
		if (ctrl->shut_down == 1)
		{
			pthread_exit(NULL);
		}
		
		#ifdef DEBUG
			HTTP_Download_show_info(ctrl);
		#endif
		
		//上锁
		pthread_cleanup_push(handler, (void *)&(ctrl->mutex));
		pthread_mutex_lock(&(ctrl->mutex));
		
		//判断是否需要加线程
		if (ctrl->now_thread_num < ctrl->max_thread_num && ctrl->set_thread_num > ctrl->now_thread_num)
		{
			//加一条线程
			HTTP_Download_add_thread(ctrl);
		}
		
		//释放线程锁,解锁函数弹出但不执行
		pthread_mutex_unlock(&(ctrl->mutex));
		pthread_cleanup_pop(0);
		
		sleep(2);
	}
	
}

//下载线程初始化
struct Http_Down_t *HTTP_Download_init(uint32_t set_num, uint32_t max_num)
{
	//分配内存空间
	struct Http_Down_t *ctrl = (struct Http_Down_t *)malloc(sizeof(struct Http_Down_t));
	if (ctrl == NULL)
	{
		perror("malloc");
		return NULL;
	}
	
	//初始化线程锁和条件变量
	pthread_mutex_init(&(ctrl->mutex), NULL);
	pthread_cond_init(&(ctrl->cond), NULL);
	
	//上锁
	pthread_mutex_lock(&(ctrl->mutex));
	
	//初始化变量
	ctrl->task_num  = 0;
	ctrl->shut_down = 0;
	ctrl->download_buf_size = D_DOWN_BUF_SIZE;//默认缓冲区大小
	
	//分配任务列表内存空间
	ctrl->task_head = (struct down_task *)malloc(sizeof(struct down_task));
	if(ctrl->task_head == NULL)
	{
		perror("head malloc");
		free(ctrl);
		return NULL;
	}
	//清空头节点
	memset(ctrl->task_head, 0, sizeof(struct down_task));
	ctrl->task_head->next = NULL;
	
	//初始化工作线程链表头
	ctrl->thread_head = (struct down_thread *)malloc(sizeof(struct down_thread));
	if(ctrl->thread_head == NULL)
	{
		perror("head malloc");
		free(ctrl->task_head);
		free(ctrl);
		return NULL;
	}
	//清空头节点
	memset(ctrl->thread_head, 0, sizeof(struct down_thread));
	ctrl->thread_head->next = NULL;
	
	//当前在线线程数
	ctrl->now_thread_num = 0;
	
	//系统最大线程数
	ctrl->max_thread_num = max_num;
	
	//系统配置线程数
	ctrl->set_thread_num = ((set_num<max_num)?(set_num):(max_num));
	
	//添加一条工作线程
	HTTP_Download_add_thread(ctrl);
	
	//启动管理者线程
	pthread_create(&(ctrl->admin_tid), NULL, admin_rout, (void *)ctrl);

	//解锁
	pthread_mutex_unlock(&(ctrl->mutex));

	return ctrl;
}

//关闭http下载线程
int HTTP_Download_close(struct Http_Down_t *ctrl)
{
	if (ctrl == NULL)
	{
		printf("not inited\n");
		return -1;
	}
	
	//上锁
	pthread_mutex_lock(&(ctrl->mutex));
	
	//关机标志位
	ctrl->shut_down = 1;
	
	//回收管理者线程
	pthread_join(ctrl->admin_tid, NULL);
	printf("join admin rout success\n");
	
	//清空任务链表
	struct down_task *p = ctrl->task_head->next;
	struct down_task *p1;
	
	//清空任务
	while(p != NULL)
	{
		p1 = p->next;
		
		free_down_task(p);
		
		p = p1;
	}
	
	ctrl->task_num = 0;
	ctrl->task_head->next = NULL;
	
	//解锁
	pthread_mutex_unlock(&(ctrl->mutex));
	
	//唤醒工作线程
	pthread_cond_broadcast(&(ctrl->cond));
	
	printf("waiting download routine\n");
	
	//循环回收工作线程
	struct down_thread *pt;
	while(ctrl->thread_head->next != NULL)
	{
		//指向头节点的下一个节点
		pt = ctrl->thread_head->next;
		
		//回收线程
		pthread_join(pt->d_tid, NULL);
		printf("join one thread\n");
		
		//回收的线程出链表
		ctrl->thread_head->next = pt->next;
		pt->next = NULL;
		
		//在线线程数减一
		ctrl->now_thread_num--;
		
		//释放当前线程
		free(pt);
	}
	
	//清空链表
	free(ctrl->thread_head);
	free(ctrl->task_head);
	
	//释放内存空间
	free(ctrl);
	
	return 0;
}

//添加下载任务
int HTTP_Download_add_task(struct Http_Down_t *ctrl, const char *host, const char *url, const char *filepath, func_t cb_func)
{
	//创建新的节点
	struct down_task *newtask = init_down_task(host, url, filepath, ctrl->download_buf_size, cb_func);
	if (newtask == NULL)
	{
		printf("init down task fail");
		return -1;
	}
	
	//上锁
	pthread_mutex_lock(&(ctrl->mutex));
	
	//找到链表尾
	struct down_task *p = ctrl->task_head;
	while(p->next != NULL)
	{
		p = p->next;
	}
	
	//加入链表
	p->next = newtask;
	
	//任务数加一
	ctrl->task_num++;
	
	//解锁
	pthread_mutex_unlock(&(ctrl->mutex));
	
	//唤醒线程
	pthread_cond_signal(&(ctrl->cond));
	
	return 0;
}

//打印线程池信息
void HTTP_Download_show_info(struct Http_Down_t *ctrl)
{
	//上锁
	pthread_cleanup_push(handler, (void *)&(ctrl->mutex));
	pthread_mutex_lock(&(ctrl->mutex));
	
	printf("----------------------------------\n");
	printf("| shut_down status	:%d\t |\n", ctrl->shut_down);
	printf("\n");
	printf("| task_num 		:%u\t |\n", ctrl->task_num);
	printf("\n");
	printf("| max_thread_num	:%u\t |\n", ctrl->max_thread_num);
	printf("| set_thread_num	:%u\t |\n", ctrl->set_thread_num);
	printf("| now_thread_num	:%u\t |\n", ctrl->now_thread_num);
	printf("\n");
	printf("| download_buf_size	:%u\t |\n", ctrl->download_buf_size);
	printf("----------------------------------\n");
	
	//释放线程锁,解锁函数弹出但不执行
	pthread_mutex_unlock(&(ctrl->mutex));
	pthread_cleanup_pop(0);
}



