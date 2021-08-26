#include <TMC2130Stepper.h>
#include "U8glib.h"

#define MAX_LENGTH 15
#define NMOTORS 3

#define RSENSE 0.11f
#define MULTIPLIER 0.5f

#define X_STEP 54
#define X_DIR 55
#define X_EN 38
#define X_CS 63

#define Y_STEP 60
#define Y_DIR 61
#define Y_EN 56
#define Y_CS 40

#define Z_STEP 46
#define Z_DIR 48
#define Z_EN 62
#define Z_CS 42

TMC2130Stepper steppers[3] = {
    TMC2130Stepper(X_EN, X_DIR, X_STEP, X_CS), // X-STEP = 54, X-DIR = 55, X-EN = 38, X-CS = 63
    TMC2130Stepper(Y_EN, Y_DIR, Y_STEP, Y_CS), // Y-STEP = 60, Y-DIR = 61, Y-EN = 56, Y-CS = 40
    TMC2130Stepper(Z_EN, Z_DIR, Z_STEP, Z_CS)  // Z-STEP = 46, Z-DIR = 48, Z-EN = 62, Z-CS = 42
};

U8GLIB_ST7920_128X64_1X u8g(23, 17, 16);

long stepsTaken[3] = {0, 0, 0};
unsigned int pulse_delay = 1000;

void instructions() {
/*    
 *     Command: "help"
 *     Prints manual
 */
    Serial.println("====================================== Instructions ======================================");
    Serial.println("Command: \"status\" -- Display current status of TMC2130 Stepper Motor Drivers.");
    Serial.println("Command: \"reinit\" -- Attempt to reinitialise TMC2130 Stepper Motor Drivers.");
    Serial.println("Command: \"reset <axis>\" -- Reset <axis> step counter(s). ");
    Serial.println("            <axis> = \"all\", \"X\", \"Y\" or \"Z\".");
    Serial.println("Command: \"ms <axis> <microsteps>\" -- Set <axis> microsteps to <microsteps>. ");
    Serial.println("            <axis> = \"all\", \"X\", \"Y\" or \"Z\".");
    Serial.println("Command: \"curr <axis> <peak_current>\" -- Set <axis> peak current to <peak_current> mA. ");
    Serial.println("            <axis> = \"all\", \"X\", \"Y\" or \"Z\".");
    Serial.println("Command: \"move <axis> <steps>\" -- Move <axis> by <steps> steps. ");
    Serial.println("            <axis> = \"X\", \"Y\" or \"Z\".");
    Serial.println();
    Serial.println("Command: \"help\" -- Display instructions.");
    Serial.println("=========================================================================================");
    Serial.println();
}

void welcome() {
    Serial.println("===============================================");
    Serial.println("== Serial Monitor Motor Controller (TMC2130) ==");
    Serial.println("===============================================");
    Serial.println();
}

bool check_SPI_connection(TMC2130Stepper driver) {
/*
 *    Checks if <driver> SPI connection is established.
 *    
 *    Returns TRUE if <driver> has established SPI connection.
 *    Returns FALSE otherwise.
 */
    uint8_t ver = driver.version();
    switch(ver) {
        case 0:
        case 0xFF: 
            return false;
            break;
        default:
            return true;
            break;
    }
}

bool steppers_init() {
/*
 *    Checks for at least ONE driver successfully establishing SPI connection.
 *    
 *    Returns FALSE if no SPI found.
 *    Returns TRUE otherwise.
 */
    bool ret_val = false;
    for (int i = 0; i < NMOTORS; i++) {
        steppers[i].begin();
        if(check_SPI_connection(steppers[i])) {
            steppers[i].microsteps(64);
            steppers[i].rms_current(600);
            steppers[i].stealthChop(1); 
            ret_val = true;
        }
    }
    return ret_val;
}

void setup() {
    SPI.begin();
    Serial.begin(115200);
    while(!Serial); 

    // print LCD
    main_screen();
  
    // print welcome screen
    welcome();

    Serial.print("Checking for SPI connection... ");
    while(!steppers_init());
    Serial.println("Done!");
    Serial.println();

    instructions();
}

void read_from_serial(char* string) {
/*
 *  This function reads input from Serial as a string, and stores it in the argument "string"
 */
    int index = 0;
    char next = '\0';
    while (next != '\n') { 
        if ((index <= MAX_LENGTH) && Serial.available() > 0) {
            next = Serial.read();
            if(next == '\n') {
                *(string + index) = '\0';
            }
            else{
                *(string + index) = next;
            }
            index++;
        }
        else if (index > MAX_LENGTH) {
            Serial.println("Error: Command exceeds 15 characters");
            *string = "\0";
            break;
        }
    }
}

