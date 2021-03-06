/*----------------------------------------------------------------
 *
 * Compute_hit
 *
 * Determine the score
 *
 *---------------------------------------------------------------*/

#include "mechanical.h"
#include "compute_hit.h"
#include "freETarget.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "arduino.h"

#define THRESHOLD (0.001)

#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)

#define R(x)  (((x)+location) % 4)    // Rotate the target by location points

/*
 *  Variables
 */

extern const char* which_one[4];

sensor_t s[4];

unsigned int  bit_mask[] = {0x01, 0x02, 0x04, 0x08};
unsigned long timer_value[4];     // Array of timer values
unsigned long minion_value[4];    // Array of timers values from minion
unsigned long minion_ref;         // Trip voltage from minion
unsigned int  pellet_calibre;     // Time offset to compensate for pellet diameter
static char nesw[]= "NESW";
/*----------------------------------------------------------------
 *
 * double speed_of_sound(double temperature)
 *
 * Return the speed of sound (mm / us)
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/

double speed_of_sound(double temperature)
{
  double speed;

  speed = (331.3d + 0.606d * temperature) * 1000.0d / 1000000.0d; 
  
  if ( read_DIP() & (VERBOSE_TRACE) )
    {
    Serial.print("\r\nSpeed of sound: "); Serial.print(speed); Serial.print("mm/us");
    Serial.print("  Worst case delay: "); Serial.print(json_sensor_dia / speed * OSCILLATOR_MHZ); Serial.print(" counts");
    }

  return speed;  
}

/*----------------------------------------------------------------
 *
 * void init_sensors()
 *
 * Setup the constants in the strucure
 *
 *----------------------------------------------------------------
 *
 *                             N     (+,+)
 *                             
 *                             
 *                      W      0--R-->E
 *
 *
 *               (-,-)         S 
 * 
 *--------------------------------------------------------------*/
void init_sensors(void)
{

/*
 * Determine the speed of sound and ajust
 */
  s_of_sound = speed_of_sound(temperature_C());
  pellet_calibre = ((double)json_calibre_x10 / s_of_sound / 2.0d / 10.0d) * OSCILLATOR_MHZ; // Clock adjustement
  
 /*
  * Work out the geometry of the sensors
  */
  s[N].index = N;
  s[N].x = json_north_x / s_of_sound * OSCILLATOR_MHZ;
  s[N].y = (json_sensor_dia /2.0d + json_north_y) / s_of_sound * OSCILLATOR_MHZ;

  s[E].index = E;
  s[E].x = (json_sensor_dia /2.0d + json_east_x) / s_of_sound * OSCILLATOR_MHZ;
  s[E].y = (0.0d + json_east_y) / s_of_sound * OSCILLATOR_MHZ;

  s[S].index = S;
  s[S].x = 0.0d + json_south_x / s_of_sound * OSCILLATOR_MHZ;
  s[S].y = -(json_sensor_dia/ 2.0d + json_south_y) / s_of_sound * OSCILLATOR_MHZ;

  s[W].index = W;
  s[W].x = -(json_sensor_dia / 2.0d  + json_west_x) / s_of_sound * OSCILLATOR_MHZ;
  s[W].y = json_west_y / s_of_sound * OSCILLATOR_MHZ;
  
 /* 
  *  All done, return
  */
  return;
}
/*----------------------------------------------------------------
 *
 * void compute_hit()
 *
 * determine the location of the hit
 *
 *----------------------------------------------------------------
 *
 * See freETarget documentaton for algorithm
 *--------------------------------------------------------------*/

unsigned int compute_hit
  (
  unsigned int sensor_status,      // Bits read from status register
  history_t*   h,                  // Storing the results
  bool         test_mode           // Fake counters in test mode
  )
{
  double        reference;         // Time of reference counter
  int           location;          // Sensor chosen for reference location
  int           discard;           // Discard the smallest timer value
  int           i, j, count;
  double        estimate;          // Estimated position
  double        last_estimate, error; // Location error
  double        r1, r2;            // Distance between points
  double        x, y;              // Computed location
  double        x_avg, y_avg;      // Running average location
  double        smallest;          // Smallest non-zero value measured
  double        constillation;     // How many constillations did we compute
  int           v_ref;             // reference voltage for this board
  
  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print("\r\ncompute_hit()");
  }
  
