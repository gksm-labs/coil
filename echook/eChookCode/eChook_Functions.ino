/**
 * @brief Performs all setup routines for the eChook board.
 * Initializes pins, debounces, serial communication, Bluetooth, EEPROM, and reference voltage.
 */
void eChookSetup() {
  pinSetup();  // Sets Input/Output for all pins. Function found in eChook_Functions.ino.

  // Initialise debounce objects for the three buttons
  cycleButtonDebounce.attach(CYCLE_BTN_IN_PIN);
  cycleButtonDebounce.interval(50);  // 50ms

  launchButtonDebounce.attach(LAUNCH_BTN_IN_PIN);
  launchButtonDebounce.interval(50);

  brakeButtonDebounce.attach(BRAKE_IN_PIN);
  brakeButtonDebounce.interval(50);


#ifdef NANO_EVERY
  // Starts USB Serial as well on Arduino Nano Every at the same baud rate as set for Bluetooth
  Serial.begin(CAL_BT_BAUDRATE);
  Serial.println(("\n\n\neChook Nano Starting Setup"));
  Serial.print(("Firmware Version: "));
  Serial.println(CODE_VERSION);
#endif

  SerialA.begin(CAL_BT_BAUDRATE);  // Nano Clone - Bluetooth and USB communications, Nano Every - BT Only.

  EEPROMSetup();

  referenceVoltage = updateReferenceVoltage();

#ifdef NANO_EVERY
  Serial.print(("\nSetup complete in "));
  Serial.print(millis());
  Serial.println(("ms."));
#endif
}

/**
 * @brief Configures pin modes and sets up interrupts for the board.
 */
void pinSetup() {

#ifdef JUMPER_I2C
  TEMP1_IN_PIN = A1;
  TEMP2_IN_PIN = A6;
#endif

  // Set up pin modes for all inputs and outputs
  pinMode(MOTOR_OUT_PIN, OUTPUT);
  digitalWrite(MOTOR_OUT_PIN, LOW);  // Ensure motor is not driven on startup

  pinMode(VBATT_IN_PIN, INPUT);
  pinMode(VBATT1_IN_PIN, INPUT);
  pinMode(THROTTLE_IN_PIN, INPUT);
  pinMode(AMPS_IN_PIN, INPUT);
  pinMode(TEMP1_IN_PIN, INPUT);
  pinMode(TEMP2_IN_PIN, INPUT);

  pinMode(LAUNCH_BTN_IN_PIN, INPUT_PULLUP);
  pinMode(CYCLE_BTN_IN_PIN, INPUT_PULLUP);
  pinMode(BRAKE_IN_PIN, INPUT_PULLUP);  // input type will depend on implementation of brake light

#ifdef JUMPER_BT_EN
  pinMode(BT_EN_PIN, OUTPUT);
  digitalWrite(BT_EN_PIN, HIGH);
#endif


  /**
   * Set up Interrupts:
   * When the specified digital change is seen on a the interrupt pin it will pause the main loop and
   * run the code in the Interrupt Service Routine (ISR) before resuming the main code.
   * The interrupt number is not the pin number on the arduino Nano. For explanation see here:
   * https://www.arduino.cc/en/Reference/AttachInterrupt
   */

#ifdef NANO_EVERY
  attachInterrupt(2, motorSpeedISR, RISING);
  attachInterrupt(3, wheelSpeedISR, RISING);
#else
  attachInterrupt(0, motorSpeedISR, RISING);
  attachInterrupt(1, wheelSpeedISR, RISING);
#endif
}

/**
 * @brief Updates sensor readings and sends data at defined intervals.
 * Handles periodic updates for throttle, voltage, current, temperature, speed, and gear ratio.
 */
