/*!
 * @file
 *   @brief Stop gap code to handle M4 initialization
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#if defined(MINIMALIST_BUILD)

#include "m4.h"

#include <QDebug>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>

#define  UART_PATH  "/dev/ttyMFD0"

static bool
UART_Set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
    int i;
    int speed_arr[] = { B230400, B115200, B38400, B19200, B9600, B4800, B2400,
                        B1200, B300
                      };
    int name_arr[] =
    { 230400, 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300 };
    struct termios options;
    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将其保存于options,该函数还可以测试配置是否正确，
     该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1*/
    if(tcgetattr(fd, &options) != 0) {
        return false;
    }
    //设置串口输入波特率和输出波特率
    for(i = 0; i < (int)(sizeof(speed_arr) / sizeof(int)); i++) {
        if(speed == name_arr[i]) {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }
    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;
    //设置数据流控制
    switch(flow_ctrl) {
        case 0: //不使用流控制
            options.c_cflag &= ~CRTSCTS;
            break;
        case 1: //使用硬件流控制
            options.c_cflag |= CRTSCTS;
            break;
        case 2: //使用软件流控制
            options.c_cflag |= IXON | IXOFF | IXANY;
            break;
    }
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch(databits) {
        case 5:
            options.c_cflag |= CS5;
            break;
        case 6:
            options.c_cflag |= CS6;
            break;
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            return false;
    }
    //设置校验位
    switch(parity) {
        case 'n':
        case 'N': //无奇偶校验位。
            options.c_cflag &= ~PARENB;
            options.c_iflag &= ~INPCK;
            break;
        case 'o':
        case 'O': //设置为奇校验
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag |= INPCK;
            break;
        case 'e':
        case 'E': //设置为偶校验
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_iflag |= INPCK;
            break;
        case 's':
        case 'S': //设置为空格
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        default:
            return false;
    }
    // 设置停止位
    switch(stopbits) {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            return false;
    }
    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //我加的
    //options.c_lflag &= ~(ISIG | ICANON);
    options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 0; /* 读取字符的最少个数为1 */
    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
    tcflush(fd, TCIFLUSH);
    //激活配置 (将修改后的termios数据设置到串口中）
    if(tcsetattr(fd, TCSANOW, &options) != 0) {
        return false;
    }
    return true;
}

static const unsigned char CRC8T[] = {
    0, 7, 14, 9, 28, 27, 18, 21, 56, 63, 54, 49, 36, 35, 42, 45, 112, 119, 126, 121, 108, 107,
    98, 101, 72, 79, 70, 65, 84, 83, 90, 93, 224, 231, 238, 233, 252, 251, 242, 245, 216, 223, 214, 209, 196,
    195, 202, 205, 144, 151, 158, 153, 140, 139, 130, 133, 168, 175, 166, 161, 180, 179, 186, 189, 199, 192,
    201, 206, 219, 220, 213, 210, 255, 248, 241, 246, 227, 228, 237, 234, 183, 176, 185, 190, 171, 172, 165,
    162, 143, 136, 129, 134, 147, 148, 157, 154, 39, 32, 41, 46, 59, 60, 53, 50, 31, 24, 17, 22, 3, 4, 13, 10,
    87, 80, 89, 94, 75, 76, 69, 66, 111, 104, 97, 102, 115, 116, 125, 122, 137, 142, 135, 128, 149, 146, 155,
    156, 177, 182, 191, 184, 173, 170, 163, 164, 249, 254, 247, 240, 229, 226, 235, 236, 193, 198, 207, 200,
    221, 218, 211, 212, 105, 110, 103, 96, 117, 114, 123, 124, 81, 86, 95, 88, 77, 74, 67, 68, 25, 30, 23, 16,
    5, 2, 11, 12, 33, 38, 47, 40, 61, 58, 51, 52, 78, 73, 64, 71, 82, 85, 92, 91, 118, 113, 120, 127, 106, 109,
    100, 99, 62, 57, 48, 55, 34, 37, 44, 43, 6, 1, 8, 15, 26, 29, 20, 19, 174, 169, 160, 167, 178, 181, 188,
    187, 150, 145, 152, 159, 138, 141, 132, 131, 222, 217, 208, 215, 194, 197, 204, 203, 230, 225, 232, 239,
    250, 253, 244, 243
};

/** x^8 + x^2 + x + 1 */
static unsigned char
crc8(unsigned char* buffer, int len)
{
    unsigned char ret = 0;
    for(int i = 0; i < len; ++i) {
        ret = CRC8T[ret ^ buffer[i]];
    }
    return ret;
}

static void
enter_run(int uart_fd)
{
    qDebug() << "Start enter_run";
    //send enter run command
    unsigned char buf[128];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x55; //sync bytes
    buf[1] = 0x55;
    int payload_len = 10;
    buf[2] = payload_len + 1;
    buf[5] = 0x03 << 2;
    buf[12] = 0x68; //enter run command
    buf[payload_len + 3] = crc8(buf + 3, payload_len);
    if(write(uart_fd, buf, payload_len + 4) != payload_len + 4) {
        qWarning() << "write failed";
    }
    qDebug() << "end enter_run";
}

namespace Yuneec {
bool
initM4()
{
    int uart_fd = open(UART_PATH, O_RDWR | O_NOCTTY);
    if(uart_fd == -1) {
        qWarning() << "open serial ttyMFD1 failed";
        return false;
    }
    if(UART_Set(uart_fd, 230400, 0, 8, 1, 'N') == false) {
        qWarning() << "uart config failed";
        return false;
    }
    enter_run(uart_fd); //enable RC
    close(uart_fd);
    return true;
}
}

#endif