/*
 *  Compute the current geometry based on the speed of sound
 */
  init_sensors();
  
 /* 
  *  Read the counter registers
  */
  if ( test_mode == false )                              // Skip if using test values
  {
    read_timers();
  }
  
  if ( read_DIP() & VERBOSE_TRACE )
  {
    if ( read_DIP() & BOSS )
    {
      Serial.print("\r\nBOSS\r\n");
    }
    else
    {
    Serial.print("\r\nminion\r\n");
    } 

    for (i=N; i <= W; i++)
    {
      Serial.print(which_one[i]); Serial.print(timer_value[i]); Serial.print(" "); 
    }
  }
  
/*
 * Compute the extrapolated values or send the clock values over to the master
 */
 
  if ( read_DIP() & BOSS )
  {
    if ( ((read_DIP() & VERSION_2) == 0) && poll_minion() )
    {
      if ( read_DIP() & (VERBOSE_TRACE) )
      {
        Serial.print("\r\nPolled v:"); Serial.print(v_ref); Serial.print(" r:"); Serial.print( minion_ref );
        Serial.print("\r\nminion     ");
        for (i=N; i <= W; i++)
        {
          Serial.print(which_one[i]); Serial.print(minion_value[i]); Serial.print("  ");
        }

        Serial.print("\r\nBOSS       ");
        for (i=N; i <= W; i++)
        {
          Serial.print(which_one[i]); Serial.print(timer_value[i]); Serial.print("  ");
        }
        
        Serial.print("\r\nCorrection ");
        for (i=N; i <= W; i++)
        {
          Serial.print(which_one[i]); Serial.print((timer_value[i]- minion_value[i]) * (long)v_ref / (minion_ref -(long)v_ref)); Serial.print("  ");
        }
      }
      for (i=N; i <= W; i++)
      {
        timer_value[i] += (timer_value[i]- minion_value[i]) * (long)v_ref / (minion_ref -(long)v_ref);
      }
      if ( read_DIP() & VERBOSE_TRACE )
      {
        Serial.print("\r\nAdjusted   ");
        for (i=N; i <= W; i++)
        {
          Serial.print(which_one[i]); Serial.print(timer_value[i]); Serial.print("  ");
        }
      }
    }
    else
    {
      if ( read_DIP() & VERBOSE_TRACE )
      {
        Serial.print("\r\nMinion timed out!");
      }
    }
    for (i=N; i <= W; i++)
    {
      timer_value[i] += pellet_calibre;
    }
  }
  else
  {
    send_BOSS();
    return 0;
  }

/*
 * Determine the location of the reference counter (longest time)
 */
  reference = timer_value[N];
  location = N;
  for (i=E; i <= W; i++)
  {
    if ( timer_value[i] > reference )
    {
      reference = timer_value[i];
      location = i;
    }
  }
  
 if ( read_DIP() & VERBOSE_TRACE )
 {
   Serial.print("\r\nReference: "); Serial.print(reference); Serial.print("  location:"); Serial.print(nesw[location]);
 }

/*
 * Correct the time to remove the shortest distance
 */
  for (i=N; i <= W; i++)
  {
    s[i].count = reference - timer_value[i];
    s[i].is_valid = true;
    if ( timer_value[i] == 0 )
    {
      s[i].is_valid = false;
    }
  }

  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print("\r\nCounts       ");
    for (i=N; i <= W; i++)
    {
     Serial.print(*which_one[i]); Serial.print(s[i].count); Serial.print(" ");
    }
    Serial.print("\r\nMicroseconds ");
    for (i=N; i <= W; i++)
    {
     Serial.print(*which_one[i]); Serial.print(((double)s[i].count) / ((double)OSCILLATOR_MHZ)); Serial.print(" ");
    }
  }

/*
 * Fill up the structure with the counter geometry
 */
  for (i=N; i <= W; i++)
  {
    s[i].b = s[i].count;
    s[i].c = sqrt(sq(s[(i) % 4].x - s[(i+1) % 4].x) + sq(s[(i) % 4].y - s[(i+1) % 4].y));
   }
  
  for (i=N; i <= W; i++)
  {
    s[i].a = s[(i+1) % 4].b;
  }
  
/*
 * Find the smallest non-zero value, this is the sensor furthest away from the sensor
 */
  smallest = s[N].count;
  discard = N;
  for (i=N+1; i <= W; i++)
  {
    if ( s[i].count < smallest )
    {
      smallest = s[i].count;
      discard = i;
    }
  }
//  s[discard].is_valid = false;    // Throw away the shortest time. (test pathc removed)
  
