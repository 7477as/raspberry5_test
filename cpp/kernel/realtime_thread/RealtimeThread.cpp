#include "RealtimeThread.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

namespace {
constexpr int kWarmupCalls = 100;
}  // namespace

uint64_t RealtimeThread::GetTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

void RealtimeThread::SetupEnvironment(int targetCore) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(targetCore, &cpuset);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
        perror("sched_setaffinity 失败 (无法绑定核心)");
        std::exit(EXIT_FAILURE);
    }
    std::printf("[-] 成功将线程强制绑定到 CPU 核心 %d\n", targetCore);

    struct sched_param param;
    std::memset(&param, 0, sizeof(param));
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler 失败 (请确保使用 sudo 运行)");
        std::exit(EXIT_FAILURE);
    }
    std::printf("[-] 成功开启 SCHED_FIFO 实时调度，最高优先级\n");

    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("mlockall 失败");
        std::exit(EXIT_FAILURE);
    }
    std::printf("[-] 成功锁定内存，防止 Swap 交换\n");
}

void RealtimeThread::LoopAndMeasure(int intervalNs, int iterations, int64_t* jitterArray) {
    uint64_t nextWakeTime = GetTimeNs();

    for (int i = 0; i < kWarmupCalls; i++) {
        (void) GetTimeNs();
    }

    for (int i = 0; i < iterations; i++) {
        nextWakeTime += intervalNs;

        uint64_t currentTime;
        do {
            currentTime = GetTimeNs();
        } while (currentTime < nextWakeTime);

        jitterArray[i] = static_cast<int64_t>(currentTime - nextWakeTime);
    }
}

void RealtimeThread::PrintStats(const int64_t* jitterArray, int iterations) const {
    int64_t maxJitter = 0;
    int64_t minJitter = 1000000;
    int64_t sumJitter = 0;

    for (int i = 0; i < iterations; i++) {
        const int64_t j = jitterArray[i];
        if (j > maxJitter) maxJitter = j;
        if (j < minJitter) minJitter = j;
        sumJitter += j;
    }
    const double avgJitter = static_cast<double>(sumJitter) / iterations;

    std::printf("\n--- 125μs 循环测试结果 (共 %d 次) ---\n", iterations);
    std::printf("最大误差 (Max Jitter): %lld 纳秒 (%.2f 微秒)\n",
                static_cast<long long>(maxJitter), maxJitter / 1000.0);
    std::printf("最小误差 (Min Jitter): %lld 纳秒 (%.2f 微秒)\n",
                static_cast<long long>(minJitter), minJitter / 1000.0);
    std::printf("平均误差 (Avg Jitter): %.2f 纳秒 (%.4f 微秒)\n",
                avgJitter, avgJitter / 1000.0);
}

int RealtimeThread::Run(const Config& cfg) {
    SetupEnvironment(cfg.targetCore);

    auto* jitterArray =
        static_cast<int64_t*>(std::malloc(static_cast<size_t>(cfg.testIterations) * sizeof(int64_t)));
    if (!jitterArray) {
        perror("内存分配失败");
        return -1;
    }

    std::printf("\n开始测试，目标间隔: %d ns (%d us)...\n",
                cfg.intervalNs, cfg.intervalNs / 1000);

    LoopAndMeasure(cfg.intervalNs, cfg.testIterations, jitterArray);
    PrintStats(jitterArray, cfg.testIterations);

    std::free(jitterArray);
    return 0;
}