void eChookRoutinesUpdate() {

  // We want to check different variables at different rates. For most variables 0.25 seconds will be good for logging and analysis.
  // Certain variables either can't have or do not need this resolution.
  // Wheel and Motor speed are accumulated over time, so the longer time left between samples, the higher the resolution of the value.
  // As such, these are only updated ever 1 second. Temperature is a reading that will not change fast, and consumes more processing
  // time to calculate than most, so this is also checked every 1s.

  SerialCheck();

  static unsigned long nextCurrentRegReadMs = millis();
  if (millis() > nextCurrentRegReadMs) {
    nextCurrentRegReadMs += 10;
    regulateMotorCurrent();
  }

  static unsigned long lastShortDataSendTime = millis();              // this is reset at the start so that the calculation time does not add to the loop time
  if (millis() - lastShortDataSendTime > CAL_DATA_TRANSMIT_INTERVAL)  // i.e. if 250ms have passed since this code last ran
  {
    lastShortDataSendTime = millis();
    static unsigned int loopCounter = 0;
    loopCounter = loopCounter + 1;  // This value will loop 1-4, the 1s update variables will update on certain loops to spread the processing time.
    // It is recommended to leave the ADC a short recovery period between readings (~1ms). To achieve this we can transmit the data between readings
    batteryVoltageTotal = readVoltageTotal();
    sendData(VOLTAGE_ID, batteryVoltageTotal);

    batteryVoltageLower = readVoltageLower();
    sendData(VOLTAGE_LOWER_ID, batteryVoltageLower);

    current = readCurrent();
    sendData(CURRENT_ID, current);

    sendData(THROTTLE_VOLTAGE_ID, throttleV);
    sendData(THROTTLE_INPUT_ID, throttleIn);
    sendData(THROTTLE_OUTPUT_ID, throttleOutput);

    motorRPM = readMotorRPM();
    sendData(MOTOR_ID, motorRPM);

    wheelSpeed = readWheelSpeed();
    sendData(SPEED_ID, wheelSpeed);

    referenceVoltage = updateReferenceVoltage();
    sendData(REF_VOLTAGE_ID, referenceVoltage);

    if (loopCounter == 1) {  // Functions to run every 1st loop
      tempOne = readTempOne();
      sendData(TEMP1_ID, tempOne);
      digitalWrite(13, HIGH);  // these are just flashing the LEDs as visual confimarion of the loop
    }

    if (loopCounter == 2) {  // Functions to run every 2nd loop
      tempTwo = readTempTwo();
      sendData(TEMP2_ID, tempTwo);
    }

    if (loopCounter == 3) {  // Functions to run every 3rd loop
      tempThree = readTempInternal();
      sendData(TEMP3_ID, tempThree);
      digitalWrite(13, LOW);
    }

    if (loopCounter == 4) {  // Functions to run every 4th loop
      loopCounter = 0;       // 4 * 0.25 makes one second, so counter resets
      gearRatio = calculateGearRatio();
      sendData(GEAR_RATIO_ID, gearRatio);
    }
  }
}

/**
 * @brief Checks the state of each button and sends updates over Bluetooth if pressed.
 */
void buttonChecks() {  // Checks state of each button, if a press is detected sends the data over bluetooth
  cycleButtonDebounce.update();
  launchButtonDebounce.update();
  brakeButtonDebounce.update();
  static unsigned int cycleButtonPrevious = LOW;                // Track state so that a button press can be detected
  unsigned int cycleButtonState = !cycleButtonDebounce.read();  // Buttons are LOW when pressed, ! inverts this, so state is HIGH when pressed
  if (cycleButtonState != cycleButtonPrevious)                  // Button has changed state - either pressed or depressed
  {
    if (cycleButtonState == HIGH)  // Button Pressed
    {
      sendData(CYCLE_VIEW_ID, 1);
    } else {
      sendData(CYCLE_VIEW_ID, 0);
    }
    cycleButtonPrevious = cycleButtonState;  // Update previous state
  }

  static unsigned int launchButtonPrevious = LOW;                 // Track state so that a button press can be detected
  unsigned int launchButtonState = !launchButtonDebounce.read();  // Buttons are LOW when pressed, ! inverts this, so state is HIGH when pressed
  if (launchButtonState != launchButtonPrevious)                  // Button has changed state - either pressed or depressed
  {
    if (launchButtonState == HIGH)  // Button Pressed
    {
      sendData(LAUNCH_MODE_ID, 1);
    } else {
      sendData(LAUNCH_MODE_ID, 0);
    }
    launchButtonPrevious = launchButtonState;  // Update previous state
  }

  static unsigned int brakeButtonPrevious = LOW;                // Track state so that a button press can be detected
  unsigned int brakeButtonState = !brakeButtonDebounce.read();  // Buttons are LOW when pressed, ! inverts this, so state is HIGH when pressed
  if (brakeButtonState != brakeButtonPrevious)                  // Button has changed state - either pressed or depressed
  {
    if (brakeButtonState == HIGH)  // Button Pressed
    {
      sendData(BRAKE_PRESSED_ID, 100);
    } else if (brakeButtonState == LOW)  // Button Released
    {
      sendData(BRAKE_PRESSED_ID, 0);
    }
    brakeButtonPrevious = brakeButtonState;  // Update previous state
  }
}

