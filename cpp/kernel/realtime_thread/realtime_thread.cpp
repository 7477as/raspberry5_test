// 解决宏重复定义的警告
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#define INTERVAL_NS 125000   // 目标间隔：125微秒
#define TEST_ITERATIONS (100 * 8000) // 测试次数：8000次 = 1秒
#define TARGET_CORE 3        // 目标绑定的 CPU 核心 (0, 1, 2, 或 3)

// 获取当前系统高精度时间（纳秒）
static inline uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// 核心优化设置：绑定核心、最高实时优先级、锁定内存
void setup_environment() {
    // 1. 绑定 CPU 核心 (CPU Affinity)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);               // 清空集合
    CPU_SET(TARGET_CORE, &cpuset);   // 把目标核心加入集合

    // 0 表示设置当前线程的亲和性
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
        perror("sched_setaffinity 失败 (无法绑定核心)");
        exit(EXIT_FAILURE);
    }
    printf("[-] 成功将线程强制绑定到 CPU 核心 %d\n", TARGET_CORE);

    // 2. 设置实时优先级 (SCHED_FIFO)
    struct sched_param param;
    // C++ 中通常需要清零结构体，虽然对 sched_param 来说只需设 priority
    memset(&param, 0, sizeof(param)); 
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler 失败 (请确保使用 sudo 运行)");
        exit(EXIT_FAILURE);
    }
    printf("[-] 成功开启 SCHED_FIFO 实时调度，最高优先级\n");

    // 3. 锁定内存 (防止 Page Fault 导致延迟)
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("mlockall 失败");
        exit(EXIT_FAILURE);
    }
    printf("[-] 成功锁定内存，防止 Swap 交换\n");
}

int main() {
    // 初始化环境
    setup_environment();

    // 解决 C++ malloc void* 无法隐式转换的错误
    // 方式一：显式类型转换 (int64_t *)
    // 方式二：使用 C++ 的 new 关键字（这里我们使用显式转换保持 C/C++ 通用）
    int64_t *jitter_array = (int64_t *)malloc(TEST_ITERATIONS * sizeof(int64_t));
    if (!jitter_array) {
        perror("内存分配失败");
        return -1;
    }

    printf("\n开始测试，目标间隔: %d ns (125 us)...\n", INTERVAL_NS);

    uint64_t next_wake_time = get_time_ns();
    
    // 预热 CPU 缓存
    for(int i=0; i<100; i++) {
        get_time_ns();
    }

    // 主测试循环
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        next_wake_time += INTERVAL_NS;

        // 【忙等待 (Busy Wait)】
        uint64_t current_time;
        do {
            current_time = get_time_ns();
        } while (current_time < next_wake_time);

        // ---------------------------------
        // 在这里执行你的 125us 任务
        // ---------------------------------

        // 记录误差 (实际时间 - 目标时间)
        jitter_array[i] = current_time - next_wake_time;
    }

    // 统计数据
    int64_t max_jitter = 0;
    int64_t min_jitter = 1000000;
    int64_t sum_jitter = 0;

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        int64_t jitter = jitter_array[i];
        if (jitter > max_jitter) max_jitter = jitter;
        if (jitter < min_jitter) min_jitter = jitter;
        sum_jitter += jitter;
    }

    double avg_jitter = (double)sum_jitter / TEST_ITERATIONS;

    printf("\n--- 125μs 循环测试结果 (共 %d 次) ---\n", TEST_ITERATIONS);
    printf("最大误差 (Max Jitter): %lld 纳秒 (%.2f 微秒)\n", (long long)max_jitter, max_jitter / 1000.0);
    printf("最小误差 (Min Jitter): %lld 纳秒 (%.2f 微秒)\n", (long long)min_jitter, min_jitter / 1000.0);
    printf("平均误差 (Avg Jitter): %.2f 纳秒 (%.4f 微秒)\n", avg_jitter, avg_jitter / 1000.0);
    
    free(jitter_array);
    return 0;
}
