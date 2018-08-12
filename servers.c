#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#include "logging.h"
#include "conf.h"
#include "public.h"
#include "utils.h"
#include "dlapi.h"


#include "servers.h"
#include "monitor.h"



#define NEWIUP_UP_ERROR_SKIP_TIME 60
#define NEWIUP_UP_ERROR_CUR_WV_TIMES 1


real_server *g_server = NULL;
int g_server_num = 0;

char * get_server_name_by_index(int index)
{
    return g_server[index].name;
} 

real_server *get_server_by_index(int index)
{
    return &g_server[index];
} 

int get_server_id_by_name(char *name)
{
	int i = 0;
	for(i=0; i<g_server_num; i++)
	{
		if(strcmp(g_server[i].name, name)==0)
		{
			return i;
		}
	}

    return NEWIUP_FAIL;
} 

real_server * get_server_by_name(char *name)
{
	int i = 0;
	for(i=0; i<g_server_num; i++)
	{
		if(strcmp(g_server[i].name, name)==0)
		{
			return &g_server[i];
		}
	}

    return NULL;
} 

int get_server_available_state(int index)
{
	if(g_server[index].cur_fst>= 0 && time(NULL) - g_server[index].cur_fst <= g_server[index].fail_sleep_time)
	{
		return NEWIUP_FAIL;
	}
	__sync_lock_test_and_set(&g_server[index].cur_fst , 0);
	return NEWIUP_OK;
}
queue_s * get_server_queue_by_index(int index)
{
	return &g_server[index].queue;
}
static volatile int g_upload_run_signal=NEWIUP_OK;
static int get_upload_run_signal(void)
{
	return g_upload_run_signal;
}
void set_upload_run_signal(int signal)
{
	g_upload_run_signal=signal;
	return ;
}
extern void *upload_handle(void *arg);