/**
 * @brief Updates and returns the reference voltage for ADC calculations. Only works on ATMEGA328 based boards.
 * @return The calculated reference voltage.
 */
float updateReferenceVoltage() {
#ifdef NANO_EVERY
  // TODO - implement properly for Arduino Nano Every
  return CAL_REFERENCE_VOLTAGE;
#else
  // This section of code is exclusive to the ATMEGA328 chip based Arduino Nano Boards.

  // This function uses the internal 1v1 reference to back calucalate the 5V rail voltage.
  // It measures the stable reference voltage, using the 5V rail as the ADC reference, then
  // uses the result to calculate an accurate value for the 5V ADC reference.

  // Set the analog reference to DEFAULT (AVcc == Vcc power rail)
  // REFS1 REFS0          --> 0b01   -Selects DEFAULT (AVcc) reference
  // Set the analog input to channel 14: the INTERNAL bandgap reference (1.1V +/- 10%)
  // MUX3 MUX2 MUX1 MUX0  --> 0b1110 -Selects channel 14, bandgap voltage, to measure
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);

  delay(2);  // Let mux settle a little to get a more stable A/D conversion

  // Start a conversion to measure the INTERNAL reference relative to the DEFAULT (Vcc) reference.
  ADCSRA |= _BV(ADSC);
  // Wait for it to complete
  while (ADCSRA & (1 << ADSC)) {
  };

  // Calculate the power rail voltage (reference voltage) relative to the known voltage
  return (float)((CAL_INTERNAL_REFERENCE_VOLTAGE * 1024UL) / ADC);

#endif
}

/**
 * @brief Reads and calculates the total battery voltage.
 * @return The total battery voltage in volts.
 */
float readVoltageTotal() {                                // Reads in the ADC value for the 24v input, converts it to a voltage, returns the voltage value.
  float tempVoltage = analogRead(VBATT_IN_PIN);           // this will give a 10 bit value of the voltage with 1024 representing the ADC reference voltage of 5V
  tempVoltage = (tempVoltage / 1024) * referenceVoltage;  // This gives the actual voltage seen at the arduino pin, assuming reference voltage of 5v
  tempVoltage = tempVoltage * CAL_BATTERY_TOTAL;          // Gives battery voltage where 6 is the division ratio of the potential divider. NEEDS TUNING!!
  return (tempVoltage);
}

/**
 * @brief Reads and calculates the lower battery voltage.
 * @return The lower battery voltage in volts.
 */
float readVoltageLower() {                                // Reads in the ADC value for the 12v input, converts it to a voltage, returns the voltage value.
  float tempVoltage = analogRead(VBATT1_IN_PIN);          // this will give a 10 bit value of the voltage with 1024 representing the ADC reference voltage of 5V
  tempVoltage = (tempVoltage / 1024) * referenceVoltage;  // This gives the actual voltage seen at the arduino pin, assuming reference voltage of 5v
  tempVoltage = tempVoltage * CAL_BATTERY_LOWER;          // Gives battery voltage where 3 is the division ratio of the potential divider. NEEDS TUNING!!
  return (tempVoltage);
}

/**
 * @brief Reads, smooths, and calculates the Current value.
 * @return The Current in amps.
 */
