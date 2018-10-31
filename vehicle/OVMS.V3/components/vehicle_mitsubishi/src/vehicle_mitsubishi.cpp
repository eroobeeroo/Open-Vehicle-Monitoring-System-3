/*
;    Project:       Open Vehicle Monitor System
;    Date:          2th Sept 2018
;
;    Changes:
;    0.1.0  Initial release: 13-oct-2018 - KommyKT (tested on Peugeot iOn 03-2012 LHD)
;       - web dashboard limit modifications
;       - all 66 temp sensor avg for battery temp
;       - all cell temperature stored
;       - basic charge states
;       - AC charger voltage / current measure
;       - DC
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2018  Mark Webb-Johnson
;    (C) 2011        Sonny Chen @ EPRO/DX
;    (C) 2018       Tamás Kovács (KommyKT)
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
#include <stdio.h>
#include "vehicle_mitsubishi.h"

#define VERSION "0.1.2"

static const char *TAG = "v-mitsubishi";
typedef enum
  {
  CHARGER_STATUS_CHARGING,
  CHARGER_STATUS_QUICK_CHARGING,
  CHARGER_STATUS_FINISHED,
  CHARGER_STATUS_INTERRUPTED
  } ChargerStatus;

static float mi_batttemps[66];  // all cell temperature
static float mi_battvolts[88];  // all cell voltage
static float m_charge_watt;


OvmsVehicleMitsubishi::OvmsVehicleMitsubishi()
  {
  ESP_LOGI(TAG, "Start Mitsubishi iMiev, Citroen C-Zero, Peugeot iOn vehicle module");

  StandardMetrics.ms_v_env_parktime->SetValue(0);
  StandardMetrics.ms_v_charge_type->SetValue("None");
  memset(m_vin,0,sizeof(m_vin));
  memset(mi_batttemps,0,sizeof(mi_batttemps));
  memset(mi_battvolts,0,sizeof(mi_battvolts));

  RegisterCanBus(1,CAN_MODE_ACTIVE,CAN_SPEED_500KBPS);

  }

OvmsVehicleMitsubishi::~OvmsVehicleMitsubishi()
  {
  ESP_LOGI(TAG, "Stop Mitsubishi  vehicle module");
  }

void vehicle_charger_status(ChargerStatus status)
        {
        switch (status)
          {
          case CHARGER_STATUS_QUICK_CHARGING:
            if (!StandardMetrics.ms_v_charge_inprogress->AsBool())
              {
                StandardMetrics.ms_v_charge_kwh->SetValue(0); // Reset charge kWh
              }
            StandardMetrics.ms_v_pos_speed->SetValue(0);
            StandardMetrics.ms_v_mot_rpm->SetValue(0);
            StandardMetrics.ms_v_door_chargeport->SetValue(true);
            StandardMetrics.ms_v_charge_pilot->SetValue(true);
            StandardMetrics.ms_v_charge_inprogress->SetValue(true);
            StandardMetrics.ms_v_charge_type->SetValue("Chademo");
            StandardMetrics.ms_v_charge_state->SetValue("charging");
            StandardMetrics.ms_v_charge_substate->SetValue("onrequest");
            StandardMetrics.ms_v_charge_mode->SetValue("Quickcharge");
            StandardMetrics.ms_v_env_charging12v->SetValue(true);
            StandardMetrics.ms_v_env_on->SetValue(true);
            StandardMetrics.ms_v_env_awake->SetValue(true);
            StandardMetrics.ms_v_charge_climit->SetValue(125);
            StandardMetrics.ms_v_charge_voltage->SetValue(StandardMetrics.ms_v_bat_voltage->AsInt());
            StandardMetrics.ms_v_charge_current->SetValue(StandardMetrics.ms_v_bat_current->AsInt());
            break;
          case CHARGER_STATUS_CHARGING:
            if (!StandardMetrics.ms_v_charge_inprogress->AsBool())
              {
                StandardMetrics.ms_v_charge_kwh->SetValue(0); // Reset charge kWh
                m_charge_watt = 0; // reset watt
              }
            StandardMetrics.ms_v_pos_speed->SetValue(0);
            StandardMetrics.ms_v_mot_rpm->SetValue(0);
            StandardMetrics.ms_v_door_chargeport->SetValue(true);
            StandardMetrics.ms_v_charge_pilot->SetValue(true);
            StandardMetrics.ms_v_charge_inprogress->SetValue(true);
            StandardMetrics.ms_v_charge_type->SetValue("Type1");
            StandardMetrics.ms_v_charge_state->SetValue("charging");
            StandardMetrics.ms_v_charge_substate->SetValue("onrequest");
            StandardMetrics.ms_v_charge_mode->SetValue("Standard");
            StandardMetrics.ms_v_env_charging12v->SetValue(true);
            StandardMetrics.ms_v_env_on->SetValue(true);
            StandardMetrics.ms_v_env_awake->SetValue(true);
            StandardMetrics.ms_v_charge_climit->SetValue(16);
            m_charge_watt += ( StandardMetrics.ms_v_charge_voltage->AsFloat() * StandardMetrics.ms_v_charge_current->AsFloat());
            break;
          case CHARGER_STATUS_FINISHED:
            StandardMetrics.ms_v_charge_kwh->SetValue(m_charge_watt/1000.0/StandardMetrics.ms_v_charge_time->AsInt());
            ESP_LOGI(TAG, "charge_watt %f",m_charge_watt);
            m_charge_watt = 0;
            StandardMetrics.ms_v_charge_climit->SetValue(0);
            StandardMetrics.ms_v_charge_current->SetValue(0);
            StandardMetrics.ms_v_charge_voltage->SetValue(0);
            StandardMetrics.ms_v_door_chargeport->SetValue(false);
            StandardMetrics.ms_v_charge_pilot->SetValue(false);
            StandardMetrics.ms_v_charge_inprogress->SetValue(false);
            StandardMetrics.ms_v_charge_type->SetValue("None");
            StandardMetrics.ms_v_charge_state->SetValue("done");
            StandardMetrics.ms_v_charge_substate->SetValue("onrequest");
            StandardMetrics.ms_v_charge_mode->SetValue("Not charging");
            StandardMetrics.ms_v_env_charging12v->SetValue(false);
            StandardMetrics.ms_v_env_on->SetValue(false);
            StandardMetrics.ms_v_env_awake->SetValue(false);
            break;
          case CHARGER_STATUS_INTERRUPTED:
            StandardMetrics.ms_v_charge_climit->SetValue(0);
            StandardMetrics.ms_v_charge_current->SetValue(0);
            StandardMetrics.ms_v_charge_voltage->SetValue(0);
            StandardMetrics.ms_v_door_chargeport->SetValue(false);
            StandardMetrics.ms_v_charge_pilot->SetValue(false);
            StandardMetrics.ms_v_charge_inprogress->SetValue(false);
            StandardMetrics.ms_v_charge_type->SetValue("None");
            StandardMetrics.ms_v_charge_state->SetValue("interrupted");
            StandardMetrics.ms_v_charge_substate->SetValue("onrequest");
            StandardMetrics.ms_v_charge_mode->SetValue("Interrupted");
            StandardMetrics.ms_v_env_on->SetValue(false);
            StandardMetrics.ms_v_env_awake->SetValue(false);
            break;
          }
        if (status != CHARGER_STATUS_CHARGING && status != CHARGER_STATUS_QUICK_CHARGING)
          {
            StandardMetrics.ms_v_charge_current->SetValue(0);
            StandardMetrics.ms_v_charge_voltage->SetValue(0);
          }
        }

void OvmsVehicleMitsubishi::IncomingFrameCan1(CAN_frame_t* p_frame)
  {
  uint8_t *d = p_frame->data.u8;

  switch (p_frame->MsgID)
    {
      case 0x101: //Key status
      {
        if(d[1] == 4)
        {
          //ready
        }else if(d[1] == 0)
        {
          // not ready
        }

      break;
      }

      case 0x208: // Brake pedal position
      {
        StandardMetrics.ms_v_env_footbrake->SetValue(((d[2] * 256 + d[3]) - 24576.0) / 640 * 100.0);
      break;
      }

      case 0x210: // Accelerator pedal position
      {
        StandardMetrics.ms_v_env_throttle->SetValue(d[2]*0.4);
      break;
      }

      case 0x231: // Brake pedal pressed?
      {
        if (d[4] == 0) {
          //Brake not pressed
        } else if (d[4] == 2) {
          /* Brake pressed*/
        }
      break;
      }

      case 0x236: // Steeering wheel
      {
        // (((d[0] * 256) +d[1])-4096)/2  //Negative angle - right, positive angle left
        // ((d[2] * 256 + d[3] - 4096) / 2.0)) //Steering wheel movement
      break;
      }

      case 0x286: // Charger/inverter temperature
      {
        StandardMetrics.ms_v_charge_temp->SetValue((float)d[3]-40);
        StandardMetrics.ms_v_inv_temp->SetValue((float)d[3]-40);
      break;
      }

      case 0x298: // Motor temperature and RPM
      {
        StandardMetrics.ms_v_mot_temp->SetValue((int)d[3]-40);
        StandardMetrics.ms_v_mot_rpm->SetValue(((d[6]*256.0)+d[7])-10000);

        if(StandardMetrics.ms_v_mot_rpm->AsInt() < 10 && StandardMetrics.ms_v_mot_rpm->AsInt() > -10)
        { // (-10 < Motor RPM > 10) Set to 0
          StandardMetrics.ms_v_mot_rpm->SetValue(0);
        }
      break;
      }

      case 0x29A: // VIN determination
      {
        if (d[0]==0)
          for (int k=0;k<7;k++) m_vin[k] = d[k+1];
          else if (d[0]==1)
          for (int k=0;k<7;k++) m_vin[k+7] = d[k+1];
          else if (d[0]==2)
          {
            m_vin[14] = d[1];
            m_vin[15] = d[2];
            m_vin[16] = d[3];
            m_vin[17] = 0;
            if ((m_vin[0] != 0)&&(m_vin[7] != 0))
            {
              StandardMetrics.ms_v_vin->SetValue(m_vin);
            }
          }
      break;
      }

      case 0x325:
      {
        /*d[0]==1?*/
      break;
      }

      case 0x346: // Estimated range , // Handbrake state
      {
        StandardMetrics.ms_v_bat_range_est->SetValue(d[7]);

        if((d[4] & 32) == 0)
        {
          StandardMetrics.ms_v_env_handbrake->SetValue(false);
          StandardMetrics.ms_v_env_parktime->SetValue(0);
          break;
        }
        StandardMetrics.ms_v_env_handbrake->SetValue(true);
      break;
      }

      case 0x373: // Main Battery volt and current
      {
        StandardMetrics.ms_v_bat_current->SetValue((((((d[2]*256)+d[3]))-32768))/100.0);
        StandardMetrics.ms_v_bat_voltage->SetValue((d[4]*256+d[5])/10.0);
        //StandardMetrics.ms_v_bat_power->SetValue((StandardMetrics.ms_v_bat_voltage->AsInt()*StandardMetrics.ms_v_bat_current->AsInt())/1000);
      break;
      }

      case 0x374: // Main Battery Soc
      {
        StandardMetrics.ms_v_bat_soc->SetValue(((int)d[1]-10)/2.0);
        StandardMetrics.ms_v_bat_cac->SetValue(d[7]/2.0);
      break;
      }

      case 0x384: //heating current?
      {
        //d[4]/10 //heating current;
      break;
      }

      case 0x389: // Charger voltage and current
      {
        StandardMetrics.ms_v_charge_voltage->SetValue(d[1]*1.0);
        StandardMetrics.ms_v_charge_current->SetValue(d[6]/10.0);
      break;
      }

      case 0x3A4: // Climate console
      {
        //heating level
        if (((int)d[0]<<4) == 112)
        {
          //Heater off
        }else if (((int)d[0]<< 4) > 112)
        {
          // Heating
        }
        else if (((int)d[0]<< 4) < 112)
        {
          //Cooling
        }

      if (((int)d[0]>>7) == 1)
      {
       // AC on
      }


      //ventilation direction
      if ((((int)d[1]>>4) == 1) || (((int)d[1]>>4) == 2))
      {
        //   Face
      }

      if (((int)d[1]>>4) == 3)
      {
       //Leg + Face
      }

      if ((((int)d[1]>>4) == 4) || (((int)d[1]>>4) == 5))
      {
        //Leg
      }

      if (((int)d[1]>>4) == 6)
      {
          //Leg + Windshield
      }

      if ((((int)d[1]>>4) == 7) || (((int)d[1]>>4) == 8) || (((int)d[1]>>4) == 9))
      {
        // Windshield
      }


      break;
    }
    case 0x408:
    {
      // all == 0
    break;
    }
    case 0x412: // Speed and odometer
    {
      if (d[1]>200)
        StandardMetrics.ms_v_pos_speed->SetValue((int)d[1]-255.0,Kph);
      else
        StandardMetrics.ms_v_pos_speed->SetValue(d[1]);

        StandardMetrics.ms_v_pos_odometer->SetValue(((int)d[2]<<16)+((int)d[3]<<8)+d[4],Kilometers);
    break;
    }

    case 0x418: // Transmissin state determination
      {
        switch (d[0]){
          case 80: //P
          {
            StandardMetrics.ms_v_env_gear->SetValue(-1);
            break;
          }
          case 82: //R
          {
            StandardMetrics.ms_v_env_gear->SetValue(-2);
            StandardMetrics.ms_v_env_parktime->SetValue(0);
            break;
          }
          case 78: //N
          {
            StandardMetrics.ms_v_env_gear->SetValue(0);
            StandardMetrics.ms_v_env_parktime->SetValue(0);
            break;
          }
          case 68: //D
          {
            StandardMetrics.ms_v_env_gear->SetValue(1);
            StandardMetrics.ms_v_env_parktime->SetValue(0);
            break;
          }
          case 131: //B
          {
            StandardMetrics.ms_v_env_gear->SetValue(2);
            StandardMetrics.ms_v_env_parktime->SetValue(0);
            break;
          }
          case 50: //C
          {
            StandardMetrics.ms_v_env_gear->SetValue(3);
            StandardMetrics.ms_v_env_parktime->SetValue(0);
            break;
          }
        }

        break;
      }

    case 0x424: // Lights and doors
      {
        //Windshield wipers	424	1	if bit5 = 1 then on else off
        //Rear window defrost	424	6	if bit5 = 1 then on else off

        if ((d[0]& 4)!=0) //headlight
        { // ON
          StandardMetrics.ms_v_env_headlights->SetValue(true);
        }
        else
        {
          StandardMetrics.ms_v_env_headlights->SetValue(false);
        }

        if ((d[0]& 16)!=0) //Fog lights rear
        {// ON

        }
        else
        {

        }

        if ((d[0]& 8)!=0) //Fog lights front
        { // ON
        }
        else
        {

        }

        if ((d[1]& 4)!=0)// Highbeam
        { // ON
        }
        else
        {

        }

        if ((d[1]& 1)!=0)// Flash lights right
        { // ON
        }
        else
        {

        }

        if ((d[1]& 2)!=0) //Flash lights left
        { // ON
        }
        else
        {

        }

        if ((d[1]& 32)!=0)  //Headlight
        {
		        ESP_LOGI(TAG, "Headlight2 on");
        }
        else
        {

        }

        if ((d[1]& 64)!=0)
        { //Parkinglight
		      ESP_LOGI(TAG, "Parkinglight on");
        }
        else
        {

        }

        if ((d[2]& 1)!=0) //Door
        { //OPEN
            StandardMetrics.ms_v_door_fr->SetValue(true);
        }
        else
        {
              StandardMetrics.ms_v_door_fr->SetValue(false);
        }


        if ((d[2]& 2)!=0) //LHD Driver Door
        { //OPEN
          StandardMetrics.ms_v_door_fl->SetValue(true);
        }
        else
        {
          StandardMetrics.ms_v_door_fl->SetValue(false);
        }

        if ((d[2]& 128)!=0) //??
        {
        }
        else
        {
        }
        break;
      }

    case 0x6e1: // Battery temperatures and voltages E1
    {
      if ((d[0]>=1)&&(d[0]<=12))
        {
        int idx = ((int)d[0]<<1)-2;
        mi_batttemps[idx] = (signed char)d[2] - 50;
        mi_batttemps[idx+1] = (signed char)d[3] - 50;
        mi_battvolts[idx] = ((d[4]*256+d[5])/100.0);
        mi_battvolts[idx+1] = (((d[6]*256+d[7])/200.0)+2.1);
        //ESP_LOGI(TAG, "battvolt e1 %i/%i",idx, idx+1);
        }
    break;
    }

    case 0x6e2: // Battery temperatures and voltages E2
    {
        if ((d[0]>=1)&&(d[0]<=12))
          {
            int idx = ((int)d[0]<<1)+22;
            if (d[0]==12) {
              mi_batttemps[idx-1] = (signed char)d[1] - 50;
            }else{
              mi_batttemps[idx] = (signed char)d[1] - 50;
            }
            if((d[0]!=6) && (d[0]<6)){
              mi_batttemps[idx+1] = (signed char)d[2] - 50;
            }else if ((d[0]!=6) && (d[0]>6) && (d[0]!=12)) {
              mi_batttemps[idx-1] = (signed char)d[2] - 50;
            }
        }
        if ((d[0]>=1)&&(d[0]<=12))
          {
            int idx = ((int)d[0]<<1)+22;

    //        ESP_LOGI(TAG, "battvolt e2 %i/%i",idx, idx+1);
            mi_battvolts[idx] = (((d[4]*256+d[5])/200.0)+2.1);
            mi_battvolts[idx+1] = (((d[6]*256+d[7])/200.0)+2.1);
          }
    break;
    }

    case 0x6e3: // Battery temperatures and voltages E3
    {
        if ((d[0]>=1)&&(d[0]<=12))
          {
            if((d[0]!=6) && (d[0]<6))
            {
              int idx = ((int)d[0]<<1)+44;
              mi_batttemps[idx] = (signed char)d[1] - 50;
              mi_batttemps[idx+1] = (signed char)d[2] - 50;
            }else if ((d[0]!=12) && (d[0]>6))
              {
                int idx = ((int)d[0]<<1)+42;
                mi_batttemps[idx] = (signed char)d[1] - 50;
                mi_batttemps[idx+1] = (signed char)d[2] - 50;
              }
            }

            if ((d[0]>=1)&&(d[0]<=12))
              {
                if((d[0]!=6) && (d[0]<6))
                {
                  int idx = ((int)d[0]<<1)+46;
      //            ESP_LOGI(TAG, "battvolt e3 <6 %i/%i",idx, idx+1);
                  mi_battvolts[idx] = (((d[4]*256+d[5])/200.0)+2.1);
                  mi_battvolts[idx+1] = (((d[6]*256+d[7])/200.0)+2.1);
                }
                else if ((d[0]!=12) && (d[0]>6))
                  {
                    int idx = ((int)d[0]<<1)+44;
        //            ESP_LOGI(TAG, "battvolt e3 <12 >6 %i/%i",idx, idx+1);
                    mi_battvolts[idx] = (((d[4]*256+d[5])/200.0)+2.1);
                    mi_battvolts[idx+1] = (((d[6]*256+d[7])/200.0)+2.1);
                  }
                }
    break;
    }

    case 0x6e4: // Battery voltages E4
    {
        if ((d[0]>=1)&&(d[0]<=12))
            {
              if((d[0]!=6) && (d[0]<6))
              {
                int idx = ((int)d[0]<<1)+66;
          //      ESP_LOGI(TAG, "battvolt e4 <6 %i/%i",idx, idx+1);
                mi_battvolts[idx] = (((d[4]*256+d[5])/200.0)+2.1);
                mi_battvolts[idx+1] = (((d[6]*256+d[7])/200.0)+2.1);
              }
              else if ((d[0]!=12) && (d[0]>6))
                {
                    int idx = ((int)d[0]<<1)+64;
            //        ESP_LOGI(TAG, "battvolt e4 <12 >6 %i/%i",idx, idx+1);
                    mi_battvolts[idx] = (((d[4]*256+d[5])/200.0)+2.1);
                    mi_battvolts[idx+1] = (((d[6]*256+d[7])/200.0)+2.1);
                }
              }
    break;
    }

    case 0x762: // Battery temperatures and voltages E4
    {
        if (d[0] == 36)
            {
              StandardMetrics.ms_v_bat_cac->SetValue(((d[3]*256)+d[4])/10.0);
              StandardMetrics.ms_v_bat_soh->SetValue(StandardMetrics.ms_v_bat_cac->AsFloat()/48);
            }
    break;
    }

    default:
    break;
    }



  }

