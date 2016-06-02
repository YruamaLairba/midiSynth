/*

 */
#include "periodes.h"

//save last ouput toggle
unsigned long previousMicros = 0;

//interval between toggle, nul interval is used when no note played
int interval = 0 ;

//last played note, negative is invalid
char playedNote = -1;

const int outPin = 9; //pin used for speaker
int outState = LOW; // status of output

//status
char midiStatus = 0;

//Number of data bytes received since last received status
//or since last complete message.
unsigned char dataNumber = 0;

//Buffer of received data bytes since last received status
//or since last complete message.
char data[2];

void setup() {
  pinMode(outPin, OUTPUT);
  dataNumber = 0;
  //set MIDI baud rate
  Serial.begin(31250);
}

void loop() {
  if(Serial.available()){
    char inByte = Serial.read();
    //if we get a status byte, chnage the midi status and reset 
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
            interval = periodes[playedNote%12]>>(playedNote/12);
          }
          dataNumber = 0;
        }
      }  
    }   
  }
  unsigned long currentMicros = micros();
  if ((currentMicros - previousMicros >= (interval)) && (interval!=0)) {
    //save last time we toggle
    previousMicros = currentMicros;
    
    //toggle the outState
    if (outState == LOW) {
      outState = HIGH;
    }
    else {
      outState = LOW;
    }  
  }
  //write the outState
  digitalWrite(outPin, outState); 
}

