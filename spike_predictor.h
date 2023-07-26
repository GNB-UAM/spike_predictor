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
#include <default_gui_model.h>

class SpikePredictor : public DefaultGUIModel
{

  Q_OBJECT

public:
  SpikePredictor(void);
  virtual ~SpikePredictor(void);

  void execute(void);
  double filter(std::vector<double> signal, int index, double v, int n_points);
  double calculate_slope(double x1, double x2, double dt);
  void createGUI(DefaultGUIModel::variable_t*, int);
  void customizeGUI(void);

protected:
  virtual void update(DefaultGUIModel::update_flags_t);

private:

  double period;

  double th_spike;
  double th_calculated;
  double sl_calculated;
  double curr_slope;
  bool updatable;
  
  bool switch_th;
  double backtime;
  int n_points;
  int n_p_slope;

  double sum;
  double sum_reset_param;
  double th_sum_param;
  double sum_error;
  double sum_min;
  double th_sum_calculated;
  double sum_reset;
  double slope_th_param;

  int vector_size;
  int cycle;

  std::vector<double> v_list;
  std::vector<double> sum_list;
  std::vector<double> th_sum_buff;

  int n_spikes;

  void initParameters();

private slots:
  // these are custom functions that can also be connected to events
  // through the Qt API. they must be implemented in plugin_template.cpp
};
