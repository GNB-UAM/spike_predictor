/*
 * Copyright (C) 2022 Grupo de Neurocomputacion Biologica, Departamento de 
 * Ingenieria Informatica, Universidad Autonoma de Madrid.
 * 
 * Authors:
 *    Garrido-Peña, Alicia
 *    Reyes-Sanchez, Manuel
 *    Sanchez-Martin, Pablo
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This module implementation is derived from DefaultGUIModel with a custom GUI.
 */

/*
 * Module to predict spike activity in intracellular recordings. 
 * This module predicts in real-time the spike time based on the previous spikes.
 * It is based in 3 different algorithms:
 *    Threshold by Voltage value
 *    Threshold by Voltage area
 *    Threshold by Slope value
 * 
 */
#include "spike_predictor.h"
#include <iostream>
#include <main_window.h>
#include <math.h>

extern "C" Plugin::Object*
createRTXIPlugin(void)
{
  return new SpikePredictor();
}

static DefaultGUIModel::variable_t vars[] = {
  /*Module variables*/
  {"Living neuron", "Signal input to analize", DefaultGUIModel::INPUT,},
  {"Integrate init input (V)", "Voltage value to reset sum", DefaultGUIModel::INPUT,},

  {"Firing threshold (V)", "Threshold to declare spike beggining", DefaultGUIModel::PARAMETER,},
  {"Time from peak (ms)", "Time before (negative) or after (positive) the peak to stimulate", DefaultGUIModel::PARAMETER,},
  {"N Points Filter", "Number of points for the filter", DefaultGUIModel::PARAMETER,},
  {"N Points Slope", "Number of points for the slope", DefaultGUIModel::PARAMETER,},
  {"Sum init (V)", "Voltage value to reset accumulated sum", DefaultGUIModel::PARAMETER,},
  {"Accumulated sum threshold", "Value of the accumulated sum that triggers x (if >=0 calculates threshold)", DefaultGUIModel::PARAMETER,},
  {"Accumulated sum threshold error", "Allowed error for v-sum_reset (recommended 0.003)", DefaultGUIModel::PARAMETER,},
  {"Slope threshold", "Value of the slope that triggers the state (if -1000 calculates threshold)", DefaultGUIModel::PARAMETER,},

  {"Filtered signal", "Filter", DefaultGUIModel::OUTPUT,},
  {"Calculated threshold", "Calculated threshold", DefaultGUIModel::OUTPUT,},
  {"Calculated slope", "Calculated slope", DefaultGUIModel::OUTPUT,},
  {"Calculated sum threshold", "Calculated Accumulated sum threshold", DefaultGUIModel::OUTPUT,},
  {"Slope output", "Slope value", DefaultGUIModel::OUTPUT,},
  {"Sum output", "Accumulated sum value as an output", DefaultGUIModel::OUTPUT,},
  {"Crossed Sum State", "Whether the sum has surpased the threshold", DefaultGUIModel::OUTPUT,},
  {"Crossed Voltage State", "Whether the voltage has surpased the threshold", DefaultGUIModel::OUTPUT,},
  {"Crossed Slope State", "Whether the sum has surpased the threshold", DefaultGUIModel::OUTPUT,},

  {"Calculated threshold state", "Calculated threshold", DefaultGUIModel::STATE,},
  {"Calculated slope state", "Calculated slope", DefaultGUIModel::STATE,},
  {"Sum init input (V)", "Minimum voltage sum", DefaultGUIModel::STATE,},
  {"Min sum", "Minimum voltage sum", DefaultGUIModel::STATE,},
  {"Calculated sum threshold state", "Calculated threshold for sum", DefaultGUIModel::STATE,},
  {"Accumulated sum", "Accumulated voltage sum", DefaultGUIModel::STATE,},

};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