//return: 0:sucess -1:fail
static int get_server_info(void)
{
    logging(LOG_INFO, "-- %s\n", __func__);

    ConfNode *child = NULL;
    ConfNode *node = NULL;
    uint32_t count = 0;
    char *str = NULL;

    node = ConfGetNode("servers");
    TAILQ_FOREACH(child, &node->head, next) 
    {
        g_server[count].index = count;
		if(ConfNodeNameCheck(child->val)){
            logging(LOG_ERROR, "--%s [%s]-[name] Name repetition error\n", __func__, child->val);
            return NEWIUP_FAIL;
		}
        strncpy(g_server[count].name, child->val, NEWIUP_NAME_LEN);
		//analyze
        if (ConfGetChildValue(child, "analyzelib", &str) == 0)
        {
            logging(LOG_ERROR, "--%s [%s]-[analyze] error \n", __func__, g_server[count].name);
            return NEWIUP_FAIL;
        }
        strcpy(g_server[count].analyze, str);
		g_server[count].analyze_lib_p = g_server[count].analyze;
		g_server[count].analyze_lib_f = strchr(g_server[count].analyze, ':');
		if(!g_server[count].analyze_lib_f){
            logging(LOG_ERROR, "--%s [%s]-[analyze] error \n", __func__, g_server[count].name);
            return NEWIUP_FAIL;
		}			
		*g_server[count].analyze_lib_f = 0;
		g_server[count].analyze_lib_f+=1;

		
		//addr
        if (ConfGetChildValue(child, "addr", &str) == 0)
        {
            logging(LOG_ERROR, "--%s [%s]-[addr] error \n", __func__, g_server[count].name);
            return NEWIUP_FAIL;
        }
        strncpy(g_server[count].addr, str, sizeof(g_server[count].addr));
		//username
        if (ConfGetChildValue(child, "username", &str) == 0)
        {
            logging(LOG_ERROR, "--%s [%s]-[username] error\n", __func__, g_server[count].name);
            return NEWIUP_FAIL;
        }
        strncpy(g_server[count].username, str, NEWIUP_NAME_LEN);
		//passwd
        if (ConfGetChildValue(child, "passwd", &str) == 0)
        {
            logging(LOG_ERROR, "--%s [%s]-[passwd] error\n",__func__, g_server[count].name);
            return NEWIUP_FAIL;
        }
        strncpy(g_server[count].passwd, str, sizeof(g_server[count].passwd));

		//finish-confirm
        if (ConfGetChildValue(child, "finish-flag", &str) == 0)
        {
            logging(LOG_ERROR, "--%s [%s]-[finish-flag] error\n",__func__, g_server[count].name);
            return NEWIUP_FAIL;
        }
		g_server[count].finish_confirm[0]='\0';
		if(str[0]!='\0'){
			strcpy(g_server[count].finish_confirm, str);	
		}
		//debug
		if (ConfGetChildValue(child, "debug", &str) == 0)
		{
			g_server[count].debug=NEWIUP_OK;
		}else{
			g_server[count].debug=NEWIUP_FAIL;
		}
		//file-suffix: 
		if (ConfGetChildValue(child, "file-suffix", &str) == 0)
		{
			logging(LOG_ERROR, "--%s [%s]-[file-suffix] error\n",__func__, g_server[count].name);
			return NEWIUP_FAIL;
		}
		g_server[count].file_suffix[0]='\0';
		if(str[0]!='\0'){
			strcpy(g_server[count].file_suffix, str);	
		}
		//thread_num: 
		if (ConfGetChildValue(child, "thread-num", &str) == 0)
		{
			logging(LOG_ERROR, "--%s [%s]-[thread-num] error\n",__func__, g_server[count].name);
			return NEWIUP_FAIL;
		}
		g_server[count].thread_num=atoi(str);
		if(g_server[count].thread_num < 1 || g_server[count].thread_num >15){
			logging(LOG_ERROR, "--%s [%s]-[thread-num] error: not support %s [1-15]\n",__func__, g_server[count].name, str);
			return NEWIUP_FAIL;
		}

		//attempts: 
		g_server[count].retry_count= NEWIUP_UP_ERROR_CUR_WV_TIMES;
		if (ConfGetChildValue(child, "attempts", &str) != 0)
		{
			g_server[count].retry_count = atoi(str);
		}
		//fsleep_time: 
		g_server[count].fail_sleep_time = NEWIUP_UP_ERROR_SKIP_TIME;
		if (ConfGetChildValue(child, "fsleep-time", &str) != 0)
		{
			g_server[count].fail_sleep_time= atoi(str);
		}

		g_server[count].make_citations = NEWIUP_FAIL;

		g_server[count].cur_rc= 0;
		g_server[count].cur_fst= NEWIUP_FAIL;

		pthread_spin_init(&g_server[count].queue.lock, PTHREAD_PROCESS_PRIVATE);
		g_server[count].queue.size=0;
		TAILQ_INIT(&g_server[count].queue.head);
		strcpy(g_server[count].queue.name, g_server[count].name);

		g_server[count].pid=(pthread_t *)malloc(sizeof(pthread_t) * g_server[count].thread_num);
		if(g_server[count].pid == NULL){
			logging(LOG_ERROR, "--%s [%s]-malloc error\n",__func__, g_server[count].name);
			return NEWIUP_FAIL;
		}
		g_server[count].func = upload_handle;
        count++;
    }

    int i;
    for (i = 0; i < g_server_num; i++) 
    {
        logging(LOG_INFO, "----number--%d\n", g_server[i].index);
        logging(LOG_INFO, "name(srv)     =%s\n", g_server[i].name);
        logging(LOG_INFO, "analyzelib    =%s\n", g_server[i].analyze);
        logging(LOG_INFO, "addr          =%s\n", g_server[i].addr);
        logging(LOG_INFO, "username      =%s\n", g_server[i].username);
        logging(LOG_INFO, "passwd        =%s\n", g_server[i].passwd);
        logging(LOG_INFO, "finish-flag   =%s\n", g_server[i].finish_confirm);
        logging(LOG_INFO, "file-suffix   =%s\n", g_server[i].file_suffix);
        logging(LOG_INFO, "thread_num    =%d\n", g_server[i].thread_num);
        logging(LOG_INFO, "attempts      =%d\n", g_server[i].retry_count);
        logging(LOG_INFO, "fsleep-time   =%d [s]\n", g_server[i].fail_sleep_time);
        logging(LOG_INFO, "debug         =%d 0:no|1:yes\n", g_server[i].debug);
    }

    return NEWIUP_OK;
} 


//return: 0:sucess -1:fail(exit)
int servers_init(void)
{
	logging(LOG_INFO, "++++++++++++++++++++\n");
    g_server_num = ConfGetChildNumber("servers");
    logging(LOG_INFO, "%s: server number = %d\n", __func__, g_server_num);
    if (g_server_num <= 0)
    {
        logging(LOG_ERROR, " --%s get [server_number] error(exit)\n", __func__);
        return NEWIUP_FAIL;
    }

    g_server = (real_server *)malloc(sizeof(real_server) * g_server_num);
    if (g_server == NULL)
    {
        logging(LOG_ERROR, " --%s malloc servers error(exit)\n", __func__);
        return NEWIUP_FAIL;
    }
    memset(g_server, 0, sizeof(real_server) * g_server_num);
    if (get_server_info() != NEWIUP_OK)
    {
		servers_exit();
        return NEWIUP_FAIL;
    }

    return NEWIUP_OK;
}

