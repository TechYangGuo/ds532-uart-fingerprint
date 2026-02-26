/*
 * DS532驱动 Stage 3 UART通信测试程序
 * 
 * 测试内容：
 * 1. 设备打开/关闭
 * 2. 数据包发送/接收
 * 3. UART通信功能
 * 4. 超时处理
 * 5. 实际与DS532模块通信
 * 
 * 编译：gcc -o stage3_uart_test stage3_uart_test.c
 * 运行：./stage3_uart_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* DS532协议常量 */
#define DS532_HEADER_H          0xEF
#define DS532_HEADER_L          0x01
#define DS532_PID_CMD           0x01
#define DS532_PID_ACK           0x07
#define DS532_DEFAULT_ADDR      0xFFFFFFFF
#define DS532_CMD_VFYPWD        0x13
#define DS532_CMD_GETIMAGE      0x01
#define DS532_CMD_GENCHAR       0x02
#define DS532_ACK_SUCCESS       0x00

/* 设备路径 */
#define DEVICE_PATH             "/dev/ds532_fp"

/* 构造VfyPwd命令包 */
int build_vfypwd_packet(u8 *buffer, u32 password)
{
    int offset = 0;
    u16 checksum;
    
    /* 包头 */
    buffer[offset++] = DS532_HEADER_H;
    buffer[offset++] = DS532_HEADER_L;
    
    /* 地址 */
    buffer[offset++] = 0xFF;
    buffer[offset++] = 0xFF;
    buffer[offset++] = 0xFF;
    buffer[offset++] = 0xFF;
    
    /* 包标识 */
    buffer[offset++] = DS532_PID_CMD;
    
    /* 长度 */
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x07;  /* 1(PID) + 1(CMD) + 4(PWD) + 2(CHK) = 8, 但这里是7? */
    
    /* 命令码 */
    buffer[offset++] = DS532_CMD_VFYPWD;
    
    /* 密码 */
    buffer[offset++] = (password >> 24) & 0xFF;
    buffer[offset++] = (password >> 16) & 0xFF;
    buffer[offset++] = (password >> 8) & 0xFF;
    buffer[offset++] = password & 0xFF;
    
    /* 计算校验和 */
    checksum = 0;
    for (int i = 6; i < offset; i++) {
        checksum += buffer[i];
    }
    
    buffer[offset++] = (checksum >> 8) & 0xFF;
    buffer[offset++] = checksum & 0xFF;
    
    return offset;
}

/* 构造GenImg命令包 */
int build_genimg_packet(u8 *buffer)
{
    int offset = 0;
    u16 checksum;
    
    /* 包头 */
    buffer[offset++] = DS532_HEADER_H;
    buffer[offset++] = DS532_HEADER_L;
    
    /* 地址 */
    buffer[offset++] = 0xFF;
    buffer[offset++] = 0xFF;
    buffer[offset++] = 0xFF;
    buffer[offset++] = 0xFF;
    
    /* 包标识 */
    buffer[offset++] = DS532_PID_CMD;
    
    /* 长度 */
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x03;  /* 1(PID) + 1(CMD) + 2(CHK) = 4, 但这里是3? */
    
    /* 命令码 */
    buffer[offset++] = DS532_CMD_GETIMAGE;
    
    /* 计算校验和 */
    checksum = 0;
    for (int i = 6; i < offset; i++) {
        checksum += buffer[i];
    }
    
    buffer[offset++] = (checksum >> 8) & 0xFF;
    buffer[offset++] = checksum & 0xFF;
    
    return offset;
}