void check_status() {
/*    
 *     Command: "status"
 *     Checks SPI connection status, microstepping and peak current for all axes.
 */
    for(int i = 0; i < NMOTORS; i++) {
        char axis = (char) i + 88;
        if (check_SPI_connection(steppers[i])) {
            Serial.print(axis);
            Serial.print("-axis SPI OK. ");
            int ms = steppers[i].microsteps();
            Serial.print("Microsteps: ");
            Serial.print(ms);
            Serial.print(". ");
            int current = steppers[i].getCurrent();
            Serial.print("Peak current: ");
            Serial.print(current);
            Serial.println(" mA.");
        }
        else {
            Serial.print(axis);
            Serial.println("-axis SPI not connected! Check power and connection.");
        }
    }
}

void reinit_steppers() {
    for (int i = 0; i < NMOTORS; i++) {
        steppers[i].begin();
        steppers[i].microsteps(64);
        steppers[i].rms_current(600);
        steppers[i].stealthChop(1); 
    }
    Serial.println("Attempted to reinitialize drivers. Use \"status\" to confirm driver status.");
}

void reset_counter(char* axis) {
/*
 *    Command: "reset <axis>"
 *    Resets the step counters. 
 *    
 *    <axis> = "all"  --> reset counters for all axes.
 *    <axis> = "X", "Y" or "Z" --> reset counters for single axis.
 */
    if (strcmp(axis, "all") == 0) {
        for (int i = 0; i < NMOTORS; i++) {
            stepsTaken[i] = 0;          
        }
        Serial.println("All counters reset.");
    }
    else if (strlen(axis) == 1) {
        int axis_index = (int) axis[0] - 88;
        if (axis_index >= 0 && axis_index < NMOTORS) {
            stepsTaken[axis_index] = 0;          
            Serial.print(axis);
            Serial.println("-axis counter reset.");
        }
        else {
            Serial.println("Error: Invalid axis");
        }
    }
    else {
        Serial.println("Error: Incorrect format");
    }
}

void change_ms(char* axis, char* ms) {
/*
 *    Command: "ms <axis> <microsteps>"
 *    Changes the microstep setting. Valid microsteps are: 1, 2, 4, 8, 16, 32, 64, 128, 256.
 *  
 *    <axis> = "all"  --> change microsteps for all axes.
 *    <axis> = "X", "Y" or "Z" --> change microsteps for single axis.  
 */
    int ms_value = atoi(ms);
    bool pow_of_2 = ((ms_value & (ms_value - 1)) == 0);
    if( (ms_value > 0) && (ms_value <= 256) && pow_of_2 ) {
        if (strcmp(axis, "all") == 0){ 
            for (int i = 0; i < NMOTORS; i++) {
                steppers[i].microsteps(ms_value);
            }
            Serial.print("All axes set to ");
            Serial.print(ms_value);
            Serial.println(" microsteps.");
        }
        else if (strlen(axis) == 1) {
            int axis_index = (int) axis[0] - 88;
            if (axis_index >= 0 && axis_index < NMOTORS) {
                steppers[axis_index].microsteps(ms_value);
                Serial.print(axis);
                Serial.print("-axis set to ");
                Serial.print(ms_value);
                Serial.println(" microsteps.");
            }
            else {
                Serial.println("Error: Invalid axis.");
            }
        }
        else {
            Serial.println("Error: Incorect format");
        }
    }
    else if (axis == NULL) {
        Serial.println("Error: Invalid axis.");
    }
    else {
        Serial.println("Error: Invalid microsteps.");
    }
}

void change_current(char* axis, char* curr) {
/*
 *    Command: "curr <axis> <peak_current>
 *    Changes the peak current setting. Peak current restricted to 200-800 mA. 
 *    
 *    <axis> = "all"  --> change peak current for all axes.
 *    <axis> = "X", "Y" or "Z" --> change peak current for single axis.    
 */
    int curr_value = atoi(curr);
    if( (curr_value >= 200) && (curr_value <= 800) ) {
        if (strcmp(axis, "all") == 0){ 
            for (int i = 0; i < NMOTORS; i++) {
                steppers[i].setCurrent(curr_value, MULTIPLIER, RSENSE);
            }
            Serial.print("All axes set to ");
            Serial.print(curr_value);
            Serial.println(" mA peak current.");
        }
        else if (strlen(axis) == 1) {
            int axis_index = (int) axis[0] - 88;
            if (axis_index >= 0 && axis_index < NMOTORS) {
                steppers[axis_index].setCurrent(curr_value, MULTIPLIER, RSENSE);
                Serial.print(axis);
                Serial.print("-axis set to ");
                Serial.print(curr_value);
                Serial.println(" mA peak current.");
            }
            else {
                Serial.println("Error: Invalid axis.");
            }
        }
        else {
            Serial.println("Error: Incorect format");
        }
    }
    else if (axis == NULL) {
        Serial.println("Error: Invalid axis.");
    }
    else {
        Serial.println("Error: Invalid peak current.");
    }
}

