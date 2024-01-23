/*
    rdt 1.0 封装 解封装

    下层的信道是完全可靠的
    - 没有比特出错
    - 没有分组丢失
*/

typedef unsigned int UINT32;

// 协议头部
struct PROTO_HEADER {
    UINT32 sn;   // 分组中加序号, 防止重复
    UINT32 cmd;  // 用于区别命令和数据
    UINT32 len;  // 数据长度
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

void make_pkt(char* packet, UINT32* packet_length, char* data, UINT32 data_len){
    // 将头部字段填充到 packet
    PROTO_HEADER proto_header;
    memset(&proto_header, 0, sizeof(PROTO_HEADER));
    char* pos = packet;

    // todo (1) 协议字段填充
    pos = encode_proto_header(packet, &proto_header);

    // payload 填充
    memcpy(pos, data, data_len); 

    // 计算包的总长度
    *packet_length = sizeof(PROTO_HEADER) + data_len;
}

// 对上层提供服务
void rdt_send(int fd, char* data, UINT32 data_len) {
     
    char packet[1400] = {0};
    UINT32 packet_length = 0;

    // 封包
    make_pkt(packet, &packet_length, data, data_len);
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

    // 解包
    extract_pkt(packet, packet_length, data, data_len);
}

void sender(){

    int fd = 3;
    char data[100] = "send data";
    printf("send data: %s\n", data);
    rdt_send(fd, data, strlen(data));
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
