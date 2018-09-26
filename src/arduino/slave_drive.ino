#include <EasyTransfer.h>
#include <avr/interrupt.h>

volatile unsigned long start_time[4], stop_time[4];
volatile bool motor_dir[4];

unsigned short period[4];
unsigned short comp_period[4];

float alpha = 0.99; // 0.9

EasyTransfer ETout_drive;

struct DRIVE_DATA_STRUCTURE {
  unsigned short filter_period[4];
};

DRIVE_DATA_STRUCTURE tx_drive;

void setup() {
  Serial.begin(115200);
  
  ETout_drive.begin(details(tx_drive), &Serial);

  cli();  // Turn off interrupts temporarily

  PCICR |= 0b00000101;    // turn on ports b and d
  PCMSK0 |= 0b00000001;   // turn on pins PB0 and PB1 -- PCINT0 & PCINT1, physical ATMEGA pin 14 & 15, UNO pins 8 & 9
  PCMSK2 |= 0b01000000;   // Pins for Port D instead

  sei();  // Re-open interrupts

  attachInterrupt(0, encoderA, CHANGE);
  attachInterrupt(1, encoderB, CHANGE);
}

void loop() {
  //Serial.print(pulse_count / (11.6666666667 * 70 * 1.02)); // 1 pulse = 2 Interrupt Lo -> Hi & Hi -> Lo
  // PG71 Incremental Encoder: 7 pulses per rev
  // Gear Overduction: Factor of 1.2 Faster output --- (2 * 7) / 1.2 = 11.66667

  for (int i = 0; i < 4; i++) {
    if (stop_time[i] > start_time[i]) period[i] = stop_time[i] - start_time[i];   // Measure period between pulse (Time duration LOW or HIGH)
    else period[i] = start_time[i] - stop_time[i];
    
    if (period[i] > 15000) period[i] = 15000;   // Cap period to 15000 micros or 15 ms -- MAX 16 bit = 65,536 Decimal

    comp_period[i] = alpha * (comp_period[i]) + ((1 - alpha) * period[i]);    // Low pass filter to attenuate spike measurements
    tx_drive.filter_period[i] = comp_period[i];

    ETout_drive.sendData();
  }
}

/////
// Caution: 1st and 3rd encoders are flipped currently!!
/////

ISR(PCINT0_vect) {
  if (PINB & (0x01 << 0)) {
    start_time[3] = micros();

    if (PINC & (0x01 << 3)) motor_dir[3] = 1;
    else motor_dir[3] = 0;
  }

  else {
    stop_time[3] = micros();

    if (PINC & (0x01 << 3)) motor_dir[3] = 0;
    else motor_dir[3] = 1;
  }
}

ISR(PCINT2_vect) {    // Port D, PCINT16 - PCINT23
  if (PIND & (0x01 << 6)) {
    start_time[2] = micros();

    if (PINC & (0x01 << 2)) motor_dir[2] = 1;
    else motor_dir[2] = 0;
  }
  else {
    stop_time[2] = micros();

    if (PINC & (0x01 << 2)) motor_dir[2] = 0;
    else motor_dir[2] = 1;
  }
}

void encoderA() {
  if (PIND & (0x01 << 2)) {
    start_time[0] = micros();

    if (PINC & (0x01 << 0)) motor_dir[0] = 1;
    else motor_dir[0] = 0;
  }

  else {
    stop_time[0] = micros();

    if (PINC & (0x01 << 0)) motor_dir[0] = 0;
    else motor_dir[0] = 1;
  }
}

void encoderB() {
  if (PIND & (0x01 << 3)) {
    start_time[1] = micros();

    if (PINC & (0x01 << 1)) motor_dir[1] = 1;
    else motor_dir[1] = 0;
  }

  else {
    stop_time[1] = micros();

    if (PINC & (0x01 << 1)) motor_dir[1] = 0;
    else motor_dir[1] = 1;
  }
}

