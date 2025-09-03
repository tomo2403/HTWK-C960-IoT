//
// Created by tinkor on 03.09.25.
//

#ifndef MOTOR_H
#define MOTOR_H

void motor_init(void);

void set_motor1(uint32_t duty, uint8_t direction);

void set_motor2(uint32_t duty, uint8_t direction);

void stop_motor1();

void stop_motor2();


#endif //MOTOR_H