SpikePredictor::SpikePredictor(void)
  : DefaultGUIModel("Spike Predictor", ::vars, ::num_vars)
{
  setWhatsThis("<p><b>Spike Predictor:</b><br>Module for spike prediction based on a threshold by voltage, area or slope.</p>");
  DefaultGUIModel::createGUI(vars,
                             num_vars); // this is required to create the GUI
  customizeGUI();
  initParameters();
  update(INIT); // this is optional, you may place initialization code directly
                // into the constructor
  refresh();    // this is required to update the GUI with parameter and state
                // values
  QTimer::singleShot(0, this, SLOT(resizeMe()));
}

SpikePredictor::~SpikePredictor(void)
{
}


double
SpikePredictor::filter(std::vector<double> signal, int cycle, double v, int n_points)
{
  // No filter case
  if (n_points == 0)
    return v;

  // Weighted average for each point and n previous
  double fv = v*0.3;
  double perc = 0.7/n_points;
  int indx;
  for (int i=1; i <= n_points; i++)
  { 
      indx = (vector_size + cycle - i) % vector_size;
      fv +=  signal[indx]*perc;
  }

  return fv;
}

double
SpikePredictor::calculate_slope(double x1, double x2, double dt)
{
  return (x2-x1)/-dt;
}

void
SpikePredictor::execute(void)
{
  double v = input(0);
  int time_from_peak_points = time_from_peak / period;
  sum_reset = input(1);
  int allow_reset = 1;

  /*SAVE NEW DATA*/
  // v_list[cycle] = v;
  // filter signal --> if n points filter > 0 v modified
  double v_filtered = filter(v_list,cycle,v,n_points);
  v_list[cycle] = v_filtered;

  output(0) = v_filtered;

  // int n_p_slope = 15;

  // Calculate slope 
  double x1 = v_list[(vector_size + cycle) % vector_size];
  double x2 = v_list[(vector_size + cycle-n_p_slope) % vector_size];
  curr_slope = calculate_slope(x1, x2, n_p_slope*period);
  output(4) = curr_slope;

  // Spike detection
  if(!got_spike)
  {
    /*OVER THE THRESHOLD*/
    if (v > th_spike && switch_th == true){
      if (v < v_list[cycle-3]){

        n_spikes++;

        /*SPIKE DETECTED*/
        if (time_from_peak <= 0)
        {
          updatable = true;
          update_in_this_cycle = true;
          // cycle --;
        }
        else{
          t_after = 0;
        }
        allow_reset = 1;
      }
      
    }
  }
  else //After the peak
  {
    if (t_after < time_from_peak_points){ //stimulate after the peak
      t_after++; //Wait for the time to stimulate
    }
    else if (t_after == time_from_peak_points)
    {
      //TODO stimulate
      update_in_this_cycle = true;
      t_after = 0; 
      got_spike = false;
    }
  }


  if(update_in_this_cycle)
  {
    // Save threshold values for next spike
    // Get threshold for V
    switch_th = false;
    th_calculated = v_list[(vector_size + cycle - time_from_peak_points) % vector_size];
    output(1) = th_calculated;

    // Get slope
    x1 = v_list[(vector_size + cycle - time_from_peak_points ) % vector_size];
    x2 = v_list[(vector_size + cycle - time_from_peak_points -n_p_slope) % vector_size];
  
    sl_calculated = calculate_slope(x1, x2, n_p_slope*period);
    output(2) = sl_calculated;

    //Get threshold for sum 
    th_sum_calculated = sum_list[(vector_size + cycle - time_from_peak_points) % vector_size];
    //Get threshold by mean of 3 last spikes. 
    //TODO: define lim of buffer and size. (use more points ¿?)
    th_sum_buff[n_spikes%10] = th_sum_calculated;
    th_sum_calculated = (th_sum_calculated + th_sum_buff[(n_spikes%10)-1] + th_sum_buff[(n_spikes%10)-2])/3;
    update_in_this_cycle = false;
  }


  // Hiperpolarization  --> spike detection ON again
  if(switch_th==false && v < th_spike){
    switch_th = true;
    // if (time_from_peak < 0)
    //   updatable = false;
  }

  // sum_reset = input(1);
  if(sum_reset != 0){ 
      sum_reset_param = sum_reset;
  }else //If there is no input get default
    sum_reset = sum_reset_param;

  if(allow_reset and ((v - sum_reset) < sum_error)){ //Voltage at reset value
    // Reset sum
      sum = 0;
      allow_reset = 0;
    }

  //increase accumulated sum
  sum += v;
  sum_list[cycle] = sum;
  output(5) = sum;

  if (th_sum_param < 0) //If param modified
    th_sum_calculated = th_sum_param;
  
  output(3) = th_sum_calculated;
 
  if (slope_th_param < -1000) //If param modified
    sl_calculated = slope_th_param;

  // minimum sum value, control state variable
  if (sum < sum_min)
    sum_min = sum;

  // Update states
  if (updatable)
  {
    // output(6) = sum <= th_sum_param; //Area threshold crossed
    output(6) = sum < th_sum_calculated; //Area threshold crossed
    output(7) = v > th_calculated; //Voltage threshold crossed
    output(8) = curr_slope > sl_calculated; //Current threshold crossed
  }
  else
  {
    output(6) = 0;
    output(7) = 0;
    output(8) = 0;
  }

  
  /*NEXT CYCLE*/
  cycle++;
  if (vector_size == cycle){
    cycle = 0;
  }

  
  return;
}

