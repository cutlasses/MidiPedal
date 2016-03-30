#include <Bounce.h>

#define MIDI_CHANNEL        1
#define MIDI_CLOCK          248
#define MIDI_START          250
#define MIDI_CONTINUE       251
#define MIDI_STOP           252
#define PULSES_PER_QUARTER  24

#define NUM_PEDAL_SWITCHES  4

#define MIDI_CLOCK_LED      21     

#define LED_BRIGHTNESS      150

#define BOUNCE_TIME         10

int midi_counter(0);

class PEDAL_SWITCH
{
  int                   m_pedal_pin;
  int                   m_led_pin;
  int                   m_midi_cc;
  bool                  m_is_toggle;
  bool                  m_is_active;
  bool                  m_led_is_toggle;    // allow led to toggle even if setup for momentary for midi purposes (to allow for Ableton Arm needing momentary even though it's really a toggle)
  bool                  m_led_active;
  int                   m_sister_led_index;

  Bounce                m_bounce;


  public:

  PEDAL_SWITCH( int pedal_pin, int led_pin, int midi_cc, bool is_toggle, bool led_is_toggle, int sister_led_index = -1 ) :
    m_pedal_pin( pedal_pin ),
    m_led_pin( led_pin ),
    m_midi_cc( midi_cc ),
    m_is_toggle( is_toggle ),
    m_is_active( false ),
    m_led_is_toggle( led_is_toggle ),
    m_led_active( false ),
    m_sister_led_index( sister_led_index ),
    m_bounce( pedal_pin, BOUNCE_TIME )
  {
  }

  bool led_active() const
  {
    return m_led_active;
  }

  int sister_led_index() const
  {
    return m_sister_led_index;
  }

  void deactivate_led()
  {
    m_led_active = false;

    analogWrite( m_led_pin, 0 );
  }

  void setup()
  {
    pinMode( m_pedal_pin, INPUT_PULLUP );
    pinMode( m_led_pin, OUTPUT );
  }

  void update()
  {
    bool prev_led_state = m_led_active;
    
    m_bounce.update();

    if( m_bounce.fallingEdge() )
    {
       // update midi
       if( m_is_toggle )
       {        
        m_is_active = !m_is_active;
        
        if( m_is_active )
        {
          usbMIDI.sendControlChange( m_midi_cc, 127, MIDI_CHANNEL );
        }
        else
        {
          usbMIDI.sendControlChange( m_midi_cc, 0, MIDI_CHANNEL );
        }
       }
       else
       {
         m_is_active = true;
         usbMIDI.sendControlChange( m_midi_cc, 127, MIDI_CHANNEL );
       }

       // update led state
       if( m_led_is_toggle )
       {
        m_led_active = !m_led_active;
       }
       else
       {
         m_led_active = true;
       }
    }
    else if( m_bounce.risingEdge() )
    {
      if( !m_is_toggle )
      {
        m_is_active = false;

        usbMIDI.sendControlChange( m_midi_cc, 0, MIDI_CHANNEL );
      }

      if( !m_led_is_toggle )
      {
        m_led_active = false;
      }
    }

    if( prev_led_state != m_led_active )
    {
      if( m_led_active )
      {
        analogWrite( m_led_pin, LED_BRIGHTNESS );
      }
      else
      {
        analogWrite( m_led_pin, 0 );
      }
    }    
  }
};

PEDAL_SWITCH pedals[ NUM_PEDAL_SWITCHES ] = { PEDAL_SWITCH( 0, 23, 16, false, false ),
                                              PEDAL_SWITCH( 1, 22, 17, false, true, 2 ),
                                              PEDAL_SWITCH( 2, 20, 18, false, true, 1 ),
                                              PEDAL_SWITCH( 3, 19, 19, false, false ) };

void setup()
{
  pinMode( MIDI_CLOCK_LED, OUTPUT );
  
  for( int i = 0; i < NUM_PEDAL_SWITCHES; ++i )
  {
    PEDAL_SWITCH& pedal = pedals[i];
    pedal.setup();    
  }
  
  usbMIDI.setHandleRealTimeSystem( midi_real_time_event );

  midi_counter = 0;
}

void midi_real_time_event( byte data )
{
  switch( data )
  {
    case MIDI_CLOCK:
    {
      ++midi_counter; 
  
      if( midi_counter == PULSES_PER_QUARTER )
      {
        midi_counter = 0;
        digitalWrite( MIDI_CLOCK_LED, HIGH );
      }
      else if( midi_counter == PULSES_PER_QUARTER / 2 )
      {
        digitalWrite( MIDI_CLOCK_LED, LOW );
      }

      break;
    }
    case MIDI_START:
    case MIDI_CONTINUE:
    {
      midi_counter = 0;
      
      digitalWrite( MIDI_CLOCK_LED, LED_BRIGHTNESS );

      break;
    }
    case MIDI_STOP:
    {
      digitalWrite( MIDI_CLOCK_LED, 0 );
      
      break;
    }
  }
}

void loop()
{
  usbMIDI.read();

  for( int i = 0; i < NUM_PEDAL_SWITCHES; ++i )
  {
    PEDAL_SWITCH& pedal = pedals[i];

    pedal.update();

    if( pedal.led_active() && pedal.sister_led_index() != -1 )
    {
      PEDAL_SWITCH& sister_pedal = pedals[ pedal.sister_led_index() ];
      sister_pedal.deactivate_led();
    }
  }
}
