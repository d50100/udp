/*
    rdt 2.0
    - 有比特出错
    - 没有分组丢失

    下层信道可能会出错：将分组中的比特翻转
    - 用校验和来检测比特差错
    - 确认(ACK)：接收方显式地告诉发送方分组已被正确接收
    - 否定确认( NAK): 接收方显式地告诉发送方分组发生了差错
     - 发送方收到NAK后，发送方重传分组

    rdt2.0中的新机制：采用差错控制编码进行差错检测
     发送方差错控制编码、缓存
     接收方使用编码检错 
     接收方的反馈：控制报文（ACK，NAK）：接收方 ->发送方
     发送方收到反馈相应的动作
*/


/*
    计算检验和的方法
*/
#include <stdint.h>
uint16_t calculate_checksum(const uint8_t *data, size_t length) {
    uint16_t sum = 0;

    size_t i = 0;
    for(; i < length; i += 2){
        if (i+1 < length){
            // 两个字节     high 8bit         low 8 bit
            sum += ((uint16_t)data[i] << 8) + data[i+1];
        }
        else {
            // high 8bit     
            sum += (uint16_t)data[i] << 8;
        }
    }

    while(sum >> 16){
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

int is_all_ones(uint16_t value){
    return (value & 0xFFFF) == 0xFFFF;
}


typedef unsigned int UINT32;

enum PacketType {
    CMD_PUSH,
    CMD_ACK, 
    CMD_NAK
} PacketType;


// 协议头部
struct PROTO_HEADER {
    UINT32 sn;   // 分组中加序号, 防止重复
    UINT32 cmd;  // 用于区别命令和数据
    UINT32 len;  // 数据长度
    uint16_t chksum; // 检验和
    uint8_t pkt_type;  // 包类型
    char data[1];
};

typedef PROTO_HEADER PROTO_HEADER;


void udt_send(int fd, char* packet, UINT32 len){
    // 调用 下层 提供的不可靠的服务  
    sendto(fd, packet, len, 0);
}


void udt_recv(int fd, char* packet, UINT32* len){
    // 调用 下层 提供的不可靠的服务
    UINT32 n = recvfrom(fd, packet, *len, 0);
    *len = n;
}

#define IOFFSET(TYPE, MEMBER) ((size_t) (&((TYPE *)0)->MEMBER))

void make_pkt(char* packet, UINT32* packet_length, enum PacketType pkt_type, char* data, UINT32 data_len){
    // 将头部字段填充到 packet
    PROTO_HEADER proto_header;
    memset(&proto_header, 0, sizeof(PROTO_HEADER));
    
    proto_header.pkt_type = pkt_type;

    char* pos = packet;
    // todo (1) 协议字段填充
    pos = encode_proto_header(packet, &proto_header);

    // 计算包的总长度
    *packet_length = sizeof(PROTO_HEADER) + data_len;

    // 计算检验和
    uint16_t chk = calculate_checksum(packet, packet_length);

    // 检验和填充
    checksum_fill((uint32_t)packet, chk);

}
void checksum_fill(uint8_t packet , uint16_t chk){
    size_t offset = IOFFSET(PROTO_HEADER, chksum);

    *((uint8_t *)packet + offset ) = (chk & 0xFF00) >> 8;
    *((uint8_t *)packet + offset + 1) = chk & 0x00FF;
}

// 对上层提供服务
void rdt_send(int fd, char* data, enum PacketType pkt_type, UINT32 data_len) {
     
    char packet[1400] = {0};
    UINT32 packet_length = 0;

    // 封包
    make_pkt(packet, &packet_length, pkt_type, data, data_len);

    // 数据发送
    udt_send(fd, packet, packet_length);
}

void extract_pkt(char* packet, UINT32 packet_length, char* data, UINT32 *data_len){

    // assert (packet_length > sizeof(PROTO_HEADER));
    if (packet_length <= sizeof(PROTO_HEADER)){
        data_len = 0;
        return;
    }

    PROTO_HEADER proto_header;
    memset(&proto_header, 0, sizeof(PROTO_HEADER));

    char *ptr = packet;
    // todo (2) 解封装
    ptr = decode_proto_header(ptr, &proto_header);

    // 根据 proto_header.cmd 做不同的业务
    data_len = proto_header.len;

    // assert (data_len <= packet_length - sizeof(PROTO_HEADER));
    if (data_len > packet_length - sizeof(PROTO_HEADER)){
        data_len = packet_length - sizeof(PROTO_HEADER);
    }

    memcpy(data, ptr, data_len);
}

void rdt_recv(int fd, char* data, UINT32 *data_len){
    char packet[1400] = {0};
    UINT32 packet_length = sizeof(packet);

    // 使用下层的服务接受数据
    udt_recv(fd, packet, &packet_length);

    // 校验和校验
    uint16_t chk = calculate_checksum(packet, packet_length);

    if (is_all_ones(chk)){
        // 校验正确 发送 ACK
        char ack_pkt[1000] = {0};
        UINT32 ack_len = sizeof(ack_pkt);
        make_pkt(ack_pkt, &ack_len, CMD_ACK, NULL, 0);
        udt_send(fd, packet, packet_length);
    }
    else {
        // 校验错误 发送 NAK
        char nak_pkt[1000] = {0};
        UINT32 nak_len = sizeof(nak_pkt);
        make_pkt(nak_pkt, &nak_len, CMD_NAK, NULL, 0);
        udt_send(fd, packet, packet_length);
    }

    // 解包
    extract_pkt(packet, packet_length, data, data_len);


}

void sender(){

    int fd = 3;
    char data[100] = "send data";
    printf("send data: %s\n", data);

    enum PacketType pkt_type = CMD_PUSH;
    rdt_send(fd, data, pkt_type, strlen(data));

    // 等待 ACK 或 NAK
    rdt_recv(fd, data, NULL);
}

void receiver(){
    int fd = 3;
    char data[100] = {0};
    UINT32 recv_length = 0;
    rdt_recv(fd, data, &recv_length);
    printf("recv data: %s\n", data);
}


int main(){
    sender();
    receiver();
    return 0;
}
