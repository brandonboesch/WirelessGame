// Filename: slave.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017


void slaveInit(NetworkInterface *interface);
void sendMessage(const char messageBuff[COMM_BUFF_SIZE]); 
void receiveMessage();
void calcAngle();

void accelMeasure_isr();
void myButton_isr();
void socket_isr();

