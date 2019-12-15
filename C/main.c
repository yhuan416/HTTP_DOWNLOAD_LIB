#include "../common.h"

#include "../lib/HTTP_download.h"

//download success call back function
void down_success(struct down_task *task)
{
	printf("download success\n");
	printf("%s, %s, %s\n", task->host, task->url, task->filepath);
}

//download interrupt call back function
void down_interrupt(struct down_task *task)
{
	printf("download interrupt\n");
	printf("%s, %s, %s\n", task->host, task->url, task->filepath);
}

//download fail call back function
void down_fail(struct down_task *task)
{
	printf("download fail\n");
	printf("%s, %s, %s\n", task->host, task->url, task->filepath);
}

//download result
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

int main(int argc, const char **argv)
{
	
	//初始化下载线程
	struct Http_Down_t *Download = HTTP_Download_init(1, 5);
	if (Download == NULL)
	{
		return -1;
	}
	
	//设置buf大小
	HTTP_Download_set_bufsize(Download, (1024*32));
	
	char cmd[20];
	uint32_t ret;
	while(1)
	{
		memset(cmd, 0, 20);
		scanf("%s", cmd);
		
		if (strcmp(cmd, "add") == 0)
		{
			//添加下载任务
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/1.bmp", "1.bmp", down_cb);
	
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/2.bmp", "2.bmp", down_cb);
	
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/4.bmp", "4.bmp", down_cb);
	
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/5.bmp", "5.bmp", down_cb);
	
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/1.bmp", "1.bmp", down_cb);
	
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/2.bmp", "2.bmp", down_cb);
	
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/4.bmp", "4.bmp", down_cb);
	
			ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/5.bmp", "5.bmp", down_cb);
	
			//ret = HTTP_Download_add_task(Download, "192.168.95.215", "/pic/1.bmp", "1.bmp", down_cb);
			//ret = HTTP_Download_add_task(Download, "192.168.95.215", "/pic/2.bmp", "2.bmp", down_cb);
			//ret = HTTP_Download_add_task(Download, "192.168.95.215", "/pic/3.bmp", "3.bmp", down_cb);
			//ret = HTTP_Download_add_task(Download, "192.168.95.215", "/pic/4.bmp", "4.bmp", down_cb);
			//ret = HTTP_Download_add_task(Download, "192.168.95.215", "/pic/5.bmp", "5.bmp", down_cb);
			//ret = HTTP_Download_add_task(Download, "192.168.95.215", "/pic/6.bmp", "6.bmp", down_cb);
			
			//ret = HTTP_Download_add_task(Download, "192.168.199.118", "/pic/1.bmp", "1.bmp");
		}
		else if (strcmp(cmd, "exit") == 0)
		{
			break;
		}
		else if (strcmp(cmd, "set") == 0)
		{
			scanf("%u", &ret);
			
			HTTP_Download_set_setthreadnum(Download, ret);
		}
	}
	
	HTTP_Download_close(Download);
	
	
	
	/*
	struct down_task *task = init_down_task("192.168.199.118", "/pic/1.bmp", "1.bmp", (1024*1024*4), NULL);
	
	int ret = HTTP_Download(task);
	
	down_cb(task, ret);
	
	free_down_task(task);
	*/
	
	return 0;
}


