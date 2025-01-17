/*
;    Project:       Open Vehicle Monitor System
;    Date:          14th March 2017
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2017  Mark Webb-Johnson
;    (C) 2011       Sonny Chen @ EPRO/DX
;    (C) 2020       Stephen Davies - BMWi3 vehicle type
;
;    2020-12-12     0.0       Work started
;    2020-12-31     0.1       Workable version - most metrics supported, works with the ABRP plugin.  Open issues:
;                               1) Detection that the car is "on" (ready to drive) is still ropey
;                               2) Don't have individual cell data
;                               3) Read-only - no sending of commands.
;                               4) Detection of DC charging is only slightly tested
;                               5) Would like to find a way to stop the OBD gateway going to sleep on us, eg when charging.
;    2020-01-04               Ready to merge as a first version
;
;
;    Note that if you leave the OVMS box connected to the OBD-II port and lock the car then the alarm will go off.
;       This can be coded off using eg Bimmercode; I'll attempt to add a command to do that to this module at some point.
;
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

#include "ovms_log.h"
static const char *TAG = "v-bmwi3";

#include <stdio.h>
#include <math.h>
#include "vehicle_bmwi3.h"

#include "ovms_command.h"
#include "can.h"

// Register the second CAN bus
void OvmsVehicleBMWi3::SetupSecondCanBus()
{
    ESP_LOGI(TAG, "Setting up second CAN bus");
    RegisterCanBus(2, CAN_MODE_ACTIVE, CAN_SPEED_500KBPS); // Initialize second CAN bus at 500 kbps
}

// Function to send a frame on the second CAN bus
void OvmsVehicleBMWi3::SendSecondCanBusCommand(uint16_t id, uint8_t *data, size_t length)
{
    if (!m_can2)
    {
        ESP_LOGW(TAG, "Second CAN bus not initialized");
        return;
    }

    CAN_frame_t frame;
    frame.origin = m_can2;
    frame.FIR.B.FF = CAN_frame_std; // Standard frame
    frame.MsgID = id;              // CAN ID
    frame.FIR.B.DLC = length;      // Data length
    memcpy(frame.data.u8, data, length); // Data payload

    ESP_LOGI(TAG, "Sending frame on second CAN bus: ID=0x%03X, Length=%d", id, length);
    m_can2->Write(&frame); // Send the frame
}

// Function to send a wake-up frame on the second CAN bus
void OvmsVehicleBMWi3::WakeUpCar()
{
    uint8_t dummy_data[1] = {0x00};
    SendSecondCanBusCommand(0x000, dummy_data, 1); // Send a dummy frame with ID 0x000
    ESP_LOGI(TAG, "Wake-up frame sent on second CAN bus");
}

// Function to activate preconditioning
void OvmsVehicleBMWi3::ActivatePreconditioning()
{
    uint8_t preconditioning_data[2] = {0x00, 0xF2};
    SendSecondCanBusCommand(0x2A2, preconditioning_data, 2);
    ESP_LOGI(TAG, "Preconditioning activated via second CAN bus");
}

// Function to unlock the car
void OvmsVehicleBMWi3::UnlockCar()
{
    uint8_t unlock_data[2] = {0xC8, 0xFF};
    SendSecondCanBusCommand(0x3A3, unlock_data, 2);
    ESP_LOGI(TAG, "Unlock command sent via second CAN bus");
}

// Function to lock the car
void OvmsVehicleBMWi3::LockCar()
{
    uint8_t lock_data[2] = {0xD0, 0xFF};
    SendSecondCanBusCommand(0x3A3, lock_data, 2);
    ESP_LOGI(TAG, "Lock command sent via second CAN bus");
}

// Modify constructor to initialize the second CAN bus
OvmsVehicleBMWi3::OvmsVehicleBMWi3()
{
    ESP_LOGI(TAG, "BMW i3/i3s vehicle module initialized");

    // Set up the second CAN bus
    SetupSecondCanBus();

    // Other initializations remain unchanged
}

// Add commands for testing functionality
void OvmsVehicleBMWi3::RegisterUserCommands()
{
    OvmsCommand *cmd_vehicle = MyCommandApp.RegisterCommand("vehicle", "Vehicle framework");
    cmd_vehicle->RegisterCommand("wake", "Wake up the car", [this](int, OvmsWriter*, OvmsCommand*, int, const char*[]) { WakeUpCar(); }, "");
    cmd_vehicle->RegisterCommand("precondition", "Activate preconditioning", [this](int, OvmsWriter*, OvmsCommand*, int, const char*[]) { ActivatePreconditioning(); }, "");
    cmd_vehicle->RegisterCommand("unlock", "Unlock the car", [this](int, OvmsWriter*, OvmsCommand*, int, const char*[]) { UnlockCar(); }, "");
    cmd_vehicle->RegisterCommand("lock", "Lock the car", [this](int, OvmsWriter*, OvmsCommand*, int, const char*[]) { LockCar(); }, "");
}
