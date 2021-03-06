/*----------------------------------------------------------------
 *
 * diag_tools.h
 *
 * Debug and test tools 
 *
 *---------------------------------------------------------------*/

#define T_HELP       0       // Help test
#define T_DIGITAL    1       // Digital test
#define T_TRIGGER    2       // Test microphone trigger
#define T_CLOCK      3       // Trigger clock internally
#define T_OSCOPE     4       // Microphone digital oscilliscope
#define T_OSCOPE_PC  5       // Display oscillisope on PC
#define T_PAPER      6       // Advance paper backer
#define T_SPIRAL     7       // Generate sprial pattern
#define T_GRID       8       // Generate grid pattern
#define T_ONCE       9       // Generate single calculation @ 45North
#define T_PASS_THRU 10       // Serial port pass through
#define T_SET_TRIP  11       // Set the microphone trip point
#define T_XFR_LOOP  12       // Transfer loopback

void self_test(uint16_t test);


