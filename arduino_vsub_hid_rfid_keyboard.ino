
#include <UsbDevice.h>
#include <avr/wdt.h>

typedef struct {
  uint8_t modifier;
  uint8_t reserved;
  uint8_t keycode[6];
} keyboard_report_t;

static keyboard_report_t keyboard_report; // sent to PC
volatile static uchar LED_state = 0xff; // received from PC
static uchar idleRate, state, rstate = 0, icounter = 0;
static uint16_t multiplexer =0, button_release_counter = 0; // repeat rate for keyboards
static char message[20]; 
static uchar imsg = 0, iSzMsg = 0;

boolean stringComplete = false;  // whether the string is complete

#define SZ_MSG 20
#define SZ_KEYCODE 6
#define NUM_LOCK 1
#define CAPS_LOCK 2
#define SCROLL_LOCK 4

#define STATE_WAIT 0
#define STATE_SEND_KEY 1
#define STATE_RELEASE_KEY 2
#define STATE_RESET 9

#define BUZZER_DELAY        500
#define BUZZER_PIN          8
#define STDBY_TOGGLE_PIN    7
#define READY_TOGGLE_PIN    6
#define SUCCESS             0
#define STD_BY              2
#define STDBY_REPETITION    2
#define FAIL_REPETITION     4

#ifdef __cplusplus
extern "C"{
#endif 

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (usbRequest_t *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        switch(rq->bRequest) {
        case USBRQ_HID_GET_REPORT: // send "no keys pressed" if asked here
            // wValue: ReportType (highbyte), ReportID (lowbyte)
            usbMsgPtr = (unsigned char *)&keyboard_report; // we only have this one
            keyboard_report.modifier = 0;
            keyboard_report.keycode[0] = 0;
            return sizeof(keyboard_report);
    case USBRQ_HID_SET_REPORT: // if wLength == 1, should be LED state
            return (rq->wLength.word == 1) ? USB_NO_MSG : 0;
        case USBRQ_HID_GET_IDLE: // send idle rate to PC as required by spec
            usbMsgPtr = &idleRate;
            return 1;
        case USBRQ_HID_SET_IDLE: // save idle rate as required by spec
            idleRate = rq->wValue.bytes[1];
            return 0;
        }
    }

    return 0; // by default don't return any data
}

usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len) {
  
  return 1; // Data read, not expecting more
}
#ifdef __cplusplus
} // extern "C"
#endif

void ringBuzzer(uint8_t);
void (*resetFunc)(void) = 0;

void convertUsbKeycode( uchar send_key )
{
  if( send_key == '\n')
  {
    keyboard_report.modifier = 0;
    keyboard_report.keycode[0] = 0x28;
    return;
  }

  switch( send_key )
  {
    case '0':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x27;
      break;

    case '1':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x1E;
      break;
      
    case '2':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x1F;
      break;

    case '3':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x20;
      break;
      
    case '4':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x21;
      break;

    case '5':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x22;
      break;
      
    case '6':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x23;
      break;

     case '7':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x24;
      break;

     case '8':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x25;
      break;

     case '9':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x26;
      break;

     case '\n':
      keyboard_report.modifier = 0;
      keyboard_report.keycode[0] = 0x28;
      break;
 
    case 'A':
      keyboard_report.modifier = 2; // shift key
      keyboard_report.keycode[0] = 0x04;
      break;

    case 'B':
      keyboard_report.modifier = 2; // shift key
      keyboard_report.keycode[0] = 0x05;
      break;
      
    case 'C':
      keyboard_report.modifier = 2; // shift key
      keyboard_report.keycode[0] = 0x06;
      break;
    
    case 'D':
      keyboard_report.modifier = 2; // shift key
      keyboard_report.keycode[0] = 0x07;
      break;
      
    case 'E':
      keyboard_report.modifier = 2; // shift key
      keyboard_report.keycode[0] = 0x08;
      break;
      
    case 'F':
      keyboard_report.modifier = 2; // shift key
      keyboard_report.keycode[0] = 0x09;
      break;

    case 'a':
      keyboard_report.modifier = 0; // shift key
      keyboard_report.keycode[0] = 0x04;
      break;

    case 'b':
      keyboard_report.modifier = 0; // shift key
      keyboard_report.keycode[0] = 0x05;
      break;
      
    case 'c':
      keyboard_report.modifier = 0; // shift key
      keyboard_report.keycode[0] = 0x06;
      break;
    
    case 'd':
      keyboard_report.modifier = 0; // shift key
      keyboard_report.keycode[0] = 0x07;
      break;
      
    case 'e':
      keyboard_report.modifier = 0; // shift key
      keyboard_report.keycode[0] = 0x08;
      break;
      
    case 'f':
      keyboard_report.modifier = 0; // shift key
      keyboard_report.keycode[0] = 0x09;
      break;

    default:
      keyboard_report.modifier = 0; // shift key
      keyboard_report.keycode[0] = 0; // ?
  }
}

