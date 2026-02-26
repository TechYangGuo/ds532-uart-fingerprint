/*
 * DS532驱动 Stage 2 协议封装测试程序
 * 
 * 测试内容：
 * 1. 协议数据包构造功能
 * 2. 校验和计算验证
 * 3. 数据包格式验证
 * 
 * 编译：gcc -o stage2_protocol_test stage2_protocol_test.c
 * 运行：./stage2_protocol_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* DS532协议常量 */
#define DS532_HEADER_H          0xEF
#define DS532_HEADER_L          0x01
#define DS532_PID_CMD           0x01
#define DS532_DEFAULT_ADDR      0xFFFFFFFF
#define DS532_CMD_VFYPWD        0x13
#define DS532_CMD_GETIMAGE      0x01
#define DS532_CMD_GENCHAR       0x02
#define DS532_CMD_SEARCH        0x04
#define DS532_BUFFER_SIZE       256

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* 协议封装函数（模拟驱动中的实现） */
u16 ds532_calculate_checksum(const u8 *data, size_t len)
{
    u16 sum = 0;
    size_t i;
    
    for (i = 0; i < len; i++)
        sum += data[i];
    
    return sum;
}

int ds532_build_packet(u8 *buffer, u8 pid, u8 cmd, const u8 *data, u16 data_len)
{
    u16 pkg_len;
    u16 checksum;
    int offset = 0;
    
    /* 包头：0xEF01 */
    buffer[offset++] = DS532_HEADER_H;
    buffer[offset++] = DS532_HEADER_L;
    
    /* 地址：0xFFFFFFFF */
    buffer[offset++] = (DS532_DEFAULT_ADDR >> 24) & 0xFF;
    buffer[offset++] = (DS532_DEFAULT_ADDR >> 16) & 0xFF;
    buffer[offset++] = (DS532_DEFAULT_ADDR >> 8) & 0xFF;
    buffer[offset++] = DS532_DEFAULT_ADDR & 0xFF;
    
    /* 包标识 */
    buffer[offset++] = pid;
    
    /* 长度 = 包标识(1) + 命令码(1) + 数据长度 + 校验和(2) */
    pkg_len = 1 + 1 + data_len + 2;
    buffer[offset++] = (pkg_len >> 8) & 0xFF;
    buffer[offset++] = pkg_len & 0xFF;
    
    /* 命令码 */
    buffer[offset++] = cmd;
    
    /* 数据内容 */
    if (data && data_len > 0) {
        memcpy(&buffer[offset], data, data_len);
        offset += data_len;
    }
    
    /* 计算校验和（从包标识开始到数据结束） */
    checksum = ds532_calculate_checksum(&buffer[6], offset - 6);
    buffer[offset++] = (checksum >> 8) & 0xFF;
    buffer[offset++] = checksum & 0xFF;
    
    return offset;
}

int ds532_build_vfypwd_packet(u8 *buffer, u32 password)
{
    u8 data[4];
    
    /* 密码数据（大端序） */
    data[0] = (password >> 24) & 0xFF;
    data[1] = (password >> 16) & 0xFF;
    data[2] = (password >> 8) & 0xFF;
    data[3] = password & 0xFF;
    
    return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_VFYPWD, data, 4);
}

int ds532_build_genimg_packet(u8 *buffer)
{
    return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_GETIMAGE, NULL, 0);
}

int ds532_build_img2tz_packet(u8 *buffer, u8 buffer_id)
{
    return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_GENCHAR, &buffer_id, 1);
}

int ds532_build_search_packet(u8 *buffer, u8 buffer_id, u16 start_page, u16 page_num)
{
    u8 data[5];
    
    data[0] = buffer_id;
    data[1] = (start_page >> 8) & 0xFF;
    data[2] = start_page & 0xFF;
    data[3] = (page_num >> 8) & 0xFF;
    data[4] = page_num & 0xFF;
    
    return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_SEARCH, data, 5);
}

/* 打印数据包内容 */
void print_packet(const char *name, const u8 *buffer, int len)
{
    int i;
    printf("%s (%d bytes): ", name, len);
    for (i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) printf("\n                    ");
    }
    printf("\n");
}

