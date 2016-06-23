/*

 */
#include <math.h> //for pow function

//interval between toggle, nul interval is used when no note played
unsigned int interval = 0 ;

//last played note, negative is invalid
char playedNote = -1;

const int outPin = 9; //pin used for speaker
int outState = LOW; // status of output

//status
char midiStatus = 0;

//timbre. we will change duty cycle
unsigned char timbre=127;

//pitchBend. beetween -8192 and 8191
int pitchBend = 0;
//scale for pitch bend
//float pitchBendScale = 0.000244140625 ;
float pitchBendScale = 12.0/8191.0  ;


//Number of data bytes received since last received status
//or since last complete message.
unsigned char dataNumber = 0;

//Buffer of received data bytes since last received status
//or since last complete message.
char data[2];

void setup() {
  /*** configuration of timer 1  ***/
  
  /* ---TCCR1A---
  7: COM1A1
  6: COM1A0
  5: COM1B1
  4: COM1B0
  3: - read only, always use 0
  2: - read only, always use 0
  1: WGM11 Waveform Generation Mode
  0: WGM10 Waveform Generation Mode
  */       //76543210
  TCCR1A = 0b11100010;
  
  /* ---TCCR1B---
  7: ICNC1: Input Capture Noise Canceler (0 = disable)
  6: ICES1: Input Capture Edge Select (0 = Falling)
  5: - read only,always zero
  4: WGM13 Waveform Generation Mode
  3: WGM12 Waveform Generation Mode
  2: CS12 clock select bit
  1: CS11 clock select bit
  0: CS10 clock select bit
  */       //76543210
  TCCR1B = 0b00011010;
  
  DDRB &= ~(1<<1);//set PINB1 as input (HIZ), pin 9 on UNO
  DDRB &= ~(1<<2);//set PINB2 as input (HIZ), pin 10 on UNO
  ICR1= 0xFFFF;//control frequency in current WGM mode
  OCR1A = ICR1/2;//duty cycle for PINB1
  OCR1B = ICR1/2;//duty cycle for PINB2
  

  dataNumber = 0;
  //set MIDI baud rate
  Serial.begin(31250);
}

void loop() {
  if(Serial.available()){
    char inByte = Serial.read();
    //if we get a status byte, change the midi status and reset 
    //Number of data bytes.
    if(inByte & 0b10000000){
      midiStatus = inByte;
      dataNumber = 0;
    }
    //else we get a data byte
    else{
      //if there is place in buffer, store data byte
      if(dataNumber <2){
        data[dataNumber]=inByte;
        dataNumber++; 
      }
      //when we have 2 bytes in data bytes buffer
      if(dataNumber == 2){
        //when we have a note off message
        if((midiStatus & 0b11110000) == 0b10000000){
          //if note off correspond to note acually played, stop
          if(playedNote == data[0]){
            DDRB &= ~(1<<1);//PINB1 as input (HIZ), pin 9 on UNO
            interval = 0;
            playedNote = -1;
          }
          dataNumber = 0;
        }
        //when we have a note on message
        else if((midiStatus & 0b11110000) == 0b10010000){
          //when we have a velocity of 0, we
          //have in fact a note off message
          if (data[1] ==  0){
            //if note off correspond to note acually played, stop
            if(playedNote == data[0]){
              DDRB &= ~(1<<1);//PINB1 as input (HIZ), pin 9 on UNO
              interval = 0;
              playedNote = -1;
            }
          }
          //normal note on message
          else{
            /*
            //play only defined note, ignore other
            if (data[0] < periodesSize){
              playedNote =  data[0];
              interval = periodes[playedNote];
            }*/
            playedNote = data[0];
            //pitch after applying pitchBend
            float pitch = (float)playedNote + ((float)pitchBend*pitchBendScale);
            interval = 64792.6340465701 * pow(2.0, (- pitch/12.0));
            ICR1 = (interval-1); //set frequency
            /* set duty cycle. the cast are to avoid overflow */
            OCR1A = ((unsigned long)interval*(unsigned long)timbre)/256;
            TCNT1 = 0;//reset timer 
            DDRB |= (1<<1);//PINB1 as output, pin 9 on UNO
          }
          dataNumber = 0;
        }
        //when we have a control change message and Channel Mode Messages
        else if ((midiStatus & 0b11110000) == 0b10110000){
          //when we the control change corresponde to "timbre"
          if (data[0] == 0x47){
            timbre=data[1];
            /* set duty cycle. the cast are to avoid overflow */
            OCR1A = ((unsigned long)interval*(unsigned long)timbre)/256;
          }
          //All Sound Off, all note off
          else if ((data[0]==120 || (data[0]<=123 && data[0] >= 127)
                    && data[1]==0)){
            DDRB &= ~(1<<1);//PINB1 as input (HIZ), pin 9 on UNO
            interval = 0;
            playedNote = -1;
          }
          //reset all controllers
          else if (data[0]==121 && data[1]==0){
            timbre=127;
            OCR1A = ((unsigned long)interval*(unsigned long)timbre)/256;
          }
          dataNumber = 0;
        }
        //when we have a pich bend message
        else if ((midiStatus & 0b11110000) == 0b11100000){
          /* 
          concatenate data byte from the right and
          substract to center dynamic on 0
          */
          pitchBend = ((data[1]<<7) | (data[0]<<0))-8192;
          float pitch = (float)playedNote + ((float)pitchBend*pitchBendScale);
          interval = 64792.6340465701 * pow(2.0, (- pitch/12.0));
          ICR1 = (interval-1); //set frequency
          OCR1A = ((unsigned long)interval*(unsigned long)timbre)/256;
        }
      }
    }
  }
  /*
  */
}