int servers_exit(void)
{
	int i;
	FileQueue *node = NULL;
	for(i=0; i<g_server_num; i++){
		if(g_server[i].pid)
			free(g_server[i].pid);
		
        while ((node = TAILQ_FIRST(&g_server[i].queue.head)) != NULL)
        {
            TAILQ_REMOVE(&g_server[i].queue.head, node, next);
			queue_node_free(node);
        }
		
	}
    if (g_server)
        free(g_server);
	g_server = NULL;
	g_server_num = 0;

    return 0;    
}

char *res[2]={"FAIL", "SUCESS"};

#define MB (1024*1024)
#define KB (1024)

int do_upload_file_checking(int index, FileQueue *node)
{
	
	if(get_server_available_state(index)==NEWIUP_FAIL){
		queue_node_free(node);
		return NEWIUP_FAIL;
	}
	node->dstfile = node->src;
	if(file_exists_state(node->dstfile) != 0){
		queue_node_free(node);
		return NEWIUP_FAIL;
	}
	node->up_size = get_file_size(node->dstfile);
	
	return NEWIUP_OK;
}

int do_upload_file(real_server *server, FileQueue *node)
{
	int ret=0;
//	int mkd_ret = NEWIUP_OK;

	ret = dynamic_call_library_func1(server->analyze_lib_p, server->analyze_lib_f, node->src);

	if(ret == -9000 || ret == -9001){
		//UPLOG_TXT(node->monitor_id, "[INFO] %s do error: %s\n", server->name, node->src);
		logging(LOG_INFO, "%s %s %s %d\n", server->analyze_lib_p, server->analyze_lib_f, node->src, ret);
	} else if(ret == NEWIUP_OK){
		delete_file(node->src);
		UPLOG_LOG(node->monitor_id, "[FINISH] %s delete upload flag file: %s\n", server->name, node->src);
	} else if( ret == NEWIUP_FAIL){
		UPLOG_TXT(node->monitor_id, "[FINISH] %s keep upload flag file: %s\n", server->name, node->src);
	}
	//logging(LOG_INFO, "%s %s %s %d\n", server->analyze_lib_p, server->analyze_lib_f, node->src, ret);


	return ret;
}



void *upload_handle(void *arg)
{
	int index = *(int *)arg;
	int ret;
	FileQueue *node = NULL;
	
    logging(LOG_INFO, "UPLOAD [%s] thread start\n", g_server[index].name);
	while(get_upload_run_signal() == NEWIUP_OK){
		usleep(1);

        node = queue_node_pop(&g_server[index].queue);
        if (node == NULL){
            sleep (QUEUE_EMPTY_SLEEP);
            continue;
        }
		
		ret = do_upload_file_checking(index, node);
		if(NEWIUP_FAIL == ret)
			continue;

		ret = do_upload_file(&g_server[index], node);
		if(ret == NEWIUP_OK)
		{
			do_upload_file_finish_single(g_server[index].name, node);
			g_server[index].cur_rc = 0;
		}else{
			if(g_server[index].cur_rc++ < g_server[index].retry_count){
				queue_node_push(&g_server[index].queue, node);
				continue;
			}
			__sync_lock_test_and_set(&g_server[index].cur_fst, time(NULL));
			g_server[index].cur_rc = 0;
		}

		queue_node_free(node);
	}

    logging(LOG_INFO, "UPLOAD [%s] thread exit\n", g_server[index].name);
	return NULL; 
}

void server_upload_handle_pthread_start(real_server *rserver)
{
	int i;
	
	if(rserver && rserver->make_citations == NEWIUP_FAIL){
		for(i=0; i<rserver->thread_num; i++){
			pthread_create(&rserver->pid[i], NULL, rserver->func, (void *)&rserver->index);
		}
		rserver->make_citations = NEWIUP_OK;
	}
}
void server_upload_handle_pthread_end(real_server *rserver)
{
	int i;
	if(rserver)
		for(i=0; i<rserver->thread_num; i++){
			pthread_join(rserver->pid[i], NULL);
		}
}