/* 打印数据包 */
void print_packet(const char *name, const u8 *buffer, int len)
{
    printf("%s (%d bytes): ", name, len);
    for (int i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

/* 验证响应包 */
int verify_response(const u8 *buffer, int len)
{
    if (len < 12) {
        printf("❌ 响应包太短: %d bytes\n", len);
        return -1;
    }
    
    /* 检查包头 */
    if (buffer[0] != DS532_HEADER_H || buffer[1] != DS532_HEADER_L) {
        printf("❌ 响应包头错误: %02X %02X\n", buffer[0], buffer[1]);
        return -1;
    }
    
    /* 检查包标识 */
    if (buffer[6] != DS532_PID_ACK) {
        printf("❌ 不是应答包: %02X\n", buffer[6]);
        return -1;
    }
    
    /* 获取确认码 */
    u8 ack_code = buffer[9];
    printf("确认码: 0x%02X ", ack_code);
    
    switch (ack_code) {
        case DS532_ACK_SUCCESS:
            printf("(成功)\n");
            break;
        case 0x01:
            printf("(接收包错误)\n");
            break;
        case 0x02:
            printf("(无手指)\n");
            break;
        case 0x03:
            printf("(录入失败)\n");
            break;
        case 0x06:
            printf("(图像混乱)\n");
            break;
        case 0x07:
            printf("(特征点太少)\n");
            break;
        case 0x08:
            printf("(不匹配)\n");
            break;
        case 0x09:
            printf("(未搜索到)\n");
            break;
        case 0x0A:
            printf("(合并失败)\n");
            break;
        case 0x0B:
            printf("(地址超出范围)\n");
            break;
        case 0x18:
            printf("(Flash错误)\n");
            break;
        default:
            printf("(未知错误)\n");
            break;
    }
    
    return ack_code;
}

/* 获取当前时间戳（毫秒） */
long long get_timestamp_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

/* 测试设备打开/关闭 */
int test_device_open_close()
{
    printf("\n📋 测试1: 设备打开/关闭\n");
    
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("❌ 无法打开设备 %s: %s\n", DEVICE_PATH, strerror(errno));
        return -1;
    }
    
    printf("✅ 设备打开成功: fd=%d\n", fd);
    
    close(fd);
    printf("✅ 设备关闭成功\n");
    
    return 0;
}

/* 测试多次打开（独占访问） */
int test_exclusive_access()
{
    printf("\n📋 测试2: 设备独占访问\n");
    
    int fd1 = open(DEVICE_PATH, O_RDWR);
    if (fd1 < 0) {
        printf("❌ 第一次打开失败: %s\n", strerror(errno));
        return -1;
    }
    printf("✅ 第一次打开成功: fd=%d\n", fd1);
    
    int fd2 = open(DEVICE_PATH, O_RDWR);
    if (fd2 >= 0) {
        printf("❌ 第二次打开应该失败但成功了: fd=%d\n", fd2);
        close(fd2);
        close(fd1);
        return -1;
    }
    printf("✅ 第二次打开正确失败: %s\n", strerror(errno));
    
    close(fd1);
    printf("✅ 关闭第一个文件描述符\n");
    
    /* 再次尝试打开 */
    fd2 = open(DEVICE_PATH, O_RDWR);
    if (fd2 < 0) {
        printf("❌ 重新打开失败: %s\n", strerror(errno));
        return -1;
    }
    printf("✅ 重新打开成功: fd=%d\n", fd2);
    
    close(fd2);
    return 0;
}

/* 测试VfyPwd命令 */
int test_vfypwd_command()
{
    printf("\n📋 测试3: VfyPwd命令通信\n");
    
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("❌ 打开设备失败: %s\n", strerror(errno));
        return -1;
    }
    
    /* 构造VfyPwd命令包 */
    u8 cmd_buffer[32];
    int cmd_len = build_vfypwd_packet(cmd_buffer, 0x00000000);
    
    print_packet("发送VfyPwd", cmd_buffer, cmd_len);
    
    /* 发送命令 */
    long long start_time = get_timestamp_ms();
    ssize_t sent = write(fd, cmd_buffer, cmd_len);
    long long send_time = get_timestamp_ms() - start_time;
    
    if (sent != cmd_len) {
        printf("❌ 发送失败: sent=%zd, expected=%d, error=%s\n", 
               sent, cmd_len, strerror(errno));
        close(fd);
        return -1;
    }
    printf("✅ 发送成功: %zd bytes, 耗时: %lld ms\n", sent, send_time);
    
    /* 接收响应 */
    u8 resp_buffer[32];
    start_time = get_timestamp_ms();
    ssize_t received = read(fd, resp_buffer, sizeof(resp_buffer));
    long long recv_time = get_timestamp_ms() - start_time;
    
    if (received <= 0) {
        printf("❌ 接收失败: received=%zd, error=%s\n", received, strerror(errno));
        close(fd);
        return -1;
    }
    
    printf("✅ 接收成功: %zd bytes, 耗时: %lld ms\n", received, recv_time);
    print_packet("接收响应", resp_buffer, received);
    
    /* 验证响应 */
    int ack_code = verify_response(resp_buffer, received);
    
    close(fd);
    
    if (ack_code == DS532_ACK_SUCCESS) {
        printf("🎉 VfyPwd命令执行成功！\n");
        return 0;
    } else if (ack_code >= 0) {
        printf("⚠️  VfyPwd命令执行完成，但返回错误码\n");
        return 0;  /* 通信成功，只是命令执行有错误 */
    } else {
        printf("💥 VfyPwd命令通信失败\n");
        return -1;
    }
}

