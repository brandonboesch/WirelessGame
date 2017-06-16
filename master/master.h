// Filename: master.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017


void masterInit(NetworkInterface *interface);
void sendMessage(const char messageBuff[BUFF_SIZE]); 
void receiveMessage();

void socket_isr();
void myButton_isr();

