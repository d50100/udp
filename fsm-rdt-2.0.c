/*
    rdt2.0 有限状态机
*/

//#include <stdbool.h>
#include <stddef.h>

// 发送端 2个状态、接收端一个状态
/*
    有三个 节点
*/
enum State {  
    // 发送方
    WAIT_SEND_DATA,  // 等待来自上层的调用
    WAIT_ACK_NAK,        // 等待ACK或者NAK

    // 接收方
    WAIT_RECV_DATA,  // 等待来自上层的调用
};


// 发送端 3个事件、接收端个事件
/*
    边 上的所有可能发生的事件
*/
enum Event {
    // 发送端事件
    SEND_DATA,  // 上层有数据需要发送
    RECV_ACK,   // 接收到数据包，是确认包 ACK
    RECV_NAK,   // 接收到数据包， 是确认包 NAK

    // 接收端事件
    RECV_CORRUPT_DATA,  // 接收的数据包, 校验不通过
    RECV_NOTCURRUPT_DATA,  // 接受的数据包， 校验不通过
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
    Transiton transitons [] = {
        // 发送端的 fsm
        // 当前状态    下一个状态    事件

        // 1. 等待发送数据，发送完成后进入等待Ack
        {WAIT_SEND_DATA, WAIT_ACK_NAK, SEND_DATA},
        // 2. 等待 ACK 如果接受ACK 进入等待发送数据状态
        {WAIT_ACK_NAK, WAIT_SEND_DATA, RECV_ACK},
        // 3. 等待 NAK 如果接受NAK 进入 等待ACK状态
        {WAIT_ACK_NAK, WAIT_ACK_NAK, RECV_NAK},


        // 接收端的 fsm

        // 1. 等待接收数据状态 如果数据校验失败 等待接收数据状态 
        {WAIT_RECV_DATA, WAIT_RECV_DATA, RECV_CORRUPT_DATA},
        // 2. 等待接收数据状态 如果数据校验成功 等待接收数据状态
        {WAIT_RECV_DATA, WAIT_RECV_DATA, RECV_NOTCURRUPT_DATA},
    };

    // 模拟 rdt 发送方的协议状态机

    /*
        在节点 WAIT_SEND_DATA 上，输入事件 SEND_DATA 时，转换下一个节点 WAIT_ACK_NAK
    */
    fsm(transitons, 5, WAIT_SEND_DATA, SEND_DATA);
}
