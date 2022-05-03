/* main.c - Hello World demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #include "uart_i2c.c"
#include <zephyr.h>
#include <sys/printk.h>
#include <errno.h>

#include "sht31.c"
#include "uart_i2c.c"
#include "lsm6ds33.c"
#include "apds9960.c"
#include "scd41.c"
#include "bmp280.h"



/*
 * The hello world demo has two threads that utilize semaphores and sleeping
 * to take turns printing a greeting message at a controlled rate. The demo
 * shows both the static and dynamic approaches for spawning a thread; a real
 * world application would likely use the static approach for both threads.
 */

#define PIN_THREADS (IS_ENABLED(CONFIG_SMP)		  \
		     && IS_ENABLED(CONFIG_SCHED_CPU_MASK) \
		     && (CONFIG_MP_NUM_CPUS > 1))

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between greetings (in ms) */
#define SLEEPTIME 500


/*
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
/* define semaphores */

// K_SEM_DEFINE(threadA_sem, 1, 1);	/* starts off "available" */
// K_SEM_DEFINE(threadB_sem, 0, 1);	/* starts off "not available" */
enum data_type{
	SENSOR_SHT31,
	SENSOR_APDS9960,
	SENSOR_BMP280,
	SENSOR_LSM6DS33,
	SENSOR_SCD41,
	SENSOR_DS18B20,
};

typedef struct {
	// id->0: sht data; id->1: apds data
	enum data_type type;
	uint32_t time_stamp;
	union 
	{
		// scd41_t scd41_data;
		sht31_t sht31_data;
		lsm6ds33_t lsm6ds33_data;
		bmp280_t bmp280_data;
		apds9960_t apds_cls_data;
		// int16_t ds18b20_temp_data;
	};
}ble_data_t;


struct k_mutex mymutex;

/* Create a MSG Queue for the threads to use for data passing*/
// struct k_msgq my_msgq;
K_MSGQ_DEFINE(my_msgq, sizeof(ble_data_t), 8, 4);

void apds9960(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	struct k_thread *current_thread;
	// apds9960_t threadC_apds9960;
	current_thread = k_current_get();
	// Write something to start another sensor.
	ble_data_t apds_local_data;
	apds_local_data.type = SENSOR_APDS9960;
	while(1){
		k_mutex_lock(&mymutex, K_FOREVER);
		read_proximity_data(&apds_local_data.apds_cls_data);
		read_als_data(&apds_local_data.apds_cls_data);
		apds_local_data.time_stamp = k_cycle_get_32();
		printk("[%d] Hello from %s\n", apds_local_data.time_stamp,k_thread_name_get(current_thread));
		k_msgq_put(&my_msgq, &apds_local_data, K_FOREVER);
		printk("APDS Ended!!!\n");
		k_mutex_unlock(&mymutex);
		k_sleep(K_FOREVER);
	}	
	return;
}

/* threadA is a static thread that is spawned automatically */
void sht31(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	/* invoke routine to ping-pong hello messages with threadB */
	struct k_thread *current_thread;
	current_thread = k_current_get();
	ble_data_t sht31_local_data;
	sht31_local_data.type = SENSOR_SHT31;
	while(1){
		k_mutex_lock(&mymutex, K_FOREVER);
		read_temp_hum(&sht31_local_data.sht31_data);
		sht31_local_data.time_stamp = k_cycle_get_32();
		printk("[%d] Hello from %s\n", sht31_local_data.time_stamp,k_thread_name_get(current_thread));
		k_msgq_put(&my_msgq, &sht31_local_data, K_FOREVER);
		printk("SHT Ended!!!\n");
		k_mutex_unlock(&mymutex);
		k_sleep(K_FOREVER);
	}
	return;
}

void bmp280(void* dummy1, void* dummy2, void* dummy3){
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	read_calibration_registers();
	struct k_thread *current_thread;
	current_thread = k_current_get();
	ble_data_t bmp280_local_data;
	bmp280_local_data.type = SENSOR_BMP280;
	while(1){
		k_mutex_lock(&mymutex, K_FOREVER);
		bmp_read_press_temp_data(&bmp280_local_data.bmp280_data);
		bmp280_local_data.time_stamp = k_cycle_get_32();
		printk("[%d] Hello from %s\n", bmp280_local_data.time_stamp,k_thread_name_get(current_thread));
		k_msgq_put(&my_msgq, &bmp280_local_data, K_FOREVER);
		printk("BMP Ended!!!\n");
		k_mutex_unlock(&mymutex);
		k_sleep(K_FOREVER);
	}
}