float readCurrent() {  // Reads in ACC input from the differential amplifier, converts it to the current value and smooths it
  float tempCurrent = analogRead(AMPS_IN_PIN);
  tempCurrent = (tempCurrent / 1024) * referenceVoltage;       // gives voltage output of current sensor.
  tempCurrent = tempCurrent * CAL_CURRENT;                     // calibration value for LEM current sensor on eChook board.
  currentSmoothingArray[currentSmoothingCount] = tempCurrent;  // updates array with latest value
  // The next 5 lines manage the smoothing count for the averaging:
  currentSmoothingCount++;  // increment smoothing count
  if (currentSmoothingCount >= currentSmoothingSetting) {
    currentSmoothingCount = 0;  // if current smoothing count is higher than max, reset to 0
  }
  // Now back to the current calculations:
  tempCurrent = 0;  // reset temp current to receive sum of array values
  for (int i = 0; i < currentSmoothingSetting; i++) {
    tempCurrent += currentSmoothingArray[i];  // sum all values in the current smoothing array
  }
  tempCurrent = tempCurrent / currentSmoothingSetting;  // divide summed value by number of samples to get mean
  return (tempCurrent);                                 // return the final smoothed value
}

/**
 * @brief Reads and processes the throttle input.
 * @return The throttle output as a percentage.
 */
float readThrottle() {
  static int currThrtlOut = 0;
  float tempThrottle = analogRead(THROTTLE_IN_PIN);

  if (CAL_THROTTLE_VARIABLE)  // Analogue throttleOutput, not push button
  {
    tempThrottle = (tempThrottle / 1023) * referenceVoltage;  // Gives the actual voltage seen on the arduino Pin
    throttleV = tempThrottle;                                 // Update Global variable for throttleOutput in voltage
    // SerialA.print(tempThrottle);
    // SerialA.print(", ");
    // The following code adds dead bands to the start and end of the throttleOutput travel
    if (tempThrottle < CAL_THROTTLE_LOW)  // less than 1V
    {
      tempThrottle = CAL_THROTTLE_LOW;
    } else if (tempThrottle > CAL_THROTTLE_HIGH)  // greater than 4 V
    {
      tempThrottle = CAL_THROTTLE_HIGH;
    }

    tempThrottle = ((tempThrottle - CAL_THROTTLE_LOW) / (float)(CAL_THROTTLE_HIGH - CAL_THROTTLE_LOW)) * (255);
  } else {
    throttleV = (tempThrottle / 1023) * referenceVoltage;  // Update Global variable for throttleOutput in voltage
    if (tempThrottle > 200)                                // Approx 1v
    {
      tempThrottle = 255;  // full throttleOutput
    } else {
      tempThrottle = 0;  // No throttleOutput
    }
  }

  throttleIn = (float)tempThrottle / 2.55;  // Convert to a float percentage for the output

  if (CAL_THROTTLE_RAMP) {
    // This code generates a simple ramp up in throttleOutput. The >100 is there as it will likely take about 40% throttleOutput to get the car moving, so this will give a quicker start.
    if (tempThrottle >= currThrtlOut && tempThrottle > 100) {  // This could be if(thrtlIn > thrtlOut && speed < threshold) to make it low speed only. Speed and threshold are undefined in this example!
      if (currThrtlOut < 100) {
        currThrtlOut = 101;
      }
      currThrtlOut = currThrtlOut + 4;    // Value dictates ramp speed. Calculated by (155/x)/10. 4 gives (155/4)/10=3.875 seconds, 2 gives 7.75 seconds, 1 gives 15.5 seconds
      if (currThrtlOut > tempThrottle) {  // Fixes the throttleOutput jitter if the increment puts output over request.
        currThrtlOut = tempThrottle;
      }
    } else {
      currThrtlOut = tempThrottle;
    }

  } else {
    currThrtlOut = tempThrottle;
  }


  if (CAL_THROTTLE_OUTPUT_EN) {
    analogWrite(MOTOR_OUT_PIN, currThrtlOut);  // This drives the motor output. Unless you are using the board to drive your motor you can comment it out.
  } else {
    analogWrite(MOTOR_OUT_PIN, 0);
  }

  return (float)currThrtlOut / 2.55;  // Convert to a float percentage for the output
}

/**
 * @brief Reads and calculates the temperature from thermistor 1.
 * @return The temperature in Celsius.
 */
float readTempOne() {
  float temp = thermistorADCToCelcius(analogRead(TEMP1_IN_PIN), 1);  // use the thermistor function to turn the ADC reading into a temperature
  return (temp);                                                     // return Temperature.
}

/**
 * @brief Reads and calculates the temperature from thermistor 2.
 * @return The temperature in Celsius.
 */
