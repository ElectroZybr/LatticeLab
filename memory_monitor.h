#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <psapi.h>
#elif defined(__APPLE__) && defined(__MACH__)
    #include <mach/mach.h>
    #include <unistd.h>
    #include <sys/resource.h>
    #include <cstdio>
#elif defined(__unix__) || defined(__unix) || defined(unix)
    #include <unistd.h>
    #include <sys/resource.h>
    #include <cstdio>
#endif

class MemoryMonitor {
public:
    /**
     * Возвращает объем занимаемой физической памяти в байтах.
     * Возвращает 0, если определить не удалось.
     */
    static size_t getRSS() {
#if defined(_WIN32) || defined(_WIN64)
        // Windows реализация
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            return static_cast<size_t>(pmc.WorkingSetSize);
        }
#elif defined(__APPLE__) && defined(__MACH__)
        // macOS реализация (через task_info для большей точности)
        struct mach_task_basic_info info;
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
            return static_cast<size_t>(info.resident_size);
        }
#elif defined(__linux__) || defined(__unix__)
        // Linux/Unix реализация через /proc/self/statm (наиболее точная для RSS)
        long pages = 0;
        long rss = 0;
        FILE* fp = fopen("/proc/self/statm", "r");
        if (fp) {
            if (fscanf(fp, "%ld %ld", &pages, &rss) == 2) {
                fclose(fp);
                return static_cast<size_t>(rss) * static_cast<size_t>(sysconf(_SC_PAGESIZE));
            }
            fclose(fp);
        }
#endif
        return 0;
    }
};