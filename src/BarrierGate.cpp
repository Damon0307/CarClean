#include "BarrierGate.h"
#include <iostream>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

unsigned int gpio_pin =49;
 

BarrierGate::BarrierGate(/* args */)
{
    InitIO();
}

BarrierGate::~BarrierGate()
{
    //unexport gpio
    FILE *unexport_file = fopen("/sys/class/gpio/unexport", "w");
    if (unexport_file == NULL) {
        
        perror("Failed to open GPIO unexport file");
        return;
    }
    fprintf(unexport_file, "%d", gpio_pin);
    fclose(unexport_file);

}


void BarrierGate::BarrierGateCtrl(bool ctrl_flag)
{
    //使用sys/class/gpio/gpio49/value文件来控制GPIO49的电平
    char value_path[50];
    snprintf(value_path, sizeof(value_path), "/sys/class/gpio/gpio%d/value", gpio_pin);
    FILE *value_file = fopen(value_path, "w");
    if (value_file == NULL) {
        perror("Failed to open GPIO value file");
        return;
    }
    if (ctrl_flag)
    {
        fprintf(value_file, "1");
    }
    else
    {
        fprintf(value_file, "0");
    }
    fclose(value_file);
     
}
 int BarrierGate::InitIO()
 {
    
 FILE *export_file = fopen("/sys/class/gpio/export", "w");
    if (export_file == NULL) {
        perror("Failed to open GPIO export file");
        return -1;
    }
    fprintf(export_file, "%d", gpio_pin);
    fclose(export_file);

    char direction_path[50];
    snprintf(direction_path, sizeof(direction_path), "/sys/class/gpio/gpio%d/direction", gpio_pin);
    FILE *direction_file = fopen(direction_path, "w");
    if (direction_file == NULL) {
        perror("Failed to open GPIO direction file");
        return -1;
    }
    fprintf(direction_file, "out");
    fclose(direction_file);

    return 0;
 
 }

 