float readTempTwo() {
  float temp = thermistorADCToCelcius(analogRead(TEMP2_IN_PIN), 2);
  return (temp);
}

// Reading the interanl arduino tempreature - notes
// on the accuracy and calibration here: https://playground.arduino.cc/Main/InternalTemperatureSensor/
/**
 * @brief Reads and calculates the internal temperature of the Arduino.
 * @return The internal temperature in Celsius.
 */
float readTempInternal(void) {
#ifdef NANO_EVERY
  // Arduino Nano Every: Read internal temperature sensor
  // Based on ATmega4809 datasheet and Arduino core implementation
  uint16_t adc;
  float temperature;

  // Save current ADC settings
  uint8_t oldSAMPCTRL = ADC0.SAMPCTRL;
  uint8_t oldCTRLC = ADC0.CTRLC;
  uint8_t oldMUXPOS = ADC0.MUXPOS;

  // Configure ADC for internal temperature sensor
  ADC0.CTRLC = ADC_PRESC_DIV4_gc | ADC_REFSEL_INTREF_gc; // 1.1V reference
  ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc; // Select temperature sensor
  ADC0.SAMPCTRL = 0x3F; // Maximum sampling time

  ADC0.COMMAND = ADC_STCONV_bm; // Start conversion
  while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)); // Wait for result ready

  adc = ADC0.RES; // Read ADC result

  // Restore ADC settings
  ADC0.SAMPCTRL = oldSAMPCTRL;
  ADC0.CTRLC = oldCTRLC;
  ADC0.MUXPOS = oldMUXPOS;

  // Convert ADC value to temperature (approximate, not factory calibrated)
  // Formula from ATmega4809 datasheet, section 27.3.1
  // Typical: 1 LSB = 1°C, 300 = 25°C
  temperature = (float)adc - 300.0f;
  temperature += 25.0f;

  return temperature; // Note: This is an approximate value and may need calibration for accuracy.

#else
  unsigned int wADC;
  float t;
  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.
  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;
  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;
  // The offset is specific to each device. This is a general figure
  t = (wADC - 324.31) / 1.22;
  // The returned temperature is in degrees Celsius.
  // Measured offset
  t = t - 7;
  return (t > 0 ? t : 0);
#endif
}

/**
 * @brief Calculates and returns the wheel speed.
 * @return The wheel speed in meters per second.
 */