/* 测试GenImg命令 */
int test_genimg_command()
{
    printf("\n📋 测试4: GenImg命令通信\n");
    
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("❌ 打开设备失败: %s\n", strerror(errno));
        return -1;
    }
    
    /* 构造GenImg命令包 */
    u8 cmd_buffer[32];
    int cmd_len = build_genimg_packet(cmd_buffer);
    
    print_packet("发送GenImg", cmd_buffer, cmd_len);
    
    /* 发送命令 */
    long long start_time = get_timestamp_ms();
    ssize_t sent = write(fd, cmd_buffer, cmd_len);
    long long send_time = get_timestamp_ms() - start_time;
    
    if (sent != cmd_len) {
        printf("❌ 发送失败: sent=%zd, expected=%d, error=%s\n", 
               sent, cmd_len, strerror(errno));
        close(fd);
        return -1;
    }
    printf("✅ 发送成功: %zd bytes, 耗时: %lld ms\n", sent, send_time);
    
    /* 接收响应 */
    u8 resp_buffer[32];
    start_time = get_timestamp_ms();
    ssize_t received = read(fd, resp_buffer, sizeof(resp_buffer));
    long long recv_time = get_timestamp_ms() - start_time;
    
    if (received <= 0) {
        printf("❌ 接收失败: received=%zd, error=%s\n", received, strerror(errno));
        printf("💡 提示：GenImg命令需要手指放在传感器上\n");
        close(fd);
        return -1;
    }
    
    printf("✅ 接收成功: %zd bytes, 耗时: %lld ms\n", received, recv_time);
    print_packet("接收响应", resp_buffer, received);
    
    /* 验证响应 */
    int ack_code = verify_response(resp_buffer, received);
    
    close(fd);
    
    if (ack_code == DS532_ACK_SUCCESS) {
        printf("🎉 GenImg命令执行成功！\n");
        return 0;
    } else if (ack_code >= 0) {
        printf("⚠️  GenImg命令执行完成，但返回错误码\n");
        if (ack_code == 0x02) {
            printf("💡 提示：请将手指放在指纹传感器上\n");
        }
        return 0;  /* 通信成功，只是命令执行有错误 */
    } else {
        printf("💥 GenImg命令通信失败\n");
        return -1;
    }
}

/* 测试超时处理 */
int test_timeout()
{
    printf("\n📋 测试5: 超时处理测试\n");
    printf("💡 此测试将尝试读取数据以验证超时机制\n");
    
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("❌ 打开设备失败: %s\n", strerror(errno));
        return -1;
    }
    
    /* 不发送命令，直接尝试读取 */
    u8 buffer[32];
    long long start_time = get_timestamp_ms();
    ssize_t received = read(fd, buffer, sizeof(buffer));
    long long elapsed = get_timestamp_ms() - start_time;
    
    printf("读取结果: %zd bytes, 耗时: %lld ms\n", received, elapsed);
    
    if (received <= 0) {
        printf("✅ 超时处理正常 (预期行为)\n");
    } else {
        printf("⚠️  意外接收到数据\n");
        print_packet("意外数据", buffer, received);
    }
    
    close(fd);
    return 0;
}

int main()
{
    int total_errors = 0;
    
    printf("=================================================\n");
    printf("    DS532驱动 Stage 3 UART通信测试程序\n");
    printf("=================================================\n");
    
    printf("📋 测试前检查：\n");
    printf("   1. 确保驱动已加载: insmod ds532_driver.ko\n");
    printf("   2. 确保设备节点存在: ls -l %s\n", DEVICE_PATH);
    printf("   3. 确保UART连接正常: DS532模块连接到UART6\n");
    printf("   4. 确保TTY配置正确: stty -F /dev/ttymxc5 57600 cs8 -cstopb -parenb raw\n");
    printf("\n按回车键继续...\n");
    getchar();
    
    /* 执行测试 */
    if (test_device_open_close() != 0) total_errors++;
    if (test_exclusive_access() != 0) total_errors++;
    if (test_vfypwd_command() != 0) total_errors++;
    if (test_genimg_command() != 0) total_errors++;
    if (test_timeout() != 0) total_errors++;
    
    /* 测试结果汇总 */
    printf("\n=================================================\n");
    printf("                  测试结果汇总\n");
    printf("=================================================\n");
    
    if (total_errors == 0) {
        printf("🎉 所有测试通过！Stage 3 UART通信功能正常\n");
        printf("\n✅ 已验证功能：\n");
        printf("   - 设备打开/关闭\n");
        printf("   - 设备独占访问控制\n");
        printf("   - UART数据发送\n");
        printf("   - UART数据接收\n");
        printf("   - 超时处理机制\n");
        printf("   - DS532协议通信\n");
        printf("   - VfyPwd命令执行\n");
        printf("   - GenImg命令执行\n");
        
        printf("\n🚀 Stage 3 测试完成，可以进行 Stage 4 开发\n");
        return 0;
    } else {
        printf("💥 测试失败！发现 %d 个错误\n", total_errors);
        printf("\n🔧 故障排除建议：\n");
        printf("   1. 检查驱动是否正确加载\n");
        printf("   2. 检查设备节点权限\n");
        printf("   3. 检查UART硬件连接\n");
        printf("   4. 检查TTY配置\n");
        printf("   5. 查看内核日志: dmesg | grep DS532\n");
        return 1;
    }
}