void lsm6ds33(void* dummy1, void* dummy2, void* dummy3){
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	struct k_thread* current_thread;
	current_thread = k_current_get();
	ble_data_t lsm6ds33_local_data;
	lsm6ds33_local_data.type = SENSOR_LSM6DS33;
	float accelX, accelY, accelZ;	
	float totalX, totalY, totalZ;
	uint8_t count;
	// Initialize the sensor by setting necessary parameters for gyroscope and accelerometer
	accel_set_power_mode(ACCEL_LOW_POWER_MODE);
	gyro_set_power_mode(GYRO_LOW_POWER_MODE);
	//  Check if the FIFO_FULL Flag is set or not.
	while(1){
		// Mutex is locked for very long time. Need to make sure how to do this.
		printk("LSM: Waiting for Mutex\n");
		while (1) {
			// k_mutex_lock(&mymutex, K_FOREVER);
			// if (lsm6ds33_fifo_status() & FIFO_FULL) == 0) {
			// 	k_mutex_unlock(&mymutex);
			// 	sleep(xx); // for some time or
			// 	or k_yield(); // relinquish CPU
			// }
			// else break;
		}
		k_mutex_lock(&mymutex, K_FOREVER);
			lsm6ds33_local_data.time_stamp = k_cycle_get_32();
		printk("[%d] Hello from %s\n", lsm6ds33_local_data.time_stamp,k_thread_name_get(current_thread));
		printk("Waiting for the FIFO mode to get the data...\n");
		while((lsm6ds33_fifo_status() & FIFO_FULL) == 0) { };
		count=0, totalX=0, totalY=0, totalZ=0;
		while( (lsm6ds33_fifo_status()& FIFO_EMPTY) == 0){
			accelX = lsm6ds33_fifo_get_accel_data(lsm6ds33_fifo_read());
			totalX+=accelX;
			lsm6ds33_local_data.lsm6ds33_data.accelX = accelX;
			// printk("AccelX_Raw: %f\n", accelX);
			accelY = lsm6ds33_fifo_get_accel_data(lsm6ds33_fifo_read());
			totalY+=accelY;
			lsm6ds33_local_data.lsm6ds33_data.accelY = accelY;
			// printk("AccelY_Raw: %f\n", accelY);
			accelZ = lsm6ds33_fifo_get_accel_data(lsm6ds33_fifo_read());
			totalZ+=accelZ;
			lsm6ds33_local_data.lsm6ds33_data.accelZ = accelZ;
			// printk("AccelZ_Raw: %f\n", accelZ);
			count++;
			// printk("AccelX_Raw: %f\n", accelX);
			// printk("AccelY_Raw: %f\n", lsm6ds33_fifo_get_accel_data(lsm6ds33_fifo_read()));
			// printk("AccelZ_Raw: %f\n", lsm6ds33_fifo_get_accel_data(lsm6ds33_fifo_read()));
		}
		printk("Average X: %f\t Y: %f\t Z: %f\n", totalX/count, totalY/count, totalZ/count);
		k_msgq_put(&my_msgq, &lsm6ds33_local_data, K_FOREVER);
		lsm6ds33_fifo_change_mode(0);
		k_sleep(K_FOREVER);
		k_mutex_unlock(&mymutex);
		printk("LSM6DS33 Finished!!!\n");
		lsm6ds33_fifo_change_mode(1);
	}
}

char* enum_to_string(ble_data_t* data){
	if(data->type == SENSOR_APDS9960){
		return "APDS9960";
	}
	else if(data->type == SENSOR_SHT31){
		return "SHT31";
	}
	else if(data->type == SENSOR_BMP280){
		return "BMP280";
	}
	else if(data->type == SENSOR_LSM6DS33){
		return "LSM6DS33";
	}
	
	return "Not Valid";
}

void consumer_thread(void* dummy1, void* dummy2, void* dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	ble_data_t consumer_local_data;
	uint32_t time=0;
	printk("Consumer Thread: Entered for the first time!!!\n");
	while(1){
		uint8_t num_used = k_msgq_num_used_get(&my_msgq);
		for(int i=0; i<num_used; i++){
			int8_t ret = k_msgq_get(&my_msgq, &consumer_local_data, K_MSEC(100));
			if(ret!=0){
				printk("Couldn't find data\n");
			}
			time = k_cycle_get_32();
			if(consumer_local_data.type == SENSOR_APDS9960)
				printk("[Current: %d\t Data_time_stamp: %d] Data type: %s\t Prox: %d\t Clear: %d\t Red: %d\t Blue: %d\t Green: %d\n",
													time,
													consumer_local_data.time_stamp,
													enum_to_string(&consumer_local_data),
													consumer_local_data.apds_cls_data.proximity, 
														consumer_local_data.apds_cls_data.clear, 
														consumer_local_data.apds_cls_data.red,
														consumer_local_data.apds_cls_data.blue, 
														consumer_local_data.apds_cls_data.green);
			
			else if(consumer_local_data.type == SENSOR_SHT31)
				printk("[Current: %d\t Data_time_stamp: %d] Data type: %s\t Temp: %f\t Humi: %f\n",
													time, 
													consumer_local_data.time_stamp,
													enum_to_string(&consumer_local_data),
														consumer_local_data.sht31_data.temp, 
														consumer_local_data.sht31_data.humidity);

			else if(consumer_local_data.type == SENSOR_BMP280)
				printk("[Current: %d\t Data_time_stamp: %d] Data type: %s\t Temp: %f\t Press: %f\n",
													time, 
													consumer_local_data.time_stamp,
													enum_to_string(&consumer_local_data),
														consumer_local_data.bmp280_data.temperature, 
														consumer_local_data.bmp280_data.pressure);

			else if(consumer_local_data.type == SENSOR_LSM6DS33)
				printk("[Current: %d\t Data_time_stamp: %d] Data type: %s\t AccelX: %f\t AccelY: %f\t AccelZ: %f\n",
													time, 
													consumer_local_data.time_stamp,
													enum_to_string(&consumer_local_data),
														consumer_local_data.lsm6ds33_data.accelX, 
														consumer_local_data.lsm6ds33_data.accelY,
														consumer_local_data.lsm6ds33_data.accelZ);
			
		}// End of if.
		k_sleep(K_FOREVER);

	}

}

