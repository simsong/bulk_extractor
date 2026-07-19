#ifndef MACHINE_STATS_H
#define MACHINE_STATS_H

#ifndef BE20_CONFIGURE_APPLIED
#error config.h with be20_api additions must be included before machine_stats.h
#endif

#ifdef HAVE_MACH_MACH_H
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <mach/message.h>  // for mach_msg_type_number_t
#include <mach/kern_return.h>  // for kern_return_t
#include <mach/task_info.h>
#endif

#ifdef HAVE_SYS_VMMETER_H
#include <sys/vmmeter.h>
#endif

#include <cmath>
#include <unistd.h>

/**
 * return the CPU percentage (0-100) used by the current process. Use 'ps -O %cpu <pid> if system call not available.
 * The popen implementation is not meant to be efficient.
 */
struct machine_stats {
    static float get_cpu_percentage() {
        char buf[100];
        snprintf(buf,sizeof(buf),"ps -O %ccpu %d",'%',getpid());
        FILE *f = popen(buf,"r");
        if(f==nullptr){
            perror("popen failed\n");
            return(0);
        }
        if (fgets(buf,sizeof(buf),f)==NULL) return nan("error1");           /* read the first line */
        if (fgets(buf,sizeof(buf),f)==NULL) return nan("error2");           /* read the second line */
        pclose(f);
        buf[sizeof(buf)-1] = 0;             // in case it needs termination
        int pid=0;
        float ff = 0;
        int count = sscanf(buf,"%d %f",&pid,&ff);
        return (count==2) ? ff : nan("get_cpu_percentage");
    };

    static uint64_t get_available_memory() {
        // If there is a /proc/meminfo, use it
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.substr(0,13)=="MemAvailable:") {
                    return std::stoll(line.substr(14))*1024;
                }
            }
        }

#ifdef HAVE_HOST_STATISTICS64
        // on macs, use this
        // https://opensource.apple.com/source/system_cmds/system_cmds-496/vm_stat.tproj/vm_stat.c.auto.html

        vm_statistics64_data_t	vm_stat;
        vm_size_t pageSize = 4096; 	/* Default */
        mach_port_t myHost = mach_host_self();
        if (host_page_size(myHost, &pageSize) != KERN_SUCCESS) {
            pageSize = 4096;                // put the default back
        }
        vm_statistics64_t stat = &vm_stat;

        unsigned int count = HOST_VM_INFO64_COUNT;
        if (host_statistics64(myHost, HOST_VM_INFO64, (host_info64_t)stat, &count) != KERN_SUCCESS) {
            return 0;
        }
        return stat->free_count * pageSize;
#else
	return 0;
#endif
    };

    static void get_memory(uint64_t *virtual_size, uint64_t *resident_size) {
        *virtual_size = 0;
        *resident_size = 0;

#ifdef HAVE_TASK_INFO
        kern_return_t error;
        mach_msg_type_number_t outCount;
        mach_task_basic_info_data_t taskinfo;

        taskinfo.virtual_size = 0;
        outCount = MACH_TASK_BASIC_INFO_COUNT;
        error = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&taskinfo, &outCount);
        if (error == KERN_SUCCESS) {
            *virtual_size = (uint64_t)taskinfo.virtual_size;
            *resident_size = (uint64_t)taskinfo.resident_size;
            return;
        }
#endif
	const char* statm_path = "/proc/self/statm";

	FILE *f = fopen(statm_path,"r");
	if(f){
	    unsigned long size, resident, share, text, lib, data, dt;
	    if(fscanf(f,"%ld %ld %ld %ld %ld %ld %ld", &size,&resident,&share,&text,&lib,&data,&dt) == 7){
		*virtual_size  = size * 4096;
		*resident_size = resident * 4096;
                fclose(f);
		return ;
	    }
	}
	fclose(f);
	return ;
    };
};


#endif
