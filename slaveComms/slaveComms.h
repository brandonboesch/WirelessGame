// Filename: slaveComms.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017


void start_slave(NetworkInterface *interface);
void start_blinking();
void cancel_blinking();   
static void blink();
static void init_socket();
static void myButton_isr();
static void send_message();
static void handle_socket();
static void receive();
static void messageTimeoutCallback();
