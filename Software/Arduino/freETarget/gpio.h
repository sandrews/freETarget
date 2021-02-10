/*
 * Global functions
 */
void init_gpio(void);                         // Initialize the GPIO ports
void arm_counters(void);                      // Make the board ready
unsigned int is_running(void);                // Return a bit mask of running sensors 
void set_LED(unsigned int led, bool state);   // Manage the LEDs
unsigned int read_DIP(void);                  // Read the DIP switch register
unsigned int read_counter(unsigned int direction);
void stop_counters(void);                     // Turn off the counter registers
bool read_in(unsigned int port);              // Read the selected port
void read_timers(void);                       // Read and return the counter registers

/*
 *  Port Definitions
 */
#define D0          37                     //  Byte Port bit locations
#define D1          36                     //
#define D2          35                     //
#define D3          34                     //
#define D4          33                     //
#define D5          32                     //
#define D6          31                     //
#define D7          30                     //

#define NORTH_HI    50                    // Address port but locations
#define NORTH_LO    51
#define EAST_HI     48
#define EAST_LO     49
#define SOUTH_HI    43                    // Address port but locations
#define SOUTH_LO    47
#define WEST_HI     41
#define WEST_LO     42

#define RUN_NORTH   25
#define RUN_EAST    26
#define RUN_SOUTH   27
#define RUN_WEST    28

#define QUIET       29
#define RCLK        40
#define CLR_N       39
#define STOP_N      52       
#define CLOCK_START 53

#define DIP_A        9                    // 
#define DIP_B       10
#define DIP_C       11
#define DIP_D       12

#define CTS_U        7
#define RTS_U        6
#define LED_PWM      5          // PWM Port
#define LED_S        4
#define LED_X        3
#define LED_Y        2

#define NORTH        0
#define EAST         1
#define SOUTH        2
#define WEST         3
 
#define PAPER       18                    // Paper advance drive active low (TX1)
#define PAPER_ON     0
#define PAPER_OFF    1

#define SPARE       19

#define J10_1      VCC
#define J10_2       14                    // TX3
#define J10_3       15                    // RX3
#define J10_4       19                    // RX1
#define J10_5       18                    // TX1
#define J10_6      GND



#define EOF 0xFF
