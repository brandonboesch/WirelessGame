// Filename: master.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017


int main(void);
void trace_printer(const char* str);
void sendMessage(const char messageBuff[COMM_BUFF_SIZE]); 
void receiveMessage();
void pairSlaves();


void goalCheck(float slave1_paddle_top, float slave2_paddle_top);
void game(void);

void socket_isr();
void myButton_isr();

