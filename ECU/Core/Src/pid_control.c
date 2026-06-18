#include "pid_control.h"

void PID_Init(pid_control *pid,float kp,float ki,float kd,float max){

    pid->kp=kp;
	pid->ki=ki;
	pid->kd=kd;
	pid->error=0.00f;
	pid->target=0.00f;
	pid->integral=0.00f;
	pid->output_min=0.00f;
	pid->output_max=max;
}
int PID_Calculate(pid_control *pid,float measured_value){
	//P项
   float error=measured_value-pid->target;
   float p_term=pid->kp*error;
	
	//I项
	pid->integral+=error;
   float integral_limit = 500;
   if(pid->integral>integral_limit){
    pid->integral=integral_limit;
   }
   float i_term=pid->ki*pid->integral;
   
   //D项
    float derivative = error - pid->error;
    float d_term = pid->kd * derivative;
    
    float term=p_term+i_term+d_term;
   pid->error=error;
   if(term>pid->output_max){
   return pid->output_max;
   }
   else if(term<pid->output_min){
   return pid->output_min;
   }
   return term;
}