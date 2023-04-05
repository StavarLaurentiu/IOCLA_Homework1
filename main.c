#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"

#define MAX 21

typedef void (*function_ptr)(void *);

void read_data(FILE *fp, sensor *Sensor, int sensor_count)
{	
	// read data in the normal order
	int i;
	sensor *Sensor_copy = (sensor *) malloc(sensor_count * sizeof(sensor));
	for (i = 0; i < sensor_count; i++) {
		int sensor_type;
		fread(&sensor_type, sizeof(int), 1, fp);
		if (sensor_type == 0) {
			Sensor_copy[i].sensor_type = TIRE;
			Sensor_copy[i].sensor_data = (tire_sensor *) malloc(1 * sizeof(tire_sensor));
			fread((tire_sensor *)Sensor_copy[i].sensor_data, sizeof(tire_sensor), 1, fp);
		} else if (sensor_type == 1) {
			Sensor_copy[i].sensor_type = PMU;
			Sensor_copy[i].sensor_data = 
						(power_management_unit *) malloc(1 * sizeof(power_management_unit));
			fread((power_management_unit *)Sensor_copy[i].sensor_data, sizeof(power_management_unit), 1, fp);
		}

		fread(&Sensor_copy[i].nr_operations, sizeof(int), 1, fp);
		Sensor_copy[i].operations_idxs = (int *) malloc(Sensor_copy[i].nr_operations * sizeof(int));
		fread(Sensor_copy[i].operations_idxs, sizeof(int), Sensor_copy[i].nr_operations, fp);
	}

	// reorder data by priority
	// the current index copied in Sensor
	int current_indx = 0;
	for (i = 0; i < sensor_count; i++) {
		if (Sensor_copy[i].sensor_type == 1) {
			Sensor[current_indx++] = Sensor_copy[i];
		}
	}
	for (i = 0; i < sensor_count; i++) {
		if (Sensor_copy[i].sensor_type == 0) {
			Sensor[current_indx++] = Sensor_copy[i];
		}
	}

	free(Sensor_copy);
}

void print_data(sensor *Sensor, int idx)
{
	if (Sensor[idx].sensor_type == 0) {
			printf("Tire Sensor\n"); 
			printf("Pressure: %.2f\n", ((tire_sensor *)Sensor[idx].sensor_data)->pressure);
			printf("Temperature: %.2f\n", ((tire_sensor *)Sensor[idx].sensor_data)->temperature);
			printf("Wear Level: %d%%\n", ((tire_sensor *)Sensor[idx].sensor_data)->wear_level);
			if (((tire_sensor *)Sensor[idx].sensor_data)->performace_score == 0) {
				printf("Performance Score: Not Calculated\n");
			} else {
				printf("Performance Score: %d\n", ((tire_sensor *)Sensor[idx].sensor_data)->performace_score);
			}
		} else if (Sensor[idx].sensor_type == 1) {
			printf("Power Management Unit\n");
			printf("Voltage: %.2f\n", ((power_management_unit *)Sensor[idx].sensor_data)->voltage);
			printf("Current: %.2f\n", ((power_management_unit *)Sensor[idx].sensor_data)->current);
			printf("Power Consumption: %.2f\n", ((power_management_unit *)Sensor[idx].sensor_data)->power_consumption);
			printf("Energy Regen: %d%%\n", ((power_management_unit *)Sensor[idx].sensor_data)->energy_regen);
			printf("Energy Storage: %d%%\n", ((power_management_unit *)Sensor[idx].sensor_data)->energy_storage);
		} 
}

void analyze_data(sensor *Sensor, int idx)
{	
	function_ptr *operation = malloc(8 * sizeof(function_ptr));
	get_operations(operation);

	int i;
	for (i = 0; i < Sensor[idx].nr_operations; i++) {
		operation[Sensor[idx].operations_idxs[i]](Sensor[idx].sensor_data);
	}

	free(operation);
} 

int correct(sensor Sensor) 
{
	if (Sensor.sensor_type == 0) {
		if (((tire_sensor *)Sensor.sensor_data)->pressure < 19 || ((tire_sensor *)Sensor.sensor_data)->pressure > 28) return 0;
		if (((tire_sensor *)Sensor.sensor_data)->temperature < 0 || ((tire_sensor *)Sensor.sensor_data)->temperature > 120) return 0;
		if (((tire_sensor *)Sensor.sensor_data)->wear_level < 0 || ((tire_sensor *)Sensor.sensor_data)->wear_level > 100) return 0;
	} else {
		if (((power_management_unit *)Sensor.sensor_data)->voltage < 10 || ((power_management_unit *)Sensor.sensor_data)->voltage > 20) return 0;
		if (((power_management_unit *)Sensor.sensor_data)->current < -100 || ((power_management_unit *)Sensor.sensor_data)->current > 100) return 0;
		if (((power_management_unit *)Sensor.sensor_data)->power_consumption < 0 || ((power_management_unit *)Sensor.sensor_data)->power_consumption > 1000) return 0;
		if (((power_management_unit *)Sensor.sensor_data)->energy_regen < 0 || ((power_management_unit *)Sensor.sensor_data)->energy_regen > 100) return 0;
		if (((power_management_unit *)Sensor.sensor_data)->energy_storage < 0 || ((power_management_unit *)Sensor.sensor_data)->energy_storage > 100) return 0;
	}

	return 1;
}

void clear_data(sensor *Sensor, int *sensor_count) 
{
	int i;
	for (i = 0; i <  *sensor_count; i++) {
		if (!correct(Sensor[i])) {
			int j;
			free(Sensor[i].sensor_data);
			free(Sensor[i].operations_idxs);
			for (j = i; j < (*sensor_count) - 1; j++) {
				Sensor[j] = Sensor[j + 1];
			}
			(*sensor_count)--;
			i--;
		}
	}
}

void free_data(sensor *Sensor, int sensor_count)
{
	int i;
	for (i = 0; i < sensor_count; i++) {
		free(Sensor[i].sensor_data);
		free(Sensor[i].operations_idxs);
	} 

	free(Sensor);
}

int main(int argc, char const *argv[])
{	
	// open the binary file to read input
	FILE *fp = fopen(argv[1], "rb");

	// read the number of sensors
	int sensor_count;
	fread(&sensor_count, sizeof(int), 1, fp);

	// read data about sensors from binary file
	sensor *Sensor = (sensor *) malloc(sensor_count * sizeof(sensor));
	read_data(fp, Sensor, sensor_count);
	fclose(fp);

	// read operations
	char operation[MAX];
	while (1) {
		fgets(operation, MAX, stdin);

		// if operation is "exit"
		if (strcmp(operation, "exit\n") == 0) break;

		// if operation is "print"
		if (strncmp(operation, "print", 5) == 0) {
			int idx;
			char aux[21];
			sscanf(operation, "%s %d", aux, &idx);
			if (idx >= 0 && idx < sensor_count) {
				print_data(Sensor, idx);
			} else {
				printf("Index not in range!\n");
			}
		}

		// if operation is "analyze"
		if (strncmp(operation, "analyze", 7) == 0) {
			int idx;
			char aux[21];
			sscanf(operation, "%s %d", aux, &idx);
			if (idx >= 0 && idx < sensor_count) {
				analyze_data(Sensor, idx);
			} else {
				printf("Index not in range!\n");
			}
		}

		// if operation is "clear"
		if (strncmp(operation, "clear", 5) == 0) {
			clear_data(Sensor, &sensor_count);
		}
	}

	free_data(Sensor, sensor_count);

	return 0;
}
