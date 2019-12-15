/*

author:YHUAN
date:19/11/30
version:1.0

HTTP 下载线程池

---------------------------------------------------------------------------------------------

用法1:直接调用下载任务函数进行下载

struct down_task *init_down_task(const char *host, const char *url, const char *filepath, uint32_t buf_size, func_t cb_func);
获取下载任务结构体
	host:http服务器的IP
	url:http请求的URL
	filepath:文件保存的本地路径
	buf_size:下载时的缓冲区大小
	cb_func:下载结束的回调函数


int HTTP_Download(struct down_task *task);
进行下载
	task:初始化成功的下载任务结构体


void free_down_task(struct down_task *task);
释放下载任务结构体
	task:初始化成功的下载任务结构体

例程:

...

//下载结果回调函数
void down_cb(struct down_task *task, int ret)
{
	if (ret == 0)
	{
		down_success(task);
	}
	else if (ret == -2)
	{
		down_interrupt(task);
	}
	else
	{
		down_fail(task);
	}
}

...

struct down_task *task = init_down_task("192.168.199.118", "/pic/1.bmp", "1.bmp", 4096, NULL);
int ret = HTTP_Download(task);

//非必须
down_cb(task, ret);

free_down_task(task);

...

---------------------------------------------------------------------------------------------

用法2:使用线程池进行下载

struct Http_Down_t *HTTP_Download_init(uint32_t set_num, uint32_t max_num);
初始化线程池:
	set_num:线程初始化后的初始工作线程数
	max_num:线程的最大线程数

注意:线程池成功初始化后只有一条工作线程,同时会有一条管理线程每两秒对工作线程数进行判断,随时添加线程
		另外,当设置线程数小于当前线程数时,非睡眠状态的工作线程会设置分离属性并退出


int HTTP_Download_close(struct Http_Down_t *ctrl);
线程池的关闭:
	ctrl:运行中的线程池

---------------------------------------------------------------------------------------------

void HTTP_Download_get_bufsize(struct Http_Down_t *ctrl, uint32_t *size);
void HTTP_Download_set_bufsize(struct Http_Down_t *ctrl, uint32_t size);
设置/获取下载缓冲区大小的设置:

注意:设置的时候,会对线程池进行上锁操作


void HTTP_Download_get_setthreadnum(struct Http_Down_t *ctrl, uint32_t *num);
void HTTP_Download_set_setthreadnum(struct Http_Down_t *ctrl, uint32_t num);
设置/获取工作线程数的设置:

注意:设置的时候,会对线程池进行上锁操作

---------------------------------------------------------------------------------------------

int HTTP_Download_add_task(struct Http_Down_t *ctrl, const char *host, const char *url, const char *filepath, func_t cb_func);
添加下载任务:
	ctrl:运行中的线程池
	host:IP
	url:URL
	filepath:文件保存的本地路径
	cb_func:下载结果回调函数

注意:使用线程池添加任务进行HTTP下载时,会自动调用上面提及的初始化下载结构体函数。
		此时,下载时的缓冲区大小使用线程池配置的大小。
		下载任务执行完毕后,会把下载的结果和任务结构体传入回调函数,用户可以配置回调函数,对下载后的文件进行处理

例程:

...

//下载结果回调函数
void down_cb(struct down_task *task, int ret)
{
	if (ret == 0)
	{
		down_success(task);
	}
	else if (ret == -2)
	{
		down_interrupt(task);
	}
	else
	{
		down_fail(task);
	}
}

...

int main()
{
	...

	//初始化下载线程
	struct Http_Down_t *Download = HTTP_Download_init(1, 5);
	if (Download == NULL)
	{
		return -1;
	}
	
	//设置buf大小
	HTTP_Download_set_bufsize(Download, (1024*32));

	//设置工作线程数
	HTTP_Download_set_setthreadnum(Download, ret);

	//添加下载任务
	HTTP_Download_add_task(Download, "192.168.95.215", "/pic/6.bmp", "6.bmp", down_cb);

	//关闭下载线程池
	HTTP_Download_close(Download);

	...

	return 0;
}

---------------------------------------------------------------------------------------------

 */
#ifndef _HTTP_DOWNLOAD_H
#define _HTTP_DOWNLOAD_H

#include <pthread.h>
#include <stdint.h>

/* http请求格式 */
//					"GET /getSysTime.do HTTP/1.1\r\nHost:quan.suning.com\r\n\r\n"
#define HTTP_FORM 	"GET %s HTTP/1.1\r\nHost:%s\r\n\r\n"

/* 默认缓冲区大小 4K */
#define D_DOWN_BUF_SIZE		(4096)

//下载任务结构体
typedef struct down_task dt_t, *dt_p;

//下载结果处理回调函数
typedef void (*func_t)(dt_p, int);

//下载任务节点
struct down_task
{	
	uint32_t buf_size;//缓冲区大小

	char *host;//IP
	char *url;//URL
	
	char *filepath;//保存文件的路径
	
	func_t cb_func;//回调函数
	
	struct down_task *next;
};

//线程节点
struct down_thread
{
	pthread_t d_tid;
	
	struct down_thread *next;
};

struct Http_Down_t
{
	//下载线程关闭标志位
	int shut_down;
	
	//线程锁/条件变量
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
	
	//管理线程
	pthread_t admin_tid;
	
	//下载任务线程头
	struct down_task *task_head;
	uint32_t task_num;
	
	//下载线程链表头
	struct down_thread *thread_head;
	
	uint32_t max_thread_num;//最大线程数
	uint32_t set_thread_num;//设置线程数
	uint32_t now_thread_num;//当前线程数
	
	//缓冲区大小
	uint32_t download_buf_size;
};

//工作线程启动参数
struct routine_arg
{
	struct Http_Down_t 	*ctrl;
	struct down_thread 	*self;
};

/* 下载任务函数 */
int HTTP_Download(struct down_task *task);

/* 初始化/释放 任务节点 */
struct down_task *init_down_task(const char *host, const char *url, const char *filepath, uint32_t buf_size, func_t cb_func);
void free_down_task(struct down_task *task);

/* 获取/设置缓冲区大小 */
void HTTP_Download_get_bufsize(struct Http_Down_t *ctrl, uint32_t *size);
void HTTP_Download_set_bufsize(struct Http_Down_t *ctrl, uint32_t size);

/* 获取/设置线程池预设工作线程数 */
void HTTP_Download_get_setthreadnum(struct Http_Down_t *ctrl, uint32_t *num);
void HTTP_Download_set_setthreadnum(struct Http_Down_t *ctrl, uint32_t num);

/* 初始化/关闭 线程下载任务 */
struct Http_Down_t *HTTP_Download_init(uint32_t set_num, uint32_t max_num);
int HTTP_Download_close(struct Http_Down_t *ctrl);

/* 添加下载任务 */
int HTTP_Download_add_task(struct Http_Down_t *ctrl, const char *host, const char *url, const char *filepath, func_t cb_func);

/* 打印信息 */
void HTTP_Download_show_info(struct Http_Down_t *ctrl);

#endif