// Now only supports letters 'a' to 'z' and 0 (NULL) to clear buttons
void buildReport(uchar send_key) {
  
  if( send_key == NULL )
  {
    keyboard_report.modifier = 0;
    keyboard_report.keycode[0] = 0;
  }
  else
  {
    convertUsbKeycode(send_key);
  }
}

void send_message()
{
  if(imsg > iSzMsg || imsg == SZ_MSG)
  {
    // reset message
    for(uchar i=0; i <= 2; i++)
      message[i] = 0;
    
    imsg = 0;
    iSzMsg = 0;

    buildReport(NULL);
    stringComplete = false;
  }
  else
  {
    buildReport(message[ imsg ] );
    imsg++;
  }
}

void setup() {
  
  pinMode(BUZZER_PIN, OUTPUT);
  UsbDevice.begin();
  state = STATE_SEND_KEY;
  reinit_serial();  
}

void reinit_serial()
{
  // initialize serial:
  Serial.begin(9600 );
  ringBuzzer(STD_BY);
}

void loop() {
  // put your main code here, to run repeatedly:
  wdt_reset();
  usbPoll();
      
  if( usbInterruptIsReady() && stringComplete )
  {
    if( state != STATE_WAIT )
    {
      switch(state) {
        case STATE_SEND_KEY:
          PORTC |= (1<<4);
          send_message();
          break;
        case STATE_RELEASE_KEY:
          state = STATE_WAIT;
          return; 
          //resetFunc(); // @todo reflash!
          break;
        default:
          state = STATE_WAIT; // should not happen
      }
      usbSetInterrupt((unsigned char*)&keyboard_report, sizeof(keyboard_report));
    }
  }
  else
  {
    serialEvent();
  }
}

void serialEvent() {
  icounter = 0;
  while (Serial.available() && !stringComplete) {
    
    // get the new byte:
    char inChar = (char)Serial.read();

    message[iSzMsg] = inChar;
    
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\0' || inChar == '\n') {
      message[iSzMsg] = '\n';
      stringComplete = true;
    }
    
    iSzMsg++;

    if( iSzMsg == SZ_MSG ){
      message[iSzMsg] = '\0';
      stringComplete = true;
    }
  }
}

void ringBuzzer(uint8_t mode = STD_BY){
   
  if(mode == SUCCESS){
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUZZER_DELAY);
    digitalWrite(BUZZER_PIN, LOW);
  }
  else if(mode == STD_BY){

    for(uint8_t iLoop = 0; iLoop < STDBY_REPETITION; iLoop++){

      digitalWrite(BUZZER_PIN, HIGH);
      delay(BUZZER_DELAY);
      digitalWrite(BUZZER_PIN, LOW);
      delay(BUZZER_DELAY);
    }
  }
  else{
    for(uint8_t iLoop = 0; iLoop < FAIL_REPETITION; iLoop++){

      digitalWrite(BUZZER_PIN, HIGH);
      delay(BUZZER_DELAY);
      digitalWrite(BUZZER_PIN, LOW);
      delay(BUZZER_DELAY);
    }
  }
}