void move_stepper(char* axis, char* steps) { 
/*
 *    Command: "move <axis> <steps>" 
 *    Moves a single axis by <steps> steps.
 *    
 *    <axis> = "X", "Y" or "Z".
 */
    long s = atol(steps);
    int sign = 1;
    long steps_to_move = s;

    // checking sign of <steps>
    if (s < 0) {
        sign = -1;
        steps_to_move = -s;
        digitalWrite(X_DIR, !digitalRead(X_DIR));
        digitalWrite(Y_DIR, !digitalRead(Y_DIR));
        digitalWrite(Z_DIR, !digitalRead(Z_DIR));
    }

    if (strlen(axis) == 1) {
        switch(axis[0]) {
            case 'X': 
                digitalWrite(X_EN, LOW);
                for (long i = 0; i < steps_to_move; i++) {
                    digitalWrite(X_STEP, HIGH);
                    delayMicroseconds(10);
                    digitalWrite(X_STEP, LOW);
                    delayMicroseconds(10 + pulse_delay);
                    stepsTaken[0] += sign;
                }
                digitalWrite(X_EN, HIGH);
                break;
            case 'Y': 
                digitalWrite(Y_EN, LOW);
                for (long i = 0; i < steps_to_move; i++) {
                    digitalWrite(Y_STEP, HIGH);
                    delayMicroseconds(10);
                    digitalWrite(Y_STEP, LOW);
                    delayMicroseconds(10 + pulse_delay);
                    stepsTaken[1] += sign;
                }
                digitalWrite(Y_EN, HIGH);
                break;
            case 'Z': 
                digitalWrite(Z_EN, LOW);
                for (long i = 0; i < steps_to_move; i++) {
                    digitalWrite(Z_STEP, HIGH);
                    delayMicroseconds(10);
                    digitalWrite(Z_STEP, LOW);
                    delayMicroseconds(10 + pulse_delay);
                    stepsTaken[2] += sign;
                }
                digitalWrite(Z_EN, HIGH);
                break;
            default: 
                Serial.println("Error: Invalid axis.");
                break;
        }
    }
    else {
        Serial.println("Error: Invalid axis.");
    }

    if (s < 0) {
        digitalWrite(X_DIR, !digitalRead(X_DIR));
        digitalWrite(Y_DIR, !digitalRead(Y_DIR));
        digitalWrite(Z_DIR, !digitalRead(Z_DIR));
    }
}

void loop() {
    // init variables
    char cmdString[MAX_LENGTH + 1] = "\0"; // Variable to store command
    int axis;
    int pulses;
    int motorIndex;
  
    // read the input from Serial Monitor
    read_from_serial(cmdString); 
    // echo the input read for verification
    Serial.print("Received: ");
    Serial.println(cmdString);
  
    // when there is input via Serial: 
    if (strcmp(cmdString, "") != 0) {
        // tokenize cmdString into 3 arguments
        char* arg1 = strtok(cmdString, " ");
        char* arg2 = strtok(NULL, " "); 
        char* arg3 = strtok(NULL, " "); 
      
        if (strtok(NULL, " ") != NULL) { 
            Serial.println("Error: Incorrect format");          // more than 3 arguments
        }
        else if (strcmp(arg1, "help") == 0 && arg2 == NULL && arg3 == NULL) {
            instructions();
        }
        else if (strcmp(arg1, "status") == 0 && arg2 == NULL && arg3 == NULL) {
            check_status();
        }
        else if (strcmp(arg1, "reinit") == 0 && arg2 == NULL && arg3 == NULL) {
            reinit_steppers();
        }
        else if (strcmp(arg1, "reset") == 0 && arg3 == NULL){
            reset_counter(arg2);
            main_screen(); 
        }
        else if (strcmp(arg1, "ms") == 0) {
            change_ms(arg2, arg3);
        }
        else if (strcmp(arg1, "curr") == 0) {
            change_current(arg2, arg3);
        }
        else if (strcmp(arg1, "move") == 0) {
            move_stepper(arg2, arg3);
            main_screen();
        }
        else {
            Serial.println("Error: Unknown command.");
        }
        Serial.println();
    }
}

void main_screen() {
    u8g.firstPage();
    u8g.setFont(u8g_font_profont10);
    do {
        for (int i = 0; i < NMOTORS; i++){
            char printString[50];
            sprintf(printString, "%c-axis: %ld steps", i + 88, stepsTaken[i]);
            u8g.drawStr(20, 15*(i + 1), printString);
        }
    } while(u8g.nextPage()); 
}
