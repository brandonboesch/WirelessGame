// Filename: master.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017


void master_init(NetworkInterface *interface);
static void myButton_isr();
static void send_message(const char messageBuff[BUFF_SIZE]); 
static void socket_isr();
static void receive();
static void messageTimeoutCallback();