/* 验证数据包格式 */
int verify_packet_format(const u8 *buffer, int len, const char *name)
{
    int errors = 0;
    
    printf("\n=== 验证 %s ===\n", name);
    
    /* 检查最小长度 */
    if (len < 12) {
        printf("❌ 错误：数据包长度太短 (%d < 12)\n", len);
        return 1;
    }
    
    /* 检查包头 */
    if (buffer[0] != DS532_HEADER_H || buffer[1] != DS532_HEADER_L) {
        printf("❌ 错误：包头不正确 (0x%02X%02X != 0xEF01)\n", buffer[0], buffer[1]);
        errors++;
    } else {
        printf("✅ 包头正确: 0x%02X%02X\n", buffer[0], buffer[1]);
    }
    
    /* 检查地址 */
    u32 addr = (buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
    if (addr != DS532_DEFAULT_ADDR) {
        printf("❌ 错误：地址不正确 (0x%08X != 0x%08X)\n", addr, DS532_DEFAULT_ADDR);
        errors++;
    } else {
        printf("✅ 地址正确: 0x%08X\n", addr);
    }
    
    /* 检查包标识 */
    if (buffer[6] != DS532_PID_CMD) {
        printf("❌ 错误：包标识不正确 (0x%02X != 0x%02X)\n", buffer[6], DS532_PID_CMD);
        errors++;
    } else {
        printf("✅ 包标识正确: 0x%02X\n", buffer[6]);
    }
    
    /* 检查长度字段 */
    u16 pkg_len = (buffer[7] << 8) | buffer[8];
    u16 expected_len = len - 9;
    if (pkg_len != expected_len) {
        printf("❌ 错误：长度字段不正确 (%d != %d)\n", pkg_len, expected_len);
        errors++;
    } else {
        printf("✅ 长度字段正确: %d\n", pkg_len);
    }
    
    /* 检查校验和 */
    u16 recv_checksum = (buffer[len-2] << 8) | buffer[len-1];
    u16 calc_checksum = ds532_calculate_checksum(&buffer[6], len - 8);
    if (recv_checksum != calc_checksum) {
        printf("❌ 错误：校验和不正确 (0x%04X != 0x%04X)\n", recv_checksum, calc_checksum);
        errors++;
    } else {
        printf("✅ 校验和正确: 0x%04X\n", recv_checksum);
    }
    
    printf("命令码: 0x%02X\n", buffer[9]);
    
    if (errors == 0) {
        printf("🎉 %s 验证通过！\n", name);
    } else {
        printf("💥 %s 验证失败，发现 %d 个错误\n", name, errors);
    }
    
    return errors;
}

int main()
{
    u8 buffer[DS532_BUFFER_SIZE];
    int len;
    int total_errors = 0;
    
    printf("=================================================\n");
    printf("    DS532驱动 Stage 2 协议封装测试程序\n");
    printf("=================================================\n");
    
    /* 测试1: VfyPwd命令包 */
    printf("\n📋 测试1: 验证密码命令包构造\n");
    len = ds532_build_vfypwd_packet(buffer, 0x00000000);
    print_packet("VfyPwd包", buffer, len);
    total_errors += verify_packet_format(buffer, len, "VfyPwd命令包");
    
    /* 测试2: GenImg命令包 */
    printf("\n📋 测试2: 采集图像命令包构造\n");
    len = ds532_build_genimg_packet(buffer);
    print_packet("GenImg包", buffer, len);
    total_errors += verify_packet_format(buffer, len, "GenImg命令包");
    
    /* 测试3: Img2Tz命令包 */
    printf("\n📋 测试3: 生成特征命令包构造\n");
    len = ds532_build_img2tz_packet(buffer, 1);
    print_packet("Img2Tz包", buffer, len);
    total_errors += verify_packet_format(buffer, len, "Img2Tz命令包");
    
    /* 测试4: Search命令包 */
    printf("\n📋 测试4: 搜索指纹命令包构造\n");
    len = ds532_build_search_packet(buffer, 1, 0, 100);
    print_packet("Search包", buffer, len);
    total_errors += verify_packet_format(buffer, len, "Search命令包");
    
    /* 测试5: 校验和算法验证 */
    printf("\n📋 测试5: 校验和算法验证\n");
    u8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    u16 checksum = ds532_calculate_checksum(test_data, 5);
    u16 expected = 0x01 + 0x02 + 0x03 + 0x04 + 0x05;
    printf("测试数据: 01 02 03 04 05\n");
    printf("计算校验和: 0x%04X\n", checksum);
    printf("期望校验和: 0x%04X\n", expected);
    if (checksum == expected) {
        printf("✅ 校验和算法正确\n");
    } else {
        printf("❌ 校验和算法错误\n");
        total_errors++;
    }
    
    /* 测试结果汇总 */
    printf("\n=================================================\n");
    printf("                  测试结果汇总\n");
    printf("=================================================\n");
    
    if (total_errors == 0) {
        printf("🎉 所有测试通过！Stage 2 协议封装功能正常\n");
        printf("\n✅ 已验证功能：\n");
        printf("   - 数据包构造函数\n");
        printf("   - 校验和计算算法\n");
        printf("   - VfyPwd命令封装\n");
        printf("   - GenImg命令封装\n");
        printf("   - Img2Tz命令封装\n");
        printf("   - Search命令封装\n");
        printf("   - 数据包格式验证\n");
        
        printf("\n🚀 Stage 2 测试完成，可以进行 Stage 3 测试\n");
        return 0;
    } else {
        printf("💥 测试失败！发现 %d 个错误\n", total_errors);
        printf("❗ 请检查协议封装实现\n");
        return 1;
    }
}