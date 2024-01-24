/*
    rdt3.0 有限状态机
        - 新的假设：下层信道可能会丢失分组（数据或ACK）

        - 发送端超时重传：如果到时没有收到ACK->重传  (需要一个倒计数定时器)
        - 发送方等待ACK一段合理的时间
            - 链路层的timeout时间确定的
            - 传输层timeout时间是适应式的
*/

//#include <stdbool.h>
#include <stddef.h>


/*
    有6个 节点
*/
enum State {  
    // 发送方 3个节点
    WAIT_UPPER_SEND_DATA_0,  // 等待来自上层的调用 0
    WAIT_UPPER_SEND_DATA_1,  // 等待来自上层的调用 1

    WAIT_ACK_0,        // 等待ACK0 或者 ACK1 (就是 rdt2.1 中的 NAK)
    WAIT_ACK_1,        // 等待ACK1 或者 ACK0 (NAK)

    // 接收方 2个节点
    WAIT_UPPER_RECV_DATA_0,  // 等待来自上层的调用 0
    WAIT_UPPER_RECV_DATA_1,  // 等待来自上层的调用 1
};


// 发送端 8 个事件、接收端 6个事件
/*
    边 上的所有可能发生的事件
*/
enum Event {
    // 发送端事件

    // S1. 上层发送数据 0 
    UPPER_SEND_DATA_0,  
    // S2. 接收数据包 0， 数据包腐败； 接受数据包，数据包是 NAK
    PKT_0_CORRUPT_OR_IS_ACK_1,
    // S3. 接收数据包 0， 数据包没有腐败，并且是 ACK
    PKT_0_NOTCORRUPT_AND_IS_ACK, 
    // S4 超时
    PKT_0_TIMEOUT,

    // S6. 上层发送数据 1
    UPPER_SEND_DATA_1,
    // S6. 接收数据包 1， 数据包腐败； 接受数据包，数据包是 NAK
    PKT_1_CORRUPT_OR_IS_ACK_0,
    // S7. 接收数据包 1， 数据包没有腐败，并且是 ACK
    PKT_1_NOTCORRUPT_AND_IS_ACK,
    // S8. 超时
    PKT_1_TIMEOUT,

    // 接收端事件
    // R1  等0 发来了0 校验通过
    WAIT_PKT_0_RECV_NOTCURRUPT_PKT_0,  
    // R2  等0  校验不通过
    WAIT_PKT_0_RECV_CURRUPT,   
    // R3  等0 发来了1
    WAIT_PKT_0_RECV_NOTCURRUPT_PKT_1,  

    // R4 等1 发来了1 校验通过
    WAIT_PKT_1_RECV_NOTCURRUPT_PKT_1, 
    // R5 等1  校验不通过
    WAIT_PKT_1_RECV_CURRUPT,  
    // R6 等1发来了0
    WAIT_PKT_1_RECV_NOTCURRUPT_PKT_0, 
};

/*
    节点之间的所有可能出现的 有向 边
*/
struct Transiton {
    enum State current_state;  // 边上的节点 1
    enum State next_state;     // 边上的节点 2 
    enum Event event;          // 发生的事件后， 从节点1转到到节点2 
    int action; // 是否存在动作
    void (*action_func)();
    void *data;  // 参数
};

typedef Transiton Transiton;

void fsm(Transiton transitions[], size_t num, enum State initial_state, enum Event input_event) {
    // initial_state 初始状态  
    // input_event 输入事件
    /*
        在节点 initial_state 上，输入事件 input_event 时，转换到节点 next_state

        initial_state 确定的是 节点
        input_event  确定的是 该节点出发的 具体的 边

        从边上，取出响应的动作（action_func）并执行
    */

    enum State current_state = initial_state;

    for (size_t i = 0; i < num; i++) {
        if (transitions[i].current_state == current_state && transitions[i].event == input_event) {
            current_state = transitions[i].next_state;
            if (transitions[i].action){
                transitions[i].action_func();
            }
        }
    }
}

int main(){

    // 总共有12条边
    Transiton transitions [] = {
        
        // Sender 发送端
        {WAIT_UPPER_SEND_DATA_0, WAIT_ACK_0, UPPER_SEND_DATA_0, 0, NULL, NULL},
        {WAIT_ACK_0, WAIT_ACK_0, PKT_0_CORRUPT_OR_IS_ACK_1, 0, NULL, NULL},
        {WAIT_ACK_0, WAIT_ACK_0, PKT_0_TIMEOUT, 0, NULL, NULL},
        {WAIT_ACK_0, WAIT_UPPER_SEND_DATA_1, PKT_0_NOTCORRUPT_AND_IS_ACK, 0, NULL, NULL},

        {WAIT_UPPER_SEND_DATA_1, WAIT_ACK_1, UPPER_SEND_DATA_1, 0, NULL, NULL},
        {WAIT_ACK_1, WAIT_ACK_1, PKT_1_CORRUPT_OR_IS_ACK_0, 0, NULL, NULL},
        {WAIT_ACK_1, WAIT_ACK_1, PKT_1_TIMEOUT, 0, NULL, NULL},
        {WAIT_ACK_1, WAIT_UPPER_SEND_DATA_0, PKT_1_NOTCORRUPT_AND_IS_ACK, 0, NULL, NULL},

        // Receiver 接收端
        {WAIT_UPPER_RECV_DATA_0, WAIT_UPPER_RECV_DATA_1, WAIT_PKT_0_RECV_NOTCURRUPT_PKT_0, 0, NULL, NULL},
        {WAIT_UPPER_RECV_DATA_0, WAIT_UPPER_RECV_DATA_0, WAIT_PKT_0_RECV_CURRUPT, 0, NULL, NULL},
        {WAIT_UPPER_RECV_DATA_0, WAIT_UPPER_RECV_DATA_1, WAIT_PKT_0_RECV_NOTCURRUPT_PKT_1, 0, NULL, NULL},
        {WAIT_UPPER_RECV_DATA_1, WAIT_UPPER_RECV_DATA_0, WAIT_PKT_1_RECV_NOTCURRUPT_PKT_0, 0, NULL, NULL},
        {WAIT_UPPER_RECV_DATA_1, WAIT_UPPER_RECV_DATA_1, WAIT_PKT_1_RECV_CURRUPT, 0, NULL, NULL},
        {WAIT_UPPER_RECV_DATA_1, WAIT_UPPER_RECV_DATA_0, WAIT_PKT_1_RECV_NOTCURRUPT_PKT_1, 0, NULL, NULL},

    };

    // 模拟 rdt 发送方的协议状态机

    // 发送端发送数据
    fsm(transitions, sizeof(transitions)/sizeof(transitions[0]), WAIT_UPPER_SEND_DATA_0, UPPER_SEND_DATA_0);

    // 接收端接收数据
    fsm(transitions, sizeof(transitions)/sizeof(transitions[0]), WAIT_UPPER_RECV_DATA_0, WAIT_PKT_0_RECV_NOTCURRUPT_PKT_0);
}