/*  
 *  Loop and calculate the unknown radius (estimate)
 */
  estimate = s[N].c - smallest + 1.0d;
 
  if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\r\nestimate: "); Serial.print(estimate);
   }
  error = 999999;                  // Start with a big error
  count = 0;

 /*
  * Iterate to minimize the error
  */
  while (error > THRESHOLD )
  {
    x_avg = 0;                     // Zero out the average values
    y_avg = 0;
    last_estimate = estimate;

    constillation = 0.0;
    for (i=N; i <= W; i++)        // Calculate X/Y for each sensor
    {
      if ( find_xy(&s[i], estimate) )
      {
        x_avg += s[i].xs;        // Keep the running average
        y_avg += s[i].ys;
        constillation += 1.0;
      }
    }

    x_avg /= constillation;      // Work out the average intercept
    y_avg /= constillation;

    estimate = sqrt(sq(s[location].x - x_avg) + sq(s[location].y - y_avg));
    error = abs(last_estimate - estimate);

    if ( read_DIP() & VERBOSE_TRACE )
    {
      Serial.print("\r\nx_avg:");  Serial.print(x_avg);   Serial.print("  y_avg:"); Serial.print(y_avg); Serial.print(" estimate:"),  Serial.print(estimate);  Serial.print(" error:"); Serial.print(error);
      Serial.println();
    }
    count++;
    if ( count > 20 )
    {
      break;
    }
  }
  
 /*
  * All done return
  */
  h->shot = shot;
  h->x = x_avg;
  h->y = y_avg;
  return location;
}

/*----------------------------------------------------------------
 *
 * find_xy
 *
 * Calaculate where the shot seems to lie
 * 
 * Return: TRUE if the shot was computed correctly
 *
 *----------------------------------------------------------------
 *
 *  Using the law of Cosines
 *  
 *                    C
 *                 /     \   
 *             b             a  
 *          /                   \
 *     A ------------ c ----------  B
 *  
 *  a^2 = b^2 + c^2 - 2(bc)cos(A)
 *  
 *  Rearranging terms
 *            ( a^2 - b^2 - c^2 )
 *  A = arccos( ----------------)
 *            (      -2bc       )
 *            
 *  In our system, a is the estimate for the shot location
 *                 b is the measured time + estimate of the shot location
 *                 c is the fixed distance between the sensors
 *                 
 *--------------------------------------------------------------*/

bool find_xy
    (
     sensor_t* s,           // Sensor to be operatated on
     double estimate        // Estimated position   
     )
{
  double ae, be;            // Locations with error added
  double rotation;          // Angle shot is rotated through

/*
 * Check to see if the sensor data is correct.  If not, return an error
 */
  if ( s->is_valid == false )
  {
    if ( read_DIP() & VERBOSE_TRACE )
    {
      Serial.print("\r\nSensor: "); Serial.print(s->index); Serial.print(" no data");
    }
    return false;           // Sensor did not trigger.
  }

/*
 * It looks like we have valid data.  Carry on
 */
  ae = s->a + estimate;     // Dimenstion with error included
  be = s->b + estimate;

  if ( (ae + be) < s->c )   // Check for an accumulated round off error
    {
    s->angle_A = 0;         // Yes, then force to zero.
    }
  else
    {  
    s->angle_A = acos( (sq(ae) - sq(be) - sq(s->c))/(-2.0d * be * s->c));
    }
  
/*
 *  Compute the X,Y based on the detection sensor
 */
  switch (s->index)
  {
    case (N): 
      rotation = PI_ON_2 - PI_ON_4 - s->angle_A;
      s->xs = s->x + ((be) * sin(rotation));
      s->ys = s->y - ((be) * cos(rotation));
      break;
      
    case (E): 
      rotation = s->angle_A - PI_ON_4;
      s->xs = s->x - ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;
      
    case (S): 
      rotation = s->angle_A + PI_ON_4;
      s->xs = s->x - ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;
      
    case (W): 
      rotation = PI_ON_2 - PI_ON_4 - s->angle_A;
      s->xs = s->x + ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;

    default:
      if ( read_DIP() & VERBOSE_TRACE )
      {
        Serial.print("\n\nUnknown Rotation:"); Serial.print(s->index);
      }
      break;
  }

/*
 * Debugging
 */
  if ( read_DIP() & VERBOSE_TRACE )
    {
    Serial.print("\r\nindex:"); Serial.print(s->index) ; 
    Serial.print(" a:");        Serial.print(s->a);       Serial.print("  b:");  Serial.print(s->b);
    Serial.print(" ae:");       Serial.print(ae);         Serial.print("  be:"); Serial.print(be);    Serial.print(" c:"),  Serial.print(s->c);
    Serial.print(" cos:");      Serial.print(cos(rotation)); Serial.print(" sin: "); Serial.print(sin(rotation));
    Serial.print(" angle_A:");  Serial.print(s->angle_A); Serial.print("  x:");  Serial.print(s->x);  Serial.print(" y:");  Serial.print(s->y);
    Serial.print(" rotation:"); Serial.print(rotation);   Serial.print("  xs:"); Serial.print(s->xs); Serial.print(" ys:"); Serial.print(s->ys);
    }
 
/*
 *  All done, return
 */
  return true;
}
  