void OvmsVehicleMitsubishi::Ticker1(uint32_t ticker)
  {

    if (StandardMetrics.ms_v_env_gear->AsInt() == -1)
    { //Charge state if transmission in P
      if((StandardMetrics.ms_v_charge_voltage->AsInt() > 90) && (StandardMetrics.ms_v_charge_voltage->AsInt() < 254) && (StandardMetrics.ms_v_charge_current->AsInt() < 16) && (StandardMetrics.ms_v_charge_current->AsInt() > 0) )
      {
        vehicle_charger_status(CHARGER_STATUS_CHARGING);
      }
      else if((StandardMetrics.ms_v_bat_soc->AsInt() > 91) && (StandardMetrics.ms_v_charge_voltage->AsInt() > 90) && (StandardMetrics.ms_v_charge_current->AsInt() < 1) )
      {
        vehicle_charger_status(CHARGER_STATUS_FINISHED);
      }
      else
      {
        vehicle_charger_status(CHARGER_STATUS_INTERRUPTED);
      }
      if ((StandardMetrics.ms_v_bat_current->AsInt() > 0) && (StandardMetrics.ms_v_charge_voltage->AsInt() > 260))
      {
        vehicle_charger_status(CHARGER_STATUS_QUICK_CHARGING);
      }
    }


    if (StandardMetrics.ms_v_bat_soc->AsInt() <= 10)
        {
          StandardMetrics.ms_v_bat_range_ideal->SetValue(0);
        }
      else
        {
        // Ideal range 150km with 100% capacity
        int SOH = StandardMetrics.ms_v_bat_soh->AsFloat();
        if (SOH == 0){
          SOH = 100; //ideal range as 100% else with SOH
        }
          StandardMetrics.ms_v_bat_range_ideal->SetValue((
          (StandardMetrics.ms_v_bat_soc->AsFloat()-10)*1.664)*SOH/100.0);
        }

  }