float readWheelSpeed() {
  if (CAL_WHEEL_MAGNETS > 0) {  // Only run if number of wheel magnets is greater than 0. Protects against a divide by 0 crash.
    float wheelSpeedMetersPerSecond = 0;

    if (CAL_USE_IMPROVED_SPEED_CALCULATION) {  // Two ways to calculate wheel RPM
      // Calculate time taken for last wheel rotation
      float wheelRPS = 0;
      if (millis() - lastWheelPollTime < 2000) {                        // if over 2 seconds since a poll is seen, assume stopped.
        if (lastWheelInterval < 2000) {                                 // catches the case of first pulse in a while
          long fullRotationMs = lastWheelInterval * CAL_WHEEL_MAGNETS;  // Ideally one motor magnet
          wheelRPS = (float)1000 / fullRotationMs;                      // 1 second, divided by time of one rotation at current speed
        }
      } else {
        wheelRPS = 0;
      }
      wheelSpeedMetersPerSecond = wheelRPS * (float)CAL_WHEEL_CIRCUMFERENCE;
    } else {
      static unsigned long lastWheelSpeedPollTime = 0;
      // Counts the number of magnet passesdetected since the last wheel speed check,
      //  converts to a speed in meters per second and returns that value.
      //  Also updates the global wheel RPM value each call.

      // First action is to take readings and reset the wheel count so that there are no change to the variables during the calculations or between reading and resetting:
      int tempWheelPoll = wheelPoll;
      wheelPoll = 0;
      unsigned long tempLastWheelPollTime = lastWheelSpeedPollTime;
      unsigned long tempWheelPollTime = millis();
      lastWheelSpeedPollTime = tempWheelPollTime;
      // Wheel RMP has been tacked into this function at a later date, so could do with re-writing really...
      // Wheel RMP Calculations:
      wheelRPM = (float)tempWheelPoll / (float)CAL_WHEEL_MAGNETS;                                   // gives number of rotations
      wheelRPM = wheelRPM / ((float)(tempWheelPollTime - tempLastWheelPollTime) / (float)60000.0);  // /60,000 converts millis to minutes

      // All integers in the folowing equation are cast to float so that the value is not converted to an integer at any point reducing accuracy through rounding.
      // Next task is to calculate the distance travelled. This is dome by takin the wheel poll, which is the number of magnets that have passed the sensor since the last check
      // and dividing it by the number of wheel magnets to give the number of revolutions, then multiply by the circumference to give distance travelled in meters:
      float wheelDistanceTravelled = (float)tempWheelPoll / (float)CAL_WHEEL_MAGNETS * (float)CAL_WHEEL_CIRCUMFERENCE;
      // Now determine how much time in seconds it took to travel this distance, and divide the distanve by time to get speed in meters per second.
      wheelSpeedMetersPerSecond = wheelDistanceTravelled / ((float)(tempWheelPollTime - tempLastWheelPollTime) / (float)1000.0);  // the /1000 converts the milliseconds to seconds
    }

    // Next section of code handles the smooting:
    speedSmoothingArray[speedSmoothingCount] = wheelSpeedMetersPerSecond;  // adds current speed into oldest position in array
    speedSmoothingCount++;                                                 // incrememnts array position for next reading
    if (speedSmoothingCount >= speedSmoothingSetting) {
      speedSmoothingCount = 0;  // reset if count exceeds array length
    }
    wheelSpeedMetersPerSecond = 0;  // reset variable ready to sum array
    for (int i = 0; i < speedSmoothingSetting; i++) {
      wheelSpeedMetersPerSecond += speedSmoothingArray[i];
    }
    wheelSpeedMetersPerSecond = wheelSpeedMetersPerSecond / speedSmoothingSetting;  // divide summed figure by array count to get mean value
    // SerialA.print("Speed MPS = ");
    // SerialA.println(wheelSpeedMetersPerSecond);
    return (wheelSpeedMetersPerSecond);  // return smoothed value
  } else {
    return (0);  // Num wheel magnets is 0
  }
}

/**
 * @brief Calculates and returns the motor RPM.
 * @return The motor RPM.
 */
float readMotorRPM() {
  if (CAL_MOTOR_MAGNETS > 0) {  // Only run if number of motor magnets is greater than 0. Protects against a divide by 0 crash.
    if (CAL_USE_IMPROVED_RPM_CALCULATION) {
      float tempRpm = 0;
      // Calculate time taken for last wheel rotation
      if (micros() - lastMotorPollTime < 1000000) {                              // If it takes longer than 2s for a rotation, consider it stopped.
        if (lastMotorInterval < 1000000) {                                       // catches the case of first pulse in a while
          unsigned long fullRotationMs = lastMotorInterval * CAL_MOTOR_MAGNETS;  // Ideally one motor magnet
          tempRpm = (float)60000000 / fullRotationMs;                            // 1 minute, divided by time of one rotation at current speed
        }
      } else {
        tempRpm = 0;
      }

      // tempRpm = motorRPM *0.5 + tempRpm *0.5;
      //
      // char buff[50];
      // sprintf(buff, "Motor Interval = %lu", lastMotorInterval);
      // SerialA.println(buff);
      // SerialA.print("RPM = ");
      // SerialA.println(tempRpm);
      // lastMotorInterval = 0;
      return (tempRpm);
    } else {
      static unsigned long lastMotorSpeedPollTime = 0;
      // Counts the number of magnet passesdetected since the last Motor rpm check, converts to revolutions per minute and returns that value
      //  First action is to take copies of the motor poll count and time so that the variables don't change during the calculations.
      int tempMotorPoll = motorPoll;
      motorPoll = 0;
      unsigned long tempLastMotorPollTime = lastMotorSpeedPollTime;
      unsigned long tempMotorPollTime = millis();
      lastMotorSpeedPollTime = tempMotorPollTime;
      // Now calculate the number of revolutions of the motor shaft
      float motorRevolutions = (float)tempMotorPoll / CAL_MOTOR_MAGNETS;
      float motorRevolutionsPerMin = motorRevolutions * 60.0;
      float timeDiffms = tempMotorPollTime - tempLastMotorPollTime;
      float timeDiffs = timeDiffms / 1000.0;
      // Now use the time time passed to convert this to revolutions per minute
      // RMP = (revolutions / latestPollTIme - lastPollTime) / 1000 to convert to Seconds) * 60 to convert to minutes
      float motorShaftRPM = motorRevolutionsPerMin / timeDiffs;
      return (motorShaftRPM);
    }
  } else {
    return (0);  // Num motor magnets is 0
  }
}

