/* gsm-cpu-model.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GSM_CPU_MODEL_H
#define GSM_CPU_MODEL_H


#include <dazzle.h>

G_BEGIN_DECLS

#define GSM_TYPE_CPU_MODEL (gsm_cpu_model_get_type())

G_DECLARE_FINAL_TYPE (GsmCpuModel, gsm_cpu_model, GSM, CPU_MODEL, DzlGraphModel)

DzlGraphModel *gsm_cpu_model_new (void);

G_END_DECLS

#endif /* GSM_CPU_MODEL_H */