/*----------------------------------------------------------------
 *
 * void send_score(void)
 *
 * Send the score out over the serial port
 *
 *----------------------------------------------------------------
 * 
 * The score is sent as:
 * 
 * {"shot":n, "x":x, "y":y, "r(adius)":r, "a(ngle)": a, debugging info ..... }
 * 
 * It is up to the PC program to convert x & y or radius and angle
 * into a meaningful score relative to the target.
 *    
 *--------------------------------------------------------------*/

void send_score
  (
  history_t* h,                   // History record
  int shot                        // Current shot
  )
{
  double x, y;                   // Shot location in mm X, Y
  double radius;
  double angle;
  unsigned int volts;
  double clock_face;
  double coeff;                 // From Alex Bird
  int    z;
  double score;
  
 /* 
  *  Work out the hole in perfect coordinates
  */
  x = h->x * s_of_sound * CLOCK_PERIOD;
  y = h->y * s_of_sound * CLOCK_PERIOD;
  radius = sqrt(sq(x) + sq(y));
  angle = atan2(h->y, h->x) / PI * 180.0d;

/*
 * Rotate the result based on the construction, and recompute the hit
 */
  angle += json_sensor_angle;
  x = radius * cos(PI * angle / 180.0d);
  y = radius * sin(PI * angle / 180.0d);

/* 
 *  Display the results
 */
  Serial.print("\r\n{");
  AUX_SERIAL.print("\r\n{");
  
#if ( S_SHOT )
  Serial.print("\"shot\":");      Serial.print(shot); Serial.print(", ");
  AUX_SERIAL.print("\"shot\":");  AUX_SERIAL.print(shot); AUX_SERIAL.print(", ");
  if ( json_name_id != 0 )
  {
    Serial.print("\"name\":\"");      Serial.print(names[json_name_id]);     Serial.print("\", ");
    AUX_SERIAL.print("\"name\":");  AUX_SERIAL.print(names[json_name_id]); AUX_SERIAL.print(", ");
  }
#endif

#if ( S_SCORE )
  coeff = 9.9 / (((double)json_1_ring_x10 + (double)json_calibre_x10) / 20.0d);
  score = 10.9 - (coeff * radius);
  Serial.print("\"score\":");      Serial.print(score); Serial.print(", ");
  AUX_SERIAL.print("\"score\":");  AUX_SERIAL.print(score); AUX_SERIAL.print(", ");
  z = 360 - (((int)angle - 90) % 360);
  clock_face = (double)z / 30.0;
  Serial.print("\"clock\":\"");    Serial.print((int)clock_face);     Serial.print(":");     Serial.print((int)(60*(clock_face-((int)clock_face))));     Serial.print("\", ");
  AUX_SERIAL.print("\"clock:\"");  AUX_SERIAL.print((int)clock_face); AUX_SERIAL.print(":"); AUX_SERIAL.print((int)60*(clock_face-((int)clock_face))); AUX_SERIAL.print("\", ");
#endif

#if ( S_XY )
  Serial.print("\"x\":");     Serial.print(x);    Serial.print(", ");
  Serial.print("\"y\":");     Serial.print(y);    Serial.print(", ");
  AUX_SERIAL.print("\"x\":"); AUX_SERIAL.print(x); AUX_SERIAL.print(", ");
  AUX_SERIAL.print("\"y\":"); AUX_SERIAL.print(y); AUX_SERIAL.print(", ");
#endif

#if ( S_POLAR )
  Serial.print("\"r\":");     Serial.print(radius);    Serial.print(", ");
  Serial.print("\"a\":");     Serial.print(angle);     Serial.print(", ");
  AUX_SERIAL.print("\"r\":"); AUX_SERIAL.print(radius); AUX_SERIAL.print(", ");
  AUX_SERIAL.print("\"a\":"); AUX_SERIAL.print(angle);  AUX_SERIAL.print(", ");
#endif

#if ( S_COUNTERS )
  Serial.print("\"N\":");     Serial.print((int)s[N].count);      Serial.print(", ");
  Serial.print("\"E\":");     Serial.print((int)s[E].count);      Serial.print(", ");
  Serial.print("\"S\":");     Serial.print((int)s[S].count);      Serial.print(", ");
  Serial.print("\"W\":");     Serial.print((int)s[W].count);      Serial.print(", ");
  AUX_SERIAL.print("\"N\":"); AUX_SERIAL.print((int)s[N].count);  AUX_SERIAL.print(", ");
  AUX_SERIAL.print("\"E\":"); AUX_SERIAL.print((int)s[E].count);  AUX_SERIAL.print(", ");
  AUX_SERIAL.print("\"W\":"); AUX_SERIAL.print((int)s[S].count);  AUX_SERIAL.print(", ");
  AUX_SERIAL.print("\"S\":"); AUX_SERIAL.print((int)s[W].count);  AUX_SERIAL.print(", ");
#endif

#if ( S_MISC ) 
  volts = analogRead(V_REFERENCE);
  Serial.print("\"V_REF\":");       Serial.print(TO_VOLTS(volts));      Serial.print(", ");
  Serial.print("\"T\":");           Serial.print(temperature_C());      Serial.print(", ");
  Serial.print("\"VERSION\":");     Serial.print(SOFTWARE_VERSION);
  AUX_SERIAL.print("\"V_REF\":");   AUX_SERIAL.print(TO_VOLTS(volts)); AUX_SERIAL.print(", ");
  AUX_SERIAL.print("\"T\":");       AUX_SERIAL.print(temperature_C()); AUX_SERIAL.print(", ");
  AUX_SERIAL.print("\"VERSION\":"); AUX_SERIAL.print(SOFTWARE_VERSION);
#endif

  Serial.print("}\r\n");
  AUX_SERIAL.print("}\r\n");
  
  return;
}


