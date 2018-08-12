#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "conf.h"
#include "conf-yaml-loader.h"
#include "public.h"
#include "logging.h"
#include "utils.h"

#include "servers.h"
#include "monitor.h"

global_config_s global_config={-1,10000,30,{0}};
db_config_s db_config;

/**
 * \brief Get a ConfNodeValue by node.
 *
 */
int ConfGetNodeValue(ConfNode *node, char *name, char **value)
{
	if(node == NULL || name == NULL)
	{
		return -1;
	}
	if(strcmp(node->name, name)==0)
	{
		*value = node->val;
		return 0;
	}
	return -1;
}

int ConfNodeNameCheck(char *name)
{
	ConfNode *child1 = NULL;
	ConfNode *child2 = NULL;
	//ConfNode *child3 = NULL;
    ConfNode *root = ConfGetRootNode();
	int num=0;

	TAILQ_FOREACH(child1, &root->head, next) 
	{		
		TAILQ_FOREACH(child2, &child1->head, next) 
		{
			if(strcmp(child2->val, name) == 0){
				num++;
				if(num>=2){
					return -1;
				}
			} 
			//printf("\t%s:%s\n",child2->name, child2->val);
		}
	}
	return 0;
}



int show_config_info()
{
    printf("-- %s\n", __func__);
	ConfNode *child1 = NULL;
	ConfNode *child2 = NULL;
	ConfNode *child3 = NULL;
    ConfNode *root = ConfGetRootNode();

	TAILQ_FOREACH(child1, &root->head, next) 
	{
		printf("%s:%s\n",child1->name, child1->val);
		
		TAILQ_FOREACH(child2, &child1->head, next) 
		{
			printf("\t%s:%s\n",child2->name, child2->val);
			TAILQ_FOREACH(child3, &child2->head, next) 
			{
				printf("\t\t%s:%s\n",child3->name, child3->val);
				
			}
		}
	}
	return 0;
}

void switch_to_s(char *src)
{
	char *p=src;
	if(p == NULL)return;
	p=strchr(p, '-');
	while(p){
		*p='_';
		p=strchr(p, '-');
	}
	return ;
}

int global_cfg_init()
{

    ConfNode *child = NULL;
    ConfNode *node = NULL;
    char *str = NULL;

    node = ConfGetNode("global-config");
    TAILQ_FOREACH(child, &node->head, next) 
    {
        if (ConfGetNodeValue(child, "affinity", &str) == 0)
        {
        	if(strcmp(str, "no") == 0){
				global_config.affinity = -1;
			}else{
				global_config.affinity = atoi(str);
			}
			continue;
        }
        if (ConfGetNodeValue(child, "max-upload", &str) == 0)
        {
			global_config.max_upload_num= atoi(str);
			continue;
        }
		if (ConfGetNodeValue(child, "log-dir-path", &str) == 0)
        {
			strncpy(global_config.log_dir_path, str, NEWIUP_PATH_LEN);
			if (logging_init(str) != 0)
			{
				log_printf(LOG_ERROR, "logging_init error --%s|%d\n", __func__, __LINE__);
				return NEWIUP_FAIL;
			}
			continue;
        }
		if (ConfGetNodeValue(child, "data-expiration-time", &str) == 0)
		{
			global_config.data_expiration_time= atoi(str);
			continue;
		}


		if (ConfGetNodeValue(child, "connect-timeout", &str) == 0)
		{
			global_config.connect_timeout= atoi(str);
			continue;
		}
		log_printf(LOG_ERROR, "--%s [%s] error\n",__func__, child->name);
		exit(1);
    }

	if (is_dir(global_config.log_dir_path) != 0)
	{
		if (recursive_make_dir(global_config.log_dir_path, DIR_MODE) != 0)
		{
			logging(LOG_ERROR, "--%s create direcotry [%s] error\n",__func__, global_config.log_dir_path);
			return NEWIUP_FAIL;
		}
		logging(LOG_INFO, "create log direcotry: [%s]\n", global_config.log_dir_path);
	}
    logging(LOG_INFO, "affinity             =%d\n", global_config.affinity);
    logging(LOG_INFO, "max_upload_num       =%d\n", global_config.max_upload_num);
    logging(LOG_INFO, "connect_timeout      =%d\n", global_config.connect_timeout);
    logging(LOG_INFO, "log_dir_path         =%s\n", global_config.log_dir_path);
    logging(LOG_INFO, "data_expiration_time =%d\n", global_config.data_expiration_time);

    return NEWIUP_OK;
	
}
int getDBConfig()
{
	ConfNode *node = NULL;

	memset((void *)&db_config, 0, sizeof(db_config_s));

	char *val=NULL;
	
    logging(LOG_INFO, "\n\n-- %s\n", __func__);
	node = ConfGetNode("DBServer");
	//TAILQ_FOREACH(child, &node->head, next) 
	{
		ConfGetChildValue(node, (char *)"DBHost", &val);
		if(val == NULL){
			log_printf(LOG_ERROR, "*************** DBHost is error\n");
			return -1;
		}
		strcpy(db_config.DBHost, val);

		ConfGetChildValue(node, (char *)"DBUser", &val);
		if(val == NULL){
			log_printf(LOG_ERROR, "*************** DBUser is error\n");
			return -1;
		}
		strcpy(db_config.DBUser, val);
		switch_to_s(db_config.DBUser);
		
		ConfGetChildValue(node, (char *)"DBPassWord", &val);
		if(val == NULL){
			log_printf(LOG_ERROR, "*************** DBPassWord is error\n");
			return -1;
		}
		strcpy(db_config.DBPassWord, val);
		
		ConfGetChildValue(node, (char *)"DBName", &val);
		if(val == NULL){
			log_printf(LOG_ERROR, "*************** DBName is error\n");
			return -1;
		}
		strcpy(db_config.DBName, val);
	
		ConfGetChildValue(node, (char *)"DBPort", &val);
		if(val != NULL){
			db_config.DBPort = atoi(val);
		}

		ConfGetChildValue(node, (char *)"DBSocket", &val);
		if(val != NULL){
			strcpy(db_config.DBSocket, val);
		}
	}

	logging(LOG_INFO, "DBHost                       =%s\n", db_config.DBHost);
	logging(LOG_INFO, "DBUser                       =%s\n", db_config.DBUser);
	logging(LOG_INFO, "DBPasswd                     =%s\n", db_config.DBPassWord);
	logging(LOG_INFO, "DBName                       =%s\n", db_config.DBName);
	logging(LOG_INFO, "DBPort                       =%u\n", db_config.DBPort);
	logging(LOG_INFO, "DBSocket                     =%s\n\n", db_config.DBSocket);
	
	return 0;	
}