K_THREAD_STACK_DEFINE(sht31_stack_area, STACKSIZE);
static struct k_thread sht31_thread_data;

K_THREAD_STACK_DEFINE(apds9960_stack_area, STACKSIZE);
static struct k_thread apds9960_thread_data;

K_THREAD_STACK_DEFINE(bmp280_stack_area, STACKSIZE);
static struct k_thread bmp280_thread_data;

K_THREAD_STACK_DEFINE(lsm6ds33_stack_area, STACKSIZE);
static struct k_thread lsm6ds33_thread_data;

K_THREAD_STACK_DEFINE(consumer_stack_area, STACKSIZE);
static struct k_thread consumer_thread_data;

/**
 * @brief  Work Handler function. This will be ran by a thread in backend depending on the priority.
 * @note   
 * @param  struct k_work*: What wok needs to be done.
 * @retval None
 */
void producer_work_handler(struct k_work *work){
	printk("Timer fired!!!\n");
	k_wakeup(&sht31_thread_data);
	k_wakeup(&apds9960_thread_data);
	k_wakeup(&bmp280_thread_data);
	k_wakeup(&lsm6ds33_thread_data);
}

void consumer_work_handler(struct k_work * work){
	printk("Consumer timer Fired!!!\n");
	k_wakeup(&consumer_thread_data);
}
// Define a work and it's work handler
K_WORK_DEFINE(producer_work, producer_work_handler);

K_WORK_DEFINE(consumer_work, consumer_work_handler);
// Define a function that will be called when a timer expires.
void producer_timer_handler(struct k_timer* dummy){
	k_work_submit(&producer_work);
}

void consumer_timer_handler(struct k_timer* dummy){
	k_work_submit(&consumer_work);
}

// Define a timer.
K_TIMER_DEFINE(producer_timer, producer_timer_handler, NULL);

K_TIMER_DEFINE(consumer_timer, consumer_timer_handler, NULL);



volatile uint8_t count;
void main(void)
{  
	// enable_uart_console();
	configure_device();
	lsm6ds33_init();
	enable_apds_sensor();
	k_mutex_init(&mymutex);
	printk("Number of CPUS: %d\n", CONFIG_MP_NUM_CPUS);

	k_thread_create(&sht31_thread_data, sht31_stack_area,
			K_THREAD_STACK_SIZEOF(sht31_stack_area),
			sht31, NULL, NULL, NULL,
			PRIORITY, 0, K_MSEC(100));
	k_thread_name_set(&sht31_thread_data, "SHT_Thread");

	// k_thread_create(&apds9960_thread_data, apds9960_stack_area, 
	// 		K_THREAD_STACK_SIZEOF(apds9960_stack_area),
	// 		apds9960, NULL, NULL, NULL, 
	// 		PRIORITY, 0, K_MSEC(100));
	// k_thread_name_set(&apds9960_thread_data, "APDS Thread");

	// k_thread_create(&bmp280_thread_data, bmp280_stack_area, 
	// 	K_THREAD_STACK_SIZEOF(bmp280_stack_area),
	// 	bmp280, NULL, NULL, NULL, 
	// 	PRIORITY, 0, K_MSEC(100));
	// k_thread_name_set(&bmp280_thread_data, "BMP Thread");

	// k_thread_create(&lsm6ds33_thread_data, lsm6ds33_stack_area, 
	// 	K_THREAD_STACK_SIZEOF(lsm6ds33_stack_area),
	// 	lsm6ds33, NULL, NULL, NULL, 
	// 	PRIORITY, 0, K_MSEC(100));
	// k_thread_name_set(&lsm6ds33_thread_data, "LSM Thread");

	k_thread_create(&consumer_thread_data, consumer_stack_area,
			K_THREAD_STACK_SIZEOF(consumer_stack_area),
			consumer_thread, NULL, NULL, NULL,
			PRIORITY-3, 0, K_MSEC(100));
	k_thread_name_set(&consumer_thread_data, "Consumer_Thread");

	k_timer_start(&producer_timer, K_SECONDS(4), K_SECONDS(4));
	k_timer_start(&consumer_timer, K_SECONDS(6), K_SECONDS(6));


}
