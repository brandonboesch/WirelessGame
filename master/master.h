// Filename: master.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017


void masterInit(NetworkInterface *interface);
void sendMessage(const char messageBuff[COMM_BUFF_SIZE]); 
void receiveMessage();
void pairSlaves();
void game(void);

void socket_isr();
void myButton_isr();