void OvmsVehicleMitsubishi::Ticker10(uint32_t ticker)
  {
  int tbattery = 0;
  for (int k=0;k<66;k++){
    tbattery += mi_batttemps[k];
  }
  StandardMetrics.ms_v_bat_temp->SetValue(tbattery/66.0);


  StandardMetrics.ms_v_env_cooling->SetValue(!StandardMetrics.ms_v_bat_temp->IsStale());

  double vbattery = 0;
  for (int l=0;l<88;l++)
  {
    //ESP_LOGI(TAG, "Battery cell %f volt: %f",l,mi_battvolts[l])
    vbattery += (double)mi_battvolts[l];
  }
  StandardMetrics.ms_v_bat_cell_level_avg->SetValue(vbattery/88.0);

  }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandSetChargeMode(vehicle_mode_t mode)
    {
    return NotImplemented;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandSetChargeCurrent(uint16_t limit)
    {
    StandardMetrics.ms_v_charge_climit->SetValue(limit);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandStartCharge()
    {
    StandardMetrics.ms_v_pos_speed->SetValue(0);
    StandardMetrics.ms_v_mot_rpm->SetValue(0);
    StandardMetrics.ms_v_env_awake->SetValue(false);
    StandardMetrics.ms_v_env_on->SetValue(false);
    StandardMetrics.ms_v_charge_inprogress->SetValue(true);
    StandardMetrics.ms_v_door_chargeport->SetValue(true);
    StandardMetrics.ms_v_charge_state->SetValue("charging");
    StandardMetrics.ms_v_charge_substate->SetValue("onrequest");
    StandardMetrics.ms_v_charge_pilot->SetValue(true);
    StandardMetrics.ms_v_charge_voltage->SetValue(220);
    StandardMetrics.ms_v_charge_current->SetValue(16);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandStopCharge()
    {
    StandardMetrics.ms_v_charge_inprogress->SetValue(false);
    StandardMetrics.ms_v_door_chargeport->SetValue(false);
    StandardMetrics.ms_v_charge_state->SetValue("done");
    StandardMetrics.ms_v_charge_substate->SetValue("stopped");
    StandardMetrics.ms_v_charge_pilot->SetValue(false);
    StandardMetrics.ms_v_charge_voltage->SetValue(0);
    StandardMetrics.ms_v_charge_current->SetValue(0);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandSetChargeTimer(bool timeron, uint16_t timerstart)
    {
    return NotImplemented;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandCooldown(bool cooldownon)
    {
    return NotImplemented;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandWakeup()
    {
    StandardMetrics.ms_v_charge_inprogress->SetValue(false);
    StandardMetrics.ms_v_door_chargeport->SetValue(false);
    StandardMetrics.ms_v_charge_state->SetValue("done");
    StandardMetrics.ms_v_charge_substate->SetValue("stopped");
    StandardMetrics.ms_v_charge_pilot->SetValue(false);
    StandardMetrics.ms_v_charge_voltage->SetValue(0);
    StandardMetrics.ms_v_charge_current->SetValue(0);
    StandardMetrics.ms_v_env_on->SetValue(true);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandLock(const char* pin)
    {
    StandardMetrics.ms_v_env_locked->SetValue(true);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandUnlock(const char* pin)
    {
    StandardMetrics.ms_v_env_locked->SetValue(false);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandActivateValet(const char* pin)
    {
    StandardMetrics.ms_v_env_valet->SetValue(true);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandDeactivateValet(const char* pin)
    {
    StandardMetrics.ms_v_env_valet->SetValue(false);

    return Success;
    }

OvmsVehicle::vehicle_command_t OvmsVehicleMitsubishi::CommandHomelink(int button, int durationms)
    {
    return NotImplemented;
    }

void OvmsVehicleMitsubishi::Notify12vCritical()
      {
      }
void OvmsVehicleMitsubishi::Notify12vRecovered()
      {
      }

class OvmsVehicleMitsubishiInit
  {
    public: OvmsVehicleMitsubishiInit();
} OvmsVehicleMitsubishiInit  __attribute__ ((init_priority (9000)));

OvmsVehicleMitsubishiInit::OvmsVehicleMitsubishiInit()
  {
  ESP_LOGI(TAG, "Registering Vehicle: Mitsubishi iMiEV, Citroen C-Zero, Peugeot iOn (9000)");

  MyVehicleFactory.RegisterVehicle<OvmsVehicleMitsubishi>("MI","Mitsubishi iMiEV, Citroen C-Zero, Peugeot iOn");
  }