void regulateMotorCurrent() {
  float rawVoltage = analogRead(AMPS_IN_PIN);
  float actualAmps = (rawVoltage / 1024.0) * referenceVoltage;
  actualAmps = actualAmps * CAL_CURRENT;

  if (actualAmps > MAX_CURRENT_LIMIT) {
    current_motor_pwm -= 5;

  } else if (actualAmps < (MAX_CURRENT_LIMIT - CURRENT_HYSTERESIS)) {
    current_motor_pwm += 1;
  }

  if (current_motor_pwm > 255) current_motor_pwm = 255;
  if (current_motor_pwm < 0) current_motor_pwm = 0;

  analogWrite(MOTOR_OUT_PIN, current_motor_pwm);

  throttleOutput = (float)current_motor_pwm / 2.55;
}

/**
 * @brief Calculates the gear ratio from motor and wheel RPM.
 * @return The calculated gear ratio.
 */
float calculateGearRatio() {
  float tempGearRatio = 0;
  if (wheelRPM) {
    tempGearRatio = motorRPM / wheelRPM;
  }
  return (tempGearRatio);
}

// Thermistor calculation
// If an active sensor such as a TMP37 is used this has a calibrated voltage output, linear to the temperature change.
// A cheaper option is to use Thermistors. The resistance across a thrmistor changes with temperature, but the change is not linear
// so some maths is required to translate the voltage reading into a temperature value.
// For more information see here: http://playground.arduino.cc/ComponentLib/Thermistor2
// This method uses the Steinhart-Hart equation to calculate the actual temperature, which requires three coefficients,
// A, B and C, that are specific to a thermistor. The ones below are for the thermistors provided with the board, however if you
// use a different thermistor the coefficients should be given in the datasheet, and if not, can be calculated using this calculator:
// http://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm

/**
 * @brief Converts a thermistor ADC reading to Celsius using the Steinhart-Hart equation.
 * @param rawADC The raw ADC value from the thermistor pin.
 * @param thermNumber The thermistor number (1 or 2) to select calibration constants.
 * @return The calculated temperature in Celsius.
 */
float thermistorADCToCelcius(int rawADC, uint8_t thermNumber) {

  // If no sensor is plugged in, rawADC reading will be close to 1023, so return 0.
  if (rawADC > 1000)
    return (0);

  // Steinhart-Hart Coefficients, see comment above
  // These coefficients are for the MF52AT NTC 10k thermistor, however due to thermistor tolerances each thermistor should be calibrated individually.
  float A, B, C;
  if (thermNumber == 1) {
    A = CAL_THERM1_A;
    B = CAL_THERM1_B;
    C = CAL_THERM1_C;
  } else {
    A = CAL_THERM2_A;
    B = CAL_THERM2_B;
    C = CAL_THERM2_C;
  }
  // Value of resistor forming potential divider with Thermistor in ohms.
  const int FIXED_RESISTOR_VALUE = 10000;  // 10k
  // Calculations:
  // The formula is: Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3} where A, B and C are the coefficients above and R is the resistance across the thermistor.
  // First step is to calculate the resistance of the thermistor using the potential divider equation V_out = (R1 + R2)/(R1 * R2)*V_in
  // As R2 is the only unknown variable we can re-write this as: R2 = R1((V_in/V_out)-1).  R2 = (R1 V2)/(V1-V2)
  // As the ADC values are our readings of the voltage, we can substitute V_in with 1024 and V_out with the reading taken from the ADC, which is passed into this function as rawADC
  // This makes the calculation:
  float thermistorResistance = ((float)FIXED_RESISTOR_VALUE * (float)rawADC) / (float)((float)1023 - (float)rawADC);
  // Next, you'll notice that the log natural (ln) of this resistance needs to be calculated 4 times in the Steinhart-Hart equation. This is a complex and long calculation for the arduino.
  // As such it is efficient to do it once and save the result for use later:
  double lnResistance = log(thermistorResistance);
  // Now plug it all into the equation:
  double temperature = 1 / (A + (B * lnResistance) + (C * lnResistance * lnResistance * lnResistance));
  // We now have the temperature in Kelvin. To convert it into Celcius we need to subtract 273.15
  temperature = temperature - 273.15;
  // if (DEBUG_MODE)
  // {
  //         SerialA.print("\n\rThemistore Resistance = ");
  //         SerialA.println(thermistorResistance);
  //         SerialA.print("Temperature = ");
  //         SerialA.println(temperature);
  // }
  // Now return the Celcius Value:
  return (temperature);
}

