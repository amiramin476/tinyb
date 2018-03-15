/*
 * Author: Petre Eftime <petre.p.eftime@intel.com>
 * Copyright (c) 2015 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <tinyb.hpp>

#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

using namespace tinyb;

/** Converts a raw temperature read from the sensor to a Celsius value.
 * @param[in] raw_temp The temperature read from the sensor (two bytes)
 * @return The Celsius value of the temperature
 */
static float celsius_temp(uint16_t raw_temp)
{
    const float SCALE_LSB = 0.03125;
    return ((float)(raw_temp >> 2)) * SCALE_LSB;
}


std::atomic<bool> running(true);

void signal_handler(int signum)
{
    if (signum == SIGINT) {
        running = false;
    }
}

/** This program reads the temperature from a
 * TI Sensor Tag(http://www.ti.com/ww/en/wireless_connectivity/sensortag2015/?INTC=SensorTag&HQS=sensortag)
 * Pass the MAC address of the sensor as the first parameter of the program.
 */
int main(int argc, char **argv)
{
	std::cout << "Intech BLE Sensor demo\n";

   BluetoothManager *manager = nullptr;
    try {
        manager = BluetoothManager::get_bluetooth_manager();
    } catch(const std::runtime_error& e) {
        std::cerr << "Error while initializing libtinyb: " << e.what() << std::endl;
        exit(1);
    }

    /* Start the discovery of devices */
    bool ret = manager->start_discovery();
    std::cout << "Started = " << (ret ? "true" : "false") << std::endl;
    
    while (running)
    {    

		BluetoothDevice *sensor_tag = NULL;
		BluetoothGattService *temperature_service = NULL;
		
		std::cout << "Discovering Intech BLE device .... " << std::endl;

		//wait indefinitely for the device
		while (true) { 
			//std::cout << "Discovered devices: " << std::endl;
			/* Get the list of devices */
			auto list = manager->get_devices();

			for (auto it = list.begin(); it != list.end(); ++it) {

				/*std::cout << "Class = " << (*it)->get_class_name() << " ";
				std::cout << "Path = " << (*it)->get_object_path() << " ";
				std::cout << "Name = " << (*it)->get_name() << " ";
				std::cout << "Connected = " << (*it)->get_connected() << " ";
				std::cout << "RSSI = " << (*it)->get_rssi() << " ";
				std::cout << std::endl;*/
				
				if(argc >= 2)
				{
					if ((*it)->get_address() == argv[1])
						sensor_tag = (*it).release();
				}
				/* Search for the device with the address given as a parameter to the program */
				else if ((*it)->get_name() == "Intech_BLE")
				{
					sensor_tag = (*it).release();
				}
			}

			/* Free the list of devices and stop if the device was found */
			if (sensor_tag != nullptr)
				break;
			/* If not, wait and try again */
			std::this_thread::sleep_for(std::chrono::seconds(1));
			//std::cout << std::endl;
		}
		

				
		//std::this_thread::sleep_for(std::chrono::seconds(1));
		

		/* Stop the discovery (the device was found or number of tries ran out */
		/*ret = manager->stop_discovery();
		std::cout << "Stopped = " << (ret ? "true" : "false") << std::endl;*/

		int discovered = 0;
		BluetoothGattCharacteristic *temp_value = nullptr;
		BluetoothGattCharacteristic *temp_config = nullptr;
		//BluetoothGattCharacteristic *temp_period = nullptr;
    
		/* Connect to the device and get the list of services exposed by it */
		
		try {
			sensor_tag->connect();		
		} catch (std::exception &e) {
			std::cout << "Error: " << e.what() << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;
		}
		
		std::cout << "Found device Name = " << sensor_tag->get_name() << " ";
		std::cout << "Address = " << sensor_tag->get_address() << " ";
		std::cout << "Connected = " << sensor_tag->get_connected() << " ";
		std::cout << "RSSI = " << sensor_tag->get_rssi() << " ";
		std::cout << std::endl;		

		//std::cout << "Discovered services: " << std::endl;
		while (true) {
			/* Wait for the device to come online */
			//std::this_thread::sleep_for(std::chrono::seconds(4));

			auto list = sensor_tag->get_services();
			if (list.empty())
				break;

			for (auto it = list.begin(); it != list.end(); ++it) {
				/*std::cout << "Class = " << (*it)->get_class_name() << " ";
				std::cout << "Path = " << (*it)->get_object_path() << " ";
				std::cout << "UUID = " << (*it)->get_uuid() << " ";
				std::cout << "Device = " << (*it)->get_device().get_object_path() << " ";
				std::cout << std::endl;*/

				/* Search for the temperature service, by UUID */
				if ((*it)->get_uuid() == "77880001-b5a3-f393-e0a9-150e24fcca8e")
					temperature_service = (*it).release();
			}
			break;
		}

		if (temperature_service == nullptr) {
			std::cout << "Could not find service 77880001-b5a3-f393-e0a9-150e24fcca8e" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));

			continue;
		}


		 /* get it's characteristics, by UUID again */
		auto list = temperature_service->get_characteristics();
		//std::cout << "Discovered characteristics: " << std::endl;
		for (auto it = list.begin(); it != list.end(); ++it) {

			/*std::cout << "Class = " << (*it)->get_class_name() << " ";
			std::cout << "Path = " << (*it)->get_object_path() << " ";
			std::cout << "UUID = " << (*it)->get_uuid() << " ";
			std::cout << "Service = " << (*it)->get_service().get_object_path() << " ";
			std::cout << std::endl; */

			if ((*it)->get_uuid() == "77880003-b5a3-f393-e0a9-150e24fcca8e")
				temp_value = (*it).release();
			else if ((*it)->get_uuid() =="77880002-b5a3-f393-e0a9-150e24fcca8e")
				temp_config = (*it).release();
			/*else if ((*it)->get_uuid() == "f000aa03-0451-4000-b000-000000000000")
				temp_period = (*it).release();*/
		}

		if (temp_config == nullptr || temp_value == nullptr /*|| temp_period == nullptr */) {
			std::cout << "Could not find characteristics." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));

		}
			


		try {
            /* Read temperature data and display it */
            std::vector<unsigned char> response = temp_value->read_value();
            unsigned char *data;
            unsigned int size = response.size();
            if (size > 0) {
                data = response.data();

                std::cout << std::endl;
                int b0 = static_cast<int>(data[0]); 
                int b1 = static_cast<int>(data[1]);
                
                //std::cout << b0 << " " << b1;

                std::cout << "Heart beat: " << (b0 + (b1 << 8)) << std::endl << std::endl;
                
            }
        
		} 
		catch (std::exception &e) {
			std::cout << "Error: " << e.what() << std::endl;
		}

		/* Disconnect from the device */
		 try {
			sensor_tag->disconnect();
		} catch (std::exception &e) {
			std::cout << "Error: " << e.what() << std::endl;
		}
    
        std::this_thread::sleep_for(std::chrono::seconds(2));

	}
    return 0;
}