int ThreadGetCPUAffinity(pthread_t pid, int cpuid)
{
	int ret;
	if(cpuid == -1){
		return 0;
	}
    cpu_set_t cs;

    CPU_ZERO(&cs);
    CPU_SET(cpuid, &cs);

	ret = pthread_getaffinity_np(pid, sizeof(cpu_set_t), &cs);
    if (ret != 0){
        //printf("pthread_getaffinity_np fail\n");
	}
	int j;
	for (j = 0; j < CPU_SETSIZE; j++)
        if (CPU_ISSET(j, &cs)){
            //printf("CPU %d\n", j);
		}
	return ret;
}

int ThreadSetCPUAffinity(pthread_t pid, int cpuid)
{
	int ret;
	if(cpuid == -1){
		return 0;
	}
    cpu_set_t cs;

    CPU_ZERO(&cs);
    CPU_SET(cpuid, &cs);

    ret =  pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cs);
	
	return ret;
}

void signal_func(int signal)
{
	logging(LOG_INFO,"exit signal id [%d].\n", signal);
	set_monitor_run_signal(NEWIUP_FAIL);
	set_upload_run_signal(NEWIUP_FAIL);
	//exit(1);
}
void signal_init()
{
    signal(SIGINT, signal_func);
    signal(SIGUSR1, set_monitor_sche_debug_switch);
#if 0
    signal(SIGTERM, signal_func);
    signal(SIGHUP, signal_func);
    signal(SIGQUIT, signal_func);
    signal(SIGILL, signal_func);
    signal(SIGTRAP, signal_func);
    signal(SIGBUS, signal_func);
    signal(SIGSEGV, signal_func);
    signal(SIGIOT, signal_func);
    signal(SIGFPE, signal_func);
    //signal(SIGKILL, signal_func);
    signal(SIGPIPE, signal_func);
    //signal(SIGSTOP, signal_func);
    signal(SIGTTOU, signal_func);
    signal(SIGTTIN, signal_func);
    signal(SIGXFSZ, signal_func);
    signal(SIGPROF, signal_func);
    signal(SIGVTALRM, signal_func);
#endif
}
int cfg_init()
{
	int ret;
	ret = global_cfg_init();
	if(ret != NEWIUP_OK)
	{
		return NEWIUP_FAIL;
	}
	ret = getDBConfig();
	if(ret != NEWIUP_OK)
	{
		return NEWIUP_FAIL;
	}
    ret = servers_init();
	if(ret != NEWIUP_OK)
	{
		return NEWIUP_FAIL;
	}
	ret = monitor_group_init();
	if(ret != NEWIUP_OK)
	{
		return NEWIUP_FAIL;
	}
	return NEWIUP_OK;
}

void newiup_pthread_start()
{
	monitor_handle_pthread_start();
}
void newiup_pthread_end()
{
	monitor_handle_pthread_end();
}

void newiup_exit()
{
    servers_exit();
    monitor_group_exit();
    logging_exit();
}

int main(int argc, char **argv)
{
	int ret = 0;

	if( argc != 2 ) 
    {
		log_printf(LOG_ERROR, "Usage: newiup config-file\n");
        exit(15);  
	}


	ConfInit();
	if (ConfYamlLoadFile(argv[1]) != 0) 
    {
		log_printf(LOG_ERROR, "Invalid format of configuration files, please check\n");
		log_printf(LOG_ERROR, "NOTE:¡¡configuration file Don't make the 'TAB' and the '_'\n");
		return NEWIUP_FAIL;
	}
	//show_config_info();

	ret = cfg_init();
	if(ret != NEWIUP_OK)
	{
		goto init_fail;
	}
	
	if (ThreadSetCPUAffinity(pthread_self(), global_config.affinity) != 0){
		logging(LOG_ERROR,"could not set CPU affinity to %d\n", global_config.affinity);
		goto init_fail;
	}
	logging(LOG_INFO,"set CPU affinity to %d\n", global_config.affinity);

	signal_init();


	newiup_pthread_start();	
	newiup_pthread_end();

init_fail:
	newiup_exit();
	return 0;
} 