// BLUETOOTH DATA PACKETING FUNCTIONS
// The two functions in this section handle packeting the data and sending it over USART to the bluetooth module. The two functions are
//  identically named so are called the in the same way, however the first is run if the value passed to it is a float and the second is
//  run if the value passed into it is an integer (an override function). For all intents and purposes you can ignore this and simply call
//  'sendData(identifier, value);' using one of the defined identifiers and either a float or integer value to send informaion over BT.
//
// identifier:  see definitions in the Globals.h file
// value:       the value to send (typically some caluclated value from a sensor)

/**
 * @brief Sends a float value over Bluetooth using a custom packet format.
 * @param identifier The data identifier.
 * @param value The float value to send.
 */
void sendData(char identifier, float value) {
  if (!DEBUG_MODE)  // Only runs if debug mode is LOW (0)
  {
    byte dataByte1;
    byte dataByte2;
    if (value <= 127) {
      // Values under 128 are sent as a float
      // i.e. value = dataByte1 + dataByte2 / 100
      int integer;
      int decimal;
      float tempDecimal;
      integer = (int)value;
      tempDecimal = (value - (float)integer) * 100;
      decimal = (int)tempDecimal;
      dataByte1 = (byte)integer;
      dataByte2 = (byte)decimal;
    } else {
      // Values above 127 are sent as integer
      // i.e. value = dataByte1 * 100 + dataByte2
      int tens;
      int hundreds;
      hundreds = (int)(value / 100);
      tens = value - hundreds * 100;
      dataByte1 = (byte)hundreds;
      // dataByte1 = dataByte1 || 0x10000000; //flag for integer send value
      dataByte1 += 128;
      dataByte2 = (byte)tens;
    }
    // Send the data in the format { [id] [1] [2] }
#ifdef NANO_EVERY
    if (inConfig) {
      Serial.write(123);
      Serial.write(identifier);
      Serial.write(dataByte1);
      Serial.write(dataByte2);
      Serial.write(125);
    }
#endif
    SerialA.write(123);
    SerialA.write(identifier);
    SerialA.write(dataByte1);
    SerialA.write(dataByte2);
    SerialA.write(125);
  } else {
    SerialA.print("Data Out: \t");
    SerialA.print(identifier);
    SerialA.print(",\t");
    SerialA.println(value);
  }
}

/**
 * @brief Sends an integer value over Bluetooth using a custom packet format.
 * @param identifier The data identifier.
 * @param value The integer value to send.
 */
void sendData(char identifier, int value) {
  if (!DEBUG_MODE) {
    byte dataByte1;
    byte dataByte2;
    if (value <= 127) {
      dataByte1 = (byte)value;
      dataByte2 = 0;  // we know there's no decimal component as an int was passed in
    } else {
      int tens;
      int hundreds;
      hundreds = (int)(value / 100);
      tens = value - (hundreds * 100);
      dataByte1 = (byte)hundreds;
      dataByte1 += 128;  // sets MSB High to indicate Integer value
      dataByte2 = (byte)tens;
    }
#ifdef NANO_EVERY
    if (inConfig) {
      Serial.write(123);
      Serial.write(identifier);
      Serial.write(dataByte1);
      Serial.write(dataByte2);
      Serial.write(125);
    }
#endif
    SerialA.write(123);
    SerialA.write(identifier);
    SerialA.write(dataByte1);
    SerialA.write(dataByte2);
    SerialA.write(125);
  } else {

    Serial.print("Data Out: \t");
    Serial.print(identifier);
    Serial.print(",\t");
    Serial.println(value);
  }
}
