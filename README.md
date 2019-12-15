Author:YHUAN  
Date:19/11/30  
Version:1.0  

**HTTP 下载线程池**

这是我自己写的HTTP下载线程池,写的不算好,功能很简单,只能实现简单的HTTP下载请求  
> 下载结果会有三种状态  
> success/interrupt/fail  
用户可以通过回调函数去处理这三种结果(具体参考下面的例子)  

---------------------------------------------------------------------------------------------

# 用法1:直接调用下载任务函数进行下载

> struct down_task *init_down_task(const char *host, const char *url, const char *filepath, uint32_t buf_size, func_t cb_func);  
获取下载任务结构体  
	host:http服务器的IP  
	url:http请求的URL  
	filepath:文件保存的本地路径  
	buf_size:下载时的缓冲区大小  
	cb_func:下载结束的回调函数  


> int HTTP_Download(struct down_task *task);  
进行下载  
	task:初始化成功的下载任务结构体  


> void free_down_task(struct down_task *task);  
释放下载任务结构体  
	task:初始化成功的下载任务结构体  

## 例程:

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

# 用法2:使用线程池进行下载  

> struct Http_Down_t *HTTP_Download_init(uint32_t set_num, uint32_t max_num);  
初始化线程池:  
	set_num:线程初始化后的初始工作线程数  
	max_num:线程的最大线程数  

注意:线程池成功初始化后只有一条工作线程,同时会有一条管理线程每两秒对工作线程数进行判断,随时添加线程  
另外,当设置线程数小于当前线程数时,非睡眠状态的工作线程会设置分离属性并退出  


> int HTTP_Download_close(struct Http_Down_t *ctrl);  
线程池的关闭:  
	ctrl:运行中的线程池  

---------------------------------------------------------------------------------------------

> void HTTP_Download_get_bufsize(struct Http_Down_t *ctrl, uint32_t *size);  
> void HTTP_Download_set_bufsize(struct Http_Down_t *ctrl, uint32_t size);  
设置/获取下载缓冲区大小的设置:  

注意:设置的时候,会对线程池进行上锁操作  


> void HTTP_Download_get_setthreadnum(struct Http_Down_t *ctrl, uint32_t *num);  
> void HTTP_Download_set_setthreadnum(struct Http_Down_t *ctrl, uint32_t num);  
设置/获取工作线程数的设置:

注意:设置的时候,会对线程池进行上锁操作

---------------------------------------------------------------------------------------------

> int HTTP_Download_add_task(struct Http_Down_t *ctrl, const char *host, const char *url, const char *filepath, func_t cb_func);  
添加下载任务:  
	ctrl:运行中的线程池  
	host:IP  
	url:URL  
	filepath:文件保存的本地路径  
	cb_func:下载结果回调函数  

注意:使用线程池添加任务进行HTTP下载时,会自动调用上面提及的初始化下载结构体函数。  
此时,下载时的缓冲区大小使用线程池配置的大小。  
下载任务执行完毕后,会把下载的结果和任务结构体传入回调函数,用户可以配置回调函数,对下载后的文件进行处理  

## 例程:

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
