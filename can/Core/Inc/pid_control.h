#ifndef __PID_CONTROL__H
#define __PID_CONTROL__H

typedef struct{

	float kp;
	float ki;
	float kd;
	float error;
	float target;
	float integral;
    float output_max;
	float output_min;
}pid_control;

void PID_Init(pid_control *pid,float kp,float ki,float kd,float max);
int PID_Calculate(pid_control *pid,float measured_value);

#endif
