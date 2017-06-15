// Filename: slave.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017


void slave_init(NetworkInterface *interface);
static void send_message(const char messageBuff[COMM_BUFF_SIZE]); 
static void receive();
static void messageTimeoutCallback();
static void calcDataValues();

static void accelMeasure_isr();
static void myButton_isr();
static void socket_isr();