/*----------------------------------------------------------------
 *
 * void show_timer(void)
 *
 * Display a timer message to identify errors
 *
 *----------------------------------------------------------------
 * 
 * The error is sent as:
 * 
 * {"error": Run Latch, timer information .... }
 *    
 *--------------------------------------------------------------*/

void send_timer
  (
  int sensor_status                        // Flip Flop Input
  )
{
  char cardinal[] = "NESW";
  int i;

  read_timers();
  
  Serial.print("{\"timer\": \"");
  for (i=0; i != 4; i++ )
  {
    if ( sensor_status & (1<<i) )
    {
      Serial.print(cardinal[i]);
    }
    else
    {
      Serial.print('.');
    }
  }
  
  Serial.print("\", ");
  Serial.print("\"N\":");       Serial.print(timer_value[N]);                     Serial.print(", ");
  Serial.print("\"E\":");       Serial.print(timer_value[E]);                     Serial.print(", ");
  Serial.print("\"S\":");       Serial.print(timer_value[S]);                     Serial.print(", ");
  Serial.print("\"W\":");       Serial.print(timer_value[W]);                     Serial.print(", ");
  Serial.print("\"V_REF\":");   Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));  Serial.print(", ");
  Serial.print("\"Version\":"); Serial.print(SOFTWARE_VERSION);
  Serial.print("}\r\n");      

  return;
}

/*----------------------------------------------------------------
 *
 * unsigned int hamming()
 *
 * Compute the Hamming weight of the input
 *
 *----------------------------------------------------------------
 *    
 * Add up all of the 1's in the sample.
 * 
 *--------------------------------------------------------------*/

 unsigned int hamming
   (
   unsigned int sample
   )
 {
  unsigned int i;

  i = 0;
  while (sample)
  {
    if ( sample & 1 )
    {
      i++;                  // Add up the number of 1s
    }
    sample >>= 1;
    sample &= 0x7FFF;
  }
  
  if ( read_DIP() & VERBOSE_TRACE )
    {
    Serial.print("\r\nHamming weight: "); Serial.print(i);
    }

 /*
  * All done, return
  */
  return i;
 }
 
/*----------------------------------------------------------------
 *
 * Patches for Version 2.99
 *
 *----------------------------------------------------------------
 
/*----------------------------------------------------------------
 *
 * void poll_minion
 *
 * Poll the minion for clock data
 *
 *----------------------------------------------------------------
 * 
 * The minion uses a very simple protocol to transfer the
 * timing information off of the offset sample clocks
 * 
 * The messages comes back as :north, east, south, west, voltage
 *    
 *--------------------------------------------------------------*/
