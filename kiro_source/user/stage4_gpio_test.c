/*
 * DS532驱动 Stage 4 GPIO中断测试程序
 * 
 * 测试内容：
 * 1. GPIO中断功能
 * 2. sysfs接口访问
 * 3. 触摸事件计数
 * 4. 触摸时间戳
 * 
 * 编译：gcc -o stage4_gpio_test stage4_gpio_test.c
 * 运行：./stage4_gpio_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>

/* sysfs路径 */
#define SYSFS_TOUCH_COUNT       "/sys/class/ds532/ds532_fp/touch_count"
#define SYSFS_LAST_TOUCH_TIME   "/sys/class/ds532/ds532_fp/last_touch_time"
#define DEVICE_PATH             "/dev/ds532_fp"

/* 读取sysfs属性 */
int read_sysfs_value(const char *path, char *buffer, size_t size)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    ssize_t len = read(fd, buffer, size - 1);
    close(fd);
    
    if (len > 0) {
        buffer[len] = '\0';
        /* 移除换行符 */
        if (buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        return 0;
    }
    
    return -1;
}

/* 获取当前时间戳（毫秒） */
long long get_timestamp_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

/* 测试sysfs接口 */
int test_sysfs_interface()
{
    printf("\n📋 测试1: sysfs接口访问\n");
    
    char buffer[64];
    
    /* 测试touch_count */
    if (read_sysfs_value(SYSFS_TOUCH_COUNT, buffer, sizeof(buffer)) == 0) {
        printf("✅ touch_count: %s\n", buffer);
    } else {
        printf("❌ 无法读取 touch_count: %s\n", strerror(errno));
        return -1;
    }
    
    /* 测试last_touch_time */
    if (read_sysfs_value(SYSFS_LAST_TOUCH_TIME, buffer, sizeof(buffer)) == 0) {
        printf("✅ last_touch_time: %s ns\n", buffer);
    } else {
        printf("❌ 无法读取 last_touch_time: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

/* 监控触摸事件 */
int test_touch_monitoring()
{
    printf("\n📋 测试2: 触摸事件监控\n");
    printf("💡 请在接下来的30秒内触摸指纹传感器...\n");
    
    char count_buffer[64];
    char time_buffer[64];
    unsigned long last_count = 0;
    long long last_time = 0;
    int events_detected = 0;
    
    /* 获取初始值 */
    if (read_sysfs_value(SYSFS_TOUCH_COUNT, count_buffer, sizeof(count_buffer)) == 0) {
        last_count = strtoul(count_buffer, NULL, 10);
        printf("初始触摸计数: %lu\n", last_count);
    }
    
    if (read_sysfs_value(SYSFS_LAST_TOUCH_TIME, time_buffer, sizeof(time_buffer)) == 0) {
        last_time = strtoll(time_buffer, NULL, 10);
        printf("初始触摸时间: %lld ns\n", last_time);
    }
    
    printf("\n开始监控...\n");
    
    /* 监控30秒 */
    for (int i = 0; i < 30; i++) {
        sleep(1);
        
        unsigned long current_count = 0;
        long long current_time = 0;
        
        /* 读取当前值 */
        if (read_sysfs_value(SYSFS_TOUCH_COUNT, count_buffer, sizeof(count_buffer)) == 0) {
            current_count = strtoul(count_buffer, NULL, 10);
        }
        
        if (read_sysfs_value(SYSFS_LAST_TOUCH_TIME, time_buffer, sizeof(time_buffer)) == 0) {
            current_time = strtoll(time_buffer, NULL, 10);
        }
        
        /* 检查是否有新的触摸事件 */
        if (current_count > last_count) {
            events_detected++;
            printf("🔔 检测到触摸事件 #%d: count=%lu, time=%lld ns\n", 
                   events_detected, current_count, current_time);
            last_count = current_count;
            last_time = current_time;
        }
        
        /* 显示进度 */
        if (i % 5 == 4) {
            printf("⏰ 监控中... %d/30 秒\n", i + 1);
        }
    }
    
    printf("\n监控结束\n");
    printf("总共检测到 %d 个触摸事件\n", events_detected);
    
    if (events_detected > 0) {
        printf("✅ GPIO中断功能正常\n");
        return 0;
    } else {
        printf("⚠️  未检测到触摸事件\n");
        printf("💡 可能的原因：\n");
        printf("   1. GPIO中断未正确配置\n");
        printf("   2. 硬件连接问题\n");
        printf("   3. 指纹传感器未正确触摸\n");
        return -1;
    }
}

/* 测试触摸响应时间 */
int test_touch_response_time()
{
    printf("\n📋 测试3: 触摸响应时间测试\n");
    printf("💡 请快速触摸指纹传感器5次...\n");
    
    char count_buffer[64];
    char time_buffer[64];
    unsigned long last_count = 0;
    long long touch_times[10];
    int touch_count = 0;
    
    /* 获取初始计数 */
    if (read_sysfs_value(SYSFS_TOUCH_COUNT, count_buffer, sizeof(count_buffer)) == 0) {
        last_count = strtoul(count_buffer, NULL, 10);
    }
    
    printf("等待触摸事件...\n");
    
    /* 等待5个触摸事件或30秒超时 */
    for (int i = 0; i < 300 && touch_count < 5; i++) {  /* 30秒，每100ms检查一次 */
        usleep(100000);  /* 100ms */
        
        unsigned long current_count = 0;
        long long current_time = 0;
        
        if (read_sysfs_value(SYSFS_TOUCH_COUNT, count_buffer, sizeof(count_buffer)) == 0) {
            current_count = strtoul(count_buffer, NULL, 10);
        }
        
        if (read_sysfs_value(SYSFS_LAST_TOUCH_TIME, time_buffer, sizeof(time_buffer)) == 0) {
            current_time = strtoll(time_buffer, NULL, 10);
        }
        
        if (current_count > last_count) {
            touch_times[touch_count] = current_time;
            touch_count++;
            printf("触摸 #%d: time=%lld ns\n", touch_count, current_time);
            last_count = current_count;
        }
    }
    
    if (touch_count >= 2) {
        printf("\n📊 响应时间分析：\n");
        for (int i = 1; i < touch_count; i++) {
            long long interval = touch_times[i] - touch_times[i-1];
            double interval_ms = interval / 1000000.0;  /* 转换为毫秒 */
            printf("触摸间隔 %d-%d: %.2f ms\n", i, i+1, interval_ms);
        }
        printf("✅ 响应时间测试完成\n");
        return 0;
    } else {
        printf("❌ 触摸事件不足，无法分析响应时间\n");
        return -1;
    }
}

/* 测试设备打开时的GPIO状态 */
int test_gpio_with_device()
{
    printf("\n📋 测试4: 设备打开时的GPIO状态\n");
    
    /* 记录打开前的状态 */
    char count_before[64], time_before[64];
    read_sysfs_value(SYSFS_TOUCH_COUNT, count_before, sizeof(count_before));
    read_sysfs_value(SYSFS_LAST_TOUCH_TIME, time_before, sizeof(time_before));
    
    printf("设备打开前: count=%s, time=%s\n", count_before, time_before);
    
    /* 打开设备 */
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("❌ 无法打开设备: %s\n", strerror(errno));
        return -1;
    }
    printf("✅ 设备打开成功\n");
    
    /* 等待一段时间，看是否有触摸事件 */
    printf("💡 设备打开状态下，请触摸传感器...\n");
    sleep(10);
    
    /* 记录打开后的状态 */
    char count_after[64], time_after[64];
    read_sysfs_value(SYSFS_TOUCH_COUNT, count_after, sizeof(count_after));
    read_sysfs_value(SYSFS_LAST_TOUCH_TIME, time_after, sizeof(time_after));
    
    printf("设备打开后: count=%s, time=%s\n", count_after, time_after);
    
    /* 关闭设备 */
    close(fd);
    printf("✅ 设备关闭\n");
    
    /* 比较前后状态 */
    unsigned long count_diff = strtoul(count_after, NULL, 10) - strtoul(count_before, NULL, 10);
    if (count_diff > 0) {
        printf("✅ 检测到 %lu 个新的触摸事件\n", count_diff);
        return 0;
    } else {
        printf("⚠️  未检测到新的触摸事件\n");
        return -1;
    }
}

int main()
{
    int total_errors = 0;
    
    printf("=================================================\n");
    printf("    DS532驱动 Stage 4 GPIO中断测试程序\n");
    printf("=================================================\n");
    
    printf("📋 测试前检查：\n");
    printf("   1. 确保驱动已加载: insmod ds532_driver.ko\n");
    printf("   2. 确保设备节点存在: ls -l %s\n", DEVICE_PATH);
    printf("   3. 确保sysfs接口存在:\n");
    printf("      ls -l %s\n", SYSFS_TOUCH_COUNT);
    printf("      ls -l %s\n", SYSFS_LAST_TOUCH_TIME);
    printf("   4. 确保GPIO连接正常: 触摸引脚连接到GPIO4_IO21\n");
    printf("   5. 确保指纹传感器工作正常\n");
    printf("\n按回车键继续...\n");
    getchar();
    
    /* 执行测试 */
    if (test_sysfs_interface() != 0) total_errors++;
    if (test_touch_monitoring() != 0) total_errors++;
    if (test_touch_response_time() != 0) total_errors++;
    if (test_gpio_with_device() != 0) total_errors++;
    
    /* 测试结果汇总 */
    printf("\n=================================================\n");
    printf("                  测试结果汇总\n");
    printf("=================================================\n");
    
    if (total_errors == 0) {
        printf("🎉 所有测试通过！Stage 4 GPIO中断功能正常\n");
        printf("\n✅ 已验证功能：\n");
        printf("   - sysfs接口访问\n");
        printf("   - 触摸事件检测\n");
        printf("   - 触摸计数统计\n");
        printf("   - 触摸时间戳记录\n");
        printf("   - GPIO中断响应\n");
        printf("   - 设备状态下的GPIO功能\n");
        
        printf("\n🚀 Stage 4 测试完成，可以进行 Stage 5 开发\n");
        return 0;
    } else {
        printf("💥 测试失败！发现 %d 个错误\n", total_errors);
        printf("\n🔧 故障排除建议：\n");
        printf("   1. 检查GPIO引脚配置\n");
        printf("   2. 检查设备树中的GPIO设置\n");
        printf("   3. 检查中断注册是否成功\n");
        printf("   4. 检查硬件连接\n");
        printf("   5. 查看内核日志: dmesg | grep DS532\n");
        printf("   6. 检查中断统计: cat /proc/interrupts | grep ds532\n");
        return 1;
    }
}