void
SpikePredictor::initParameters(void)
{
  //TODO fix for other period times
  vector_size = 10*4000; // 10 reads per ms * 100 ms buffer
  // vector_size = 100 /  RT::System::getInstance()->getPeriod() * 1e-6; // 0.1 ms per read * 100 ms buffer
  cycle = 0;
  v_list.resize(vector_size, 0);
  sum_list.resize(vector_size, 0);
  switch_th = false;
  time_from_peak = 0;
  th_spike = 0;
  n_points = 0;
  n_p_slope = 0;

  th_calculated = 0;
  th_sum_calculated = -0.05;
  sl_calculated = 0;

  th_sum_buff.resize(vector_size, 0);

  sum = 0;
  sum_reset_param = -0.05;
  th_sum_param = 5;
  sum_error = 0.003;
  sum_min = 100;
  slope_th_param = -1000;

  sum_reset = 0;

  got_spike = false;
  update_in_this_cycle = false;

  updatable = true;

  n_spikes = 0;
}

void
SpikePredictor::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag) {
    case INIT:
      period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
      setParameter("Firing threshold (V)", th_spike);
      setParameter("Time from peak (ms)", time_from_peak);
      setParameter("N Points Filter", n_points);
      setParameter("N Points Slope", n_p_slope);
      setParameter("Accumulated sum threshold", th_sum_param);
      setParameter("Accumulated sum threshold error", sum_error);
      setParameter("Sum init (V)", sum_reset_param);
      setParameter("Slope threshold", slope_th_param);

      setState("Calculated threshold state", th_calculated);
      setState("Calculated sum threshold state", th_sum_calculated);
      setState("Calculated slope state", sl_calculated);
      setState("Accumulated sum", sum);
      setState("Min sum", sum_min);
      setState("Sum init input (V)", sum_reset);

      break;

    case MODIFY:
      th_spike = getParameter("Firing threshold (V)").toDouble();
      time_from_peak = getParameter("Time from peak (ms)").toDouble();
      n_points = getParameter("N Points Filter").toDouble();
      n_p_slope = getParameter("N Points Slope").toDouble();
      sum_reset_param = getParameter("Sum init (V)").toDouble();
      th_sum_param = getParameter("Accumulated sum threshold").toDouble();
      sum_error = getParameter("Accumulated sum threshold error").toDouble();
      slope_th_param = getParameter("Slope threshold").toDouble();


      break;

    case UNPAUSE:
      break;

    case PAUSE:
      break;

    case PERIOD:
      period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
      break;

    default:
      break;
  }
}

void
SpikePredictor::customizeGUI(void)
{
}