#define P_q 0         // Wait for : to come back
#define P_n 1         // Wait for north value to come back
#define P_e 2         // Wait for east value to come back
#define P_s 3         // Wait for south value to come back
#define P_w 4         // Wait for west value to come back
#define P_v 5         // Wait for V_REF value to come back
#define P_x 6         // Exit state
#define WDT_TIME 1000 // Watchdog timer value, 1000ms
#define START_SENTINEL  ':'
#define SPACE           ' '
#define COMMA           ','
#define QUERY           '?'
#define BANG            '!'

bool poll_minion(void)
{
  unsigned int  p_state;            // Operating state
  unsigned int  wdt;                // watch dog timer
  char          ch;                 //
  unsigned int  i;

  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print("\r\npoll_minion: ");
  }

  for (i=N; i <= W; i++)
  {
    minion_value[i] = 0;            // Reset the old counts
  }
  minion_ref = 0;
  
/*
 * Loop waiting for data to come back
 */
  wdt = 0;
  p_state = P_q;
  i=0;
  while ( p_state != P_x )          // Wait here for the state to be completed
  {
    if ( (wdt % 100) == 0 )
    {
      MINION_SERIAL.write(QUERY);
    } 
    
    if ( MINION_SERIAL.available() != 0 )
    {
      ch = MINION_SERIAL.read();       // Read the input depending on the state
      if ( read_DIP() & VERBOSE_TRACE )
      {
        Serial.print(ch);
      }
      if ( ch == COMMA )
      {
        continue;
      }

      if ( ch == BANG )
      {
        break;
      }
      
      if ( ch != SPACE )
      {
        switch ( p_state )
        {
          default:
          case P_q:                   // Wait for the start sentinel to arrive
            if ( ch == START_SENTINEL )
            {
              p_state = P_n;
              break;
            }
            break;

          case P_n:                   // Bring in the timer
          case P_e:
          case P_s:
          case P_w:
            minion_value[p_state - P_n] *= 10;// Read in the value and
            minion_value[p_state - P_n] += ch - '0';// save it
            break;
          
          case P_v:                   // Bring in the voltage
            minion_ref *= 10;         // Read in the value and
            minion_ref += ch - '0';   // save it
          break;

        case P_x:
          break;
        }
      } // end if (space)
    }   // end if (available)
    else
    {
      delay (100);
      wdt += 100;
    }
    
    if ( wdt >= WDT_TIME )
    {
      return false;
    }
  } // end while

/*
 * The data is in
 */
  return true;
  
}

/*----------------------------------------------------------------
 *
 * void send_BOSS
 *
 * Send clock data to the master
 *
 *----------------------------------------------------------------
 * 
 * The minion will send the clock data to the master when
 * it receives a command.
 * 
 * Messsage ->   :north, east, south, west, voltage!
 *--------------------------------------------------------------*/
 void send_BOSS(void)
 {
    char ch;
    unsigned int wdt;

  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print("\r\nsend_BOSS()");
  } 
 /*
  * Wait here until queried. Then send the message
  */
  wdt = 0;
  while ( wdt <= WDT_TIME )
  {
    if ( MINION_SERIAL.available() )
    {
      ch = MINION_SERIAL.read();

      if ( ch == QUERY )                  // Send the message
      {
        MINION_SERIAL.print(START_SENTINEL);
        MINION_SERIAL.print(timer_value[N]);
        MINION_SERIAL.print(',');
        MINION_SERIAL.print(timer_value[E]);
        MINION_SERIAL.print(',');
        MINION_SERIAL.print(timer_value[S]);
        MINION_SERIAL.print(',');
        MINION_SERIAL.print(timer_value[W]);
        MINION_SERIAL.print(',');
        MINION_SERIAL.print(analogRead(V_REFERENCE)); 
        MINION_SERIAL.print(BANG);
        while ( MINION_SERIAL.available() )
        {
          MINION_SERIAL.read();
        }
        if ( read_DIP() & VERBOSE_TRACE )
        {
          Serial.print(" - Done");
        } 
        return;
      }
    }
    else
    {
      delay(100);                           // Wait
      wdt += 100;                           // but not too long
    }
  }

 /*
  * All gone
  */
  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print(" - Failed");
  } 
  return;
 }
